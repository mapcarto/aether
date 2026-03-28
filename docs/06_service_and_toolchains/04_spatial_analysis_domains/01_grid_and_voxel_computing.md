---
title: 离散格网计算域 (Grid Domain)
description: 莫干山标准对接、地图代数架构与高阶地统计插值实现路径
sidebar_position: 1
---

# 离散格网计算域 (Grid Computing Domain)

在 Aether 处理数以亿计的空间对象查询与碰撞时，离散格网计算域是引擎的第一道感知防线。

**核心哲学：标准无关与全量接驳 (Standard-Agnostic Mapping)**
Aether 引擎在底座层面不强制绑定任何单一的格网系统。其架构本质是提供一套针对**“空间连续剖分 (Continuous Partitioning)”**规范的极致计算映射（Mapping）。只要满足几何拓扑连续性的剖分标准，均可无缝映射至 Aether 进行位运算级别的执行加速。

---

## 1. 离散格网映射机制 (Standard Mapping)

AE 引擎通过降维编码技术，将复杂的多维格网 ID 映射为 Aether 内部的 64 位金字塔序列，实现空间连接（Spatial Joins）的指数级加速。

### 1.1 空间填充曲线与降维编码 (Space-Filling Curves)
任何格网系统的核心在于如何保持**空间局部性 (Spatial Locality)**，即地理邻近的点在内存索引上也邻近：
- **Z-Order (Morton) 编码**：交错经纬度二进制位生成 Z-value。虽然位运算开销极低，但在空间过渡时会产生剧烈的“突变 (Jumps)”，导致边缘相邻但索引值差异巨大，引发范围查询中的“BIGMIN”性能衰退。
- **Hilbert (希尔伯特) 曲线**：Aether 存储层的首选。作为连续遍历机制，其单元格跳转永远只移动至物理相邻网格，极大缓解了突变效应，保障了更极致的局部搜索性能。

### 1.2 主流格网协议兼容性
- **莫干山实验室标准 (前瞻适配)**：原生支持由**莫干山实验室（陈军院士团队）**制定的三维正方体空间剖分格网。该规范作为 2026.04 国家强制标准的底层逻辑，Aether 完成了对其编码规则的原生加速。
- **GeoSOT (地球剖分格网)**：兼容北大的 GeoSOT 全球等经纬剖分体系，支持基于“度、分、秒”逻辑的二进制剖分映射。
- **S2 / H3 插件**：
    - **S2 (Google)**：基于二次变换投影 (Quadratic projection) 的球面立方体映射。
    - **H3 (Uber)**：正二十面体展开架构。利用六边形“单邻接类”特性（到所有邻居中心点距离相等）支持卷积操作。注：H3 采用 Aperture 7 系统，子格网边界会略微超出父格网，存在面积残差。

---

## 2. 地图代数框架 (Map Algebra)

Aether 严格实现了 Dana Tomlin 定义的四大核心空间算子上下文，用于处理气象、高程等连续空间场数据：

### 2.1 局部算子 (Local Operations)
针对独立像元位置执行点对点计算。包含算术（加减乘除）、逻辑（阈值重分类）以及三角函数模型。用于 NDVI 提取、多源危险地图叠加等。

### 2.2 邻域/焦点算子 (Focal Operations)
通过滑动几何核 (Kernel) 计算中心像元新值。
- **微地形衍生算法 (Horne Method)**：在 $3 \times 3$ 窗口计算 X、Y 方向高程偏导数 ($\partial z/\partial x, \partial z/\partial y$)，通过勾股定理求得坡度 (Slope) 与坡向 (Aspect)。
- **卷积滤波**：实现平滑 (FocalMean)、高通滤波与热点统计。

### 2.3 区域算子 (Zonal Operations)
基于掩膜 (Mask) 进行离散区域聚合。调度器汇总具有相同 Zone ID 的像元值，应用统计公式（如 ZonalMean）将结果回写至该区域。

### 2.4 全局与增量推演算子 (Global & Incremental)
- **成本距离 (Cost Distance)**：基于推扫式 (Pushbroom) 或类 Dijkstra 的栅格波面扩展算法，评估物理摩擦力表面下的最小累积通行成本。
- **元胞自动机 (CA)**：推演城市蔓延或森林火灾。结合转换规则 (Transition Rules)、焦点效应与随机微扰项进行动态空间异质性仿真。

---

## 3. 空间表面插值算法 (Spatial Interpolation)

### 3.1 自适应反距离权重 (Adaptive IDW)
坚守“距离衰减”原则。Aether 通过 **KD 树优化** 进行 $k$-最近邻 ($k$-NN) 搜索。
$$Z(x) = \frac{\sum_{i=1}^{n} \frac{Z_i}{d_i^p}}{\sum_{i=1}^{n} \frac{1}{d_i^p}}$$
为规避非真实的“牛眼效应 (Bull's eye effect)”，系统自动调整幂指数 $p$ 并通过多核并行显著降低大范围插值的延时。

### 3.2 克里金地统计插值 (Kriging)
最佳无偏线性估计算法，相比 IDW 能更精准反映趋势并抑制牛眼效应。
- **变异函数拟合 (Semivariogram)**：分析块金效应 (Nugget)、基台值 (Sill) 与变程 (Range)。
- **矩阵求解**：通过求解包含拉格朗日乘子的线性方程组得到最优权重。
- **风险量化**：除了预测面，Aether 还会输出**预测方差面 (Kriging Variance)**，量化各区域结果的不确定性风险。
- **工程加速**：结合高性能数学计算库 (BLAS/LAPACK) 与 GPU 并行化，处理 $O(n^3)$ 复杂度的求逆矩阵挑战。

---

## 5. 层级一致性与跨尺度聚合 (LOD Consistency)

针对莫干山或 GeoSOT 等具有严格层级体系的格网，Aether 在执行空间降采样 (Downsampling) 或多尺度分析时，实施以下几何一致性协议：
- **面积残差修正 (Area Residual Correction)**：在从高分辨率（如 Level 20）向低分辨率（如 Level 16）聚合时，针对球面投影带来的单格网面积差异，应用加权平均算子。
- **拓扑稳定性验证**：确保在 LOD 切换过程中，关键点位的“包含关系 (Containment)”逻辑不因浮点截断导致假阴性。
- **父子格网编码的一致性快照**：支持基于前缀匹配的 $\mathcal{O}(1)$ 复杂度的跨层级实体统计。
