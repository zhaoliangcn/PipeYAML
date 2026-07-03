#ifndef PYPEYAML_STREAM_H
#define PYPEYAML_STREAM_H

#include "exception.h"

#include <deque>
#include <istream>
#include <memory>
#include <string>

namespace YAML {

/// Low-level character stream abstraction with encoding detection and position tracking.
class Stream {
public:
    explicit Stream(std::istream& input);
    explicit Stream(std::string_view input);

    // Character access
    char peek() const;                    // Look at next char without consuming
    char get();                           // Consume and return next char
    void eat(size_t n = 1);              // Consume n chars without returning
    bool eof() const;                     // End of stream reached

    // Position info
    Mark get_mark() const { return mark_; }

    // Look-ahead: peek at character relative to current position (0 = next)
    char look_ahead(size_t offset = 0);

private:
    void init(std::string_view data);
    void detect_encoding();
    void fill_buffer(size_t min_size);

    std::deque<char> buffer_;
    Mark mark_;
    bool eof_ = false;
};

} // namespace YAML

#endif // PYPEYAML_STREAM_H
