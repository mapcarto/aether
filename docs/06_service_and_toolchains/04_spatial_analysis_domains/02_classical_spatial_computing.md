---
title: 经典空间计算域 (Classical Domain)
description: Martinez-Rueda 裁切、Bentley-Ottmann 扫描线与高鲁棒数值计算机制
sidebar_position: 2
---

# 经典空间计算域 (Classical Spatial Computing Domain)

Aether (AE) 引擎将“粗颗粒度的碰撞预警 (体素感官)”和“极其精密的轮廓解析”进行了绝对割裂。经典空间计算域是引擎处理地理测绘、法定边界（无容错率）运算的“高精度手术台”。

---

## 1. 多边形布尔运算核心 (Polygon Boolean Operations)

AE 引擎直接封装了以 `Martinez-Rueda-Feito (MRF)` 与 `Vatti` 为代表的巅峰级多边形运算机制。

### 1.1 Martinez-Rueda-Feito (MRF) 算法
这是 Aether 布尔运算的核心首选算法。相比于 Weiler-Atherton 的点级跟踪，MRF 实现的是更为稳健的**线段级 (Segment level)** 处理，时间复杂度为 $\mathcal{O}((n+k)\log n)$。
- **线段打断与交点计算**：利用扫描线寻找所有线段交点，并在交点处精确打断。
- **填充状态标注 (Fill Annotations)**：对每个打断后的线段赋予严密的拓扑标记，记录该线段的“左侧”与“右侧”是否被多边形 A 或 B 的内部所填充。基于奇偶校验规则，即使是深度嵌套的孔洞也能精准识别。
- **逻辑过滤与重建**：根据 Union, Intersection 等布尔规则提取有效边，重连多边形轮廓。
- **架构优势**：原生无缝处理多部件多边形 (Multipolygons)、自交几何体。

### 1.2 Vatti 扫描束算法 (Scanbeam Clipping)
作为特定的高容错底层组件，Vatti 算法通过垂直切片出的**扫描束 (Scanbeams)** 将复杂的多边形降维为单调线段。其对自交与空洞的极度宽容，使其成为数据清洗阶段的理想选择。

### 1.3 拓扑退化治理与修复 (Topology Repair for Degenerate Cases)
在处理真实地理数据时，共线 (Collinear)、重合点及方向相反的重叠边是算法崩溃的主因。Aether 实施了以下修复策略：
- **重合边合并 (Coincident Edge Overlap)**：在 MRF 标注阶段，若探测到两条边完全重叠且方向相反，系统会自动执行“空洞消除”逻辑。
- **狭缝多边形剔除 (Sliver Removal)**：通过设定极小面积阈值，在布尔运算后自动剔除由于浮点抖动产生的无物理意义的微小多边形（狭缝）。
- **坐标点吸附 (Snapping)**：内置稳健的 $k$-d Tree 索引，在毫秒级内完成对极小公差（Tolerance）范围内的顶点强制对齐，从源头闭合拓扑缺口。

---

## 2. 离散线段求交：Bentley-Ottmann 扫描线

处理数百万量级的线段交点是所有布尔运算的前提。Aether 部署了 **Bentley-Ottmann 扫描线算法**。

- **核心运行机制**：
    - **事件优先队列 (Q)**：最小堆结构。存储所有线段左、右端点。扫描线离散停驻。
    - **扫描线状态树 (T)**：平衡二叉搜索树（红黑树）。按与扫描线交点的 $Y$ 坐标排序。
- **拓扑翻转 (Swap)**：当扫描线通过交点事件时，相邻两线段在状态树中的位置发生互换。
- **效率优势**：成功将 $T(n^2)$ 的暴力求交复杂度压缩至稳健的 $\mathcal{O}((n+k)\log n)$，k 为实际交点数。

### 2.1 数值鲁棒性保障 (Numerical Robustness)
针对浮点精度丢失引发的排序混乱或段错误，Aether 内核在执行底层判定时集成了 Jonathan Shewchuk 的**自适应精度稳健几何谓词 (Fast Robust Geometric Predicates)**。
- 采用浮点扩展 (Floating-point expansions) 技术（如 double-double 或 double-quad 组合表达）。
- 无需高代价的多精度包，即可实现在极端共线或三线交汇场景下的绝对一致性，保障 Bentley-Ottmann 扫描线状态树的拓扑排序绝对准确。

---

## 3. 包含测试与环路判定 (PIP: Point-in-Polygon)

在电子围栏触发场景中，Aether 放弃了脆弱的射线投射法，采用 **Dan Sunday 非零环绕数优化算法 (Non-Zero Winding Number)**。

- **算法机制**：计算多边形边界围绕待测点旋转的总度数。若环绕数（Winding Number, WN）不为 0，则在内部。
- **工程优化**：Dan Sunday 改良版无需反三角函数运算。只需将射线分为两个向度，判断线段是从射线下方跨越（+1）还是上方向下方跨越（-1）。
- **计算性能**：通过简单的符号比较和二维叉积（Cross-product 映射）判断点与线段左右关系。配合 MBR/R*Tree 的 Broad-phase 过滤，在 Narrow-phase 实现极速实时的感知。

---

## 4. 算法性能矩阵对照

| 算法 | 操作级别 | 核心机制 | Aether 架构应用建议 |
| --- | --- | --- | --- |
| **Martinez-Rueda** | 线段级 | 扫描线 + 填充标注 | **核心首选**。支持复杂孔洞、多部件与自交。 |
| **Vatti** | 扫描束 | 空间水平切片 | **后备选型**。对脏数据极度包容。 |
| **Bentley-Ottmann** | 线段求交 | BST 状态树 + 事件堆 | **底层基石**。配合 Shewchuk 稳健谓词。 |
| **Dan Sunday WN** | 包含判定 | 环绕数计算 | **感知模块**。完美解决自交多边形检测噩梦。 |
