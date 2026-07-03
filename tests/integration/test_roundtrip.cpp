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

TINY_TEST_MAIN();
