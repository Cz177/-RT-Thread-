# 智能器件存储盒 - 系统整体框架图

---

## 一、硬件架构图

```mermaid
graph TB
    subgraph ART-PI开发板[ART-PI开发板]
        MCU[STM32H750XBHX<br/>Cortex-M7 @ 480MHz]
        
        subgraph 存储单元
            Flash[QSPI Flash 16MB<br/>0x90000000]
            SRAM[AXI SRAM 512KB<br/>0x24000000]
            SDRAM[SDRAM 32MB<br/>0xC0000000]
        end
        
        subgraph 人机交互外设
            LCD[800×480 TFT LCD]
            Touch[电容触摸屏]
            Buzzer[蜂鸣器 PG11]
            LED[LED0 PI8]
        end
        
        subgraph 通信接口
            UART1[UART1 PA9/PA10]
            UART4[UART4 PA0/PI9]
            UART6[UART6 PC6/PC7]
        end
        
        MCU --- Flash
        MCU --- SRAM
        MCU --- SDRAM
        MCU --- LCD
        MCU --- Touch
        MCU --- Buzzer
        MCU --- LED
        MCU --- UART1
        MCU --- UART4
        MCU --- UART6
    end
    
    subgraph 外部执行机构
        Motor[直流电机]
        Encoder[脉冲编码器 pulse3]
        Hall[霍尔传感器 PA15]
        Servo1[舵机1 PWM4-CH4]
        Servo2[舵机2 PWM4-CH3]
        Servo3[舵机3 PWM2-CH2]
    end
    
    subgraph 预留扩展模块
        Voice[语音模块]
        Wifi[Wi-Fi模块]
    end
    
    MCU --- Motor
    MCU --- Encoder
    MCU --- Hall
    MCU --- Servo1
    MCU --- Servo2
    MCU --- Servo3
    UART6 --- Voice
    
    style MCU fill:#4CAF50,color:#fff,stroke:#333,stroke-width:2px
    style ART-PI开发板 fill:#e3f2fd,stroke:#2196F3,stroke-width:2px
    style 外部执行机构 fill:#fff3e0,stroke:#FF9800,stroke-width:2px
    style 预留扩展模块 fill:#f3e5f5,stroke:#9C27B0,stroke-width:2px
```

---

## 二、软件架构图

```mermaid
graph TB
    subgraph 驱动层[驱动层 Driver Layer]
        PWM[PWM驱动]
        Pulse[脉冲编码器驱动]
        UARTDrv[UART驱动]
        TouchDrv[触摸屏驱动]
        LCDDrv[LCD驱动]
        PinDrv[GPIO引脚驱动]
    end
    
    subgraph 操作系统层[操作系统层 OS Layer]
        RTKernel[RT-Thread Kernel]
        
        subgraph IPC机制
            MsgQueue[消息队列]
            Event[事件标志]
        end
        
        subgraph 设备管理
            DeviceMgr[设备管理器]
            MemoryMgr[内存管理]
        end
        
        RTKernel --- MsgQueue
        RTKernel --- Event
        RTKernel --- DeviceMgr
        RTKernel --- MemoryMgr
    end
    
    subgraph 中间件层[中间件层 Middleware Layer]
        LVGL[LVGL 8.3.11]
        FlashDB[FlashDB]
        cJSON[cJSON]
    end
    
    subgraph 应用层[应用层 Application Layer]
        subgraph GUI模块
            MainScreen[主界面]
            AddScreen[添加界面]
            FindScreen[查找界面]
        end
        
        subgraph 控制模块
            MotorCtrl[电机控制线程]
            ServoCtrl[舵机控制线程]
            BuzzerCtrl[蜂鸣器线程]
        end
        
        subgraph 数据模块
            DB[器件数据库]
            KVStore[KV存储]
        end
    end
    
    驱动层 --- 操作系统层
    操作系统层 --- 中间件层
    中间件层 --- 应用层
    
    TouchDrv --> LVGL
    LCDDrv --> LVGL
    LVGL --> MainScreen
    LVGL --> AddScreen
    LVGL --> FindScreen
    
    MsgQueue --> MotorCtrl
    MsgQueue --> ServoCtrl
    Event --> MotorCtrl
    Event --> ServoCtrl
    Event --> BuzzerCtrl
    
    MotorCtrl --> PWM
    MotorCtrl --> Pulse
    MotorCtrl --> PinDrv
    
    ServoCtrl --> PWM
    BuzzerCtrl --> PinDrv
    
    DB --> KVStore
    KVStore --> FlashDB
```

---

## 三、电源架构图

