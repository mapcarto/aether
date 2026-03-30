---
title: API参考手册与结构体字典
description: Aether 引擎 API 规范、内存所有权定义与并发调用约束
sidebar_position: 4
---

# API 参考手册与结构体字典 (API Reference Dictionary)

Aether 的底层 API 设计严格遵循内存所有权约定。为确保接口调用的安全性与性能一致性，所有暴露的 C 接口均需符合 Doxygen 标注规范，以便于自动化文档工具进行静态分析。

## 1. 结构化标签系统 (Standard Tags)
所有头文件中的函数与结构体需按照以下标准维护：
- **@brief**：描述函数或数据结构的具体数学功能或计算逻辑。
- **@param**：定义输入输出参数。涉及指针（如 `void *user_data`）时，需注明生命周期归属。
- **@return**：定义返回值标准及错误代码含义。
- **@warning**：标注线程安全性、可重入性以及非阻塞约束。
- **@see**：关联相关的设计说明或架构约束章节。

---

## 2. 核心引擎回调钩子 (Core Callbacks)

开发人员通过挂载引擎提供的钩子函数实现业务逻辑注入。按功能分类如下：

### 2.1 空间索引与生命周期 (Index & Lifecycle)
- **`partition_filter` (分区裁剪回调)**：用于判定复杂实体（如长距离线性对象）在金字塔网格中的切分策略，确立 LOD 子节点的存储分布。
- **`data_free_cb` (内存释放回调)**：挂接于引用计数系统。当实体的子节点被剔除且引用归零时，触发主内存块的安全释放逻辑。

### 2.2 广域体素查询 (Voxel Query)
- **`pyramid_query` (范围查询回调)**：实现空间选框功能。用于检索指定界限内的候选对象列表，支持包含假阳性 (False Positives) 在内的粗选过滤。
- **`overlap_occupancy_cb` (体素占用判定回调)**：在网格层次对比占用状态，适用于低功耗场景下的避障初选与大规模遮挡剔除分析。

### 2.3 精密空间演算 (Precise Computation)
- **`polygon_intersection_test` (求交测试回调)**：对初选结果进行几何求交验证，通过数学拓扑公式计算精确边界冲突。
- **布尔运算/三角化回调**：封装外部几何内核接口。支持多边形裁剪、打洞以及面向渲染层的三角形索引生成。

---

## 3. ABI 集成规范 (Integration Standards)

Aether 通过标准的 C 语言 ABI 确保计算底座与业务逻辑的物理隔离。

- **业务插件 API 与 ABI 集成规范**：阐述上层业务动态库（`.so`/`.dylib`）如何遵循事件钩子（Hooks）标准，无损注册至引擎主循环，实现业务能力的平滑扩展。详情参见：**[业务插件集成规范](./01_plugin_abi_integration.md)**。
- **AE-EXT 协议适配器与存量资产接入**：面向存量 Cesium/ArcGIS 系统，提供动态 3DTiles 合成与 OSGB 隐式格网回归能力。详情参见：**[AE-EXT 协议适配器](./02_ae_ext_interoperability.md)**。
