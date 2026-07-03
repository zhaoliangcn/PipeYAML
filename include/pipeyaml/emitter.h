#ifndef PYPEYAML_EMITTER_H
#define PYPEYAML_EMITTER_H

#include "exception.h"
#include "node.h"

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace YAML {

// =============================================================================
// Emitter manipulators
// =============================================================================
enum class EmitterManip : uint8_t {
    BeginMap,
    EndMap,
    BeginSeq,
    EndSeq,
    Key,
    Value,
    Flow,
    Block,

    // Scalar style hints
    DoubleQuoted,
    SingleQuoted,
    Literal,     // '|'
    Folded,      // '>'

    // Formatting
    Default,
    _Count
};

// Global manipulator constants
extern const EmitterManip BeginMap;
extern const EmitterManip EndMap;
extern const EmitterManip BeginSeq;
extern const EmitterManip EndSeq;
extern const EmitterManip Key;
extern const EmitterManip Value;
extern const EmitterManip Flow;
extern const EmitterManip Block;
extern const EmitterManip DoubleQuoted;
extern const EmitterManip SingleQuoted;
extern const EmitterManip Literal;
extern const EmitterManip Folded;

// =============================================================================
// Emitter - serializes Node tree back to YAML text
// =============================================================================
class Emitter {
public:
    Emitter();

    // Stream output interface
    Emitter& operator<<(const Node& node);
    Emitter& operator<<(const EmitterManip& manip);
    Emitter& operator<<(const std::string& value);
    Emitter& operator<<(int value);
    Emitter& operator<<(unsigned int value);
    Emitter& operator<<(long value);
    Emitter& operator<<(unsigned long value);
    Emitter& operator<<(long long value);
    Emitter& operator<<(unsigned long long value);
    Emitter& operator<<(float value);
    Emitter& operator<<(double value);
    Emitter& operator<<(bool value);
    Emitter& operator<<(const char* value);

    // Output
    std::string str() const;
    const std::string& c_str() const;

    // Formatting control
    void set_indent_size(int spaces) { indent_size_ = spaces; }
    void set_output_width(int width) { output_width_ = width; }
    void set_bool_format(const std::string& fmt) { bool_format_ = fmt; }
    void set_null_format(const std::string& fmt) { null_format_ = fmt; }

    // State check
    bool good() const { return !has_error_; }
    std::string get_last_error() const { return last_error_; }

    // Main emit entry point
    void emit(const Node& node);

private:
    // State machine
    enum class State : uint8_t {
        Ready,
        InSequence,
        InMap,
        InFlowSequence,
        InFlowMap,
        EndOfSequence,
        EndOfMap,
    };

    struct StateEntry {
        State state;
        int indent;
        bool needs_comma;
        bool needs_key;
    };

    // Emit helpers
    void emit_node(const Node& node);
    void emit_scalar(const std::string& value);
    void emit_sequence(const Node& node);
    void emit_map(const Node& node);

    void increase_indent();
    void decrease_indent();
    void output_newline();
    void output_indent();
    void output(const std::string& text);
    void output_char(char c);

    bool needs_quotes(const std::string& value) const;
    std::string escape_string(const std::string& value) const;

    // State management
    void push_state(State s);
    void pop_state();
    StateEntry& current_state();
    State current_top() const;

    // Error handling
    void set_error(const std::string& msg);

    std::vector<StateEntry> state_stack_;
    int indent_level_ = 0;
    int indent_size_ = 2;
    int output_width_ = 80;
    std::string bool_format_ = "true/false";
    std::string null_format_ = "null";
    bool has_error_ = false;
    std::string last_error_;
    std::ostringstream output_;
    bool needs_separator_ = false;
};

// =============================================================================
// Dump: one-shot serialization
// =============================================================================
std::string Dump(const Node& node);
void Dump(const Node& node, std::ostream& output);

} // namespace YAML

#endif // PYPEYAML_EMITTER_H
