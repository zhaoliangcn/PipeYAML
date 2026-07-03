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

TINY_TEST_MAIN();