```mermaid
graph TB
    subgraph 电源输入
        Input[12V 电源适配器]
    end
    
    subgraph 电源分配
        DC12V[12V 电源总线]
        DC5V[5V 电源总线<br/>LM2596降压]
        DC3V3[3.3V 电源总线<br/>AMS1117]
    end
    
    subgraph 主控制器供电
        MCU[STM32H750]
        LCD[LCD屏幕]
        Touch[触摸屏]
        Flash[QSPI Flash]
        SDRAM[SDRAM]
    end
    
    subgraph 电机驱动供电
        MotorDriver[L298N驱动板]
        Motor[直流电机]
    end
    
    subgraph 舵机供电
        Servo1[舵机1]
        Servo2[舵机2]
        Servo3[舵机3]
    end
    
    subgraph 传感器供电
        Encoder[编码器]
        Hall[霍尔传感器]
        Buzzer[蜂鸣器]
    end
    
    Input --> DC12V
    DC12V --> DC5V
    DC12V --> MotorDriver
    DC5V --> DC3V3
    DC5V --> Servo1
    DC5V --> Servo2
    DC5V --> Servo3
    DC3V3 --> MCU
    DC3V3 --> LCD
    DC3V3 --> Touch
    DC3V3 --> Flash
    DC3V3 --> SDRAM
    DC3V3 --> Encoder
    DC3V3 --> Hall
    DC3V3 --> Buzzer
    MotorDriver --> Motor
    
    style Input fill:#ff5252,color:#fff,stroke:#333,stroke-width:2px
    style DC12V fill:#ff9800,color:#fff,stroke:#333,stroke-width:2px
    style DC5V fill:#ffc107,color:#333,stroke:#333,stroke-width:2px
    style DC3V3 fill:#4caf50,color:#fff,stroke:#333,stroke-width:2px
```

---

## 四、软件模块交互图

```mermaid
sequenceDiagram
    participant User as 用户
    participant GUI as LVGL GUI
    participant Event as 事件处理
    participant DB as FlashDB
    participant MotorMQ as motor_mq
    participant ServoMQ as servo_mq
    participant Motor as 电机线程
    participant Servo as 舵机线程
    participant EventFlag as global_event
    
    User->>GUI: 触摸操作(添加/查找/取件)
    GUI->>Event: 触发事件回调
    
    alt 添加器件
        Event->>Event: 组装JSON数据
        Event->>DB: components_add_kv()
        DB-->>Event: 返回存储位置key
        Event->>MotorMQ: 发送pos
        Event->>ServoMQ: 发送layer
        GUI->>User: 等待完成提示
        User->>GUI: 点击完成
        GUI->>Event: EVENT_PICK_FINNISH
    else 查找器件
        Event->>Event: 组装筛选条件
        Event->>DB: components_find_kv()
        DB-->>Event: 返回匹配器件列表
        Event->>GUI: 更新列表显示
    else 取件操作
        Event->>DB: 更新数量/删除记录
        Event->>MotorMQ: 发送pos
        Event->>ServoMQ: 发送layer
        GUI->>User: 等待取件提示
        User->>GUI: 点击完成
        GUI->>Event: EVENT_PICK_FINNISH
    end
    
    Motor->>MotorMQ: 接收pos
    Motor->>Motor: PID定位
    Motor->>EventFlag: EVENT_POSITION_OK
    
    Servo->>ServoMQ: 接收layer
    Servo->>EventFlag: 等待EVENT_POSITION_OK
    EventFlag-->>Servo: 位置到达
    
    Servo->>Servo: 舵机开门
    Servo->>EventFlag: 等待EVENT_PICK_FINNISH
    EventFlag-->>Servo: 取件完成
    Servo->>Servo: 舵机关门
```

---

## 五、数据流程图

```mermaid
flowchart TD
    subgraph 输入层
        A[用户输入] --> B[触摸屏]
        C[语音输入] --> D[UART6]
    end
    
    subgraph 处理层
        B --> E[LVGL事件处理]
        D --> F[语音解析]
        E --> G{操作类型}
        F --> G
        
        G -->|添加| H[组装JSON数据]
        G -->|查找| I[组装筛选条件]
        G -->|取件| J[解析取件参数]
        
        H --> K[FlashDB写入]
        I --> L[FlashDB查询]
        J --> M[更新数量]
        M --> K
        
        K --> N[分配存储位置]
        L --> O[返回匹配结果]
    end
    
    subgraph 输出层
        N --> P[发送位置指令]
        O --> Q[更新界面列表]
        P --> R[电机定位]
        R --> S[舵机控制]
        S --> T[取件门开关]
    end
    
    subgraph 存储层
        U[(QSPI Flash)]
        K --> U
        L --> U
        M --> U
    end
    
    style A fill:#f9f,stroke:#333,stroke-width:2px
    style C fill:#f9f,stroke:#333,stroke-width:2px
    style U fill:#bbf,stroke:#333,stroke-width:2px
```

