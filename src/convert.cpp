#include "pipeyaml/node.h"

#include <cstdlib>
#include <cmath>
#include <limits>
#include <sstream>

namespace YAML {

// ---------------------------------------------------------------------------
// bool
// ---------------------------------------------------------------------------
bool convert<bool>::decode(const Node& node, bool& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();

    // YAML 1.2 boolean values
    if (s == "true" || s == "True" || s == "TRUE" || s == "yes" || s == "Yes" || s == "YES" || s == "on" || s == "On" || s == "ON") {
        value = true; return true;
    }
    if (s == "false" || s == "False" || s == "FALSE" || s == "no" || s == "No" || s == "NO" || s == "off" || s == "Off" || s == "OFF") {
        value = false; return true;
    }
    return false;
}

bool convert<bool>::encode(bool value, Node& node) {
    node.set_scalar(value ? "true" : "false");
    return true;
}

// ---------------------------------------------------------------------------
// int
// ---------------------------------------------------------------------------
bool convert<int>::decode(const Node& node, int& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 0);
    if (end == s.c_str() || *end != '\0') return false;
    if (v < static_cast<long>(std::numeric_limits<int>::min()) ||
        v > static_cast<long>(std::numeric_limits<int>::max())) return false;
    value = static_cast<int>(v);
    return true;
}

bool convert<int>::encode(int value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// unsigned int
// ---------------------------------------------------------------------------
bool convert<unsigned int>::decode(const Node& node, unsigned int& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    unsigned long v = std::strtoul(s.c_str(), &end, 0);
    if (end == s.c_str() || *end != '\0') return false;
    if (v > std::numeric_limits<unsigned int>::max()) return false;
    value = static_cast<unsigned int>(v);
    return true;
}

bool convert<unsigned int>::encode(unsigned int value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// long
// ---------------------------------------------------------------------------
bool convert<long>::decode(const Node& node, long& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    value = std::strtol(s.c_str(), &end, 0);
    return end != s.c_str() && *end == '\0';
}

bool convert<long>::encode(long value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// unsigned long
// ---------------------------------------------------------------------------
bool convert<unsigned long>::decode(const Node& node, unsigned long& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    value = std::strtoul(s.c_str(), &end, 0);
    return end != s.c_str() && *end == '\0';
}

bool convert<unsigned long>::encode(unsigned long value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// long long
// ---------------------------------------------------------------------------
bool convert<long long>::decode(const Node& node, long long& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    value = std::strtoll(s.c_str(), &end, 0);
    return end != s.c_str() && *end == '\0';
}

bool convert<long long>::encode(long long value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// unsigned long long
// ---------------------------------------------------------------------------
bool convert<unsigned long long>::decode(const Node& node, unsigned long long& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    char* end = nullptr;
    value = std::strtoull(s.c_str(), &end, 0);
    return end != s.c_str() && *end == '\0';
}

bool convert<unsigned long long>::encode(unsigned long long value, Node& node) {
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// float
// ---------------------------------------------------------------------------
bool convert<float>::decode(const Node& node, float& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    if (s == ".inf" || s == ".Inf" || s == ".INF") { value = std::numeric_limits<float>::infinity(); return true; }
    if (s == "-.inf" || s == "-.Inf" || s == "-.INF") { value = -std::numeric_limits<float>::infinity(); return true; }
    if (s == ".nan" || s == ".NaN" || s == ".NAN") { value = std::numeric_limits<float>::quiet_NaN(); return true; }
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0') return false;
    value = static_cast<float>(v);
    return true;
}

bool convert<float>::encode(float value, Node& node) {
    if (std::isnan(value)) { node.set_scalar(".nan"); return true; }
    if (std::isinf(value)) { node.set_scalar(value > 0 ? ".inf" : "-.inf"); return true; }
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// double
// ---------------------------------------------------------------------------
bool convert<double>::decode(const Node& node, double& value) {
    if (!node.is_scalar()) return false;
    const auto& s = node.scalar();
    if (s == ".inf" || s == ".Inf" || s == ".INF") { value = std::numeric_limits<double>::infinity(); return true; }
    if (s == "-.inf" || s == "-.Inf" || s == "-.INF") { value = -std::numeric_limits<double>::infinity(); return true; }
    if (s == ".nan" || s == ".NaN" || s == ".NAN") { value = std::numeric_limits<double>::quiet_NaN(); return true; }
    char* end = nullptr;
    value = std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

bool convert<double>::encode(double value, Node& node) {
    if (std::isnan(value)) { node.set_scalar(".nan"); return true; }
    if (std::isinf(value)) { node.set_scalar(value > 0 ? ".inf" : "-.inf"); return true; }
    node.set_scalar(std::to_string(value));
    return true;
}

// ---------------------------------------------------------------------------
// string
// ---------------------------------------------------------------------------
bool convert<std::string>::decode(const Node& node, std::string& value) {
    if (!node.is_scalar()) return false;
    value = node.scalar();
    return true;
}

bool convert<std::string>::encode(const std::string& value, Node& node) {
    node.set_scalar(value);
    return true;
}

// ---------------------------------------------------------------------------
// vector<T> (container)
// ---------------------------------------------------------------------------
template<typename T>
bool convert<std::vector<T>>::decode(const Node& node, std::vector<T>& value) {
    if (!node.is_sequence()) return false;
    value.clear();
    value.reserve(node.size());
    for (std::size_t i = 0; i < node.size(); ++i) {
        T elem;
        if (!convert<T>::decode(node[i], elem)) return false;
        value.push_back(std::move(elem));
    }
    return true;
}

template<typename T>
bool convert<std::vector<T>>::encode(const std::vector<T>& value, Node& node) {
    for (const auto& elem : value) {
        Node child;
        convert<T>::encode(const_cast<T&>(elem), child);
        node.push_back(child);
    }
    return true;
}

// Explicit instantiations for common container types
template struct convert<std::vector<bool>>;
template struct convert<std::vector<int>>;
template struct convert<std::vector<double>>;
template struct convert<std::vector<std::string>>;

} // namespace YAML
