#include <tiny_test.h>
#include <pipeyaml/scanner.h>
#include <pipeyaml/stream.h>

using namespace YAML;

TEST_CASE("Scanner - simple scalar", "scanner", "core") {
    Stream stream("hello");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "hello");
}

TEST_CASE("Scanner - block sequence entry", "scanner", "core") {
    Stream stream("- item1\n- item2");
    Scanner scanner(stream);

    // First '-'
    CHECK_EQ(scanner.peek().type, TokenType::BlockSequenceStart);
    scanner.pop();

    // Scalar "item1"
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "item1");
    scanner.pop();

    // Newline
    CHECK_EQ(scanner.peek().type, TokenType::Newline);
    scanner.pop();

    // Second '-'
    CHECK_EQ(scanner.peek().type, TokenType::BlockSequenceStart);
    scanner.pop();

    // Scalar "item2"
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "item2");
    scanner.pop();

    // End
    CHECK_EQ(scanner.peek().type, TokenType::EndOfStream);
}

TEST_CASE("Scanner - quoted scalar double", "scanner", "core") {
    Stream stream("\"hello world\"");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "hello world");
}

TEST_CASE("Scanner - quoted scalar single", "scanner", "core") {
    Stream stream("'hello world'");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "hello world");
}

TEST_CASE("Scanner - anchor and alias", "scanner", "core") {
    Stream stream("&anchor *alias");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Anchor);
    CHECK_EQ(scanner.peek().value, "anchor");
    scanner.pop();
    // Whitespace is skipped; next token is Alias
    CHECK_EQ(scanner.peek().type, TokenType::Alias);
    CHECK_EQ(scanner.peek().value, "alias");
    scanner.pop();
    CHECK_EQ(scanner.peek().type, TokenType::EndOfStream);
}

TEST_CASE("Scanner - document markers", "scanner", "core") {
    Stream stream("---\nhello\n...\n");
    Scanner scanner(stream);

    CHECK_EQ(scanner.peek().type, TokenType::DocumentStart);
    scanner.pop();

    CHECK_EQ(scanner.peek().type, TokenType::Newline);
    scanner.pop();

    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "hello");
    scanner.pop();

    // Skip newlines and document end
    while (scanner.peek().type == TokenType::Newline) scanner.pop();
    CHECK_EQ(scanner.peek().type, TokenType::DocumentEnd);
}

TEST_CASE("Scanner - comment", "scanner", "core") {
    Stream stream("# this is a comment\nvalue");
    Scanner scanner(stream);

    CHECK_EQ(scanner.peek().type, TokenType::Comment);
    CHECK(scanner.peek().value.find("this is a comment") != std::string::npos);
    scanner.pop();

    CHECK_EQ(scanner.peek().type, TokenType::Newline);
    scanner.pop();

    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "value");
}

TEST_CASE("Scanner - empty string", "scanner", "edge") {
    Stream stream("");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::EndOfStream);
    CHECK(scanner.empty());
}

TEST_CASE("Scanner - flow sequence start", "scanner", "core") {
    Stream stream("[");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::FlowSequenceStart);
}

TEST_CASE("Scanner - flow map", "scanner", "core") {
    Stream stream("{}");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::FlowMapStart);
    scanner.pop();
    CHECK_EQ(scanner.peek().type, TokenType::FlowMapEnd);
}

TEST_CASE("Scanner - key separator", "scanner", "core") {
    Stream stream("key: value");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "key");
    scanner.pop();
    CHECK_EQ(scanner.peek().type, TokenType::Key);
    scanner.pop();
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "value");
}

TEST_CASE("Scanner - escape sequences", "scanner", "core") {
    Stream stream("\"line1\\nline2\"");
    Scanner scanner(stream);
    CHECK_EQ(scanner.peek().type, TokenType::Scalar);
    CHECK_EQ(scanner.peek().value, "line1\nline2");
}

TINY_TEST_MAIN();
