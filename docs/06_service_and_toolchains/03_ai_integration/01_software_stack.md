---
title: 需要准备的软件
description: 详细描述大模型接入 AeServer 所需的各层软件：MCP Gateway、AE CLI、第三方 CLI 与 Skill 层。
sidebar_position: 1
---

# 需要准备的软件

## 1. AE MCP Gateway

AE MCP Gateway 是 AeServer 面向大模型的关键适配层，负责把后端能力转换为标准 MCP 接口。

### 1.1 对外暴露的三类接口

- **MCP tools**：把 AeServer 的查询与执行能力封装成可调用的工具，模型通过工具名和参数契约发起调用。
- **MCP resources**：把大结果集以资源形式暴露，支持摘要、句柄、分页、流式四种返回形式，避免把原始数据直接写入模型上下文。
- **MCP prompts**：把空间任务模板注册为可发现的 prompt，任何连入的 CLI 都可以直接取用，不需要自己实现任务编排。

### 1.2 View Token 透传机制

当查询结果触碰百万级体素时，Gateway 不做文本序列化，而是生成一个轻量级 View Token（如 `VIEW_TOKEN_8F9A2C`）返回给模型。

- 模型只持有这个短字符串，将其作为参数传递给客户端。
- 客户端收到 Token 后，通过直连通道向 AeServer 渲染管道索取二进制流，完全绕过大模型的文本解析瓶颈。
- 如需视觉理解，可调用 `Visual_Snapshot_Bake` 将 Token 对应区域烘焙成 Base64 JPEG，供多模态模型直接读取。

View Token 有 TTL，过期前客户端需续租；拉取失败时自动降级返回低保真度的热力摘要以维持最低可用性。

### 1.3 鉴权与权限控制

Gateway 自行执行身份验证与工具级权限控制，不依赖调用方 CLI 兜底：

- 通过 token 或 API key 识别调用者身份（Principal）。
- 每个 Principal 对应授权的工具集合，越权请求直接拒绝。
- 每次工具调用写入审计日志，记录调用者、工具名、时间戳与结果。
- 对每个接入客户端实施配额与速率限制。

### 1.4 UDP 同步封装

MCP 调用是请求-响应语义，而 AeServer 使用 UDP 协议。Gateway 负责弥合这个差距：

- 为每个请求管理配对、等待响应与超时。
- 超时后按策略重试，重试次数与退避策略可配。
- 对执行类工具保证幂等，避免网络抖动导致重复提交。

### 1.5 工具清单（12 个原子工具）

工具分三类：**感知类**（幂等只读）、**分析类**（计算卸载，不改状态）、**执行类**（改变 AeServer 状态，必须走 Propose→Commit）。

| 工具名 | 类别 | 关键约束 |
|---|---|---|
| `Grid_Heatmap_Query` | 感知 | 仅允许查询宏观网格（LOD 0-2），超出层级强制阻断 |
| `Voxel_Data_Pull` | 感知 | 体素数量超限时自动降级为 View Token |
| `Event_Stream_Subscribe` | 感知 | 并发挂起订阅流不超过 3 个，防内存泄漏 |
| `Entity_State_Snapshot` | 感知 | 返回指定实体的语义属性摘要 |
| `Spatial_Boolean_Check` | 分析 | AI 涉及碰撞/包含判断时必须调用此工具，禁止自行推导 |
| `Trajectory_Predict` | 分析 | 预测时间窗口须在计算配额内 |
| `Constraint_Compliance_Check` | 分析 | 校验操作是否违反空域规则或业务约束 |
| `Visual_Snapshot_Bake` | 分析 | 渲染耗时较长，禁止在高频控制循环中调用 |
| `Propose_State_Transition` | 执行 | 将多个执行意图打包进入 AeServer 沙盒校验，返回 `transaction_id` |
| `Commit_State_Transition` | 执行 | 只接受未过期的合法 `transaction_id` |
| `Entity_Heading_Adjust` | 执行 | 输入向量受动力学极限约束，必须先生成提案 |
| `Airspace_Freeze` | 执行 | 顶级高危操作，强制要求人工确认或最高级代理审核 |

