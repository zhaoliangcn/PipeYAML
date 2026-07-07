# PipeYAML

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://github.com/zhaoliangcn/PipeYAML/actions/workflows/ci.yml/badge.svg)](https://github.com/zhaoliangcn/PipeYAML/actions/workflows/ci.yml)

A modern C++17 YAML 1.2 parser and emitter library, designed from scratch with a clean pipeline architecture. **7–10× faster than yaml-cpp** across typical workloads.

## Features

- ✅ **YAML 1.2** — scalars, sequences, maps, anchors & aliases, multi-document support
- ⚡ **High performance** — batch I/O stream, switch-dispatch scanner, inline string buffer emitter
- 🔋 **Type caching** — int/double/bool conversion results cached on first access (O(1) subsequent reads)
- 🧩 **Seamless C++ integration** — `as<T>()`, `operator=` with rvalue move semantics
- 📦 **Header-only style API** — single include `<pipeyaml/pipeyaml.h>`

## Performance vs yaml-cpp 0.8

| Benchmark | PipeYAML | yaml-cpp 0.8 | Speedup |
|-----------|:--------:|:------------:|:-------:|
| **Load** (medium, ~75 lines) | **14.3 μs** | 150 μs | **10.5×** |
| **Load** (complex, anchors) | **15.1 μs** | 146 μs | **9.7×** |
| **Load** (deeply nested) | **4.5 μs** | 42.7 μs | **9.4×** |
| **Dump** (medium) | **15.5 μs** | 81.6 μs | **5.3×** |
| **Dump** (complex) | **18.4 μs** | 92.6 μs | **5.0×** |
| **Roundtrip** (load→modify→dump) | **29.9 μs** | 225 μs | **7.5×** |
| **Build sequence** (1000 nodes) | **95.9 μs** | 463 μs | **4.8×** |
| **Build map** (100 keys) | **32.8 μs** | 239 μs | **7.3×** |
| **Convert** int (cached) | **10.4 ns** | 357 ns | **34×** |
| **Convert** double (cached) | **10.6 ns** | 574 ns | **54×** |

*Environment: Intel Xeon, GCC 9.4, C++17, Google Benchmark.*

## Quick Start

### Build

```bash
git clone https://github.com/zhaoliangcn/PipeYAML.git
cd PipeYAML
mkdir build && cd build
cmake ..
make
```

### Parse a YAML string

```cpp
#include <pipeyaml/pipeyaml.h>
#include <iostream>

int main() {
    std::string yaml = R"(
name: PipeYAML
version: 0.1.0
language: C++17
)";

    YAML::Node doc = YAML::Load(yaml);
    std::cout << doc["name"].as<std::string>() << "\n";    // PipeYAML
    std::cout << doc["version"].as<std::string>() << "\n"; // 0.1.0
    return 0;
}
```

### Build a Node tree programmatically

```cpp
YAML::Node root(YAML::NodeType::Map);
root["title"] = "Example";
root["count"] = 42;

YAML::Node items(YAML::NodeType::Sequence);
items.push_back("alpha");
items.push_back("beta");
root["items"] = items;

std::string out = YAML::Dump(root);
// title: Example
// count: 42
// items:
//   - alpha
//   - beta
```

### Load from file

```cpp
auto doc = YAML::LoadFile("/path/to/config.yaml");
int port = doc["server"]["port"].as<int>();
```

## API Overview

| Operation | API |
|-----------|-----|
| Parse a string | `YAML::Load(str)` → `Node` |
| Parse from file | `YAML::LoadFile(path)` → `Node` |
| Serialize | `YAML::Dump(node)` → `std::string` |
| Multi-document | `YAML::LoadAll(str)` → `std::vector<Node>` |
| Access map value | `node["key"]` |
| Access sequence element | `node[std::size_t(i)]` |
| Type conversion | `node.as<T>()` or `node.as<T>(fallback)` |
| Assignment | `node = value` (rvalue-aware) |
| Sequence append | `node.push_back(child)` or `node.push_back("string")` |
| Emitter stream | `emitter << node` |

## Dependencies

- **C++17** compiler (GCC 7+, Clang 5+, MSVC 2017+)
- *(optional)* [tinyTest](https://github.com/zhaoliangcn/tinyTest) — for unit tests
- *(optional)* [Google Benchmark](https://github.com/google/benchmark) — for benchmarks
- *(optional)* [yaml-cpp](https://github.com/jbeder/yaml-cpp) — for comparison benchmarks

## Documentation

- [Architecture Design Document (Chinese)](PipeYAML%20架构设计文档.md)

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by [yaml-cpp](https://github.com/jbeder/yaml-cpp)'s API design
- [YAML 1.2 Specification](http://www.yaml.org/spec/1.2/spec.html)
