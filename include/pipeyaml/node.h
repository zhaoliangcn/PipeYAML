#ifndef PYPEYAML_NODE_H
#define PYPEYAML_NODE_H

#include "exception.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace YAML {

// =============================================================================
// NodeType
// =============================================================================
enum class NodeType : uint8_t {
    Null = 0,
    Scalar,
    Sequence,
    Map
};

// =============================================================================
// Forward declarations
// =============================================================================
struct node_data;
struct node_ref;
class Node;
class Emitter;

// =============================================================================
// node_data - the actual data storage layer (ref-counted by node_ref)
// =============================================================================
struct node_data {
    using MapPair = std::pair<std::shared_ptr<node_data>, std::shared_ptr<node_data>>;

    NodeType type_ = NodeType::Null;
    bool is_defined_ = false;

    // Scalar
    std::string scalar_;

    // Variant-like storage: at any time only one of these is active
    std::vector<std::shared_ptr<node_data>> sequence_;
    std::vector<MapPair> map_;

    // Cached converted values for fast repeated access
    mutable std::optional<int> cached_int_;
    mutable std::optional<double> cached_double_;
    mutable std::optional<bool> cached_bool_;

    node_data() = default;
    explicit node_data(NodeType t, bool defined = true)
        : type_(t), is_defined_(defined) {}

    // Scalar access
    const std::string& scalar() const { return scalar_; }
    void set_scalar(const std::string& val) {
        type_ = NodeType::Scalar;
        is_defined_ = true;
        scalar_ = val;
        cached_int_.reset();
        cached_double_.reset();
        cached_bool_.reset();
    }
    void set_scalar(std::string&& val) {
        type_ = NodeType::Scalar;
        is_defined_ = true;
        scalar_ = std::move(val);
        cached_int_.reset();
        cached_double_.reset();
        cached_bool_.reset();
    }

    // Sequence access
    const std::vector<std::shared_ptr<node_data>>& sequence() const { return sequence_; }
    void sequence_push_back(std::shared_ptr<node_data> child) {
        type_ = NodeType::Sequence;
        sequence_.push_back(std::move(child));
    }
    void sequence_remove(std::size_t index) {
        if (index < sequence_.size())
            sequence_.erase(sequence_.begin() + static_cast<long long>(index));
    }
    std::size_t sequence_size() const { return sequence_.size(); }

    // Map access
    const std::vector<MapPair>& map() const { return map_; }
    void map_insert(std::shared_ptr<node_data> key, std::shared_ptr<node_data> value) {
        type_ = NodeType::Map;
        map_.emplace_back(std::move(key), std::move(value));
    }
    void map_remove(const std::shared_ptr<node_data>& key) {
        for (auto it = map_.begin(); it != map_.end(); ++it) {
            if (it->first == key) {
                map_.erase(it);
                break;
            }
        }
    }
    std::size_t map_size() const { return map_.size(); }
    // Linear search map key
    std::shared_ptr<node_data> map_find(const std::string& key) const {
        for (const auto& pair : map_) {
            if (pair.first && pair.first->type_ == NodeType::Scalar
                && pair.first->scalar_ == key) {
                return pair.second;
            }
        }
        return nullptr;
    }
};

// =============================================================================
// node_ref - reference counting layer
// =============================================================================
struct node_ref {
    std::shared_ptr<node_data> data_;

    explicit node_ref(std::shared_ptr<node_data> data = nullptr)
        : data_(std::move(data)) {}
};

// =============================================================================
// Node - public user-level handle (value semantics)
// =============================================================================
class Node {
public:
    // Construction
    Node();
    explicit Node(NodeType type);
    explicit Node(std::shared_ptr<node_data> data);

    // Type queries
    NodeType type() const;
    bool is_defined() const;
    bool is_null() const;
    bool is_scalar() const;
    bool is_sequence() const;
    bool is_map() const;

    // Scalar access
    const std::string& scalar() const;
    void set_scalar(const std::string& value);
    void set_scalar(std::string&& value);  // rvalue: avoid string copy

    // Sequence operations
    Node operator[](std::size_t index) const;
    void push_back(const Node& node);
    void push_back(Node&& node);             // rvalue: move shared_ptr
    void push_back(NodeType type);           // fast path: create child directly
    void push_back(const std::string& value);  // fast path: create scalar directly
    void remove(std::size_t index);
    std::size_t size() const;

    // Map operations (non-const: can create entries)
    Node operator[](const std::string& key);
    // No operator[](const char*) to avoid ambiguity with seq[0]

    // Map operations (const: read-only)
    Node operator[](const std::string& key) const;
    void remove(const std::string& key);

    // Type conversion
    template<typename T> T as() const;
    template<typename T> T as(const T& fallback) const;