所有工具的 schema 包含：`version`、必填字段、类型约束、前置条件、错误码。

### 1.6 错误码与 AI 行为指导

| 错误码 | 含义 | AI 应该怎么做 |
|---|---|---|
| `400_BAD_PROMPT_SCHEMA` | 参数不符合 schema 契约 | 反思并修正 JSON 格式；连续失败 2 次后挂起任务 |
| `403_BLAST_RADIUS_VIOLATION` | 越权或影响范围超限 | 禁止重试相同参数；将授权请求抛回给用户，或缩小操作范围 |
| `410_TWIN_SNAPSHOT_STALE` | 基于过期数据作出决策 | 调用 `Event_Stream_Subscribe` 更新上下文后重新推演 |
| `422_SPATIAL_LOGIC_VIOLATION` | 几何或动力学边界冲突 | 立即调用 `Spatial_Boolean_Check` 重新评估，禁止强行推进 |
| `429_CONTEXT_BUDGET_EXCEEDED` | 数据提取超出 Token 或计算配额 | 切换为 `Grid_Heatmap_Query` 获取宏观摘要，或请求 View Token |
| `500_AE_KERNEL_DEADLOCK` | 底层引擎死锁 | 立即挂起所有执行类工具调用，仅记录诊断日志，等待心跳恢复 |

### 1.7 版本管理

- 每个工具的 schema 携带版本号，Gateway 升级时保证向后兼容。
- 不兼容的 schema 变更通过新版本号区分，旧版本保留一定过渡期。
- Skill 层的 prompts 与 tool schema 版本绑定，Gateway 升级后 Skill 不会静默失效。

---

## 2. AE CLI（Agent Runtime）

AE CLI 是 Aether 自己构建的 AI agent 运行时，也是 Gateway 的第一个官方 MCP 客户端。

### 2.1 核心职责

- 接收用户目标，维护多轮会话与上下文记忆。
- 连接 Gateway，完成工具/资源/模板发现，并在本地缓存清单。
- 把用户目标与可用工具一起注入大语言模型，驱动模型生成调用计划。
- 执行权限 hooks，拦截不符合策略的工具调用。
- 执行 Propose→Commit 状态机，确保执行类操作经过校验再提交。
- 向用户呈现结果，或驱动下游系统（渲染、报告、外部接口）。

### 2.2 工具与模板发现

- 启动时向 Gateway 发送发现请求，拉取全量 tools/resources/prompts 清单。
- 本地缓存发现结果，定期刷新（或手动触发）。
- 发现失败时进入降级模式，只保留离线可用的能力。

### 2.3 执行状态机

执行类操作经过以下 8 个状态：

1. **INIT**：启动任务，组装上下文（历史记忆 + 动态提示词边界信息）。
2. **PERCEIVE**：调用感知与分析类工具，无副作用。查询超限时触发 View Token 截断。
3. **ANALYZE**：AI 建立当前拓扑的内部模型，形成操作规划。
4. **PROPOSE**：调用 `Propose_State_Transition`，将所有动作意图打包提交给 AeServer 沙盒校验。
5. **VALIDATE**：AeServer 在沙盒中执行假想执行（物理碰撞模拟 + 规则校验）。通过则返回 `transaction_id`；拒绝则附带错误码，状态机强制回退至 ANALYZE。
6. **COMMIT**：持有合法 `transaction_id` 后调用 `Commit_State_Transition`，状态写入主引擎并广播执行事件。
7. **AUDIT**：结果写入审计链，Gateway 记录完整调用因果记录。
8. **REFLECT**：真实变动的摘要回注 AI 短时记忆，支撑下一步推演。

