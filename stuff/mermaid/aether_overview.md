# Aether 核心与服务封装架构图

```mermaid
flowchart TB
    style KERNEL fill:#fff4db,stroke:#d97706,stroke-width:2px,color:#7c2d12
    style SERVER fill:#e8f3ff,stroke:#2563eb,stroke-width:2px,color:#1e3a8a
    style VEXT fill:#eef7ff,stroke:#2563eb,stroke-width:2px,color:#1e3a8a
    style SEXT fill:#eef7ff,stroke:#2563eb,stroke-width:2px,color:#1e3a8a
    style CEXT fill:#eef7ff,stroke:#2563eb,stroke-width:2px,color:#1e3a8a
    style OPS fill:#f8fafc,stroke:#64748b,stroke-width:2px,color:#1e293b

    subgraph KERNEL[1. Aether Kernel 计算内核]
        direction TB
        K0[libae.so]
        K1[Spatial Grid]
        K2[Archetypeless ECS]
        K3[Pyraman Event Timing]
        K4[Ringbuf Eventfd MemArena]

        K0 --> K1
        K0 --> K2
        K0 --> K3
        K0 --> K4

        K1 ~~~ K2
        K2 ~~~ K3
        K3 ~~~ K4
    end

    subgraph SERVER[2. AE Server 服务层]
        S0[AE Server]
        S1[ST UDP Gateway]
        S2[AS Aether Sphinx]
        S3[AP Aether Pyramid]
        S4[API and Protocol]

        S0 --> S1
        S0 --> S2
        S0 --> S3
        S0 --> S4
    end

    subgraph EXT[3. 独立扩展工具集]
        direction LR

        subgraph VEXT[AVA 可视化扩展]
            direction TB
            V0[AVA 调度与渲染中枢]
            V1[Cairo 2D]
            V2[Vulkan 3D]
            V3[Gaussian Splatting]
            V4[Visual Debug Tools]

            V0 --> V1
            V0 --> V2
            V0 --> V3
            V0 --> V4
            V1 ~~~ V2
            V2 ~~~ V3
            V3 ~~~ V4
        end

        subgraph SEXT[空间分析扩展]
            direction TB
            A0[空间分析调度与计算中枢]
            A1[离散格网计算域]
            A2[经典空间计算域]
            A3[时空知识图谱域]

            A0 --> A1
            A0 --> A2
            A0 --> A3
            A1 ~~~ A2
            A2 ~~~ A3
        end

        subgraph CEXT[AE-EXT 客户端扩展]
            direction TB
            C0[AE-EXT 协议转换中枢]
            C1[Cesium]
            C2[ArcGIS]
            C3[QGIS]
            C4[OSGB]

            C0 --> C1
            C0 --> C2
            C0 --> C3
            C0 --> C4
            C1 ~~~ C2
            C2 ~~~ C3
            C3 ~~~ C4
        end
    end

    subgraph AI[4. AI 扩展]
        direction TB
        MCP[MCP Server]
        LLM[外部模型 LLM]
        SK[Skills 工具编排层]

        LLM <--> SK
        LLM <--> MCP
    end

    subgraph OPS[5. 运维工具]
        direction TB
        OP1[数据导入]
        OP2[监控告警与日志]

        OP1 ~~~ OP2
    end

    KERNEL ~~~ SERVER
    SERVER ~~~ OPS
    EXT ~~~ AI

    S3 --> K0
    S2 --> S3
    S1 --> S2
    S3 --> V0
    S4 --> V0
    S3 --> A0
    S4 --> A0
    S3 --> C0
    S4 --> C0
    MCP --> S4
    SK -.调用工具.-> EXT
    S4 --> OP1
    OP1 --> S3
    S3 --> OP2
```

## 说明
- 只有两层：1 内核，2 服务。
- AE Server 负责把内核包起来并提供可运行服务。
- AP 进程链接内核后，Server 体系即可对外运行。
- 扩展模块是独立生态，不属于 AE Server 本体。
- AE Server 通过 API and Protocol 对接可视化、空间分析、客户端三大扩展。
- AI 侧采用外部模型 + MCP + Skills 的扩展结构，通过 Aether API 获取数据并调用扩展工具处理。
- 图中 `Skills -> 独立扩展工具集` 的虚线表示逻辑调用关系，实际执行链路仍通过 MCP/API。
- AVA 作为可视化中枢，从 AE Server 获取数据后分派到不同可视化引擎绘图。
- 空间分析扩展也采用同样模式：从 AE Server 取数后，由分析中枢分派到具体分析工具执行。
- 存量客户端扩展以 AE-EXT 为基础：从 AE Server 取数后完成协议转换，再下发到 Cesium/ArcGIS/QGIS/OSGB。
