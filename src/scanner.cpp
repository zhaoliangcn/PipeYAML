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
    if (tokens_.empty()) {
        // Return a static end-of-stream token
        static Token eos(TokenType::EndOfStream, stream_.get_mark());
        return eos;
    }
    return tokens_.front();
}

Token Scanner::pop() {
    ensure_tokens_in_queue();
    if (tokens_.empty()) {
        return Token(TokenType::EndOfStream, stream_.get_mark());
    }
    Token t = std::move(tokens_.front());
    tokens_.pop_front();
    return t;
}

bool Scanner::empty() {
    ensure_tokens_in_queue();
    if (tokens_.empty()) return true;
    return tokens_.front().type == TokenType::EndOfStream;
}

void Scanner::scan_all() {
    while (!ended_) {
        scan_next_token();
    }
}

void Scanner::ensure_tokens_in_queue() {
    // Keep scanning until we have at least one token, or we hit EOF
    while (tokens_.empty() && !ended_) {
        scan_next_token();
    }
}

// ===========================================================================
// Indentation
// ===========================================================================
int Scanner::read_indent() {
    int indent = 0;
    while (true) {
        char c = stream_.peek();
        if (c == ' ') {
            indent++;
            stream_.get();
        } else if (c == '\t') {
            // YAML forbids tabs for indentation
            throw ScannerException("Tab characters are not allowed for indentation",
                                   stream_.get_mark());
        } else if (c == '\r') {
            // Carriage return - skip (handled as part of newline)
            stream_.get();
        } else {
            break;
        }
    }
    return indent;
}

// ===========================================================================
// Main scan dispatcher
// ===========================================================================
void Scanner::scan_next_token() {
    // Skip whitespace (except newlines)
    while (is_whitespace(stream_.peek())) {
        stream_.get();
    }

    char c = stream_.peek();

    if (c == '\0' || stream_.eof()) {
        // End of stream
        if (!ended_) {
            ended_ = true;
            // Pop indentation stack
            while (indents_.size() > 1) {
                indents_.pop_back();
            }
            tokens_.emplace_back(TokenType::EndOfStream, stream_.get_mark());
        }
        return;
    }

    // Handle based on first character
    if (c == '\n') {
        scan_newline();
    } else if (c == '#') {
        scan_comment();
    } else if (c == '-') {
        // Could be block sequence entry or document start or negative number
        if (stream_.look_ahead(1) == ' ' || stream_.look_ahead(1) == '\n'
            || stream_.look_ahead(1) == '\t' || stream_.look_ahead(1) == '\0') {
            scan_block_sequence_entry();
        } else if (stream_.look_ahead(1) == '-' && stream_.look_ahead(2) == '-') {
            scan_document_marker();
        } else {
            scan_scalar();
        }
    } else if (c == ':') {
        // Could be key separator or part of a scalar
        char next = stream_.look_ahead(1);
        if (next == ' ' || next == '\n' || next == '\t' || next == '\0' || next == ','
            || next == ']' || next == '}' || next == '#') {
            stream_.get(); // consume ':'
            tokens_.emplace_back(TokenType::Key, stream_.get_mark());
        } else {
            scan_scalar();
        }
    } else if (c == '[') {
        scan_flow_sequence();
    } else if (c == ']') {
        stream_.get();
        tokens_.emplace_back(TokenType::FlowSequenceEnd, stream_.get_mark());
        if (!flows_.empty()) flows_.pop_back();
    } else if (c == '{') {
        scan_flow_map();
    } else if (c == '}') {
        stream_.get();
        tokens_.emplace_back(TokenType::FlowMapEnd, stream_.get_mark());
        if (!flows_.empty()) flows_.pop_back();
    } else if (c == '&') {
        scan_anchor_or_alias(); // anchor
    } else if (c == '*') {
        scan_anchor_or_alias(); // alias
    } else if (c == ',' || c == '?') {
        // Flow separator or explicit key indicator
        stream_.get();
        // For now, skip these - the parser handles them via context
        // Comma is a flow separator, '?' is explicit mapping key
    } else if (c == '%') {
        scan_directive();
    } else if (c == '.') {
        if (stream_.look_ahead(1) == '.' && stream_.look_ahead(2) == '.') {
            scan_document_marker();
        } else {
            scan_scalar();
        }
    } else {
        scan_scalar();
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
        tokens_.emplace_back(TokenType::Newline, stream_.get_mark());
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

    tokens_.emplace_back(TokenType::Newline, stream_.get_mark());
}

// ===========================================================================
// Block sequence entry '-'
// ===========================================================================
void Scanner::scan_block_sequence_entry() {
    auto mark = stream_.get_mark();
    stream_.get(); // consume '-'
    tokens_.emplace_back(TokenType::BlockSequenceStart, mark);
}

// ===========================================================================
// Flow sequence '[ ... ]'
// ===========================================================================
void Scanner::scan_flow_sequence() {
    stream_.get(); // consume '['
    flows_.push_back(1);
    tokens_.emplace_back(TokenType::FlowSequenceStart, stream_.get_mark());
}

// ===========================================================================
// Flow map '{ ... }'
// ===========================================================================
void Scanner::scan_flow_map() {
    stream_.get(); // consume '{'
    flows_.push_back(1);
    tokens_.emplace_back(TokenType::FlowMapStart, stream_.get_mark());
}

// ===========================================================================
// Anchor (&id) or Alias (*id)
// ===========================================================================
void Scanner::scan_anchor_or_alias() {
    auto mark = stream_.get_mark();
    char prefix = stream_.get(); // '&' or '*'

    std::string name;
    while (true) {
        char c = stream_.peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_'
            || c == '.' || c == '/') {
            name += stream_.get();
        } else {
            break;
        }
    }

    if (name.empty()) {
        throw ScannerException("Empty anchor/alias name", mark);
    }

    TokenType type = (prefix == '&') ? TokenType::Anchor : TokenType::Alias;
    tokens_.emplace_back(type, mark, name);
}

