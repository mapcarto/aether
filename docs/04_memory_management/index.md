---
title: 内存所有权与生命周期
description: memarena 逆向内存生长原理与纯C高压指针契约
sidebar_position: 4
---

# 内存所有权与生命周期流转 (Memory Management Rules)

绝对禁止在业务与关键内环中使用 `malloc`/`free`，Aether 自研的竞技场池 `memarena` 是系统的供血中枢。释放操作系统干预所带来的碎片化灾难与系统锁。

## 1. 极致分配：尾栈记录器 (Tail-stack Recorder)
- 内部分配策略逆向生长分配数据标头。
- 无须进行列表或 Metadata 的节点迭代，瞬间进行 $O(1)$ 精确定位。
- 单次操作消耗 8 纳秒；千万次级操作性能全面逆向碾压顶级开源管理器 (`jemalloc`, `tcmalloc`)。

## 2. 高压严格的生命周期语义契约
当缺乏闭包和对象绑定的 C 环境进行传参交互时，稍有不慎即会因为转移错误诱发 **Dangling Pointers** 与 **Double-free**。这需要以下标准化的所有权约定并在 Doxygen 注释中予以贯彻：

### 2.1 分配持有 (Allocate & Own)
获取对象的绝对所有权（如 `sys_arena_alloc()` 返回），开发者通过在循环尾帧进行水位线回撤 (Watermark Rollback, $O(1)$) 进行安全回收处理。

### 2.2 借用引用 (Borrow Reference)
指针传递 (如 `void calc_bounds(const entity_t *e)` ) 期间，底层严禁越权将其赋值储存于外部全局数组，禁止内部执行释放逻辑。其所有权仍在原始分配的 Arena 事件内。

### 2.3 转移消费 (Transfer Ownership)
传入接口的值 (如 `void system_consume(data_t *d)`) 意味着它在业务调用栈中生命期即刻宣告逻辑终结。即使尚未触发释放，底层流转也不再让其可以安全利用。

## 3. 跨存储层融合的高维操作
在外部显存 (VRAM) 寻址区内，配合 memarena 构建极端的零拷贝渲染网格管理。
