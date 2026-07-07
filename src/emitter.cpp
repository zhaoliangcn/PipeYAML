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
    output_.append(text);
}

void Emitter::output_char(char c) {
    output_ += c;
}

void Emitter::output_newline() {
    output_ += '\n';
}

void Emitter::output_indent() {
    int n = indent_level_ * indent_size_;
    if (n > 0) output_.append(n, ' ');
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
    return output_;
}

const std::string& Emitter::c_str() const {
    return output_;
}

// ===========================================================================
// Quoting helpers
// ===========================================================================
bool Emitter::needs_quotes(const std::string& value) const {
    if (value.empty()) return true;

    // Fast path: check first char for common indicators
    char first = value[0];
    if (first == '#' || first == '[' || first == ']' || first == '{' || first == '}'
        || first == ',' || first == '!' || first == '&' || first == '*'
        || first == '|' || first == '>' || first == '\'' || first == '"'
        || first == '%' || first == '@' || first == '`')
        return true;

    // Dash/colon followed by space/tab indicates block sequence/map indicator
    if ((first == '-' || first == ':')
        && (value.size() == 1 || value[1] == ' ' || value[1] == '\t'))
        return true;

    // Check special prefixes
    if (value.size() >= 3 && value[0] == '-' && value[1] == '-' && value[2] == '-')
        return true;
    if (value.size() >= 3 && value[0] == '.' && value[1] == '.' && value[2] == '.')
        return true;

    // Boolean/null keywords (only if whole string matches)
    if (value == "true" || value == "True" || value == "TRUE"
        || value == "false" || value == "False" || value == "FALSE"
        || value == "yes" || value == "Yes" || value == "YES"
        || value == "no" || value == "No" || value == "NO"
        || value == "on" || value == "On" || value == "ON"
        || value == "off" || value == "Off" || value == "OFF"
        || value == "null" || value == "Null" || value == "NULL"
        || value == "~"
        || value == ".inf" || value == ".Inf" || value == ".INF"
        || value == ".nan" || value == ".NaN" || value == ".NAN")
        return true;

    // Check for leading/trailing whitespace
    if (first == ' ' || value.back() == ' ') return true;

    // Check for special newline/escape characters (SIMD-optimized find)
    if (value.find('\n') != std::string::npos
        || value.find('\r') != std::string::npos
        || value.find('\x1B') != std::string::npos)
        return true;

    return false;
}

std::string Emitter::escape_string(const std::string& value) const {
    std::string result;
    result.reserve(value.size() + 8);  // pre-reserve for quotes + escapes
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

void Emitter::emit_node_data(const std::shared_ptr<node_data>& data) {
    if (!data || !data->is_defined_) {
        output(null_format_);
        return;
    }

    switch (data->type_) {
        case NodeType::Null:
            output(null_format_);
            break;
        case NodeType::Scalar:
            emit_scalar(data->scalar());
            break;
        case NodeType::Sequence:
            {
                const auto& seq = data->sequence();
                StateEntry& st = current_state();
                bool is_flow = (st.state == State::InFlowSequence || st.state == State::InFlowMap);
                if (is_flow) {
                    output("[");
                    for (size_t i = 0; i < seq.size(); ++i) {
                        if (i > 0) output(", ");
                        emit_node_data(seq[i]);
                    }
                    output("]");
                } else {
                    for (size_t i = 0; i < seq.size(); ++i) {
                        output_newline();
                        output_indent();
                        output("- ");
                        increase_indent();
                        emit_node_data(seq[i]);
                        decrease_indent();
                    }
                }
            }
            break;
        case NodeType::Map:
            {
                const auto& map_data = data->map();
                for (size_t i = 0; i < map_data.size(); ++i) {
                    const auto& pair = map_data[i];
                    output_newline();
                    output_indent();

                    if (pair.first && pair.first->type_ == NodeType::Scalar) {
                        emit_scalar(pair.first->scalar());
                    } else {
                        output("? ");
                        if (pair.first) emit_node_data(pair.first);
                        output_newline();
                        output_indent();
                    }

                    output(": ");

                    if (pair.second && pair.second->is_defined_) {
                        if (pair.second->type_ == NodeType::Scalar) {
                            emit_scalar(pair.second->scalar());
                        } else if (pair.second->type_ == NodeType::Null) {
                            output(null_format_);
                        } else {
                            increase_indent();
                            emit_node_data(pair.second);
                            decrease_indent();
                        }
                    }
                }
            }
            break;
    }
}

void Emitter::emit_sequence(const Node& node) {
    StateEntry& st = current_state();
    bool is_flow = (st.state == State::InFlowSequence || st.state == State::InFlowMap);
    auto data = node.get_data();
    if (!data) return;
    const auto& seq = data->sequence();

    if (is_flow) {
        output("[");
        for (size_t i = 0; i < seq.size(); ++i) {
            if (i > 0) output(", ");
            emit_node_data(seq[i]);
        }
        output("]");
    } else {
        for (size_t i = 0; i < seq.size(); ++i) {
            output_newline();
            output_indent();
            output("- ");
            increase_indent();
            emit_node_data(seq[i]);
            decrease_indent();
        }
    }
}

void Emitter::emit_map(const Node& node) {
    auto data = node.get_data();
    if (!data) return;
    emit_node_data(data);
}

void Emitter::emit(const Node& node) {
    // Reset state
    state_stack_.clear();
    push_state(State::Ready);
    indent_level_ = 0;
    has_error_ = false;
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
