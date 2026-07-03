#include "pipeyaml/scanner.h"

#include <cctype>
#include <sstream>

namespace YAML {

// ===========================================================================
// Construction
// ===========================================================================
Scanner::Scanner(Stream& stream)
    : stream_(stream)
{
    // Initial indentation level is 0
    indents_.push_back({0, false});
}

// ===========================================================================
// Token queue operations
// ===========================================================================
const Token& Scanner::peek() {
    ensure_tokens_in_queue();
    if (token_head_ >= tokens_.size()) {
        static Token eos(TokenType::EndOfStream, stream_.get_mark());
        return eos;
    }
    return tokens_[token_head_];
}

Token Scanner::pop() {
    ensure_tokens_in_queue();
    if (token_head_ >= tokens_.size()) {
        return Token(TokenType::EndOfStream, stream_.get_mark());
    }
    return tokens_[token_head_++];
}

bool Scanner::empty() {
    ensure_tokens_in_queue();
    if (token_head_ >= tokens_.size()) return true;
    return tokens_[token_head_].type == TokenType::EndOfStream;
}

void Scanner::scan_all() {
    while (!ended_) {
        scan_next_token();
    }
}

void Scanner::ensure_tokens_in_queue() {
    while (token_head_ >= tokens_.size() && !ended_) {
        scan_next_token();
    }
}

// ===========================================================================
// Indentation
// ===========================================================================
int Scanner::read_indent() {
    int indent = 0;
    auto seg = stream_.current_segment();
    for (char c : seg) {
        if (c == ' ') {
            indent++;
        } else if (c == '\t') {
            // YAML forbids tabs for indentation
            throw ScannerException("Tab characters are not allowed for indentation",
                                   stream_.get_mark());
        } else if (c == '\r') {
            // Carriage return - skip (handled as part of newline)
            indent++;
        } else {
            break;
        }
    }
    stream_.advance(indent);
    return indent;
}

// ===========================================================================
// Main scan dispatcher
// ===========================================================================
void Scanner::scan_next_token() {
    // Skip whitespace (except newlines) - batch mode
    {
        auto seg = stream_.current_segment();
        size_t n = 0;
        for (char c : seg) {
            if (c == ' ' || c == '\t') n++;
            else break;
        }
        if (n > 0) stream_.advance(n);
    }

    char c = stream_.peek();

    // End of stream
    if (c == '\0' || stream_.eof()) {
        if (!ended_) {
            ended_ = true;
            while (indents_.size() > 1) {
                indents_.pop_back();
            }
            push_token(TokenType::EndOfStream, stream_.get_mark());
        }
        return;
    }

    // Dispatch via lookup table for common characters
    switch (c) {
        case '\n':
            scan_newline();
            return;
        case '#':
            scan_comment();
            return;
        case '-':
            // Could be block sequence entry or document start or negative number
            if (stream_.look_ahead(1) == ' ' || stream_.look_ahead(1) == '\n'
                || stream_.look_ahead(1) == '\t' || stream_.look_ahead(1) == '\0') {
                scan_block_sequence_entry();
            } else if (stream_.look_ahead(1) == '-' && stream_.look_ahead(2) == '-') {
                scan_document_marker();
            } else {
                scan_scalar();
            }
            return;
        case ':':
            // Could be key separator or part of a scalar
            {
                char next = stream_.look_ahead(1);
                if (next == ' ' || next == '\n' || next == '\t' || next == '\0' || next == ','
                    || next == ']' || next == '}' || next == '#') {
                    stream_.get();
                    push_token(TokenType::Key, stream_.get_mark());
                } else {
                    scan_scalar();
                }
            }
            return;
        case '[':
            scan_flow_sequence();
            return;
        case ']':
            stream_.get();
            push_token(TokenType::FlowSequenceEnd, stream_.get_mark());
            if (!flows_.empty()) flows_.pop_back();
            return;
        case '{':
            scan_flow_map();
            return;
        case '}':
            stream_.get();
            push_token(TokenType::FlowMapEnd, stream_.get_mark());
            if (!flows_.empty()) flows_.pop_back();
            return;
        case '&':
        case '*':
            scan_anchor_or_alias();
            return;
        case ',':
        case '?':
            // Flow separator or explicit key indicator
            stream_.get();
            return;
        case '%':
            scan_directive();
            return;
        case '.':
            if (stream_.look_ahead(1) == '.' && stream_.look_ahead(2) == '.') {
                scan_document_marker();
            } else {
                scan_scalar();
            }
            return;
        default:
            scan_scalar();
            return;
    }
}

// ===========================================================================
// Newline
// ===========================================================================
void Scanner::scan_newline() {
    stream_.get(); // consume '\n'

    // Handle indentation on the next line
    int indent = read_indent();

    // Check if this is an empty line (all whitespace)
    if (stream_.peek() == '\n' || stream_.peek() == '\0') {
        push_token(TokenType::Newline, stream_.get_mark());
        return;
    }

    // Check indentation against stack
    int current = current_indent();
    if (indent > current) {
        // Increased indentation - start a new block
        // (Either sequence or map - Parser determines which)
        indents_.push_back({indent, false});
        // Don't emit a token for block start; Parser handles via indent level
    } else if (indent < current) {
        // Decreased indentation - pop back
        while (!indents_.empty() && indents_.back().indent > indent) {
            indents_.pop_back();
        }
        // If we popped below zero, reset to zero
        if (indents_.empty()) {
            indents_.push_back({0, false});
        }
    }

    push_token(TokenType::Newline, stream_.get_mark());
}

// ===========================================================================
// Block sequence entry '-'
// ===========================================================================
void Scanner::scan_block_sequence_entry() {
    auto mark = stream_.get_mark();
    stream_.get(); // consume '-'
    push_token(TokenType::BlockSequenceStart, mark);
}

// ===========================================================================
// Flow sequence '[ ... ]'
// ===========================================================================
void Scanner::scan_flow_sequence() {
    stream_.get(); // consume '['
    flows_.push_back(1);
    push_token(TokenType::FlowSequenceStart, stream_.get_mark());
}

// ===========================================================================
// Flow map '{ ... }'
// ===========================================================================
void Scanner::scan_flow_map() {
    stream_.get(); // consume '{'
    flows_.push_back(1);
    push_token(TokenType::FlowMapStart, stream_.get_mark());
}

// ===========================================================================
// Anchor (&id) or Alias (*id)
// ===========================================================================
void Scanner::scan_anchor_or_alias() {
    auto mark = stream_.get_mark();
    char prefix = stream_.get(); // '&' or '*'

    // Batch-read the name
    auto seg = stream_.current_segment();
    size_t n = 0;
    for (char c : seg) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_'
            || c == '.' || c == '/') {
            n++;
        } else {
            break;
        }
    }

    if (n == 0) {
        throw ScannerException("Empty anchor/alias name", mark);
    }

    std::string name(seg.data(), n);
    stream_.advance(n);

    TokenType type = (prefix == '&') ? TokenType::Anchor : TokenType::Alias;
    push_token(type, mark, name);
}

