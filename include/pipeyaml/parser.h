#ifndef PYPEYAML_PARSER_H
#define PYPEYAML_PARSER_H

#include "exception.h"
#include "node.h"
#include "scanner.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace YAML {

// =============================================================================
// Parser - recursive descent parser
// =============================================================================
class Parser {
public:
    explicit Parser(Scanner& scanner);

    // Parse API
    std::shared_ptr<node_data> parse_next_document();
    bool load_next_document(Node& node);

private:
    // Recursive descent parsing methods
    std::shared_ptr<node_data> parse_document();
    std::shared_ptr<node_data> parse_node(bool try_map = true);
    std::shared_ptr<node_data> parse_sequence();
    std::shared_ptr<node_data> parse_map();
    std::shared_ptr<node_data> parse_scalar(Token token);
    std::shared_ptr<node_data> parse_flow_sequence();
    std::shared_ptr<node_data> parse_flow_map();

    // Helper methods
    int current_indent() const { return scanner_.current_indent(); }
    bool is_flow_context() const { return scanner_.is_flow_context(); }

    // Consume helpers
    Token expect_token(TokenType type);
    bool peek_token(TokenType type) const;
    Token consume_token();

    // Anchors & aliases
    void register_anchor(const std::string& id, std::shared_ptr<node_data> node);
    std::shared_ptr<node_data> resolve_alias(const std::string& id);

    // Depth protection
    static constexpr int MAX_RECURSION_DEPTH = 1024;
    void check_depth();

    Scanner& scanner_;
    std::unordered_map<std::string, std::shared_ptr<node_data>> anchors_;
    int recursion_depth_ = 0;

    // Track the indent of the current block
    int current_block_indent_ = -1;
};

} // namespace YAML

#endif // PYPEYAML_PARSER_H
