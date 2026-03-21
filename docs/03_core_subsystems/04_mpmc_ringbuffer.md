---
title: 原子状态机与无锁并发模型
description: 解密 MPMC 无锁环形缓冲区，彻底摒弃系统级互斥锁的调度底座
sidebar_position: 4
---

# 原子状态机与无锁并发模型 (Concurrency & MPMC Ring Buffer)

> 在千万级以上空间事件高压突发的场景下，系统调用级别的线程沉睡（互斥锁阻塞）是灾难性的。Aether 选择使用“基于原子状态机的无锁并发”实现极致压榨总线。

## 1. MPMC 无锁环形缓冲区 (Multi-Producer Multi-Consumer Ring Buffer)
与传统采用 `std::mutex` 或 `pthread_mutex_t` 对队列加锁的并发设计截然不同。本模块解密了 Aether 调度事件流真正的输送带：

- **多生产者多消费者模型**：支持海量网格系统线程同时投递事件，以及消费线程进行纳秒级摄取。
- **内存屏障与原子操作 (Memory Barriers & Atomics)**：深入论述如何依赖 C11 标准原生的原子 Compare-And-Swap (CAS) 和 Acquire/Release 强内存顺序，保证在不进入内核态睡眠的情况下安全流转队列数据。

## 2. 原子状态机的并发回退与削峰 (Contention Backoff Strategy)
高并发下 MPMC 最忌讳极端的自旋碰撞 (Spin Contention)。

- 分析在遭遇写冲突雪崩时，系统是如何利用指数退避 (Exponential Backoff) 和微架构级的 `pause` 汇编指令来降低功耗与总线发热的。
- 说明该状态机模型在如何避免常规锁护送效应 (Lock Convoy) 方面提供的卓越表现。

## 3. 并发安全与线程死锁免疫
传统架构在复杂的金字塔网格边界切换并触发跨层重排时极易死锁，而本系统由于事件状态流仅仅是环形槽位上的掩码状态机（Status Flags），其本质上是线程免疫的。
