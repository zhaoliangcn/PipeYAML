#include <tiny_test.h>
#include <pipeyaml/stream.h>

using namespace YAML;

TEST_CASE("Stream - basic string input", "stream", "core") {
    Stream stream("hello");
    CHECK_EQ(stream.peek(), 'h');
    CHECK_EQ(stream.get(), 'h');
    CHECK_EQ(stream.get(), 'e');
    CHECK_EQ(stream.get(), 'l');
    CHECK_EQ(stream.get(), 'l');
    CHECK_EQ(stream.get(), 'o');
    CHECK(stream.eof());
}

TEST_CASE("Stream - empty string", "stream", "edge") {
    Stream stream("");
    CHECK(stream.eof());
}

TEST_CASE("Stream - position tracking", "stream", "core") {
    Stream stream("ab\ncd");
    auto m1 = stream.get_mark();
    CHECK_EQ(m1.line, 1);
    CHECK_EQ(m1.column, 1);

    stream.get(); // 'a'
    stream.get(); // 'b'
    auto m2 = stream.get_mark();
    CHECK_EQ(m2.line, 1);
    CHECK_EQ(m2.column, 3);

    stream.get(); // '\n'
    auto m3 = stream.get_mark();
    CHECK_EQ(m3.line, 2);
    CHECK_EQ(m3.column, 1);

    stream.get(); // 'c'
    auto m4 = stream.get_mark();
    CHECK_EQ(m4.line, 2);
    CHECK_EQ(m4.column, 2);
}

TEST_CASE("Stream - look_ahead", "stream", "core") {
    Stream stream("abc");
    CHECK_EQ(stream.look_ahead(0), 'a');
    CHECK_EQ(stream.look_ahead(1), 'b');
    CHECK_EQ(stream.look_ahead(2), 'c');
    CHECK_EQ(stream.look_ahead(3), '\0'); // past end
}

TEST_CASE("Stream - eat", "stream", "core") {
    Stream stream("abcdef");
    stream.eat(3);
    CHECK_EQ(stream.get(), 'd');
    CHECK_EQ(stream.get(), 'e');
}

TEST_CASE("Stream - newline tracking", "stream", "core") {
    Stream stream("line1\nline2\nline3");
    std::string result;
    while (!stream.eof()) {
        result += stream.get();
    }
    CHECK_EQ(result, "line1\nline2\nline3");
}

TEST_CASE("Stream - UTF-8 BOM skip", "stream", "core") {
    // UTF-8 BOM: 0xEF 0xBB 0xBF
    std::string bom = "\xEF\xBB\xBFhello";
    Stream stream(bom);
    CHECK_EQ(stream.get(), 'h');
    CHECK_EQ(stream.get(), 'e');
}

TEST_CASE("Stream - peek without consume", "stream", "core") {
    Stream stream("xyz");
    CHECK_EQ(stream.peek(), 'x');
    CHECK_EQ(stream.peek(), 'x'); // still 'x'
    CHECK_EQ(stream.get(), 'x');
    CHECK_EQ(stream.peek(), 'y');
}

TINY_TEST_MAIN();
