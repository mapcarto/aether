---
title: Aether 底层引擎概念总览
description: 高性能纯C金字塔空间网格与事件驱动引擎
---

# AE: Aether (æ)

> **数字宇宙底层运行时：高性能时空数据管理引擎**
> —— **极简 · 透明 · 可控** 的纯C构建空间金字塔事件驱动底层底座。

## 核心设计哲学 (Core Philosophy)
在现代软件工程与高并发计算领域，Aether 通过深度融合 **金字塔空间格网索引 (Pyramid Spatial Index)**、**无原型实体组件系统 (Archetype-less ECS)** 以及 **“万物皆事件”的统一回调架构**，为极限场景提供了一套颠覆性范式。

彻底摒弃传统引擎的黑盒内存分配器、复杂的 C++ 模板元编程与不可控的 GC 机制，Aether 采用：
- 完全透明的数据布局
- 无锁多生产者多消费者 (MPMC) 环形缓冲区
- 自研的极速轻量级内存池 (`memarena`)

静态测试达 **800 万次事件/秒**，极限峰值吞吐量高达 **8亿次/秒**，专为国防、航天、工业仿真及大规模数字孪生打造。

## 文档架构索引 (Map of Content)
本文档库打破了传统按静态 API 划分的模式，以数据流、事件流和内存所有权为骨架，划分为以下核心层级：

1. **[01_概念总览与架构哲学](./docs/01_philosophy/index.md)** - 阐释“万物皆事件”的统一时空观与零黑盒的执行环境。
2. **[02_快速上手与实战指南](./docs/02_quickstart/index.md)** - 最小可行性全工作流：从内存池初始化到事件调度闭环。
3. **[03_核心底层引擎 (Core Subsystems)](./docs/03_core_subsystems/)** - 理论主干与 4 大物理基石：
   - [01_空间网格](./docs/03_core_subsystems/01_spatial_grid.md)
   - [02_无原型 ECS](./docs/03_core_subsystems/02_archetypeless_ecs.md)
   - [03_事件总线](./docs/03_core_subsystems/03_event_timing.md)
   - [04_无锁并发与内存竞技场](./docs/03_core_subsystems/04_concurrency_and_memory.md)
4. **[04_API参考与接口字典](./docs/04_api_reference/index.md)** - 结构体与核心回调机制开发手册。
5. **[05_基准测试与架构落地](./docs/05_benchmarks/index.md)** - 实战百万级吞吐定标数据与生态集成规范。
6. **[06_AI大模型集成 (AI Integration)](./docs/06_ai_integration/)** - 聚焦大语言模型的架构解耦对接流：
   - [LLM 上下文交互工作流](./docs/06_ai_integration/01_llm_interaction_workflow.md)
7. **[07_外部渲染与可视化客户端 (External Rendering Clients)](./docs/07_external_rendering_clients/index.md)** - 剖析 Vulkan/高斯泼溅等渲染前端读取无锁快照的独立挂靠范式。
8. **[08_空间计算分析管线 (Spatial Analysis)](./docs/08_spatial_analysis_pipelines/index.md)** - 拆分 离散体素侧、连续解析几何侧 与 4D 图谱逻辑推演侧 的严格架构界线。
