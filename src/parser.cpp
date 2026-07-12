#include "pipeyaml/parser.h"

#include <sstream>

namespace YAML {

// ===========================================================================
// Construction
// ===========================================================================
Parser::Parser(Scanner& scanner)
    : scanner_(scanner) {}

// ===========================================================================
// Public API
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_next_document() {
    return parse_document();
}

bool Parser::load_next_document(Node& node) {
    auto data = parse_next_document();
    if (!data) return false;
    node = Node(data);
    return true;
}

// ===========================================================================
// Token helpers
// ===========================================================================
Token Parser::expect_token(TokenType type) {
    const Token& token = scanner_.peek();
    if (token.type != type) {
        std::ostringstream msg;
        msg << "Expected token type " << static_cast<int>(type)
            << " but got " << static_cast<int>(token.type);
        throw ParserException(msg.str(), token.mark);
    }
    return scanner_.pop();
}

bool Parser::peek_token(TokenType type) {
    return scanner_.peek().type == type;
}

Token Parser::consume_token() {
    return scanner_.pop();
}

// ===========================================================================
// Anchor/alias management
// ===========================================================================
void Parser::register_anchor(const std::string& id, std::shared_ptr<node_data> node) {
    anchors_[id] = std::move(node);
}

std::shared_ptr<node_data> Parser::resolve_alias(const std::string& id) {
    auto it = anchors_.find(id);
    if (it == anchors_.end()) {
        throw ParserException("Unknown alias: " + id);
    }
    return it->second;
}

// ===========================================================================
// Depth protection
// ===========================================================================
void Parser::check_depth() {
    if (recursion_depth_ >= MAX_RECURSION_DEPTH) {
        throw DeepRecursionException();
    }
}

// ===========================================================================
// Document parsing
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_document() {
    // Scan all tokens upfront
    scanner_.scan_all();

    // Skip newlines before document
    while (peek_token(TokenType::Newline)) {
        consume_token();
    }

    // Optional document start marker
    if (peek_token(TokenType::DocumentStart)) {
        consume_token();
        // Skip newlines after document start
        while (peek_token(TokenType::Newline)) {
            consume_token();
        }
    }

    // Parse the document content (it could be empty)
    if (peek_token(TokenType::DocumentEnd) || peek_token(TokenType::EndOfStream)) {
        // Empty document
        if (peek_token(TokenType::DocumentEnd)) {
            consume_token();
        }
        return nullptr;
    }

    auto node = parse_node();

    // Optional document end marker
    while (peek_token(TokenType::Newline)) {
        consume_token();
    }
    if (peek_token(TokenType::DocumentEnd)) {
        consume_token();
    }

    return node;
}

// ===========================================================================
// Node dispatch
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_node(bool try_map) {
    // TODO: Full tag/anchor handling
    check_depth();
    recursion_depth_++;

    std::shared_ptr<node_data> result;

    const Token& token = scanner_.peek();

    switch (token.type) {
        case TokenType::BlockSequenceStart:
            result = parse_sequence();
            break;
        case TokenType::FlowSequenceStart:
            result = parse_flow_sequence();
            break;
        case TokenType::FlowMapStart:
            result = parse_flow_map();
            break;
        case TokenType::Scalar:
            // When try_map is true, check if this scalar starts a mapping
            if (try_map) {
                if (scanner_.has_next() && scanner_.peek_next_type() == TokenType::Key) {
                    result = parse_map();
                    break;
                }
            }
            result = parse_scalar(consume_token());
            break;
        case TokenType::Anchor:
            // Anchor preceding a node: &id value
            {
                std::string anchor_id = token.value;
                consume_token();
                result = parse_node();
                register_anchor(anchor_id, result);
            }
            break;
        case TokenType::Alias:
            {
                result = resolve_alias(token.value);
                consume_token();
            }
            break;
        case TokenType::Key:
            // ':' at start of line means an empty key followed by a value
            // This is part of a map context - parse as map
            {
                // Go back and try parsing as map
                result = parse_map();
            }
            break;
        default:
            // Try to determine context: if we're at a new indent level after
            // a newline, it could be the start of a map entry
            if (token.type == TokenType::Newline) {
                consume_token();
                result = parse_node();
            } else if (token.type == TokenType::EndOfStream) {
                result = std::make_shared<node_data>(NodeType::Null, false);
            } else {
                throw ParserException("Unexpected token in node context", token.mark);
            }
            break;
    }

    recursion_depth_--;
    return result;
}

// ===========================================================================
// Block sequence parsing
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_sequence() {
    auto result = std::make_shared<node_data>(NodeType::Sequence);

    while (true) {
        // Skip newlines before items
        while (peek_token(TokenType::Newline)) {
            consume_token();
        }

        if (peek_token(TokenType::EndOfStream) || peek_token(TokenType::DocumentEnd)) {
            break;
        }

        // Check if we're still in the sequence (next item starts with '-')
        if (!peek_token(TokenType::BlockSequenceStart)) {
            break;
        }

        // Consume the '-'
        consume_token();

        // Parse the value - may be a scalar, sequence, or map
        auto value = parse_node();
        result->sequence_push_back(value);
    }

    return result;
}

// ===========================================================================
// Flow sequence parsing: [a, b, c]
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_flow_sequence() {
    consume_token(); // consume '['
    auto result = std::make_shared<node_data>(NodeType::Sequence);

    while (!peek_token(TokenType::FlowSequenceEnd)) {
        // Skip commas and whitespace
        while (peek_token(TokenType::Newline)) consume_token();
        const Token& t = scanner_.peek();
        if (t.type == TokenType::FlowSequenceEnd || t.type == TokenType::EndOfStream) break;

        // Parse element
        auto value = parse_node();
        if (value) {
            result->sequence_push_back(value);
        }
    }

    // If we hit end-of-stream without finding the closing bracket, throw
    if (peek_token(TokenType::EndOfStream)) {
        throw ParserException("Unterminated flow sequence");
    }
    expect_token(TokenType::FlowSequenceEnd);
    return result;
}

// ===========================================================================
// Flow map parsing: {k: v, k2: v2}
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_flow_map() {
    consume_token(); // consume '{'
    auto result = std::make_shared<node_data>(NodeType::Map);

    while (!peek_token(TokenType::FlowMapEnd)) {
        while (peek_token(TokenType::Newline)) consume_token();
        if (peek_token(TokenType::FlowMapEnd)) break;
        if (peek_token(TokenType::EndOfStream)) {
            throw ParserException("Unterminated flow mapping");
        }

        // Parse key (false = don't try block map; we're in a flow context)
        auto key = parse_node(false);

        // Expect ':'
        if (!peek_token(TokenType::Key)) {
            throw ParserException("Expected ':' in flow mapping");
        }
        consume_token();

        // Parse value (also flow context)
        auto value = parse_node(false);

        result->map_insert(std::move(key), std::move(value));

        // Commas are already consumed by the Scanner, no need to skip them here
    }

    // If we hit end-of-stream without finding the closing brace, throw
    if (peek_token(TokenType::EndOfStream)) {
        throw ParserException("Unterminated flow mapping");
    }

    expect_token(TokenType::FlowMapEnd);
    return result;
}

// ===========================================================================
// Map parsing (block style)
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_map() {
    auto result = std::make_shared<node_data>(NodeType::Map);
    int map_indent = -1;  // set from first key token

    while (true) {
        // Skip newlines before entries
        while (peek_token(TokenType::Newline)) {
            consume_token();
        }

        if (peek_token(TokenType::EndOfStream) || peek_token(TokenType::DocumentEnd)) {
            break;
        }

        // If next token is BlockSequenceStart, the map is done
        if (peek_token(TokenType::BlockSequenceStart) ||
            peek_token(TokenType::FlowSequenceEnd) ||
            peek_token(TokenType::FlowMapEnd) ||
            peek_token(TokenType::DocumentStart)) {
            break;
        }

        // Check indent of the current token against map indent
        if (map_indent >= 0 && scanner_.tokens_avail() > 0) {
            int tok_indent = scanner_.token_at(0).indent;
            if (tok_indent >= 0 && tok_indent < map_indent) {
                break;  // indent went back, map is done
            }
        }

        // Parse the key
        std::shared_ptr<node_data> key;

        if (peek_token(TokenType::Scalar)) {
            // Record indent from first key token
            if (map_indent < 0 && scanner_.tokens_avail() > 0) {
                map_indent = scanner_.token_at(0).indent;
            }
            key = parse_scalar(consume_token());
        } else if (peek_token(TokenType::Anchor)) {
            auto tok = consume_token();
            std::string anchor_id = tok.value;  // copy before move
            key = std::make_shared<node_data>(NodeType::Scalar);
            key->set_scalar(std::move(tok.value));
            register_anchor(anchor_id, key);  // register with the actual key node
        } else if (peek_token(TokenType::Alias)) {
            auto tok = consume_token();
            key = resolve_alias(tok.value);
        } else if (peek_token(TokenType::FlowSequenceStart)
                   || peek_token(TokenType::FlowMapStart)) {
            key = parse_node(false);
        } else {
            break;
        }

        if (!key) break;

        // Check for ':'
        if (peek_token(TokenType::Key)) {
            consume_token();
        } else {
            throw ParserException("Expected ':' after map key");
        }

        // Parse value
        std::shared_ptr<node_data> value;

        // Check what's on the same line
        if (peek_token(TokenType::Newline) || peek_token(TokenType::EndOfStream)) {
            // Skip the newline and check if value follows at deeper indent
            consume_token(); // consume newline
            while (peek_token(TokenType::Newline)) {
                consume_token();
            }
            // Check if we have a block sequence at deeper indent
            if (peek_token(TokenType::BlockSequenceStart)) {
                value = parse_sequence();
            } else if (peek_token(TokenType::FlowSequenceStart)) {
                value = parse_flow_sequence();
            } else if (peek_token(TokenType::FlowMapStart)) {
                value = parse_flow_map();
            } else if (peek_token(TokenType::Scalar) && scanner_.has_next() && 
                       scanner_.peek_next_type() == TokenType::Key) {
                // Scalar followed by Key at deeper indent = nested map
                value = parse_map();
            } else {
                // Single scalar at deeper indent = value
                if (peek_token(TokenType::Scalar)) {
                    value = parse_scalar(consume_token());
                } else {
                    value = std::make_shared<node_data>(NodeType::Null, true);
                }
            }
        } else if (peek_token(TokenType::BlockSequenceStart)) {
            value = parse_sequence();
        } else {
            value = parse_node();
        }

        result->map_insert(std::move(key), std::move(value));
    }

    return result;
}

// ===========================================================================
// Scalar
// ===========================================================================
std::shared_ptr<node_data> Parser::parse_scalar(Token token) {
    // YAML 1.2 null keywords: null, Null, NULL, ~, empty
    if (token.value.empty()
        || token.value == "null" || token.value == "Null" || token.value == "NULL"
        || token.value == "~") {
        auto result = std::make_shared<node_data>(NodeType::Null);
        result->is_defined_ = true;
        return result;
    }
    auto result = std::make_shared<node_data>(NodeType::Scalar);
    result->set_scalar(std::move(token.value));
    return result;
}

} // namespace YAML