// ===========================================================================
// Scalar
// ===========================================================================
void Scanner::scan_scalar() {
    auto mark = stream_.get_mark();
    std::string value;

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
        // Read quoted scalar
        while (true) {
            c = stream_.get();

            if (c == quote_char) {
                // Check for escaped quote (double quote)
                if (quote_char == '\'' && stream_.peek() == '\'') {
                    value += stream_.get(); // escaped single quote
                    continue;
                }
                break; // end of quoted scalar
            }
            if (c == '\0' || stream_.eof()) {
                throw ScannerException("Unterminated quoted scalar", mark);
            }
            if (c == '\\' && quote_char == '"') {
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
    } else {
        // Unquoted scalar - read until delimiter
        while (true) {
            c = stream_.peek();
            // Delimiters for unquoted scalars
            if (c == '\0' || stream_.eof() || c == '\n' || c == '\r'
                || c == '#' || c == '[' || c == ']' || c == '{' || c == '}'
                || c == ',' || c == '`') {
                break;
            }
            // ':' is a delimiter only when followed by space/newline/tab/end
            if (c == ':') {
                char next = stream_.look_ahead(1);
                if (next == ' ' || next == '\n' || next == '\t'
                    || next == '\0' || next == ',' || next == ']'
                    || next == '}' || next == '#') {
                    break;
                }
            }
            value += stream_.get();
        }

        // Trim trailing whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
    }

    if (!value.empty() || is_quoted) {
        tokens_.emplace_back(TokenType::Scalar, mark, value);
    }
}

// ===========================================================================
// Comment '#'
// ===========================================================================
void Scanner::scan_comment() {
    auto mark = stream_.get_mark();
    std::string text;
    text += stream_.get(); // consume '#'

    while (true) {
        char c = stream_.peek();
        if (c == '\n' || c == '\0' || stream_.eof()) break;
        text += stream_.get();
    }

    tokens_.emplace_back(TokenType::Comment, mark, text);
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
        tokens_.emplace_back(TokenType::DocumentStart, mark);
    } else if (marker == "...") {
        tokens_.emplace_back(TokenType::DocumentEnd, mark);
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
