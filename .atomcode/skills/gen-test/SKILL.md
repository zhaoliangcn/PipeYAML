---
name: gen-test
description: Generate tinyTest test case skeletons for PipeYAML modules
disable_model_invocation: true
user_invocable: true
---
# gen-test — 生成 tinyTest 测试骨架

为 PipeYAML 的指定模块生成 tinyTest 格式的测试代码。

## 用法

`/gen-test <ModuleName> [<method_or_feature>]`

示例：
- `/gen-test Scanner` — 生成 Scanner 模块完整测试骨架
- `/gen-test Scanner scan_scalar` — 只为 scan_scalar 方法生成测试
- `/gen-test Parser parse_map` — 生成 parse_map 测试

## 模板

```cpp
#include <tiny_test.h>
#include "pipeyaml/<module>.h"

// -----------------------------------------------------------------------------
// <ModuleName> 测试
// -----------------------------------------------------------------------------

TEST_CASE("<ModuleName> - <feature description>", "<module>", "core") {
    // Arrange
    // Act
    // Assert
    CHECK(/* ... */);
}
```

## 生成规则

1. **测试文件路径**：`tests/<module>.cpp`
2. **断言优先使用**：`CHECK_EQ`、`CHECK_NE`、`CHECK_THROW` 等语义化断言
3. **有状态测试使用** `TEST_F` + Fixture
4. **标签规则**：第一个标签是模块名，第二个是类别（core/edge/error）
5. **每个 TEST_CASE 只测一个行为**
