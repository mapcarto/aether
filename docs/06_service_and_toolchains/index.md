---
title: 服务封装与配套工具链
description: Aether 服务化形态及配套工具链的架构导读。
sidebar_position: 6
---

# 服务封装与配套工具链 (Service Integration & Toolchains)

如 `03_core_subsystems` 所述，Aether 引擎代码首先是一套底层计算内核。面向外部团队交付时，通常需要将内核封装为可独立运行的基础服务节点，以屏蔽底层指针与内存管理细节。

Aether 的服务化体系由三个核心组件簇构成：
* **AP (Aether Pyramid)**：计算核心，负责空间隔离与实时并发计算。
* **AS (Aether Sphinx)**：路由中枢，负责集群拓扑映射、请求寻址与分片管理。
* **AVA (Aether Visualization)**：可视化中枢，负责将高吞吐数据转换为可视化结果。

---

## 1. AP 与 AS 算网矩阵 (Aether Core Server)

基于 `libae.so` 与 State Threads 搭建的无锁、零拷贝架构，承载着低空大动态 UTM (无人机交通管理系统) 等时空骨干的核心计算流转。

* **[1.1 Aether Server 服务架构核心总结](./01_aether_server.md)**

## 2. 周边配套生态与工具链 (Toolchains & Ecology)

一旦 Aether 被升级为“基础服务节点”，就必须配备一套成熟的外围工具链生态来保障其持久性与业务扩展能力：

* **[2.1 AVA 视效引擎与渲染客户端](./02_ava_visualization/index.md)**
  产品矩阵之“眼”，承接高频事件推送以进行优雅的 UI/3D 渲染表现与监控态势打通。
* **[2.2 大语言模型 (LLM) 空间感知集成](./03_ai_integration/index.md)**
  面向“空间大脑”愿景，探讨如何利用 MCP 协议与 LOD 策略将 Aether 的时空能力透传至 AI 决策层。
* **[2.3 空间分析计算域 (Spatial Analysis Domains)](./04_spatial_analysis_domains/index.md)**
  涵盖离散格网计算、经典空间几何、时空知识图谱以及双金字塔动态场读写四大算法域。
* **[2.4 AE-EXT 协议适配器与存量资产接入 (Interoperability)](../04_api_reference/02_ae_ext_interoperability.md)**
  面向存量 Cesium/ArcGIS 系统，提供动态 3DTiles 合成与 OSGB 隐式格网回归能力，实现平滑升级。
* **空间数据导入引擎 (Data Importers / ETL)**
  负责清洗超大规模外部地形、空域、限制区离线数据，生成 AP 直接加载的快照阵列表。
* **运维水位总控台 (Ops & Monitoring)**
  提供物理主存水位、`eventfd` 消息积压、透明大页占用等内核级监控指标的可视化。
