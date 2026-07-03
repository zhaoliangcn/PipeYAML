#include "pipeyaml/emitter.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace YAML {

// Global manipulator instances
const EmitterManip BeginMap = EmitterManip::BeginMap;
const EmitterManip EndMap = EmitterManip::EndMap;
const EmitterManip BeginSeq = EmitterManip::BeginSeq;
const EmitterManip EndSeq = EmitterManip::EndSeq;
const EmitterManip Key = EmitterManip::Key;
const EmitterManip Value = EmitterManip::Value;
const EmitterManip Flow = EmitterManip::Flow;
const EmitterManip Block = EmitterManip::Block;
const EmitterManip DoubleQuoted = EmitterManip::DoubleQuoted;
const EmitterManip SingleQuoted = EmitterManip::SingleQuoted;
const EmitterManip Literal = EmitterManip::Literal;
const EmitterManip Folded = EmitterManip::Folded;

// ===========================================================================
// Construction
// ===========================================================================
Emitter::Emitter() {
    push_state(State::Ready);
    indent_level_ = 0;
}

// ===========================================================================
// State management
// ===========================================================================
void Emitter::push_state(State s) {
    StateEntry e;
    e.state = s;
    e.indent = indent_level_;
    e.needs_comma = false;
    e.needs_key = (s == State::InMap || s == State::InFlowMap);
    state_stack_.push_back(e);
}

void Emitter::pop_state() {
    if (!state_stack_.empty()) {
        state_stack_.pop_back();
    }
}

Emitter::StateEntry& Emitter::current_state() {
    if (state_stack_.empty()) {
        static StateEntry default_state{State::Ready, 0, false, false};
        return default_state;
    }
    return state_stack_.back();
}

Emitter::State Emitter::current_top() const {
    return state_stack_.empty() ? State::Ready : state_stack_.back().state;
}

// ===========================================================================
// Output helpers
// ===========================================================================
void Emitter::output(const std::string& text) {
    output_ << text;
}

void Emitter::output_char(char c) {
    output_ << c;
}

void Emitter::output_newline() {
    output_ << '\n';
}

void Emitter::output_indent() {
    for (int i = 0; i < indent_level_ * indent_size_; ++i) {
        output_ << ' ';
    }
}

void Emitter::increase_indent() {
    indent_level_++;
}

void Emitter::decrease_indent() {
    indent_level_ = std::max(0, indent_level_ - 1);
}

// ===========================================================================
// Error handling
// ===========================================================================
void Emitter::set_error(const std::string& msg) {
    has_error_ = true;
    last_error_ = msg;
}

// ===========================================================================
// Output retrieval
// ===========================================================================
std::string Emitter::str() const {
    return output_.str();
}

const std::string& Emitter::c_str() const {
    static std::string s;
    s = output_.str();
    return s;
}

// ===========================================================================
// Quoting helpers
// ===========================================================================
bool Emitter::needs_quotes(const std::string& value) const {
    if (value.empty()) return true;

    // YAML indicators that require quoting
    const char* special_starts = "&*!|>?'\"%@`";
    if (value[0] == '-' && (value.size() == 1 || value[1] == ' ' || value[1] == '\t')) {
        return true;
    }
    if (value[0] == ':' && (value.size() == 1 || value[1] == ' ' || value[1] == '\t')) {
        return true;
    }
    if (value[0] == '#') return true;
    if (value[0] == '[' || value[0] == ']' || value[0] == '{' || value[0] == '}') {
        return true;
    }
    if (value[0] == ',') return true;

    // Check special prefixes
    if (value.size() >= 3 && value.substr(0, 3) == "---") return true;
    if (value.size() >= 3 && value.substr(0, 3) == "...") return true;
    if (value.size() >= 4 && value.substr(0, 4) == "true") return true;
    if (value.size() >= 5 && value.substr(0, 5) == "false") return true;
    if (value.size() >= 3 && (value == "yes" || value == "no" || value == "on" || value == "off")) return true;
    if (value.size() >= 4 && (value.substr(0, 4) == "null" || value.substr(0, 4) == "Null" || value.substr(0, 4) == "NULL")) return true;
    if (value == "~") return true;
    if (value == ".inf" || value == ".Inf" || value == ".INF") return true;
    if (value == ".nan" || value == ".NaN" || value == ".NAN") return true;

    // Check for special characters
    for (char c : value) {
        if (c == '\n' || c == '\r' || c == '\x1B') return true;
    }

    // Check for leading/trailing whitespace
    if (!value.empty() && (value.front() == ' ' || value.back() == ' ')) return true;

    return false;
}

std::string Emitter::escape_string(const std::string& value) const {
    std::string result;
    result += '"';
    for (char c : value) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    result += '"';
    return result;
}

