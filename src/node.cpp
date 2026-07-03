#include "pipeyaml/node.h"

#include <algorithm>

namespace YAML {

// ---------------------------------------------------------------------------
// node_data
// ---------------------------------------------------------------------------
void node_data::map_remove(const std::shared_ptr<node_data>& key) {
    auto it = std::remove_if(map_.begin(), map_.end(),
        [&](const MapPair& p) { return p.first == key; });
    map_.erase(it, map_.end());
}

// ---------------------------------------------------------------------------
// Node construction
// ---------------------------------------------------------------------------
Node::Node()
    : data_(std::make_shared<node_data>()) {}

Node::Node(NodeType type)
    : data_(std::make_shared<node_data>(type)) {}

Node::Node(std::shared_ptr<node_data> data)
    : data_(std::move(data)) {}

// ---------------------------------------------------------------------------
// Type queries
// ---------------------------------------------------------------------------
NodeType Node::type() const { return data_ ? data_->type_ : NodeType::Null; }
bool Node::is_defined() const { return data_ && data_->is_defined_; }
bool Node::is_null() const { return type() == NodeType::Null; }
bool Node::is_scalar() const { return type() == NodeType::Scalar; }
bool Node::is_sequence() const { return type() == NodeType::Sequence; }
bool Node::is_map() const { return type() == NodeType::Map; }

// ---------------------------------------------------------------------------
// Scalar
// ---------------------------------------------------------------------------
const std::string& Node::scalar() const {
    if (!data_ || data_->type_ != NodeType::Scalar)
        throw BadConversionException("Node is not a scalar");
    return data_->scalar_;
}

void Node::set_scalar(const std::string& value) {
    if (!data_) data_ = std::make_shared<node_data>();
    data_->set_scalar(value);
}

// ---------------------------------------------------------------------------
// Sequence operations
// ---------------------------------------------------------------------------
Node Node::operator[](std::size_t index) const {
    if (!data_ || data_->type_ != NodeType::Sequence)
        throw BadConversionException("Node is not a sequence");
    if (index >= data_->sequence_.size())
        throw RepresentationException("Sequence index out of bounds");
    return Node(data_->sequence_[index]);
}

void Node::push_back(const Node& node) {
    if (!data_)
        data_ = std::make_shared<node_data>();
    if (data_->type_ != NodeType::Sequence && data_->type_ != NodeType::Null)
        throw RepresentationException("Cannot push_back on a non-sequence node");
    data_->sequence_push_back(node.data_);
}

void Node::remove(std::size_t index) {
    if (!data_ || data_->type_ != NodeType::Sequence)
        throw BadConversionException("Node is not a sequence");
    data_->sequence_remove(index);
}

std::size_t Node::size() const {
    if (!data_ || !data_->is_defined_) return 0;
    if (data_->type_ == NodeType::Sequence) return data_->sequence_size();
    if (data_->type_ == NodeType::Map) return data_->map_size();
    return 0;
}

// ---------------------------------------------------------------------------
// Map operations
// ---------------------------------------------------------------------------
static std::shared_ptr<node_data> find_map_key(
    const std::vector<node_data::MapPair>& map,
    const std::string& key)
{
    for (const auto& pair : map) {
        if (pair.first && pair.first->type_ == NodeType::Scalar
            && pair.first->scalar_ == key) {
            return pair.second;
        }
    }
    return nullptr;
}

Node Node::operator[](const std::string& key) {
    if (!data_) {
        data_ = std::make_shared<node_data>();
        data_->type_ = NodeType::Map;
        data_->is_defined_ = true;
    }

    if (data_->type_ == NodeType::Null) {
        data_->type_ = NodeType::Map;
        data_->is_defined_ = true;
    }

    if (data_->type_ != NodeType::Map)
        throw BadConversionException("Node is not a map");

    // Check already existing key
    auto existing = find_map_key(data_->map_, key);
    if (existing) {
        return Node(existing);
    }

    // Create a new key-value pair with undefined value
    auto key_node = std::make_shared<node_data>(NodeType::Scalar);
    key_node->scalar_ = key;
    auto value_node = std::make_shared<node_data>(NodeType::Null, false);
    data_->map_.emplace_back(key_node, value_node);
    data_->undefined_pairs_.emplace_back(key_node, value_node);
    return Node(value_node);
}

// Const version: read-only lookup
Node Node::operator[](const std::string& key) const {
    if (!data_ || data_->type_ != NodeType::Map) {
        throw BadConversionException("Node is not a map");
    }
    auto existing = find_map_key(data_->map_, key);
    if (existing) {
        return Node(existing);
    }
    return Node(); // undefined node
}

void Node::remove(const std::string& key) {
    if (!data_ || data_->type_ != NodeType::Map) return;
    auto key_node = std::make_shared<node_data>(NodeType::Scalar);
    key_node->scalar_ = key;
    data_->map_remove(key_node);
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------
bool Node::operator==(const Node& other) const {
    if (!data_ && !other.data_) return true;
    if (!data_ || !other.data_) return false;
    if (data_->type_ != other.data_->type_) return false;
    if (data_->is_defined_ != other.data_->is_defined_) return false;

    if (data_->type_ == NodeType::Scalar)
        return data_->scalar_ == other.data_->scalar_;

    // For sequences and maps, simple pointer equality on children
    // (deep equality would be expensive and is rarely needed)
    if (data_->type_ == NodeType::Sequence) {
        if (data_->sequence_.size() != other.data_->sequence_.size())
            return false;
        for (size_t i = 0; i < data_->sequence_.size(); ++i) {
            if (data_->sequence_[i] != other.data_->sequence_[i])
                return false;
        }
        return true;
    }

    if (data_->type_ == NodeType::Map) {
        return data_->map_.size() == other.data_->map_.size();
    }

    return true;
}

} // namespace YAML
