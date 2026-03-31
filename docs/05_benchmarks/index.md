---
title: 性能基准与硬件分级指南
description: Aether 引擎在不同规模数据集与硬件平台下的基准表现与部署分级建议。
sidebar_position: 5
---

# 性能基准与硬件分级指南 (Benchmarks & Deployment Matrix)

Aether 的性能特征由多个因素决定：核心寻址算法（Pyraman 调度策略）、内存访问效率（memarena 连续内存池）、以及部署环境（硬件配置、编译参数）。本章分为两部分阐述：

## 📊 核心性能指标与复现方法

[**详见 01_performance_metrics.md**](./01_performance_metrics.md)  
涵盖两大核心指标（网格寻址吞吐、ECS演化）与基准测试复现的硬件配置、负载模型规范。适合**性能优化工程师**与**压测人员**查阅。

---

## 🏗️ 部署方案与专项场景

[**详见 02_deployment_capacity.md**](./02_deployment_capacity.md)  
包含三大专项场景（工业指令、高斯泼溅、协议转换）与三阶硬件部署分级（边缘微节点、边缘计算站、机架服务器）的容量预测。适合**架构师**与**部署规划者**查阅。

---

## 🎯 快速参考

| 用户角色 | 关键指标 | 推荐阅读 |
| :--- | :--- | :--- |
| **性能优化工程师** | 单线程吞吐、缓存命中、NUMA亲和性 | 01_performance_metrics.md §2 |
| **系统集成师** | 部署规模、内存容量、硬件分级 | 02_deployment_capacity.md §4 |
| **应用开发者** | 专项场景适配（工业、VR/AR、协议转换） | 02_deployment_capacity.md §3 |
| **架构评审员** | 全景对标、可扩展性建议、成本优化 | 本页 + 两份详细文档 |