// ===========================================================================
// Scalar emission
// ===========================================================================
void Emitter::emit_scalar(const std::string& value) {
    if (needs_quotes(value)) {
        output(escape_string(value));
    } else {
        output(value);
    }
}

// ===========================================================================
// Node emission
// ===========================================================================
void Emitter::emit_node(const Node& node) {
    if (!node.is_defined()) {
        output(null_format_);
        return;
    }

    switch (node.type()) {
        case NodeType::Null:
            output(null_format_);
            break;
        case NodeType::Scalar:
            emit_scalar(node.scalar());
            break;
        case NodeType::Sequence:
            emit_sequence(node);
            break;
        case NodeType::Map:
            emit_map(node);
            break;
    }
}

void Emitter::emit_sequence(const Node& node) {
    StateEntry& st = current_state();
    bool is_flow = (st.state == State::InFlowSequence || st.state == State::InFlowMap);

    if (is_flow) {
        output("[");
        for (size_t i = 0; i < node.size(); ++i) {
            if (i > 0) output(", ");
            emit_node(node[i]);
        }
        output("]");
    } else {
        for (size_t i = 0; i < node.size(); ++i) {
            output_newline();
            output_indent();
            output("- ");
            increase_indent();
            emit_node(node[i]);
            decrease_indent();
        }
    }
}

void Emitter::emit_map(const Node& node) {
    const auto& map_data = node.get_data()->map();

    for (size_t i = 0; i < map_data.size(); ++i) {
        const auto& pair = map_data[i];
        Node key_node(pair.first);
        Node value_node(pair.second);

        output_newline();
        output_indent();

        // Emit key
        if (key_node.is_scalar()) {
            emit_scalar(key_node.scalar());
        } else {
            // Complex key - wrap with '?'
            output("? ");
            emit_node(key_node);
            output_newline();
            output_indent();
        }

        output(": ");

        if (value_node.is_defined()) {
            if (value_node.is_scalar() || value_node.is_null()) {
                emit_node(value_node);
            } else {
                increase_indent();
                emit_node(value_node);
                decrease_indent();
            }
        }
    }
}

void Emitter::emit(const Node& node) {
    // Reset state
    state_stack_.clear();
    push_state(State::Ready);
    indent_level_ = 0;
    has_error_ = false;
    output_.str("");
    output_.clear();

    emit_node(node);
    output_newline();
}

// ===========================================================================
// Stream operators
// ===========================================================================
Emitter& Emitter::operator<<(const Node& node) {
    emit(node);
    return *this;
}

Emitter& Emitter::operator<<(const EmitterManip& manip) {
    switch (manip) {
        case EmitterManip::BeginSeq:
            push_state(State::InSequence);
            break;
        case EmitterManip::EndSeq:
            pop_state();
            break;
        case EmitterManip::BeginMap:
            push_state(State::InMap);
            break;
        case EmitterManip::EndMap:
            pop_state();
            break;
        case EmitterManip::Key:
            current_state().needs_key = true;
            break;
        case EmitterManip::Value:
            output(": ");
            current_state().needs_key = false;
            break;
        default:
            break;
    }
    return *this;
}

Emitter& Emitter::operator<<(const std::string& value) {
    StateEntry& st = current_state();
    if (st.state == State::InSequence) {
        output_newline();
        output_indent();
        output("- ");
        emit_scalar(value);
    } else if (st.needs_key) {
        emit_scalar(value);
        st.needs_key = false;
    } else {
        emit_scalar(value);
    }
    return *this;
}

Emitter& Emitter::operator<<(const char* value) {
    return *this << std::string(value);
}

Emitter& Emitter::operator<<(int value) { return *this << std::to_string(value); }
Emitter& Emitter::operator<<(unsigned int value) { return *this << std::to_string(value); }
Emitter& Emitter::operator<<(long value) { return *this << std::to_string(value); }
Emitter& Emitter::operator<<(unsigned long value) { return *this << std::to_string(value); }
Emitter& Emitter::operator<<(long long value) { return *this << std::to_string(value); }
Emitter& Emitter::operator<<(unsigned long long value) { return *this << std::to_string(value); }

Emitter& Emitter::operator<<(float value) {
    std::ostringstream oss;
    oss.precision(8);
    oss << value;
    return *this << oss.str();
}

Emitter& Emitter::operator<<(double value) {
    std::ostringstream oss;
    oss.precision(15);
    oss << value;
    return *this << oss.str();
}

Emitter& Emitter::operator<<(bool value) {
    return *this << (value ? "true" : "false");
}

// ===========================================================================
// Dump
// ===========================================================================
std::string Dump(const Node& node) {
    Emitter emitter;
    emitter.emit(node);
    return emitter.str();
}

void Dump(const Node& node, std::ostream& output) {
    output << Dump(node);
}

} // namespace YAML
