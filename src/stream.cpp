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
    pos_ = 0;
    mark_ = Mark{1, 1, 0};

    if (buffer_.size() > 1) {
        detect_encoding();
    }
}

// ---------------------------------------------------------------------------
// Encoding detection
// ---------------------------------------------------------------------------
void Stream::detect_encoding() {
    const unsigned char* b = reinterpret_cast<const unsigned char*>(buffer_.data());

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
        buffer_.erase(0, 3);
        mark_.pos = 3;
        mark_.column = 1 - 3;
    }
}

// ---------------------------------------------------------------------------
// Character access
// ---------------------------------------------------------------------------
char Stream::peek() const {
    if (pos_ >= buffer_.size()) return '\0';
    if (buffer_[pos_] == '\0') return '\0';
    return buffer_[pos_];
}

char Stream::get() {
    if (pos_ >= buffer_.size() || buffer_[pos_] == '\0') {
        eof_ = true;
        return '\0';
    }

    char c = buffer_[pos_++];

    // Advance position tracking
    if (c == '\n') {
        mark_.line++;
        mark_.column = 1;
    } else {
        mark_.column++;
    }
    mark_.pos++;

    return c;
}

void Stream::eat(size_t n) {
    pos_ += n;
    // Update position tracking approximately
    mark_.column += static_cast<int>(n);
    mark_.pos += static_cast<int>(n);
}

bool Stream::eof() const {
    return eof_ || pos_ >= buffer_.size() || buffer_[pos_] == '\0';
}

char Stream::look_ahead(size_t offset) {
    size_t idx = pos_ + offset;
    if (idx >= buffer_.size()) {
        return '\0';
    }
    return buffer_[idx];
}

std::string_view Stream::current_segment() const {
    if (pos_ >= buffer_.size()) return {};
    return std::string_view(buffer_.data() + pos_, buffer_.size() - pos_);
}

void Stream::advance(size_t n) {
    pos_ += n;
    mark_.column += static_cast<int>(n);
    mark_.pos += static_cast<int>(n);
}

} // namespace YAML
