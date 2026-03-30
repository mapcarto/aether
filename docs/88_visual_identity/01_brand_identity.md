---
title: 品牌与识别规范 (Brand & Identity)
description: Aether 引擎的官方视觉符号识别手册与 æ 连字标准
sidebar_position: 1
---

# 品牌与识别规范 (Brand & Identity)

Aether (æ) 引擎的视觉识别系统是其“物理世界格点化”核心哲学的符号缩影。

## 1. 核心标识：格点连字 (The Grid Ligature)

标识核心基于古英语连字 **`æ` (Ash Ligature)**，通过极简的 **7x5 离散网格** 比例建模。它象征着 Aether 作为“时空媒介”将离散的运算（Algorithms）与连续的环境（Environments）无缝耦合。

### 视觉资产 (Standard Assets)
*   **矢量图标 (Official)**: [logo.svg](./assets/logo.svg)
*   **预览图**: ![Aether Logo Standard](./assets/aetherlogo.png)
*   **设计源文件**: [logo.ai](./assets/logo.ai)

### 官方 ASCII 签名 (Standard Variant)
用于控制台输出、核心代码 Header 记录。基于 13x5（或等比扩展 7x5）的格点逻辑：

```text
  ████  ████
       █     █
 █████████████
 █     █     
  ████  ████
```

---

## 2. 设计规范

这套标识的核心在于 **143.26 单位** 的基础正方形网格（Grid Unit）。

*   **比例控制**：标识整体宽高比为 **7:5** (1002.84 / 716.31)。
*   **构造细节**：
    *   通过正方形格点的有序堆叠形成 `a` 与 `e` 的融合体。
    *   中心有一条贯穿各层、代表“时空一致性”的实体中轴线 (Crossbar)。

---

## 3. 色彩与应用场景

为了保持底层、高性能且及其克制的工程感，Aether 仅使用二元高对比度配色：

| 元素 | 颜色 | 建议环境 |
| --- | --- | --- |
| **Grid Black** | #000000 | 浅色文档、源码打印、浅色网页背景 |
| **Pure White** | #FFFFFF | 深色 IDE (Dark Mode)、终端控制台 |

### 应用指南
1.  **代码注释头**：建议每个核心子系统入口文件包含此 ASCII 签名。
2.  **CLI 欢迎语**：引擎 `init` 成功后应输出标准 Logo，代表系统物理空间已就绪。
3.  **禁止行为**：严禁对 Logo 进行非等比拉伸、阴影羽化或添加彩色渐变。