---

## 六、线程调度与通信图

> **RT-Thread优先级说明**：数字越小优先级越高（priority:10 > priority:11 > priority:20）

```mermaid
graph LR
    subgraph 线程优先级[线程优先级: 数字越小优先级越高]
        style Threads fill:#fff,stroke:#333,stroke-width:2px
        
        Motor[motor线程<br/>priority:10]
        Servo[servo线程<br/>priority:11]
        Main[main线程<br/>priority:默认]
        Buzzer[buzzer线程<br/>priority:20<br/>最低优先级]
        GUI[LVGL线程<br/>优先级低于10]
    end
    
    subgraph 通信机制
        style Comm fill:#f0f,stroke:#333,stroke-width:2px
        
        MQ1[motor_mq<br/>uint8_t]
        MQ2[servo_mq<br/>uint8_t]
        Event[global_event<br/>rt_uint32_t]
    end
    
    subgraph 事件定义
        style Events fill:#ff0,stroke:#333,stroke-width:2px
        
        EV1[EVENT_PICK_FINNISH<br/>1<<2]
        EV2[EVENT_POSITION_OK<br/>1<<3]
        EV3[EVENT_TOUCH_SCREEN<br/>1<<4]
        EV4[EVENT_ZERO_OK<br/>1<<5]
    end
    
    Main -->|初始化| Motor
    Main -->|初始化| Servo
    Main -->|初始化| Buzzer
    
    GUI -->|发送| MQ1
    GUI -->|发送| MQ2
    GUI -->|发送| EV1
    GUI -->|发送| EV3
    
    MQ1 -->|接收| Motor
    MQ2 -->|接收| Servo
    
    Motor -->|发送| EV2
    Servo -->|等待| EV2
    Servo -->|等待| EV1
    Buzzer -->|等待| EV3
    
    Event --- EV1
    Event --- EV2
    Event --- EV3
    Event --- EV4
    
    Motor -.-> Servo -.-> Main -.-> Buzzer -.-> GUI
```

---

## 七、硬件连接示意图

```mermaid
graph TB
    subgraph ART-PI开发板
        MCU[STM32H750]
        
        subgraph 板载外设
            LCD[LCD屏幕]
            Touch[触摸屏]
            Flash[QSPI Flash]
            SDRAM[32MB SDRAM]
        end
        
        subgraph 扩展接口
            PWM15["PWM15-CH2<br/>PH2/PH3"]
            PWM4["PWM4-CH3/CH4<br/>PB8/PB9"]
            PWM2["PWM2-CH2<br/>PA7"]
            Encoder["pulse3<br/>编码器"]
            Hall["PA15<br/>霍尔传感器"]
            Buzzer["PG11<br/>蜂鸣器"]
            LED["PI8<br/>LED0"]
            UART6["PC6/PC7<br/>UART6"]
        end
        
        MCU --- LCD
        MCU --- Touch
        MCU --- Flash
        MCU --- SDRAM
    end
    
    subgraph 外部设备
        Motor[直流电机<br/>+ L298N驱动]
        Servo1[舵机1]
        Servo2[舵机2]
        Servo3[舵机3]
        EncoderDev[编码器]
        HallDev[霍尔传感器]
        BuzzerDev[蜂鸣器]
        Voice[语音模块<br/>(预留)]
    end
    
    PWM15 --- Motor
    Encoder --- EncoderDev
    Hall --- HallDev
    PWM4 --- Servo1
    PWM4 --- Servo2
    PWM2 --- Servo3
    Buzzer --- BuzzerDev
    UART6 --- Voice
    
    style MCU fill:#4CAF50,color:#fff,stroke:#333,stroke-width:2px
    style ART-PI开发板 fill:#e3f2fd,stroke:#2196F3,stroke-width:2px
    style 外部设备 fill:#fff3e0,stroke:#FF9800,stroke-width:2px
```

---

## 八、存储架构图

