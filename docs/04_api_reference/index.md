---
title: API参考手册与结构体字典
description: Aether 引擎 API 规范、内存所有权定义与并发调用约束。
sidebar_position: 4
---

# API 参考手册与结构体字典 (API Reference Dictionary)

Aether 底层 API 采用明确的内存所有权约定。为确保接口调用的安全性与一致性，所有对外 C 接口均建议遵循 Doxygen 标注规范，以便进行自动化文档生成与静态分析。

## 1. 结构化标签系统 (Standard Tags)
所有头文件中的函数与结构体需按照以下标准维护：
- **@brief**：描述函数或数据结构的具体数学功能或计算逻辑。
- **@param**：定义输入输出参数。涉及指针（如 `void *user_data`）时，需注明生命周期归属。
- **@return**：定义返回值标准及错误代码含义。
- **@warning**：标注线程安全性、可重入性以及非阻塞约束。
- **@see**：关联相关的设计说明或架构约束章节。

---

## 2. 核心引擎回调钩子 (Core Callbacks)

开发人员可通过引擎钩子函数注入业务逻辑。按功能分类如下：

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

---

## 4. E2E 集成示例：简易碰撞检测插件

以下示例展示如何编写一个完整的业务插件，从注册钩子、投递事件、到消费结果的全流程。

```c
#include <aether/core.h>
#include <stdio.h>
#include <stdint.h>

// 业务上下文结构
typedef struct {
    uint32_t collision_count;
    ae_handle_t pyramid_handle;
} collision_ctx_t;

// 1. 事件消费回调：被 Pyraman 调度器自动触发
void on_entity_move_event(const ae_event_t *evt, void *user_data) {
    collision_ctx_t *ctx = (collision_ctx_t *)user_data;
    
    // 解析事件payload
    uint64_t entity_id = evt->entity_id;
    ae_coord3d_t new_pos = evt->position;
    
    // 调用Aether金字塔API做范围查询
    ae_query_result_t result = ae_pyramid_query(
        ctx->pyramid_handle,
        &new_pos,
        10.0f,  // 查询半径 10m
        AE_QUERY_ENTITIES_ONLY
    );
    
    // 逐个检查返回的候选实体
    for (uint32_t i = 0; i < result.count; i++) {
        uint64_t candidate_id = result.entities[i].id;
        
        // 精确求交测试
        if (candidate_id != entity_id) {
            ae_polygon_intersection_test(
                ctx->pyramid_handle,
                entity_id,
                candidate_id
            );
            
            if (/* 碰撞发生 */) {
                ctx->collision_count++;
                printf("Collision detected: %lu vs %lu\n", 
                       entity_id, candidate_id);
            }
        }
    }
    
    ae_query_result_free(&result);
}

// 2. 插件初始化入口（由引擎dlopen调用）
int ae_plugin_init(ae_engine_t *engine, void *config) {
    collision_ctx_t *ctx = (collision_ctx_t *)malloc(sizeof(*ctx));
    ctx->collision_count = 0;
    ctx->pyramid_handle = ae_pyramid_acquire(engine);
    
    // 3. 向引擎注册事件钩子
    ae_event_hook_t hook = {
        .event_type = AE_EVENT_ENTITY_MOVE,
        .callback = on_entity_move_event,
        .user_data = ctx,
        .priority = 10  // 中等优先级
    };
    
    uint32_t hook_id = ae_register_hook(engine, &hook);
    if (hook_id == 0) {
        fprintf(stderr, "Failed to register hook\n");
        free(ctx);
        return -1;
    }
    
    printf("Collision plugin initialized (hook_id=%u)\n", hook_id);
    return 0;  // 返回0表示成功
}

// 4. 插件清理出口
int ae_plugin_fini(ae_engine_t *engine) {
    collision_ctx_t *ctx = (collision_ctx_t *)ae_plugin_get_context(engine);
    printf("Total collisions detected: %u\n", ctx->collision_count);
    free(ctx);
    return 0;
}
```

### 编译与部署
```bash
# 以共享库方式编译插件
gcc -shared -fPIC -O3 collision_plugin.c -o libcollision.so -laether_core

# 在应用启动时动态加载
ae_engine_t *engine = ae_engine_create(...);
ae_plugin_load(engine, "./libcollision.so", NULL);
ae_engine_run(engine);  // 此后插件钩子会被自动触发
```

### 数据流
1. **投递阶段**：当外部系统（如MQTT、UDP网关）检测到实体移动时，构造`AE_EVENT_ENTITY_MOVE`事件并向 MPMC 队列投递
2. **调度阶段**：Pyraman 时间轮根据事件时戳触发
3. **执行阶段**：`on_entity_move_event` 被调度器在独立执行线程中异步拉起
4. **结果**：碰撞计数累积至上下文，可通过 HTTP 端点或 Prometheus 暴露供外部监控系统查询
