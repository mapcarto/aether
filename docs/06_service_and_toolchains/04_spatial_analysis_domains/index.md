---
title: 空间分析计算域
description: 离散格网架构、Martinez-Rueda 裁剪算子与时空知识图谱逻辑推理
sidebar_position: 3
---

# 空间分析计算域 (Spatial Analysis Domains)

Aether (æ) 引擎通过统一的时空底座，支撑了从底层量化碰撞、精密图形学到高阶 AI 推理的跨度极大的空间计算需求。为了维持高密度执行效能并在不同的技术体系间确立界线定则，Aether 严格按照计算复杂度和业务诉求将空间能力切割为三条相互独立、层层递进的**计算域 (Domains)**。

## 协奏逻辑：数据流向与处理域

```mermaid
flowchart LR
    classDef source fill:#4a5568,stroke:#2d3748,stroke-width:2px,color:#fff
    classDef domain1 fill:#2b6cb0,stroke:#2c5282,stroke-width:2px,color:#fff
    classDef domain2 fill:#319795,stroke:#285e61,stroke-width:285e61,color:#fff
    classDef domain3 fill:#805ad5,stroke:#553c9a,stroke-width:2px,color:#fff

    RAW["宏观原始数据流<br/>(无人机群/遥感/轨迹)"]:::source
    
    D1["第一级：离散格网域<br/>(S2 / H3)<br/>99% O(1) 预过滤拦截"]:::domain1
    D2["第二级：经典拓扑域<br/>(MRF 布尔裁剪)<br/>精密几何解析与归属"]:::domain2
    D3["第三级：知识图谱域<br/>(DE-9IM / PageRank)<br/>事件逻辑与深度溯源"]:::domain3
    
    RAW --> D1
    D1 -->|"提取碰撞候选(假阳性过滤)"| D2
    D2 -->|"输出验证拓扑关系"| D3
    D3 -->|"反馈高层指挥网络"| ACTION(("业务<br/>动作"))
```

---

## 1. [离散格网计算域：高速感知与对齐](./01_grid_and_voxel_computing.md)
这是最贴近 Aether 物理内存基础的执行域。它负责绝大多数海量实体的初始广域状态判别与数据对齐。
- **核心机制**：集成离散全球格网系统 (DGGS, S2/H3) 以及地图代数 (Map Algebra) 框架。通过 Hilbert 填充曲线锁定 $\mathcal{O}(1)$ 的定址开销，直接接管 $99\%$ 的宏观过滤请求。
- **典型应用**：雷达视域模拟、区域离散化重分类、多源栅格图层叠加分析。

## 2. [经典空间计算域：精密拓扑判定](./02_classical_spatial_computing.md)
此计算域针对传统空间测绘必须具备的**“绝对拓扑无损”**进行严密设计。它负责处理矢量坐标基元（点、线、多边形）的解析几何运算。
- **核心机制**：对焦点区域利用 **Martinez-Rueda-Feito (MRF)** 布尔裁剪算法进行精准求交。底层引入自适应精度几何谓词以彻底规避浮点灾难。
- **典型应用**：法理级地块归属校验、精密电子围栏判定 (PIP Winding Number)、多边形布尔裁剪。

## 3. [知识图谱推理域：时空关系挖掘](./03_spatiotemporal_knowledge_graph.md)
这一领域彻底凌驾于点线面浮点推算之上，其运算本质是针对海量 **事件事务日志 (Transaction Events)**，进发更深层次的关系链网络推理。
- **核心机制**：实现基于 **DE-9IM** 的拓扑关系生成与 **RCC8** 逻辑推理，同时整合 **Spatial PageRank** 权重算法与 **HDBSCAN** 密度聚类。
- **典型应用**：兵棋推演中的合围意图识别、路网拥堵的因果链溯源。

---

**各域协作机制**：
**离散格网域**利用时空索引实现对冗余假阳性数据的预过滤 (Pre-filtering)；**经典几何域**在格网语义之上提供高精度的解析计算支持；而**知识图谱域**则在异构物理数据之上封装出语义级逻辑关联。
