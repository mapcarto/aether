---
title: 快速上手与实战指南
description: 探索并配置极简的物理世界模拟闭环
sidebar_position: 2
---

# 快速上手与实战指南 (Quickstart & Practical Guides)

技术文档的成功与否很大程度上取决于其初期的学习曲线。本章节提供一套**渐进式的上手机制**。

## 为什么选择 Aether？(Value Propositions)

作为开发者或架构决策者，在刚接触 Aether 框架时，抛开底层复杂的数学原理，您和您的业务团队将直接获得以下 **4 大颠覆性的工程红利**：

### 1. 资产绝对保全：极其霸道的“零数据侵入”
**在 Aether**：我们极度克制，坚决不碰您的业务数据。您只需要把引擎当作一个“极速空间外挂”。**您的祖传机密算法原封不动**，接入 Aether 重构成本近乎为零。您无需继承任何引擎提供的臃肿基类对象。

**示例体会：完全解耦的数据推送**
```c
// 您的私有、极其复杂且涉密的业务数据对象（引擎甚至不知道它的存在）
struct SecretDrone {
    double ai_tensor_weights[500]; // 机密的神经网络权重
    char pilot_name[32];           // 业务飞手信息
};
struct SecretDrone my_drone;

// 将无人机的空间外包框传给引擎即可，用一个 64位 ID 建立绑定
uint64_t drone_id = 9527;
float bbox_min[3] = {10.0f, 20.0f, 100.0f};
float bbox_max[3] = {12.0f, 22.0f, 101.0f};

// Aether 引擎只接受空间坐标与 ID 锚点，它绝对不会去 Copy 哪怕 1-byte 您的 SecretDrone 结构！
ae_pyramid_insert_box(pyramid_ctx, drone_id, bbox_min, bbox_max); 
```

### 2. 人才结构降维：用“普通开发者的成本”写出“千万级高并发”
**在 Aether**：引擎在内核深处已经把所有极度复杂的并发调度处理得干干净净。向您释放的，是如同流水般清晰单向的“事件回调”。**从此，您的团队只需会写最简单的单线程业务逻辑，就能在宏大的城市级沙盘中斩获顶级算力**。

**示例体会：天下无锁的单线程回调处理**
```c
// 这就是你的一线业务开发者要写的全部代码（完全没有任何互斥锁 mutex 或原子等待的代码污染）
// 引擎内核已经把并发碰撞过滤干净了，只会极其安全地按序投递给你
void on_drone_collision(uint64_t id_A, uint64_t id_B) {
    // 根据安全传回的 ID，去您自己的内存地址里拉取回业务对象
    struct SecretDrone* drone_a = get_my_drone(id_A);
    struct SecretDrone* drone_b = get_my_drone(id_B);
    
    // 执行您的单线程紧急避让逻辑代码，毫无底层心智负担
    execute_avoidance(drone_a, drone_b); 
}

// 注册回调，剩下的高并发网格大逃杀检索全交给 Aether
ae_subscribe_event(EVENT_COLLISION, on_drone_collision);
```

### 3. 稳如磐石的系统沙盒：前端哪怕画风崩损，底层绝对不死机
**在 Aether**：我们确立了极度严苛的“黑盒读写界线”。无论是酷炫的虚幻/Vulkan 渲染框架，还是最前沿的大模型，在 Aether 面前都只能作为“只读旁观者”存在。这确保了主线物理推演**永远以全速且不受干扰的姿态运转**。

**示例体会：完全异步的无阻塞上帝快照**
```c
// 在引擎的另一侧扩展管网中，或者在另外一台负责 3D 渲染/AI 的微服务机器上：
// 渲染大屏不管卡顿到只有 10 帧，还是 AI 大模型因为网络原因思考了 5 分钟
// 它也只能通过此接口“抽走”那一瞬间的副本，Aether 的核心时空引擎依然向 800万次/秒 的吞吐上限狂奔
const ae_snapshot_t* read_only_world = ae_take_snapshot_async(engine_ctx);

// 渲染器基于快照慢慢画，哪怕崩溃了也绝不会拖死引擎的一丝一毫
vulkan_render_frame(read_only_world); 
```

### 4. 极致轻盈与信创自由：把“超算能力”塞进低端盒子里
**在 Aether**：零第三方庞杂依赖，纯正透明的 C 代码结构。从百万级服务器的云原生节点，到算力匮乏的无人机边缘计算板端，甚至是要求极度严苛的**纯国产化信创操作系统**，Aether 都能像水一样无缝渗入。

**示例体会：极致干净的集成部署**
```bash
# 不需要配置庞大的 Java JVM 虚拟机，也不需要动辄几十万行的构建树环境依赖
# 只要带上头文件，极简的动态链接库即可在任意环境（ARM/x86/MIPS）秒级起飞
gcc my_business_logic.c -laether_core -O3 -o my_server

# 零依赖包袱，直接在新安装的国产化“银河麒麟 OS”低空指令机里极其低调地运行
./my_server 
```

---

## 最小可行性示例 (Minimum Viable Example)
> 核心原则：“代码先行，解释在后”

本节将提供可直接利用 C Compiler (`gcc`/`clang`) 编译并观测到终端输出的代码闭环，流程紧扣以下步骤：

1. **初始化 `memarena` 内存池** (底层依赖注入)。
2. **建立基于事件的循环调度器** (Event Loop 构建)。
3. **创建 ECS 组件注册表** (无原型架构的数据布局骨架)。
4. **划分金字塔空间网格系统** (环境拓扑框架建立)。
5. **注入体素化物理对象** (构建极简的三维物体测试)。
6. **触发事件回调观测执行** (利用网格更新事件展示空间变换)。