```mermaid
graph TD
    subgraph FlashDB存储布局
        style FlashDB fill:#bbf,stroke:#333,stroke-width:2px
        
        KVDB[components_kv_db]
        
        subgraph 默认KV节点
            BootCount["boot_count<br/>uint16_t"]
        end
        
        subgraph 器件存储区
            direction LR
            K0["00: component_info"]
            K1["01: component_info"]
            K2["02: component_info"]
            K3["..."]
            K35["35: component_info"]
        end
        
        KVDB --- BootCount
        KVDB --- K0
        KVDB --- K1
        KVDB --- K2
        KVDB --- K3
        KVDB --- K35
    end
    
    subgraph component_info结构
        style component_info fill:#fbb,stroke:#333,stroke-width:2px
        
        ParaInfo["para_info[128]<br/>JSON格式"]
        Quantity["quantity<br/>uint32_t"]
        
        ParaInfo --- Quantity
    end
    
    subgraph JSON数据结构
        style JSON fill:#bfb,stroke:#333,stroke-width:2px
        
        Type["type: Resistor/Capacitor/Inductor"]
        Val["val: 10k/100n/10u"]
        Pkg["pkg: 0603/0805/1206"]
        Rated["rated val: 1/4W/6.3V"]
        Acc["accuracy: 5%/10%"]
        Other["other: 贴片/直插"]
        
        Type --- Val --- Pkg --- Rated --- Acc --- Other
    end
    
    K0 --- component_info
    component_info --- JSON
```

---

## 九、取件流程时序图

```mermaid
sequenceDiagram
    participant User as 用户
    participant GUI as LVGL界面
    participant DB as FlashDB
    participant Motor as 电机线程
    participant Servo as 舵机线程
    participant Event as global_event
    
    User->>GUI: 选择器件并点击取件
    GUI->>DB: 更新器件数量
    alt 数量取完
        DB->>DB: 删除KV记录
    else 还有剩余
        DB->>DB: 更新数量
    end
    
    GUI->>Motor: 发送pos到motor_mq
    GUI->>Servo: 发送layer到servo_mq
    
    Motor->>Motor: PID定位循环
    loop PID控制
        Motor->>Motor: 读取编码器
        Motor->>Motor: 计算误差
        Motor->>Motor: 输出PWM
    end
    
    Motor->>Event: EVENT_POSITION_OK
    
    Servo->>Event: 等待EVENT_POSITION_OK
    Event-->>Servo: 位置到达
    
    Servo->>Servo: 舵机开门(PWM=1625000ns)
    
    User->>GUI: 完成取件点击按钮
    GUI->>Event: EVENT_PICK_FINNISH
    
    Servo->>Event: 等待EVENT_PICK_FINNISH
    Event-->>Servo: 取件完成
    
    Servo->>Servo: 舵机关门(PWM=500000ns)
```

---

## 十、系统整体架构全景图

```mermaid
graph TD
    subgraph 硬件层
        H1[STM32H750]
        H2[LCD+触摸屏]
        H3[电机+编码器]
        H4[3个舵机]
        H5[霍尔传感器]
        H6[蜂鸣器/LED]
        H7[UART通信]
        H8[Flash/SDRAM]
    end
    
    subgraph 驱动层
        D1[PWM驱动]
        D2[编码器驱动]
        D3[触摸屏驱动]
        D4[LCD驱动]
        D5[GPIO驱动]
        D6[UART驱动]
    end
    
    subgraph RT-Thread
        OS1[线程管理]
        OS2[消息队列]
        OS3[事件标志]
        OS4[设备管理]
    end
    
    subgraph 中间件
        M1[LVGL]
        M2[FlashDB]
        M3[cJSON]
    end
    
    subgraph 应用层
        A1[GUI界面]
        A2[电机控制]
        A3[舵机控制]
        A4[数据存储]
        A5[事件处理]
    end
    
    subgraph 用户交互
        U1[触摸操作]
        U2[取件/添加]
        U3[语音控制]
    end
    
    H1 --> D1 & D2 & D3 & D4 & D5 & D6
    D1 & D2 & D3 & D4 & D5 & D6 --> OS1 & OS4
    OS1 & OS2 & OS3 & OS4 --> M1 & M2 & M3
    M1 & M2 & M3 --> A1 & A2 & A3 & A4 & A5
    
    U1 --> H2 --> D3 --> M1 --> A1 --> A5
    A5 --> OS2 --> A2 --> D1 --> H3
    A5 --> OS2 --> A3 --> D1 --> H4
    A5 --> OS3 --> A2 --> D5 --> H5
    A5 --> OS3 --> A6 --> D5 --> H6
    A5 --> A4 --> M2 --> D6 --> H8
    U3 --> H7 --> D6 --> M3 --> A5
    
    style H1 fill:#4CAF50,color:#fff
    style OS1 fill:#2196F3,color:#fff
    style M1 fill:#FF9800,color:#fff
    style A1 fill:#9C27B0,color:#fff
```

---

**文档版本**：V1.1  
**创建日期**：2026-06-29  
**项目类型**：嵌入式智能存储系统  
**运行平台**：ART-PI (STM32H750) + RT-Thread