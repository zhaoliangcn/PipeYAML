---
name: code-reviewer
description: C++ code reviewer for PipeYAML — focuses on memory safety, exception safety, and correctness
disable_model_invocation: false
user_invocable: true
---
# Code Reviewer — PipeYAML C++ 代码审查

审查 PipeYAML 的 C++ 代码，重点关注以下方面：

## 审查清单

### 内存安全
- [ ] 是否存在裸指针或潜在的悬空引用？
- [ ] `shared_ptr` 使用是否合理？有无循环引用？
- [ ] Node 三层架构中引用计数是否正确？
- [ ] 资源是否 RAII 封装？

### 异常安全
- [ ] 构造函数中是否抛异常导致资源泄漏？
- [ ] `emplace_back` / `push_back` 是否在异常时保持数据一致性？
- [ ] Parser 解析过程中异常是否传播正确？

### 边界条件
- [ ] 空输入 / 空节点 / null 值是否正确处理？
- [ ] 超长标量 / 深度嵌套是否有限制处理？
- [ ] EOF / 流错误是否处理？

### YAML 语义
- [ ] 缩进解析是否正确处理 tab vs space？
- [ ] 锚点/别名解析是否处理循环引用？
- [ ] 流式（flow）与块式（block）上下文切换是否正确？
- [ ] 简单键（simple key）延迟验证是否正确？
