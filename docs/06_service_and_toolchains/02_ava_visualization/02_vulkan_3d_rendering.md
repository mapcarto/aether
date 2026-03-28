---
title: Vulkan 与 3D 渲染架构
description: ECS 全量架构下基于 Vulkan 的数据驱动与 VRAM 直写间接绘制
sidebar_position: 2
---

# Vulkan 3D 渲染端：面向下一代间接绘制的 VRAM 直接投射

Aether 引擎由于剥离了内置渲染器，这意味着它在面对 Vulkan 这一类极度底层、要求数据“绝对精确铺陈”的高级图形 API 时，反而具备了传统引擎无法企及的“裸机对接 (Bare-Metal)”优势。

## 1. 为什么采用 Data-Oriented 渲染对接

传统面向对象 (OOP) 的渲染方式需要逐个游戏实体进行 `Update()` -> 拿矩阵 -> 告诉 GPU `SetUniform()` -> 触发 `Draw()`。当实体数量突破百万，这一流程的 CPU 开销将会引发灾难级的性能悬崖。

Aether 所提倡的 **面向数据 (Data-Oriented)** 与最新一代的 Vulkan API 在理念上达成了极其恐怖的完美契合：

- **组件池 (Component Pools)** 数据阵列全部是纯 C 语言的连续内存块（Struct of Arrays）。
- 这种绝对纯净的连续阵列，与 Vulkan / Direct3D 12 极力渴求的 **SSBO (Shader Storage Buffer Object)** 结构分毫不差！
  
渲染时甚至可以直接通过内存映射指令将这数以万计的变换矩阵热拷贝到显卡 VRAM 处，达到零解析损耗的数据喂送。

## 2. 核心架构范式：Vulkan 间接绘制 (Indirect Drawing) 挂载

Aether 给 Vulkan 的最佳实践方案即为 **“无锁式批量间接绘制”**：

1. **粗筛与快照 (Snapshot & Culling)**：渲染线程从 Pyraman（看门人引擎）拿到系统 15 毫秒前推演好的“世界绝对定格快照”。利用 3D 金字塔底层网格瞬间筛选出当前摄像机视锥内的上万个卡片实体 ID。
2. **GPU 绘制指令列装 (Command Assembly)**：将符合这批 ID 所对应的所有渲染参数通过 Aether 独有的“扁平哈希层”极速打包装入 Vulkan 预设好的连续内存。
3. **`vkCmdDrawIndirect` 终极释放**：由 GPU 端依据发送过去的间接指令缓冲（Indirect Buffer），自主分发成千上万个几何体的渲染流。CPU 仅仅在这里充当了向硬件发射坐标信号的快递员，绝不像传统游戏引擎一样深陷渲染树的轮回计算。

## 3. 避免双线竞争的只读协议

必须强调，外部 Vulkan 的帧率起伏不会引发 AE 框架“心脏骤停”。这套体系内，若是 Vulkan 因为某种重度着色器导致的渲染卡在 20 帧，AE 底层的事件循环依然以千万级高并发吞吐的物理基准坚定跑在自己所在的 CPU 独立线程集上，渲染端只管按照自己的步调，贪婪地索取它能拿到的“最新鲜”系统快照画面。
