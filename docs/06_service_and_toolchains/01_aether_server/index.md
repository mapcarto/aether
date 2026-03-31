---
title: Aether Server 集群与配置
description: AP 分片、AS 路由中枢与 ST UDP 网关的分布式架构及场景化配置说明。
sidebar_position: 1
---

# Aether Server 集群与部署配置 (Aether Server Cluster)

Aether 内核（`libae.so`）通常需要挂载到分布式服务网络，才能承载城市级或省级空域的碰撞检测与推演任务。

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

本章节定义 Aether Server 在海量信标流场景下的算力切分方式与分布式拓扑（AP/AS/UDP 网关），并给出高频密集数据场景下的配置建议（如双金字塔备援模式）。
