#ifndef PYPEYAML_EXCEPTION_H
#define PYPEYAML_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace YAML {

/// Position information for error reporting
struct Mark {
    int line = 0;
    int column = 0;
    int pos = 0;       // absolute position in input

    Mark() = default;
    Mark(int l, int c, int p) : line(l), column(c), pos(p) {}
};

/// Base exception for all PipeYAML errors
class Exception : public std::runtime_error {
public:
    Exception(const std::string& msg, const Mark& mark = Mark())
        : std::runtime_error(build_message(msg, mark)), mark_(mark) {}
    
    const Mark& mark() const { return mark_; }

private:
    static std::string build_message(const std::string& msg, const Mark& mark) {
        if (mark.line == 0 && mark.column == 0)
            return msg;
        return "[" + std::to_string(mark.line) + ":" + std::to_string(mark.column) + "] " + msg;
    }

    Mark mark_;
};

/// Scanner (lexical) errors
class ScannerException : public Exception {
public:
    using Exception::Exception;
};

/// Parser (syntax) errors
class ParserException : public Exception {
public:
    using Exception::Exception;
};

/// Emitter errors
class EmitterException : public Exception {
public:
    using Exception::Exception;
};

/// Representation / type conversion errors
class RepresentationException : public Exception {
public:
    using Exception::Exception;
};

/// Bad type conversion
class BadConversionException : public RepresentationException {
public:
    BadConversionException(const std::string& msg, const Mark& mark = Mark())
        : RepresentationException(msg, mark) {}
};

/// Deep recursion guard
class DeepRecursionException : public ParserException {
public:
    DeepRecursionException()
        : ParserException("Maximum recursion depth exceeded") {}
};

} // namespace YAML

#endif // PYPEYAML_EXCEPTION_H
