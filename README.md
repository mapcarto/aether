---
title: Aether 底层引擎概念总览
description: 高性能纯C金字塔空间网格与事件驱动引擎
---

```text
  ████  ████
       █     █
 █████████████
 █     █     
  ████  ████
```

# Aether (æ) 引擎：数字宇宙底层运行时

> **空间计算底座 / 高性能时空推演 / 链接世界模型**
> —— **极简 · 透明 · 可控** 的纯 C 构建空间金字塔事件驱动底层底座。

## 三大核心支柱 (Core Pillars)

Aether (æ) 的诞生不是为了再造一个传统的 3D 渲染器，而是为下一代超大规模时空并发、全局态势数字孪生与通用大模型 (LLMs) 构建真实可信的物理法则底座：

1. **[空间计算底座 (Spatial Computing Foundation)](./docs/03_core_subsystems/index.md)**
   彻底摒弃传统引擎的黑盒内存分配与锁争用泥潭。依托 `memarena` 物理连贯内存、Pyraman 单核无锁调度与金字塔格网，Aether 提供单线程 800 万次/秒吞吐保障的纯 C 语言硬核下盘。

2. **[高性能时空推演 (High-Performance Spatiotemporal Simulation)](./docs/06_service_and_toolchains/04_spatial_analysis_domains/index.md)**
   提供多套极致性能的计算域。从千万级海量实体的离散格网预筛，到法理级绝对精准的 MRF 布尔解析；从气象与传感器动态场双金字塔 (Double Pyramid) 的 $\mathcal{O}(1)$ 指针翻转，到 RCC8 时空逻辑推演。

3. **[链接世界模型 (Linking the World Model)](./docs/01_philosophy/02_ontology_paradigm.md)**
   基于前沿实战范式，打造数字孪生体的时空本体论 (Ontology)。通过向通用人工智能 (AGI) 开放结构化的 MCP 代理权限与句柄透传网络，使得 AI 终于能够读懂冰冷坐标背后的物理因果，实现真正“知行合一”的自治环境。

---

## 工程准则 (Engineering Principles)

在现代软件工程与高并发计算领域，Aether 通过深度融合 **金字塔空间格网索引 (Pyramid Spatial Index)**、**无原型实体组件系统 (Archetype-less ECS)** 以及 **“万物皆事件”的统一回调架构**，为极限场景提供了一套颠覆性范式。

彻底摒弃传统引擎的黑盒内存分配器、复杂的 C++ 模板元编程与不可控的 GC 机制，Aether 采用：
- 完全透明的数据布局
- 无锁多生产者多消费者 (MPMC) 环形缓冲区
- 自研的极速轻量级内存池 (`memarena`)

单线程坚守 **800 万次事件/秒** 的物理吞吐下限，从底层支撑全场景的千万级极速高并发，专为海量算力调度、工业仿真及大规模空间推演构建。

---

## 文档架构索引 (Map of Content)

本文档库打破了传统按静态 API 划分的模式，以数据流、事件流和内存所有权为骨架，划分为以下核心层级：

1. **[01 哲学与世界模型](./docs/01_philosophy/index.md)** - 阐释“万物皆事件”的统一时空观、时空本体论与零黑盒的执行环境。
2. **[02 快速部署架构](./docs/02_quickstart/index.md)** - 部署 Aether Kernel 和 Aether Server 的规范，以及存量系统无感接入方案。
3. **[03 核心底层引擎](./docs/03_core_subsystems/index.md)** - 理论主干与 5 大物理基石：空间网格、无原型 ECS、事件总线、并发内存以及双金字塔机制。
4. **[04 业务插件与API规范](./docs/04_api_reference/index.md)** - 聚焦“引擎底座+业务动态库”的零侵入沙盒分发与 ABI (C语言原生) 挂载规范。
5. **[05 性能基准与网格测试](./docs/05_benchmarks/index.md)** - 实战百万级吞吐定标数据与架构多端硬件部署落地方案。
6. **[06 服务封装与周边工具链](./docs/06_service_and_toolchains/index.md)** - 整合了从服务端部署到视效呈现的完整宇宙生态：
   - **AP/AS 服务端组网**：基于共享内存的高维算网矩阵。
   - **AVA 视效宇宙**：分离管线的视觉调度中枢 (VKDK / Cairo / 3DGS)。
   - **大模型认知代理流**：为语境准备的 LLM 空间感知流水线与 MCP 接入协议。
   - **横向空间分析核心域**：离散格网、解析拓扑与时空知识推理网。
7. **[88 视觉资产与识别规范](./docs/88_visual_identity/index.md)** - Aether 视觉标志、UI 统一风格体系及设计物料。
