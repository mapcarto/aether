---
title: Aether Server 集群与配置
description: AP 分片、AS 神谕、ST UDP 网关的分布式架构矩阵与特殊极限高动态场景配置
sidebar_position: 1
---

# Aether Server 集群与部署配置 (Aether Server Cluster)

纯粹的 Aether 内核（`libae.so`）必须被挂载在分布式的骨干网络上，才能面对宏大的城市级甚至省级空域物理防撞与推演。

## 核心架构组件

```mermaid
flowchart TD
    classDef core fill:#2d3748,stroke:#4fd1c5,stroke-width:2px,color:#fff
    classDef as fill:#2b6cb0,stroke:#2c5282,stroke-width:2px,color:#fff
    classDef ap fill:#c53030,stroke:#9b2c2c,stroke-width:2px,color:#fff
    
    CLIENTS["多端同步请求 (Clients)"]
    UDP["ST UDP Gateway (强并发网关)<br/>SO_REUSEPORT 分流"]:::core
    AS_NODE["AS (Aether Sphinx 路由中枢)<br/>Fan-out / Reduce 聚合"]:::as
    
    CLIENTS <-->|"高频态势上报 / 获取"| UDP
    UDP <-->|"Eventfd O(1) 分发"| AS_NODE
    
    subgraph "计算分片集群 (Bare-metal NUMA)"
        direction LR
        AP1["AP 分片 1 (Aether Pyramid)<br/>管辖网格区 A"]:::ap
        AP2["AP 分片 2 (Aether Pyramid)<br/>管辖网格区 B"]:::ap
        APn["AP 分片 N (Aether Pyramid)<br/>管辖网格区 N"]:::ap
    end
    
    AS_NODE <-->|"0拷贝本地环传递"| AP1
    AS_NODE <-->|"0拷贝本地环传递"| AP2
    AS_NODE <-->|"0拷贝本地环传递"| APn
    
    AP1 <-.->|"极速切片热更接管"| AP1_NEW["AP 分片 1 (新版本)"]
```

本章节详细定义了 Aether Server 应对海量信标流的算力切分、分布式拓扑架构（AP/AS/UDP网关），并提供了针对特殊情况（如遇到不可抗极高频密集流体场、雷达扫描等极端数据）时，**Aether Server 如何通过配置文件激活基于内核内存的高级处理策略（如双金字塔备灾模式）**。
