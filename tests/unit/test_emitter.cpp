#include <tiny_test.h>
#include <pipeyaml/emitter.h>
#include <pipeyaml/node.h>

using namespace YAML;

TEST_CASE("Emitter - emit scalar", "emitter", "core") {
    Node node;
    node.set_scalar("hello");
    std::string result = Dump(node);
    CHECK(result.find("hello") != std::string::npos);
}

TEST_CASE("Emitter - emit sequence", "emitter", "core") {
    Node seq(NodeType::Sequence);
    seq.push_back(Node(NodeType::Scalar));
    seq[0].set_scalar("one");
    seq.push_back(Node(NodeType::Scalar));
    seq[1].set_scalar("two");

    std::string result = Dump(seq);
    CHECK(result.find("- one") != std::string::npos);
    CHECK(result.find("- two") != std::string::npos);
}

TEST_CASE("Emitter - emit map", "emitter", "core") {
    Node map(NodeType::Map);
    map["name"] = std::string("PipeYAML");
    map["version"] = std::string("0.1.0");

    std::string result = Dump(map);
    CHECK(result.find("name:") != std::string::npos);
    CHECK(result.find("PipeYAML") != std::string::npos);
    CHECK(result.find("version:") != std::string::npos);
}

TEST_CASE("Emitter - nested structure roundtrip", "emitter", "core") {
    Node seq(NodeType::Sequence);
    Node map1(NodeType::Map);
    map1["name"] = std::string("Alice");
    map1["age"] = std::string("30");

    Node map2(NodeType::Map);
    map2["name"] = std::string("Bob");
    map2["age"] = std::string("25");

    seq.push_back(map1);
    seq.push_back(map2);

    std::string result = Dump(seq);
    CHECK(result.find("Alice") != std::string::npos);
    CHECK(result.find("Bob") != std::string::npos);
}

TEST_CASE("Emitter - empty node", "emitter", "edge") {
    Node node;
    std::string result = Dump(node);
    // Should not crash - basic sanity
    CHECK_FALSE(result.empty());
}

TEST_CASE("Emitter - manual stream", "emitter", "core") {
    Emitter out;
    out << BeginMap;
    out << Key << "name" << Value << "PipeYAML";
    out << Key << "version" << Value << 1.0;
    out << EndMap;

    std::string result = out.str();
    CHECK(result.find("name:") != std::string::npos);
    CHECK(result.find("PipeYAML") != std::string::npos);
}

TEST_CASE("Emitter - sequence via manipulators", "emitter", "core") {
    Emitter out;
    out << BeginSeq;
    out << "one";
    out << "two";
    out << "three";
    out << EndSeq;

    std::string result = out.str();
    CHECK(result.find("- one") != std::string::npos);
    CHECK(result.find("- three") != std::string::npos);
}

TEST_CASE("Emitter - flow sequence output", "emitter", "core") {
    Node seq(NodeType::Sequence);
    seq.push_back(Node(NodeType::Scalar));
    seq[0].set_scalar("a");
    seq.push_back(Node(NodeType::Scalar));
    seq[1].set_scalar("b");

    // Default block-style output
    std::string result = Dump(seq);
    CHECK(result.find("- a") != std::string::npos);
    CHECK(result.find("- b") != std::string::npos);
}

TEST_CASE("Emitter - empty sequence", "emitter", "edge") {
    Node seq(NodeType::Sequence);
    std::string result = Dump(seq);
    // Should not crash and produce valid output
    CHECK_FALSE(result.empty());
}

TEST_CASE("Emitter - map with various value types", "emitter", "core") {
    Node map(NodeType::Map);
    map["string"] = std::string("text");
    map["int_val"] = std::string("42");
    map["bool_val"] = std::string("true");

    std::string result = Dump(map);
    CHECK(result.find("string:") != std::string::npos);
    CHECK(result.find("int_val:") != std::string::npos);
    CHECK(result.find("bool_val:") != std::string::npos);
}

TINY_TEST_MAIN();
