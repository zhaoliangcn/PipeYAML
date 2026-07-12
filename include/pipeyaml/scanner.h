#ifndef PYPEYAML_SCANNER_H
#define PYPEYAML_SCANNER_H

#include "exception.h"
#include "stream.h"

#include <deque>
#include <string>
#include <vector>

namespace YAML {

// =============================================================================
// Token types
// =============================================================================
enum class TokenType : uint8_t {
    // Structure
    BlockSequenceStart,      // '-'
    BlockMapStart,           // implicit (indentation-based)
    FlowSequenceStart,       // '['
    FlowSequenceEnd,         // ']'
    FlowMapStart,            // '{'
    FlowMapEnd,              // '}'

    // Key-value
    Key,                     // ':'
    Value,                   // implicit (context-based)

    // Document
    DocumentStart,           // '---'
    DocumentEnd,             // '...'

    // Scalar
    Scalar,

    // Anchors & aliases
    Anchor,                  // '&id'
    Alias,                   // '*id'

    // Special
    Comment,
    Newline,
    EndOfStream,
    Error
};

// =============================================================================
// Token
// =============================================================================
struct Token {
    TokenType type = TokenType::Error;
    Mark mark;
    std::string value;
    int indent = -1;  // indent level when token was scanned (-1 = unknown)

    Token() = default;
    Token(TokenType t, const Mark& m, std::string v = "")
        : type(t), mark(m), value(std::move(v)) {}
};

// =============================================================================
// SimpleKey - YAML simple key tracking for implicit mapping keys
// =============================================================================
struct SimpleKey {
    bool possible = false;
    bool required = false;
    int line = 0;
    int column = 0;
    size_t token_index = 0;   // index into tokens_ queue
};

// =============================================================================
// Scanner
// =============================================================================
class Scanner {
public:
    explicit Scanner(Stream& stream);

    // Token queue operations
    const Token& peek();                     // Look at next token
    Token pop();                             // Consume and return next token
    bool empty();                            // Check if stream is exhausted
    void scan_all();                         // Scan all tokens upfront

    // Position
    Mark get_mark() const { return stream_.get_mark(); }

    // Look-ahead: check if next token (without consuming) is available
    bool has_next() { ensure_more_tokens(); return token_head_ + 1 < tokens_.size(); }
    TokenType peek_next_type() { return has_next() ? tokens_[token_head_ + 1].type : TokenType::EndOfStream; }

private:
    friend class Parser;

    // Core scanning
    void ensure_tokens_in_queue();
    void ensure_more_tokens() { while (token_head_ + 1 >= tokens_.size() && !ended_) scan_next_token(); }
    void scan_next_token();
    void push_token(TokenType t, const Mark& m, std::string v = "") {
        Token tok(t, m, std::move(v));
        tok.indent = current_indent();
        tokens_.push_back(std::move(tok));
    }
    // Token queue access helpers (vector + head index)
    size_t tokens_avail() const { return tokens_.size() - token_head_; }
    const Token& token_at(size_t i) const { return tokens_[token_head_ + i]; }
    size_t token_offset() const { return token_head_; }
    void scan_scalar();

    // Structure scanners
    void scan_block_sequence_entry();
    void scan_flow_sequence();
    void scan_flow_map();
    void scan_anchor_or_alias();
    void scan_directive();
    void scan_document_marker();
    void scan_comment();
    void scan_newline();

    // Indentation
    struct IndentMarker {
        int indent = 0;
        bool is_sequence = false;   // sequence-style indent vs map indent
    };

    int current_indent() const {
        return indents_.empty() ? 0 : indents_.back().indent;
    }

    bool is_whitespace(char c) const { return c == ' ' || c == '\t'; }
    bool is_blank(char c) const { return is_whitespace(c) || c == '\r'; }

    // Simple key management
    void add_simple_key();
    void remove_all_simple_keys();
    void verify_simple_keys();
    bool is_flow_context() const { return !flows_.empty(); }

    // Helper: read a line indent and return number of spaces
    int read_indent();

    Stream& stream_;
    std::vector<Token> tokens_;        // vector-based queue (push_back + head index)
    size_t token_head_ = 0;            // logical front of the queue
    std::vector<IndentMarker> indents_;
    std::vector<int> flows_;           // flow context stack (>0 means in flow)
    std::vector<SimpleKey> simple_keys_;

    // State
    bool started_ = false;
    bool ended_ = false;
    int doc_count_ = 0;

    // End-of-stream sentinel token (replaces thread_local static)
    Token eos_token_;
};

} // namespace YAML

#endif // PYPEYAML_SCANNER_H
