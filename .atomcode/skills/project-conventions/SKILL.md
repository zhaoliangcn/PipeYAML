---
name: project-conventions
description: PipeYAML project conventions and coding guidelines
disable_model_invocation: false
user_invocable: false
---
# PipeYAML 项目约定

## 项目概述

PipeYAML 是一个 C++17/20 YAML 1.2 解析与发射库，采用管道架构：
`Stream → Scanner → Parser → Node → Emitter`

## 代码规范

### 命名
- **类**：PascalCase（`Stream`, `Scanner`, `NodeData`）
- **方法/函数**：snake_case（`parse_next_document`, `get_mark`）
- **成员变量**：trailing underscore（`stream_`, `tokens_`）
- **常量/枚举**：PascalCase 或 UPPER_SNAKE_CASE
- **模板参数**：大写 T / K / V

### 文件结构
- 头文件：`include/pipeyaml/<module>.h`
- 实现文件：`src/<module>.cpp`
- 测试文件：`tests/test_<module>.cpp`

### 头文件规范
- 使用 `#pragma once`
- 最小化包含（前向声明优先）
- 公开 API 使用 `std::shared_ptr` 返回值

### 错误处理
- 解析错误抛出 `ParserException`（含行/列位置）
- 类型转换错误抛出 `BadConversionException`
- 运行时错误使用标准异常

### 内存管理
- Node 系统使用三层架构：`Node`(句柄) → `node_ref` → `node_data`
- 引用计数由 `std::shared_ptr` 管理
- 避免裸 `new`/`delete`

## 测试约定（tinyTest）
- 使用 `TEST_CASE(Name, "tag1", "tag2")` 定义测试
- 有状态测试使用 `TEST_F(FixtureClass, Name)`
- 标签规则：模块名 + 类别（core/edge/error）
- 使用语义化断言：`CHECK_EQ`、`CHECK_NE`、`CHECK_THROW`
