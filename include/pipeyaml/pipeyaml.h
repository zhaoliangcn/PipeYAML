#ifndef PYPEYAML_H
#define PYPEYAML_H

/// @file pipeyaml.h
/// Main public API header for PipeYAML - a YAML 1.2 parser and emitter library.

#include "pipeyaml/exception.h"
#include "pipeyaml/emitter.h"
#include "pipeyaml/node.h"
#include "pipeyaml/parser.h"
#include "pipeyaml/scanner.h"
#include "pipeyaml/stream.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace YAML {

// Forward declarations for Load/Parse
Node Load(const std::string& input);
std::vector<Node> LoadAll(const std::string& input);

// =============================================================================
// Parsing API
// =============================================================================

/// Load a YAML document from a string.
inline Node Load(const std::string& input) {
    Stream stream(input);
    Scanner scanner(stream);
    Parser parser(scanner);
    Node node;
    if (!parser.load_next_document(node)) {
        throw ParserException("No document found");
    }
    return node;
}

inline Node Load(const char* input) {
    return Load(std::string(input));
}

inline Node Load(std::string_view input) {
    return Load(std::string(input));
}

/// Load a YAML document from a stream.
inline Node Load(std::istream& input) {
    std::string data((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
    return Load(data);
}

/// Load a YAML document from a file path.
inline Node LoadFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        throw ParserException("Cannot open file: " + filename);
    }
    return Load(fin);
}

inline Node LoadFile(const std::filesystem::path& path) {
    return LoadFile(path.string());
}

/// Load all documents from a multi-document YAML string.
inline std::vector<Node> LoadAll(const std::string& input) {
    Stream stream(input);
    Scanner scanner(stream);
    Parser parser(scanner);
    std::vector<Node> docs;

    Node node;
    while (parser.load_next_document(node)) {
        if (node.is_defined()) {
            docs.push_back(node);
        }
        node = Node{};
    }
    return docs;
}

inline std::vector<Node> LoadAll(std::istream& input) {
    std::string data((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
    return LoadAll(data);
}

} // namespace YAML

#endif // PYPEYAML_H
