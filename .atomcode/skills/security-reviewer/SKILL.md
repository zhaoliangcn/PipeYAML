---
name: security-reviewer
description: Security-focused code reviewer for PipeYAML — YAML parser attack surface analysis
disable_model_invocation: false
user_invocable: true
---
# Security Reviewer — PipeYAML 安全审查

审查 PipeYAML 的安全相关代码，重点关注 YAML 解析器的攻击面。

## 审查清单

### 拒绝服务（DoS）
- [ ] 深度嵌套保护（`MAX_RECURSION_DEPTH` 检查是否生效？）
- [ ] 超大文档解析有没有内存爆炸风险？
- [ ] 别名循环引用是否会造成无限递归？
- [ ] 超长标量是否可能导致内存耗尽？

### 输入验证
- [ ] BOM 检测是否正确？是否有 BOM 注入风险？
- [ ] 缩进溢出 / 负数缩进是否处理？
- [ ] 控制字符在标量中的处理是否安全？
- [ ] 非 UTF-8 编码输入是否被拒绝或安全处理？

### 内存安全
- [ ] 是否有潜在的缓冲区溢出？（Scanner 中的字符串操作）
- [ ] `Token::value` 是否有大小限制？
- [ ] Emitter 输出是否有畸形节点导致崩溃的可能？

### 逻辑安全
- [ ] 锚点覆盖是否有歧义？（重复 &id 的处理）
- [ ] 多文档边界是否正确处理？
- [ ] Tag 处理是否有注入风险？
