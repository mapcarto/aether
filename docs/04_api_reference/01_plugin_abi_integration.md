---
title: 业务插件 API 与 ABI 集成规范
description: 说明上层行业应用如何通过动态库与 C ABI 接口挂载业务逻辑，实现“核心引擎 + 业务插件”的部署模式。
sidebar_position: 1
---

# 业务插件 API 与 ABI 集成规范 (Business Plugin Integration)

Aether (ae) 的架构要求核心引擎保持通用性，不直接承载“航路审批”“载荷限制”等行业规则。对于低空管控等商用场景，推荐采用 **“核心引擎 + 业务插件 (Core Engine + Business Plugins)”** 的交付模式。

## 1. 赋予业务开发者的核心价值 (Developer Value Proposition)
“数据结构不可知论 (Data Structure Agnosticism)” 在插件体系中的体现如下：

- **Bring Your Own Data/Algorithm**：业务侧可沿用既有 C++/Rust 数据结构与算法，无需继承引擎内部基类。引擎仅依赖 64 位 ID、包围盒与标准事件结构。
- **并发能力复用**：插件聚焦业务判定逻辑，空间初筛与事件分发由金字塔索引与 MPMC 总线承担。
- **代码边界隔离**：业务算法可通过 `.so` / `.dylib` 以二进制形式接入，降低源码耦合与交付边界冲突。

## 2. 动态库注入与动态符号解析 (Dynamic Loading)

为隔离核心态与业务态，Aether 在运行时加载包含业务逻辑的动态链接库（Linux 下 `.so`，macOS 下 `.dylib`）。

- **动态加载接口**：引擎通过 `dlopen()` 于冷启动或热更时将外部业务插件库挂载进主存。
- **纯 C ABI 入口 (Pure C ABI)**：无论插件使用 C、C++ 或 Rust 编写，对外生命周期入口（如 `plugin_init`, `plugin_step`, `plugin_shutdown`）都应使用 `extern "C"`，避免 C++ Name Mangling 造成符号解析失败。
- **函数指针注册**：通过 `dlsym()` 获取的函数指针统一注册到钩子数组（Hook Array），并在调度周期内按规则调用。

## 3. 事件驱动钩子与订阅机制 (Event-Driven Hooks)

Aether 的插件交互采用事件驱动模型。外部插件通过订阅 `ringbuf`（MPMC 无锁环形缓冲区）上的事件标签参与业务处理：

- **订阅特定状态通道**：插件可以通过 API（形如 `ae_subscribe_event(EVENT_WEATHER_UPDATE, weather_handler_cb);`）接管自己关注的具体业务流。
- **执行行业算法**：例如天气事件触发后，防撞插件可基于只读快照进行航路规划，并将结果以“修改事件（Mutation Events）”回写主引擎。
- **故障隔离**：业务逻辑在插件回调中执行。若回调异常，可通过故障隔离机制与调度隔离策略降低对核心计算链路的影响。

## 4. 内存所有权不可越界 (Memory Boundary Enforcement)

这是 API 集成规范中的关键约束：插件只读取引擎授予的数据视图，不直接接管其内存所有权。

在将系统状态通过引用的方式传递给插件（如 `void flight_check(const ae_snapshot_t* snap)`）时：
1. **必须修饰为 `const`**：所有传递给业务端插件的指针必须强制指向恒定常量操作，彻底杜绝插件层直接通过修改指针内容改变引擎内存。
2. **禁止直接释放引擎内存**：插件接收的实体指针生命周期由底层 `memarena` 管理。插件不应对其调用 `free()`，也不应将其长期缓存到全局静态变量。
3. **隔离分配的原则**：如果业务插件计算航线需要动态开辟大量的缓存节点，它必须使用自身代码里的系统堆 `malloc` 或是请求引擎为它单开一个临时的 `memarena` 挂载点。计算结束后必须自行兜底擦洗，永远不得污染基础引擎库池。

## 5. 商业交付边界

这套 ABI 规范不仅用于技术解耦，也支撑商业交付边界：
引擎本体可按统一 SDK 交付，业务方在接口约束内以闭源插件实现差异化规则，从而保持职责边界与协作稳定性。
