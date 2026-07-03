#include "pipeyaml/stream.h"

#include <algorithm>
#include <cstring>

namespace YAML {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
Stream::Stream(std::istream& input) {
    std::string data;
    if (input.good()) {
        input.seekg(0, std::ios::end);
        data.reserve(static_cast<size_t>(input.tellg()));
        input.seekg(0, std::ios::beg);
        data.assign(std::istreambuf_iterator<char>(input),
                    std::istreambuf_iterator<char>());
    }
    init(data);
}

Stream::Stream(std::string_view input) {
    init(input);
}

void Stream::init(std::string_view data) {
    buffer_.assign(data.begin(), data.end());
    buffer_.push_back('\0');   // sentinel for EOF
    mark_ = Mark{1, 1, 0};

    if (buffer_.size() > 1) {
        detect_encoding();
    }
}

// ---------------------------------------------------------------------------
// Encoding detection
// ---------------------------------------------------------------------------
void Stream::detect_encoding() {
    // BOM detection: UTF-8, UTF-16 (BE/LE), UTF-32 (BE/LE)
    const unsigned char* b = reinterpret_cast<const unsigned char*>(&buffer_[0]);

    // UTF-32 LE
    if (buffer_.size() >= 4 && b[0] == 0xFF && b[1] == 0xFE && b[2] == 0x00 && b[3] == 0x00) {
        throw ScannerException("UTF-32 LE encoding not yet supported");
    }
    // UTF-32 BE
    if (buffer_.size() >= 4 && b[0] == 0x00 && b[1] == 0x00 && b[2] == 0xFE && b[3] == 0xFF) {
        throw ScannerException("UTF-32 BE encoding not yet supported");
    }
    // UTF-16 BE
    if (buffer_.size() >= 2 && b[0] == 0xFE && b[1] == 0xFF) {
        throw ScannerException("UTF-16 BE encoding not yet supported");
    }
    // UTF-16 LE
    if (buffer_.size() >= 2 && b[0] == 0xFF && b[1] == 0xFE) {
        throw ScannerException("UTF-16 LE encoding not yet supported");
    }
    // UTF-8 BOM
    if (buffer_.size() >= 3 && b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + 3);
        mark_.pos = 3;
        mark_.column = 1 - 3; // will be corrected on first get()
    }
    // Otherwise assume UTF-8 (native)
}

// ---------------------------------------------------------------------------
// Buffer management
// ---------------------------------------------------------------------------
void Stream::fill_buffer(size_t min_size) {
    while (buffer_.size() < min_size + 1) {
        buffer_.push_back('\0');
    }
}

// ---------------------------------------------------------------------------
// Character access
// ---------------------------------------------------------------------------
char Stream::peek() const {
    return buffer_.empty() ? '\0' : buffer_.front();
}

char Stream::get() {
    if (buffer_.empty() || buffer_.front() == '\0') {
        eof_ = true;
        return '\0';
    }

    char c = buffer_.front();
    buffer_.pop_front();

    // Advance position tracking
    if (c == '\n') {
        mark_.line++;
        mark_.column = 1;
    } else if (c == '\t') {
        // Tab counts as 1 column in YAML (alignment is visual, but
        // YAML treats tab as a single character for column counting)
        mark_.column++;
    } else {
        mark_.column++;
    }
    mark_.pos++;

    return c;
}

void Stream::eat(size_t n) {
    for (size_t i = 0; i < n; ++i) {
        get();
    }
}

bool Stream::eof() const {
    return eof_ || buffer_.empty() || buffer_.front() == '\0';
}

char Stream::look_ahead(size_t offset) {
    fill_buffer(offset);
    if (offset >= buffer_.size()) {
        return '\0';
    }
    return buffer_[offset];
}

} // namespace YAML
