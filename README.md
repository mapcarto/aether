---
title: Aether 底层引擎概念总览
description: 高性能纯C金字塔空间网格与事件驱动引擎
---

# Aether 

> **极简 · 透明 · 可控** —— 纯C构建的高性能空间金字塔事件驱动底层引擎。

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
   - [01_空间网格](./docs/03_core_subsystems/01_spatial_grid.md) (`pyramid2.c/h`, `pyramid3.c/h`...)
   - [02_无原型 ECS](./docs/03_core_subsystems/02_archetypeless_ecs.md) (`ecs.c/h`)
   - [03_事件总线](./docs/03_core_subsystems/03_event_timing.md) (`taskwheel.c`, `hitimer.c`)
   - [04_无锁并发与内存竞技场](./docs/03_core_subsystems/04_concurrency_and_memory.md) (`ringbuf.c/h`, `memarena.c/h`...)
4. **[04_API参考与接口字典](./docs/05_api_reference/index.md)** - 自动化结构体与回调函数签名指南。
5. **[05_基准测试与架构落地](./docs/06_benchmarks/index.md)** - 实战打压数据支撑与第三方库零拷贝集成指南。
