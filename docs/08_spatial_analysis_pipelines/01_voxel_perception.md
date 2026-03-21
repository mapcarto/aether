---
title: 离散体素侧与感知查定
description: 基于多层级金字塔查询回调的广域极速筛选感知管线
sidebar_position: 1
---

# 离散体素侧：高速查定初筛管线 (Voxel Perception)

在 Aether 处理数以亿计的空间对象碰撞查询时，如果所有的碰撞都完全抛给浮点连续几何去计算，即使是当今世界最强的处理器架构也会瞬间垮塌。因此，Aether 为空间分析确立了第一条防线——**离散体素感知管线**。

它通过高度极简的网格定址匹配机制锁定 $O(1)$ 的开销，直接接管了整个系统内 $99\%$ 宏观范畴内的查定过滤请求。

## 1. 核心回调函数注入：盲查与防碰撞 (Blind Query & Collision)

如同引擎“万物均需解耦”的理念，所有的体素宏观检索并不会在底层堵死，而是以回调函数 (`Callbacks`) 的形态返还给上层应用：

### 1.1 范围搜索提取器 (`pyramid_query_cb`)

在进行大规模的雷达扫描视野（如查找某街道 500 米半径内所有车辆）时，金字塔直接对空间框柱进行降维抓取。

```c
/**
 * @brief 广域离散体素查询匹配回调 (Voxel Search Callback)
 * 
 * @param card_id 命中的金字塔卡片（Shared Card / Part Card）标识，可能带有一定的矩阵假阳性
 * @param grid_x 命中当层网格的绝对法线空间 X 坐标
 * @param grid_y 命中当层网格的绝对法线空间 Y 坐标
 * @param user_data 供调用者（如 AI Agent 或防碰撞模块）存储检索结果列表的栈内存指引
 */
typedef void (*pyramid_query_cb)(uint64_t card_id, int grid_x, int grid_y, void *user_data);
```

### 1.2 体素重叠防碰撞报警 (`overlap_occupancy_cb`)

当两台智能驾驶机动车物理坐标迫近时，它们将自然而然地通过系统跌落进同一个 Level 的网格节点内，此时将以最高频的节拍触发该回调进行避障系统告警：

```c
/**
 * @brief 网格空间占用重叠监控钩子
 * 
 * @param host_entity_id 当前主视角的实体（例如：主查探车辆）
 * @param intruding_entity_id 侵入同一网格粒度层的肇事实体 ID
 * @param precision_level 发生重叠的金字塔网格 LOD 警告级别（极可能即将发生擦碰）
 */
typedef void (*overlap_occupancy_cb)(uint64_t host_entity_id, uint64_t intruding_entity_id, int precision_level);
```

## 2. 工程优势：向大模型 (LLM) 抛出定频状态集

之所以强调“体素化”而不是直接给 LLM 发送浮点，是因为现代神经网络张量天生就极其适配处理离散化矩阵。

1. **天然矩阵格式**：由于 `pyramid_query_cb` 抓取出来的数据自带 `grid_x` 和 `grid_y` 这些纯正交的整数属性。在传递给大语言模型时，根本无需重新降维投影，直接可以将其作为多维数组灌入 AI 推理链路。
2. **容错与鲁棒性增强**：哪怕系统在这个阶段扫除了少量的假阳性冗余包（把没撞上但靠得非常近的东西也带上了），在安全系统的宽泛扫描容忍度之内也是极度保险的决策。

此管线为接下来的“精细化连续几何分析（管线 2）”完美挡住了几乎所有的无运算价值数据。