    template<typename T>
    Node& operator=(const T& value);

    // Rvalue string overload — avoids copy for temporaries
    Node& operator=(std::string&& value);

    // Custom copy assignment (declared; defined after convert<Node>)
    Node& operator=(const Node& other);

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const { return !(*this == other); }

    std::shared_ptr<node_data> get_data() const { return data_; }
    void set_data(std::shared_ptr<node_data> data) { data_ = std::move(data); }
    void reset() { data_ = std::make_shared<node_data>(); }

private:
    friend class Emitter;
    std::shared_ptr<node_data> data_;
};

// =============================================================================
// convert<T> - type conversion traits
// =============================================================================
template<typename T>
struct convert {
    static bool decode(const Node& node, T& value);
    static bool encode(T& value, Node& node);
};

// Built-in type specializations (declared, defined in convert.cpp)
template<> struct convert<bool> {
    static bool decode(const Node& node, bool& value);
    static bool encode(bool value, Node& node);
};
template<> struct convert<int> {
    static bool decode(const Node& node, int& value);
    static bool encode(int value, Node& node);
};
template<> struct convert<unsigned int> {
    static bool decode(const Node& node, unsigned int& value);
    static bool encode(unsigned int value, Node& node);
};
template<> struct convert<long> {
    static bool decode(const Node& node, long& value);
    static bool encode(long value, Node& node);
};
template<> struct convert<unsigned long> {
    static bool decode(const Node& node, unsigned long& value);
    static bool encode(unsigned long value, Node& node);
};
template<> struct convert<long long> {
    static bool decode(const Node& node, long long& value);
    static bool encode(long long value, Node& node);
};
template<> struct convert<unsigned long long> {
    static bool decode(const Node& node, unsigned long long& value);
    static bool encode(unsigned long long value, Node& node);
};
template<> struct convert<float> {
    static bool decode(const Node& node, float& value);
    static bool encode(float value, Node& node);
};
template<> struct convert<double> {
    static bool decode(const Node& node, double& value);
    static bool encode(double value, Node& node);
};
template<> struct convert<std::string> {
    static bool decode(const Node& node, std::string& value);
    static bool encode(const std::string& value, Node& node);
};

// Container conversions (declared)
template<typename T> struct convert<std::vector<T>> {
    static bool decode(const Node& node, std::vector<T>& value);
    static bool encode(const std::vector<T>& value, Node& node);
};

// Node-to-Node copy (inline full definition)
template<> struct convert<Node> {
    static bool decode(const Node& node, Node& value) {
        if (!node.get_data()) return false;
        auto data = std::make_shared<node_data>();
        data->type_ = node.type();
        data->is_defined_ = node.is_defined();
        if (node.is_scalar()) data->scalar_ = node.scalar();
        value = Node(data);
        return true;
    }
    static bool encode(Node& value, Node& node) {
        // Modify node's data in-place so shared references (map entries) are updated
        auto data = node.get_data();
        if (!data) data = std::make_shared<node_data>();
        data->type_ = value.type();
        data->is_defined_ = value.is_defined();
        data->scalar_.clear();
        data->sequence_.clear();
        data->map_.clear();
        if (value.is_scalar()) data->scalar_ = value.scalar();
        if (value.is_sequence()) {
            for (size_t i = 0; i < value.size(); ++i)
                data->sequence_push_back(value[i].get_data());
        }
        if (value.is_map()) {
            for (const auto& pair : value.get_data()->map())
                data->map_.push_back(pair);
        }
        return true;
    }
};

// Copy assignment for Node (must be defined after convert<Node>)
inline Node& Node::operator=(const Node& other) {
    convert<Node>::encode(const_cast<Node&>(other), *this);
    return *this;
}

// =============================================================================
// Node template method implementations (inline)
// =============================================================================
template<typename T>
T Node::as() const {
    T value{};
    if (!convert<T>::decode(*this, value)) {
        throw BadConversionException("Requested type does not match node data");
    }
    return value;
}

template<typename T>
T Node::as(const T& fallback) const {
    T value{};
    if (convert<T>::decode(*this, value)) {
        return value;
    }
    return fallback;
}

template<typename T>
Node& Node::operator=(const T& value) {
    convert<T>::encode(const_cast<T&>(value), *this);
    return *this;
}

// Rvalue string overload — moves instead of copying
inline Node& Node::operator=(std::string&& value) {
    if (!data_) data_ = std::make_shared<node_data>();
    data_->type_ = NodeType::Scalar;
    data_->is_defined_ = true;
    data_->scalar_ = std::move(value);
    data_->cached_int_.reset();
    data_->cached_double_.reset();
    data_->cached_bool_.reset();
    return *this;
}

} // namespace YAML

#endif // PYPEYAML_NODE_H
