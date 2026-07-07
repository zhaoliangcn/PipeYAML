// PipeYAML benchmark suite
// Measures parsing, serialization, and node manipulation performance

#include <benchmark/benchmark.h>

#include <yaml-cpp/yaml.h>
#include "bench_data.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// =============================================================================
// Loading / Parsing Benchmarks
// =============================================================================

static void BM_Load_Small(benchmark::State& state) {
    const std::string input = bench_data::small_yaml;
    for (auto _ : state) {
        auto doc = YAML::Load(input);
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_Load_Small);

static void BM_Load_Medium(benchmark::State& state) {
    const std::string input = bench_data::medium_yaml;
    for (auto _ : state) {
        auto doc = YAML::Load(input);
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_Load_Medium);

static void BM_Load_Nested(benchmark::State& state) {
    const std::string input = bench_data::nested_yaml;
    for (auto _ : state) {
        auto doc = YAML::Load(input);
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_Load_Nested);

static void BM_Load_Complex(benchmark::State& state) {
    const std::string input = bench_data::complex_yaml;
    for (auto _ : state) {
        auto doc = YAML::Load(input);
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_Load_Complex);

// Load from file
static void BM_LoadFile_Small(benchmark::State& state) {
    if (state.thread_index() == 0) {
        std::ofstream f("/tmp/bench_small.yaml");
        f << bench_data::small_yaml;
    }
    for (auto _ : state) {
        auto doc = YAML::LoadFile(std::string("/tmp/bench_small.yaml"));
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_LoadFile_Small);

// =============================================================================
// Serialization / Dump Benchmarks
// =============================================================================

static void BM_Dump_Medium(benchmark::State& state) {
    auto doc = YAML::Load(bench_data::medium_yaml);
    for (auto _ : state) {
        auto output = YAML::Dump(doc);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_Dump_Medium);

static void BM_Dump_Complex(benchmark::State& state) {
    auto doc = YAML::Load(bench_data::complex_yaml);
    for (auto _ : state) {
        auto output = YAML::Dump(doc);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_Dump_Complex);

// =============================================================================
// Node Access Benchmarks
// =============================================================================

static void BM_Access_Scalar(benchmark::State& state) {
    auto doc = YAML::Load(bench_data::medium_yaml);
    for (auto _ : state) {
        auto val = doc["server"]["host"].as<std::string>();
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Access_Scalar);

static void BM_Access_Deep(benchmark::State& state) {
    auto doc = YAML::Load(bench_data::nested_yaml);
    for (auto _ : state) {
        auto val = doc["level1"]["level2"]["level3"]["level4"]["level5"]
                      ["level6"]["level7"]["level8"]["level9"]["level10"]
                      ["value"].as<std::string>();
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Access_Deep);

static void BM_Access_Sequence(benchmark::State& state) {
    auto doc = YAML::Load(bench_data::complex_yaml);
    const auto& features = doc["features"];
    for (auto _ : state) {
        auto first = features[std::size_t(0)]["name"].as<std::string>();
        benchmark::DoNotOptimize(first);
    }
}
BENCHMARK(BM_Access_Sequence);

// =============================================================================
// Type Conversion Benchmarks
// =============================================================================

static void BM_Convert_Int(benchmark::State& state) {
    auto doc = YAML::Load("value: 42\n");
    const auto& val = doc["value"];
    for (auto _ : state) {
        int n = val.as<int>();
        benchmark::DoNotOptimize(n);
    }
}
BENCHMARK(BM_Convert_Int);

static void BM_Convert_Bool(benchmark::State& state) {
    auto doc = YAML::Load("flag: true\n");
    const auto& val = doc["flag"];
    for (auto _ : state) {
        bool b = val.as<bool>();
        benchmark::DoNotOptimize(b);
    }
}
BENCHMARK(BM_Convert_Bool);

static void BM_Convert_Double(benchmark::State& state) {
    auto doc = YAML::Load("pi: 3.14159265358979\n");
    const auto& val = doc["pi"];
    for (auto _ : state) {
        double d = val.as<double>();
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK(BM_Convert_Double);

// =============================================================================
// Node Manipulation Benchmarks
// =============================================================================

static void BM_Build_Sequence(benchmark::State& state) {
    for (auto _ : state) {
        YAML::Node seq(YAML::NodeType::Sequence);
        for (int i = 0; i < 1000; ++i) {
            seq.push_back(YAML::Node(YAML::NodeType::Scalar));
            seq[std::size_t(i)] = std::to_string(i);
        }
        benchmark::DoNotOptimize(seq);
    }
}
BENCHMARK(BM_Build_Sequence);

static void BM_Build_Map(benchmark::State& state) {
    for (auto _ : state) {
        YAML::Node map(YAML::NodeType::Map);
        for (int i = 0; i < 100; ++i) {
            map["key_" + std::to_string(i)] = std::string("value_") + std::to_string(i);
        }
        benchmark::DoNotOptimize(map);
    }
}
BENCHMARK(BM_Build_Map);

// =============================================================================
// Roundtrip (Parse → Modify → Dump) Benchmarks
// =============================================================================

static void BM_Roundtrip_Modify(benchmark::State& state) {
    const std::string input = bench_data::medium_yaml;
    for (auto _ : state) {
        auto doc = YAML::Load(input);
        doc["server"]["port"] = std::string("9090");
        doc["database"]["pool_size"] = std::string("20");
        doc["logging"]["level"] = std::string("debug");
        auto output = YAML::Dump(doc);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_Roundtrip_Modify);

// =============================================================================
// Multi-document benchmark
// =============================================================================

static void BM_LoadAll(benchmark::State& state) {
    std::string input;
    for (int i = 0; i < 10; ++i) {
        input += "---\ndoc_" + std::to_string(i) + ":\n  id: " + std::to_string(i) + "\n  value: test\n";
    }
    for (auto _ : state) {
        auto docs = YAML::LoadAll(input);
        benchmark::DoNotOptimize(docs);
    }
}
BENCHMARK(BM_LoadAll);

// =============================================================================
// Main
// =============================================================================
BENCHMARK_MAIN();
