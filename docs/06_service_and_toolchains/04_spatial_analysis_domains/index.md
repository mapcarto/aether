---
title: 空间分析计算域
description: 离散格网架构、Martinez-Rueda 裁剪算子与时空知识图谱推理框架。
sidebar_position: 3
---

# 空间分析计算域 (Spatial Analysis Domains)

Aether (æ) 引擎通过统一时空底座，覆盖从底层碰撞检测、几何计算到高阶 AI 推理的空间计算需求。为保证执行效率与职责边界，Aether 按计算复杂度和业务诉求将能力划分为三条相互独立、层层递进的**计算域 (Domains)**。

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
此计算域面向高精度拓扑判定，负责处理矢量坐标基元（点、线、多边形）的解析几何运算。
- **核心机制**：对焦点区域利用 **Martinez-Rueda-Feito (MRF)** 布尔裁剪算法进行求交，并通过自适应精度几何谓词降低浮点误差风险。
- **典型应用**：法理级地块归属校验、精密电子围栏判定 (PIP Winding Number)、多边形布尔裁剪。

## 3. [知识图谱推理域：时空关系挖掘](./03_spatiotemporal_knowledge_graph.md)
该领域在点线面计算结果之上，进一步基于海量 **事件事务日志 (Transaction Events)** 执行关系链推理。
- **核心机制**：实现基于 **DE-9IM** 的拓扑关系生成与 **RCC8** 逻辑推理，同时整合 **Spatial PageRank** 权重算法与 **HDBSCAN** 密度聚类。
- **典型应用**：兵棋推演中的合围意图识别、路网拥堵的因果链溯源。

---

**各域协作机制**：
**离散格网域**利用时空索引实现对冗余假阳性数据的预过滤 (Pre-filtering)；**经典几何域**在格网语义之上提供高精度的解析计算支持；而**知识图谱域**则在异构物理数据之上封装出语义级逻辑关联。