// ===========================================================================
// Scalar
// ===========================================================================
void Scanner::scan_scalar() {
    auto mark = stream_.get_mark();
    std::string value;
    value.reserve(64);  // pre-reserve to reduce reallocations

    // Check for quoted scalars
    char c = stream_.peek();
    bool is_quoted = false;
    char quote_char = 0;

    if (c == '"') {
        is_quoted = true;
        quote_char = '"';
        stream_.get(); // consume opening quote
    } else if (c == '\'') {
        is_quoted = true;
        quote_char = '\'';
        stream_.get(); // consume opening quote
    }

    if (is_quoted) {
        if (quote_char == '\'') {
            // Single-quoted scalar: batch-read for speed
            // Only special case is '' (escaped single quote)
            while (true) {
                auto seg = stream_.current_segment();
                if (seg.empty()) {
                    if (stream_.eof())
                        throw ScannerException("Unterminated quoted scalar", mark);
                    break;
                }
                // Find next quote
                auto pos = seg.find('\'');
                if (pos == std::string_view::npos) {
                    // No quote in this segment — append everything
                    value.append(seg.data(), seg.size());
                    stream_.advance(seg.size());
                    continue;
                }
                // Found a quote — append preceding text
                if (pos > 0)
                    value.append(seg.data(), pos);
                stream_.advance(pos + 1); // consume up to and including quote
                // Check for escaped quote (two consecutive quotes)
                auto next_seg = stream_.current_segment();
                if (!next_seg.empty() && next_seg[0] == '\'') {
                    value += '\'';
                    stream_.advance(1);
                    continue;
                }
                break; // end of quoted scalar
            }
        } else {
            // Double-quoted scalar: character-by-character (escape sequences)
            while (true) {
                c = stream_.get();

                if (c == '"') {
                    break; // end of quoted scalar
                }
                if (c == '\0' || stream_.eof()) {
                    throw ScannerException("Unterminated quoted scalar", mark);
                }
                if (c == '\\') {
                    // Escape sequence in double-quoted string
                    char next = stream_.get();
                    switch (next) {
                        case '0': value += '\0'; break;
                        case 'a': value += '\a'; break;
                        case 'b': value += '\b'; break;
                        case 't': case '\t': value += '\t'; break;
                        case 'n': value += '\n'; break;
                        case 'v': value += '\v'; break;
                        case 'f': value += '\f'; break;
                        case 'r': value += '\r'; break;
                        case 'e': value += '\x1B'; break;
                        case ' ': value += ' '; break;
                        case '"': value += '"'; break;
                        case '/': value += '/'; break;
                        case '\\': value += '\\'; break;
                        case 'N': value += '\x85'; break;
                        case '_': value += '\xA0'; break;
                        case 'L': value += "\xE2\x80\xA8"; break;
                        case 'P': value += "\xE2\x80\xA9"; break;
                        default:
                            if (next == 'x') {
                                char hex[3] = {stream_.get(), stream_.get(), 0};
                                value += static_cast<char>(std::strtol(hex, nullptr, 16));
                            } else if (next == 'u') {
                                value += '\\'; value += 'u';
                                for (int i = 0; i < 4; i++) value += stream_.get();
                            } else if (next == 'U') {
                                value += '\\'; value += 'U';
                                for (int i = 0; i < 8; i++) value += stream_.get();
                            }
                            break;
                    }
                } else {
                    value += c;
                }
            }
        }
    } else {
        // Unquoted scalar - read until delimiter, batch mode
        {
            auto seg = stream_.current_segment();
            size_t i = 0;
            size_t len = seg.size();

            while (i < len) {
                char c = seg[i];
                // Delimiters for unquoted scalars
                if (c == '\0' || c == '\n' || c == '\r'
                    || c == '#' || c == '[' || c == ']' || c == '{' || c == '}'
                    || c == ',' || c == '`') {
                    break;
                }
                // ':' is a delimiter only when followed by space/newline/tab/end
                if (c == ':') {
                    char next = (i + 1 < len) ? seg[i + 1] : '\0';
                    if (next == ' ' || next == '\n' || next == '\t'
                        || next == '\0' || next == ',' || next == ']'
                        || next == '}' || next == '#') {
                        break;
                    }
                }
                i++;
            }

            if (i > 0) {
                // Trim trailing whitespace (but still consume from stream)
                size_t content_end = i;
                while (content_end > 0 && (seg[content_end - 1] == ' ' || seg[content_end - 1] == '\t')) {
                    content_end--;
                }
                value.append(seg.data(), content_end);
                stream_.advance(i);
            }
        }

        // Trim trailing whitespace (backup: handle any whitespace before delimiter)
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
    }

    if (!value.empty() || is_quoted) {
        push_token(TokenType::Scalar, mark, std::move(value));
    }
}

