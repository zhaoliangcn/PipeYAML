#include <tiny_test.h>
#include <pipeyaml/node.h>

using namespace YAML;

TEST_CASE("Node - default construction", "node", "core") {
    Node node;
    CHECK(node.is_null());
    CHECK_FALSE(node.is_defined());
    CHECK_FALSE(node.is_scalar());
    CHECK_FALSE(node.is_sequence());
    CHECK_FALSE(node.is_map());
}

TEST_CASE("Node - typed construction", "node", "core") {
    Node scalar(NodeType::Scalar);
    CHECK(scalar.is_scalar());

    Node seq(NodeType::Sequence);
    CHECK(seq.is_sequence());

    Node map(NodeType::Map);
    CHECK(map.is_map());
}

TEST_CASE("Node - scalar set/get", "node", "core") {
    Node node;
    node.set_scalar("hello");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "hello");
}

TEST_CASE("Node - sequence push_back and access", "node", "core") {
    Node seq(NodeType::Sequence);
    seq.push_back(Node(NodeType::Scalar));
    seq[0].set_scalar("item1");
    seq.push_back(Node(NodeType::Scalar));
    seq[1].set_scalar("item2");

    CHECK_EQ(seq.size(), 2);
    CHECK_EQ(seq[0].scalar(), "item1");
    CHECK_EQ(seq[1].scalar(), "item2");
}

TEST_CASE("Node - map insert and access", "node", "core") {
    Node map(NodeType::Map);
    map["key1"].set_scalar("value1");
    map["key2"] = std::string("value2");

    CHECK_EQ(map.size(), 2);
    CHECK_EQ(map["key1"].scalar(), "value1");
    CHECK_EQ(map["key2"].scalar(), "value2");
}

TEST_CASE("Node - int conversion", "node", "core") {
    Node node;
    node.set_scalar("42");
    int val = node.as<int>();
    CHECK_EQ(val, 42);
}

TEST_CASE("Node - double conversion", "node", "core") {
    Node node;
    node.set_scalar("3.14");
    double val = node.as<double>();
    CHECK_FLOAT_NEAR(val, 3.14, 0.001);
}

TEST_CASE("Node - bool conversion", "node", "core") {
    Node t;
    t.set_scalar("true");
    CHECK_EQ(t.as<bool>(), true);

    Node f;
    f.set_scalar("false");
    CHECK_EQ(f.as<bool>(), false);
}

TEST_CASE("Node - string conversion", "node", "core") {
    Node node;
    node.set_scalar("hello world");
    std::string val = node.as<std::string>();
    CHECK_EQ(val, "hello world");
}

TEST_CASE("Node - as with fallback", "node", "core") {
    Node node;
    node.set_scalar("not_a_number");
    int val = node.as<int>(42);
    CHECK_EQ(val, 42); // fallback value
}

TEST_CASE("Node - sequence conversion to vector", "node", "core") {
    Node seq(NodeType::Sequence);
    seq.push_back(Node(NodeType::Scalar));
    seq[0].set_scalar("1");
    seq.push_back(Node(NodeType::Scalar));
    seq[1].set_scalar("2");
    seq.push_back(Node(NodeType::Scalar));
    seq[2].set_scalar("3");

    std::vector<int> vec = seq.as<std::vector<int>>();
    CHECK_EQ(vec.size(), 3);
    CHECK_EQ(vec[0], 1);
    CHECK_EQ(vec[1], 2);
    CHECK_EQ(vec[2], 3);
}

TEST_CASE("Node - delete sequence item", "node", "core") {
    Node seq(NodeType::Sequence);
    seq.push_back(Node(NodeType::Scalar));
    seq[0].set_scalar("keep");
    seq.push_back(Node(NodeType::Scalar));
    seq[1].set_scalar("remove");
    CHECK_EQ(seq.size(), 2);
    seq.remove(1);
    CHECK_EQ(seq.size(), 1);
    CHECK_EQ(seq[0].scalar(), "keep");
}

TEST_CASE("Node - equality", "node", "edge") {
    Node a, b;
    a.set_scalar("hello");
    b.set_scalar("hello");
    CHECK_EQ(a, b);

    b.set_scalar("world");
    CHECK_NE(a, b);
}

TEST_CASE("Node - operator[] creates map on null", "node", "core") {
    Node node; // null
    node["key"] = std::string("value");
    CHECK(node.is_map());
    CHECK_EQ(node["key"].scalar(), "value");
}

TEST_CASE("Node - nested structure", "node", "core") {
    Node root(NodeType::Map);
    root["name"] = std::string("PipeYAML");

    Node versions(NodeType::Sequence);
    versions.push_back(Node(NodeType::Scalar));
    versions[0].set_scalar("0.1.0");
    versions.push_back(Node(NodeType::Scalar));
    versions[1].set_scalar("0.2.0");

    root["versions"] = versions;

    CHECK_EQ(root["name"].scalar(), "PipeYAML");
    CHECK(root["versions"].is_sequence());
    CHECK_EQ(root["versions"][0].scalar(), "0.1.0");
}

TINY_TEST_MAIN();