连续收到同一错误码 3 次时，CLI 强制切断执行回路，将控制权交回用户。

### 2.4 提示词分层与缓存边界

CLI 将提示词严格分为两区，禁止混淆：

- **静态前缀区**（可缓存）：系统规则、执行纪律、所有工具的 JSON Schema 定义、基础术语字典。这部分内容跨数百次交互保持不变，API 服务商对其进行 Prompt Cache，命中后 Token 费用可降低 90%。
- **动态后缀区**（不缓存）：会话记忆摘要、从 AeServer ringbuf 订阅的增量事件流、最新错误反馈与状态变化。仅保留近期有效信息，避免无边界增长。

动态事件治理：对海量低价值的位置微调事件做聚类摘要（如"某路段流量攀升 15%"），禁止将易变状态写入静态前缀区。

### 2.5 查询计划机制

面对复杂空间任务，CLI 强制要求 AI 先提交查询计划再执行拉取：

1. AI 输出一个查询图（Query Graph），明确每步查询的目标区域和范围。
2. CLI 编排层评估预计 Token 消耗，超限时直接驳回并提示"请缩小搜索半径"。
3. 通过评估后，按计划分步执行，禁止全域扫描。

该机制从源头阻断 AI 盲目拉取大量数据的行为。

### 2.6 审计链

CLI 编排层拦截所有出向请求，为每次状态转移生成 SHA-256 哈希承诺链：

$$Hash_n = SHA256(Hash_{n-1} + Payload_n)$$

任何事后修改早期记录的行为都会导致链条断裂，在审核中即刻被检出。即使操作失败，也作为空载记录占据一个哈希位，保持链条时序完整性。

---

## 3. 第三方 CLI 接入

由于 Gateway 遵循 MCP 标准协议，实现 MCP client 的第三方 CLI 通常可以在不改 Gateway 的情况下接入。

### 3.1 当前可接入的 CLI

| CLI | 接入方式 | 备注 |
|---|---|---|
| Claude Code | stdio / HTTP MCP client | 官方支持完整 |
| Gemini CLI | stdio / HTTP MCP client | streaming 与 resources/read 需进行 capability negotiation |
| OpenClaw | stdio MCP client | 依赖其自身 MCP client 实现版本 |

### 3.2 接入前提

- Gateway 自带 AuthN/AuthZ，客户端须在握手时提供合法凭证。
- 每次工具调用携带调用者身份，Gateway 据此执行授权与审计。
- Gateway 对每个接入客户端独立核算配额，互不干扰。

### 3.3 接入限制

- 第三方 CLI 只能调用其 Principal 被授权的工具集合。
- 执行类工具的 Propose→Commit 状态机仍由 Gateway 和 AeServer 强制执行，第三方 CLI 无法绕过。
- 高风险工具可要求人工确认，这一策略在 Gateway 侧配置，不依赖客户端实现。

---

## 4. Skill 层

Skill 是把"常见空间任务模式"封装成可复用调用模板的能力包。

### 4.1 一个 Skill 包含什么

- 任务目标描述与输入约束（坐标系、bbox、最大记录数、时间窗口）。
- 工具调用顺序与每一步的输入输出映射。
- 失败时的降级策略（跳过某步、换用低精度工具、返回部分结果）。
- 版本号与变更说明。

### 4.2 Skill 的发现与注册

- Skill 通过 MCP Gateway 注册为 `prompts`，所有 CLI 均可通过标准发现接口获取。
- 注册时需通过格式校验，确保与当前 tool schema 版本兼容。
- 版本绑定：Skill 与它依赖的 tool schema 版本对应，Gateway 升级后旧 Skill 仍可继续使用，不兼容时标记为 deprecated。

### 4.3 Skill 与直接调用工具的区别

- 直接调用工具：模型自行规划步骤，适合探索性任务。
- 使用 Skill：任务步骤已固化，适合标准化、重复性任务，出错概率低。