// ===========================================================================
// Comment '#'
// ===========================================================================
void Scanner::scan_comment() {
    auto mark = stream_.get_mark();
    // Consume '#'
    stream_.get();
    
    // Read rest of comment line in batch
    auto seg = stream_.current_segment();
    size_t n = 0;
    for (char c : seg) {
        if (c == '\n' || c == '\0') break;
        n++;
    }
    std::string text;
    text.reserve(n + 1);
    text += '#';
    if (n > 0) {
        text.append(seg.data(), n);
        stream_.advance(n);
    }
    
    push_token(TokenType::Comment, mark, std::move(text));
}

// ===========================================================================
// Document marker '---' or '...'
// ===========================================================================
void Scanner::scan_document_marker() {
    auto mark = stream_.get_mark();
    std::string marker;
    marker += stream_.get(); // first char
    marker += stream_.get(); // second char
    marker += stream_.get(); // third char

    if (marker == "---") {
        doc_count_++;
        push_token(TokenType::DocumentStart, mark);
    } else if (marker == "...") {
        push_token(TokenType::DocumentEnd, mark);
    } else {
        // This shouldn't happen since we checked the pattern
        throw ScannerException("Invalid document marker", mark);
    }
}

// ===========================================================================
// Directive '%...'
// ===========================================================================
void Scanner::scan_directive() {
    auto mark = stream_.get_mark();
    std::string directive;
    directive += stream_.get(); // consume '%'

    // Read directive name
    while (true) {
        char c = stream_.peek();
        if (is_blank(c) || c == '\n' || c == '\0') break;
        directive += stream_.get();
    }

    // Read parameters until end of line
    std::string params;
    while (true) {
        char c = stream_.peek();
        if (c == '\n' || c == '\0' || stream_.eof()) break;
        params += stream_.get();
    }

    // Handle known directives
    if (directive == "YAML") {
        // YAML directive: %YAML 1.2
        // We don't need to do anything special for this
    } else if (directive == "TAG") {
        // TAG directive: %TAG !prefix! !uri
        // For now, skip
    }

    // Don't emit a token for directives
}

} // namespace YAML
