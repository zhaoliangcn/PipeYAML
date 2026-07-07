#include <tiny_test.h>
#include <pipeyaml/pipeyaml.h>

using namespace YAML;

TEST_CASE("Roundtrip - scalar parse and dump", "integration", "core") {
    Node node = Load("hello world");
    CHECK(node.is_scalar());
    CHECK_EQ(node.scalar(), "hello world");

    std::string dumped = Dump(node);
    CHECK(dumped.find("hello world") != std::string::npos);
}

TEST_CASE("Roundtrip - sequence", "integration", "core") {
    const std::string input = "- one\n- two\n- three\n";
    Node node = Load(input);
    CHECK(node.is_sequence());
    CHECK_EQ(node.size(), 3);
    CHECK_EQ(node[0].scalar(), "one");
    CHECK_EQ(node[1].scalar(), "two");
    CHECK_EQ(node[2].scalar(), "three");
}

TEST_CASE("Roundtrip - map", "integration", "core") {
    const std::string input = "name: PipeYAML\nversion: 0.1.0\n";
    Node node = Load(input);
    CHECK(node.is_map());
    CHECK_EQ(node["name"].scalar(), "PipeYAML");
    CHECK_EQ(node["version"].scalar(), "0.1.0");
}

TEST_CASE("Roundtrip - nested map sequence", "integration", "core") {
    const std::string input = "items:\n  - name: a\n    val: 1\n  - name: b\n    val: 2\n";
    Node node = Load(input);
    CHECK(node.is_map());
    CHECK(node["items"].is_sequence());
    CHECK_EQ(node["items"].size(), 2);
    CHECK_EQ(node["items"][0]["name"].scalar(), "a");
    CHECK_EQ(node["items"][0]["val"].scalar(), "1");
    CHECK_EQ(node["items"][1]["name"].scalar(), "b");
}

TEST_CASE("Roundtrip - modify then dump", "integration", "core") {
    Node node = Load("count: 0\n");
    node["count"] = std::string("42");
    CHECK_EQ(node["count"].scalar(), "42");

    std::string dumped = Dump(node);
    CHECK(dumped.find("count:") != std::string::npos);
    CHECK(dumped.find("42") != std::string::npos);
}

TEST_CASE("Roundtrip - YAML spec example", "integration", "core") {
    const std::string input = R"(---
receipt:     Oz-Ware Purchase Invoice
date:        2012-08-06
customer:
    given:   Dorothy
    family:  Gale
items:
    - part_no:   A4786
      descrip:   Water Bucket (Filled)
      price:     1.47
      quantity:  4
    - part_no:   E1628
      descrip:   High Heeled "Ruby" Slippers
      price:     100.27
      quantity:  1
)";

    Node doc = Load(input);
    CHECK(doc.is_map());
    CHECK_EQ(doc["receipt"].scalar(), "Oz-Ware Purchase Invoice");
    CHECK(doc["customer"].is_map());
    CHECK_EQ(doc["customer"]["given"].scalar(), "Dorothy");
    CHECK(doc["items"].is_sequence());
    CHECK_EQ(doc["items"].size(), 2);
    CHECK_EQ(doc["items"][0]["part_no"].scalar(), "A4786");
    CHECK_EQ(doc["items"][0]["price"].scalar(), "1.47");
    CHECK_EQ(doc["items"][1]["descrip"].scalar(), "High Heeled \"Ruby\" Slippers");
}

TEST_CASE("Roundtrip - multiple documents", "integration", "core") {
    const std::string input = "---\ndoc1: first\n---\ndoc2: second\n";
    auto docs = LoadAll(input);
    CHECK_EQ(docs.size(), 2);
    CHECK_EQ(docs[0]["doc1"].scalar(), "first");
    CHECK_EQ(docs[1]["doc2"].scalar(), "second");
}

TEST_CASE("Roundtrip - LoadFile API", "integration", "core") {
    // Write a temp YAML file
    const std::string path = "/tmp/test_pipeyaml_loadfile.yaml";
    {
        std::ofstream f(path);
        f << "test: value\nnumber: 123\n";
    }
    Node node = LoadFile(path);
    CHECK(node.is_map());
    CHECK_EQ(node["test"].scalar(), "value");
    CHECK_EQ(node["number"].scalar(), "123");
    std::remove(path.c_str());
}

// Regression: needs_quotes must quote scalars starting with "- " or ": "
TEST_CASE("Roundtrip - scalar starting with dash-space", "integration", "regression") {
    Node root(NodeType::Map);
    root["indicator"] = std::string("- foo");
    root["colon"] = std::string(": bar");
    root["lone_dash"] = std::string("-");
    root["lone_colon"] = std::string(":");

    std::string out = Dump(root);
    // Reload and verify values preserved
    Node reloaded = Load(out);
    CHECK_EQ(reloaded["indicator"].scalar(), "- foo");
    CHECK_EQ(reloaded["colon"].scalar(), ": bar");
    CHECK_EQ(reloaded["lone_dash"].scalar(), "-");
    CHECK_EQ(reloaded["lone_colon"].scalar(), ":");
}

// Regression: null map values must emit as null format, not ""
TEST_CASE("Roundtrip - null map value emits null format", "integration", "regression") {
    // Use Load to create a true Null-typed node (operator= with empty
    // string is defined as Scalar, not Null, by design).
    Node root = Load("present: value\nabsent: null\n");
    CHECK(root["absent"].is_null());

    std::string out = Dump(root);
    CHECK(out.find("null") != std::string::npos);
    CHECK(out.find("\"\"") == std::string::npos);

    // Reload: null should remain null, not empty string
    Node reloaded = Load(out);
    CHECK(reloaded["absent"].is_null());
}

// Regression: copy assignment must clear stale conversion caches
TEST_CASE("Roundtrip - copy assignment clears caches", "integration", "regression") {
    Node a;
    a = 42;  // sets cached_int_ = 42
    CHECK_EQ(a.as<int>(), 42);

    Node b;
    b = std::string("hello");
    a = b;  // copy assign: cache must be cleared
    CHECK_EQ(a.scalar(), "hello");
    // as<int>() should now fail to parse "hello", not return stale 42
    bool threw = false;
    try {
        (void)a.as<int>();
    } catch (...) {
        threw = true;
    }
    CHECK(threw);
}

TINY_TEST_MAIN();
