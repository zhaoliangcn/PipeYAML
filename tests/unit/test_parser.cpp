#include <tiny_test.h>
#include <pipeyaml/parser.h>
#include <pipeyaml/scanner.h>
#include <pipeyaml/stream.h>

using namespace YAML;

// Helper to parse a string directly
static Node parse_string(const std::string& input) {
    Stream stream(input);
    Scanner scanner(stream);
    Parser parser(scanner);
    Node node;
    parser.load_next_document(node);
    return node;
}

TEST_CASE("Parser - simple scalar", "parser", "core") {
    Node node = parse_string("hello");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "hello");
}

TEST_CASE("Parser - quoted scalar", "parser", "core") {
    Node node = parse_string("\"hello world\"");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "hello world");
}

TEST_CASE("Parser - block sequence", "parser", "core") {
    Node node = parse_string("- one\n- two\n- three");
    CHECK(node.is_sequence());
    CHECK_EQ(node.size(), 3);
    CHECK_EQ(node[0].scalar(), "one");
    CHECK_EQ(node[1].scalar(), "two");
    CHECK_EQ(node[2].scalar(), "three");
}

TEST_CASE("Parser - block mapping", "parser", "core") {
    Node node = parse_string("name: PipeYAML\nversion: 0.1");
    CHECK(node.is_map());
    CHECK_EQ(node.size(), 2);
    CHECK_EQ(node["name"].scalar(), "PipeYAML");
    CHECK_EQ(node["version"].scalar(), "0.1");
}

TEST_CASE("Parser - nested block sequence in map", "parser", "core") {
    Node node = parse_string("items:\n  - a\n  - b\n  - c");
    CHECK(node.is_map());
    CHECK(node["items"].is_sequence());
    CHECK_EQ(node["items"].size(), 3);
    CHECK_EQ(node["items"][0].scalar(), "a");
    CHECK_EQ(node["items"][2].scalar(), "c");
}

TEST_CASE("Parser - nested map in sequence", "parser", "core") {
    Node node = parse_string("- name: Alice\n  age: 30\n- name: Bob\n  age: 25");
    CHECK(node.is_sequence());
    CHECK_EQ(node.size(), 2);
    CHECK_EQ(node[0]["name"].scalar(), "Alice");
    CHECK_EQ(node[0]["age"].scalar(), "30");
    CHECK_EQ(node[1]["name"].scalar(), "Bob");
}

TEST_CASE("Parser - document markers", "parser", "core") {
    Node node = parse_string("---\nhello\n...\n");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "hello");
}

TEST_CASE("Parser - empty input returns undefined", "parser", "edge") {
    Node node = parse_string("");
    CHECK_FALSE(node.is_defined());
}

TEST_CASE("Parser - boolean values", "parser", "core") {
    Node node = parse_string("true");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "true");
    CHECK_EQ(node.as<bool>(), true);
}

TEST_CASE("Parser - integer parsing", "parser", "core") {
    Node node = parse_string("42");
    CHECK(node.is_scalar());
    CHECK_EQ(node.as<int>(), 42);
}

TEST_CASE("Parser - float parsing", "parser", "core") {
    Node node = parse_string("3.14");
    CHECK(node.is_scalar());
    double val = node.as<double>();
    CHECK_FLOAT_NEAR(val, 3.14, 0.001);
}

TEST_CASE("Parser - anchor and alias", "parser", "core") {
    Node node = parse_string("defaults: &defaults\n  timeout: 30\nserver:\n  <<: *defaults\n  port: 8080");
    CHECK(node.is_map());
    CHECK(node["defaults"].is_map());
    CHECK(node["server"].is_map());
}

TINY_TEST_MAIN();
