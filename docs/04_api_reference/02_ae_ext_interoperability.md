---
title: "02. AE-EXT 协议适配层 (Interoperability)"
description: "兼容存量资产 (Cesium/ArcGIS/OSGB) 的 Aether 实时数据接入方案"
---

# AE-EXT：异构客户端协议映射与适配

## 1. 业务背景：兼容存量资产，降低迁移成本
在实际工程实践中，企业用户通常拥有基于 Cesium (3DTiles)、ArcGIS (I3S) 或 PagedLOD (OSGB) 的成熟业务系统。AE-EXT 的目标是在不更改客户端核心逻辑的前提下，实现 Aether 动态空间数据的高效分发。

---

## 2. 核心架构：协议合成器 (Protocol Synthesizer)
AE-EXT 是位于 Aether Server 与外部网络间的协议转换层，具备时空感知映射能力。

### 2.1 显式寻址适配 (Explicit Addressing Adapter)
*   **适用对象**：3DTiles, I3S
*   **技术细节**：AE-EXT 实时观测 Aether 核心格网状态，动态生成 `tileset.json` 索引。客户端发起的 Bounding Volume 寻址请求，将直接映射至 Aether 内存中的 Voxel 节点。
*   **收益**：基于 Cesium 等标准协议的前端系统，仅需通过修改数据源 URL 即可接入 Aether 实时数据流。

### 2.2 隐式寻址回归 (Implicit Addressing Adapter)
*   **适用对象**：OSGB, PagedLOD 类传统客户端
*   **技术细节**：针对对寻址逻辑有严格预定义要求的协议（如特定的格网编号规则），AE-EXT 采用 **Profile (预设配置)** 模式。通过模拟物理文件的目录偏移与命名逻辑，确保客户端能按照固有规则读取 Aether 动态引擎数据。

---

## 3. 自动化配置流程 (Automatic Configuration)
AE-EXT 提供 Profile (预设) 机制，旨在批量解决不同厂商、不同标准的数据接入配置。

### 3.1 预设重用机制 (Profile Reuse)
针对特定客户端（如某品牌 OSG 浏览器）的寻址规则仅需编写一次 Profile。后续同标准的图层仅需在元数据中声明 Profile ID 即可实现兼容。

### 3.2 坐标元数据解析 (Metadata Ingestion)
对于导入的静态底图数据，AE-EXT 支持：
*   **自动提取**：自动读取 `metadata.xml` 等标准元数据文件，提取坐标偏移与投影系统。
*   **精度对齐**：系统自动完成 Aether 全球坐标系与局部工程坐标系的映射，实现 5cm 级的空间位置一致性。

---

## 4. 性能指标 (Performance Metrics)
AE-EXT 与 Aether 核心共享内存空间，协议封装过程低延迟且非阻塞：
*   **单次寻址延迟**：平均增加耗时约 **0.8ms ~ 2.4ms**。
*   **系统吞吐损耗**：对核心物理引擎的计算性能干扰 **< 0.1%**。
*   **并发承载**：单节点 AE-EXT 可稳定承载 5000+ 客户端的高频异步请求。

---

**[注：本文档仅包含工程事实与数据指标]**
