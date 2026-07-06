# 智能器件存储盒 - 软件系统设计文档

---

## 1.1 软件系统介绍

### 1.1.1 系统概述

本项目软件系统基于**RT-Thread**嵌入式实时操作系统开发，运行于ART-PI开发板（STM32H750XBHX）。系统采用分层架构设计，从底层硬件驱动到上层应用界面，实现了器件管理、位置定位、取件控制、数据持久化等核心功能。

### 1.1.2 技术架构

| 层次 | 组件 | 职责 |
|------|------|------|
| **应用层** | LVGL GUI、电机控制线程、舵机控制线程、数据存储、事件处理 | 业务逻辑实现 |
| **中间件层** | LVGL 8.3.11、FlashDB、cJSON | 图形界面、数据存储、JSON解析 |
| **操作系统层** | RT-Thread Kernel | 线程管理、IPC通信、设备管理 |
| **驱动层** | PWM驱动、编码器驱动、触摸屏驱动、LCD驱动、UART驱动 | 硬件设备访问 |

### 1.1.3 核心特性

| 特性 | 实现方式 | 关键技术 |
|------|---------|---------|
| **实时控制** | 多线程调度 | RT-Thread线程管理 |
| **精确定位** | PID闭环控制 | 脉冲编码器反馈 |
| **数据持久化** | 键值存储 | FlashDB |
| **人机交互** | 触摸屏界面 | LVGL 8.3.11 |
| **线程同步** | 消息队列+事件标志 | RT-Thread IPC机制 |

---

## 2.3.1 软件整体介绍

### 2.3.1.1 纯嵌入式架构

本项目当前为**纯嵌入式系统**，无PC端或云端软件。所有数据处理和控制逻辑均在ART-PI开发板上完成，系统架构如下：

```mermaid
graph TB
    subgraph 硬件层
        MCU[STM32H750]
        LCD[800×480 LCD]
        Touch[电容触摸屏]
        Motor[直流电机]
        Encoder[编码器]
        Servo[3×舵机]
        Flash[QSPI Flash]
    end
    
    subgraph 驱动层
        PWM[PWM驱动]
        Pulse[编码器驱动]
        TouchDrv[触摸屏驱动]
        LCDDrv[LCD驱动]
    end
    
    subgraph RT-Thread
        Thread[线程管理]
        MQ[消息队列]
        Event[事件标志]
    end
    
    subgraph 中间件
        LVGL[LVGL]
        FlashDB[FlashDB]
        cJSON[cJSON]
    end
    
    subgraph 应用层
        GUI[LVGL界面]
        MotorCtrl[电机控制]
        ServoCtrl[舵机控制]
        DB[数据存储]
        EventProc[事件处理]
    end
    
    MCU --> PWM & Pulse & TouchDrv & LCDDrv
    PWM & Pulse & TouchDrv & LCDDrv --> Thread & MQ & Event
    Thread & MQ & Event --> LVGL & FlashDB & cJSON
    LVGL & FlashDB & cJSON --> GUI & MotorCtrl & ServoCtrl & DB & EventProc
    
    Touch --> TouchDrv --> LVGL --> GUI --> EventProc
    EventProc --> MQ --> MotorCtrl --> PWM --> Motor
    EventProc --> MQ --> ServoCtrl --> PWM --> Servo
    EventProc --> DB --> FlashDB --> Flash
    
    style MCU fill:#4CAF50,color:#fff
    style Thread fill:#2196F3,color:#fff
    style LVGL fill:#FF9800,color:#fff
    style GUI fill:#9C27B0,color:#fff
```

### 2.3.1.2 云端/AI扩展方案

**ESP32-S3智能语音交互扩展**：

| 扩展模块 | 功能 | 通信方式 |
|---------|------|---------|
| ESP32-S3开发板 | 小智AI语音识别、网络连接 | UART6（PC6/PC7） |
| 麦克风 | 语音输入 | I2S/模拟输入 |
| 扬声器 | 语音反馈 | PWM/DAC |

**扩展架构图**：

```mermaid
graph TB
    subgraph ART-PI主控
        MCU[STM32H750]
        LVGL[LVGL界面]
        FlashDB[FlashDB]
        MotorCtrl[电机控制]
        ServoCtrl[舵机控制]
    end
    
    subgraph ESP32-S3智能节点
        AI[小智AI语音识别]
        Wifi[Wi-Fi模块]
        Mic[麦克风]
        Speaker[扬声器]
    end
    
    subgraph 云端服务
        Cloud[云端AI大模型]
        Server[数据同步服务器]
    end
    
    MCU -->|UART6| AI
    AI --> Mic
    AI --> Speaker
    AI --> Wifi
    Wifi --> Cloud
    Wifi --> Server
    
    LVGL -->|命令| MCU
    MCU -->|执行| MotorCtrl
    MCU -->|执行| ServoCtrl
    MCU -->|存储| FlashDB
    
    style MCU fill:#4CAF50,color:#fff
    style AI fill:#FF9800,color:#fff
    style Cloud fill:#2196F3,color:#fff
```

**通信协议（JSON格式）**：

```json
{
    "type": "command",
    "action": "get_component",
    "params": {
        "type": "Resistor",
        "val": "10k",
        "pkg": "0603"
    }
}
```

**ESP32-S3与ART-PI交互流程**：

```mermaid
sequenceDiagram
    participant User as 用户
    participant ESP as ESP32-S3
    participant ART as ART-PI
    participant Motor as 电机线程
    participant Servo as 舵机线程
    
    User->>ESP: 语音命令"取10k电阻"
    ESP->>ESP: 小智AI语音识别
    ESP->>ART: UART6发送JSON命令
    ART->>ART: 解析JSON，查询FlashDB
    ART->>Motor: 发送位置到motor_mq
    ART->>Servo: 发送层数到servo_mq
    Motor->>Motor: PID定位
    Motor->>ART: EVENT_POSITION_OK
    Servo->>Servo: 舵机开门
    Servo->>ART: 等待EVENT_PICK_FINNISH
    User->>ART: 取件完成
    ART->>ESP: UART6发送完成状态
    ESP->>User: 语音反馈"已完成"
```

---

## 2.3.2 软件各模块介绍

### 2.3.2.1 主控模块（main.c）

#### 模块概述

主控模块是系统的入口，负责初始化所有子模块、创建全局事件标志、启动系统主循环。

#### 模块流程图

```mermaid
flowchart TD
    A[系统上电启动] --> B[初始化LED0引脚]
    B --> C{RT_USING_WIFI?}
    C -->|是| D[初始化Wi-Fi自动连接]
    C -->|否| E[跳过Wi-Fi初始化]
    D --> F[创建全局事件标志]
    E --> F
    F --> G{事件创建成功?}
    G -->|否| H[打印错误信息，退出]
    G -->|是| I[初始化FlashDB数据库]
    I --> J[初始化蜂鸣器线程]
    J --> K[初始化舵机控制线程]
    K --> L[初始化电机控制线程]
    L --> M[系统主循环]
    M --> N[LED0闪烁]
    N --> M
```

#### 关键函数说明

**main()** - 系统主函数

| 项目 | 说明 |
|------|------|
| **函数签名** | `int main(void)` |
| **输入参数** | 无 |
| **输出/返回值** | `int` - 0表示正常退出 |
| **核心逻辑** | 初始化硬件和软件模块，创建全局事件标志，启动子线程，进入主循环 |

```mermaid
flowchart TD
    A[main()] --> B[rt_pin_mode(LED0_PIN, OUTPUT)]
    B --> C{RT_USING_WIFI}
    C -->|true| D[wlan_autoconnect_init()]
    C -->|false| E[跳过]
    D --> F[rt_wlan_config_autoreconnect]
    E --> F
    F --> G[rt_event_create]
    G --> H{global_event != NULL?}
    H -->|false| I[rt_kprintf error]
    H -->|true| J[database_init()]
    J --> K[buzzer_thread_init()]
    K --> L[servo_thread_init()]
    L --> M[encoder_thread_init()]
    M --> N[while(1)循环]
    N --> O[rt_pin_write LED0 HIGH]
    O --> P[rt_thread_mdelay 500]
    P --> Q[rt_pin_write LED0 LOW]
    Q --> R[rt_thread_mdelay 500]
    R --> N
```

---

### 2.3.2.2 事件定义模块（pv_event.h）

#### 模块概述

定义全局事件标志位，用于线程间同步通信。

#### 事件定义表

| 事件名称 | 位掩码 | 触发时机 | 用途 |
|---------|--------|---------|------|
| EVENT_PICK_FINNISH | `1 << 2` | 用户完成取件操作 | 通知舵机关门 |
| EVENT_POSITION_OK | `1 << 3` | 电机定位完成 | 通知舵机开门 |
| EVENT_TOUCH_SCREEN | `1 << 4` | 触摸屏被触摸 | 触发蜂鸣器反馈 |
| EVENT_ZERO_OK | `1 << 5` | 电机归零完成 | 通知系统就绪 |

---

### 2.3.2.3 电机控制模块（encoder_motor.c）

#### 模块概述

电机控制模块负责直流电机的PID闭环控制，实现精确定位到目标位置，并支持电机归零功能。

#### 模块流程图

```mermaid
flowchart TD
    A[encoder_thread_init] --> B[创建motor_mq消息队列]
    B --> C[打开脉冲编码器设备]
    C --> D[打开PWM设备]
    D --> E[设置电机方向引脚]
    E --> F[设置霍尔传感器引脚]
    F --> G[电机归零]
    G --> H[创建电机控制线程]
    H --> I[电机线程循环]
    I --> J[接收目标位置pos]
    J --> K[计算目标脉冲数 target=pos*290]
    K --> L[PID控制循环]
    L --> M[读取编码器计数]
    M --> N[计算误差 error=target-actual]
    N --> O{误差<5且连续10次?}
    O -->|是| P[发送EVENT_POSITION_OK]
    O -->|否| Q[PID计算输出PWM]
    Q --> R[延时20ms]
    R --> L
    P --> S[等待EVENT_PICK_FINNISH]
    S --> I
```

#### 关键函数说明

**encoder_thread_init()** - 电机线程初始化

| 项目 | 说明 |
|------|------|
| **函数签名** | `void encoder_thread_init(void)` |
| **输入参数** | 无 |
| **输出/返回值** | 无 |
| **核心逻辑** | 创建消息队列、打开编码器和PWM设备、初始化引脚、执行归零、创建控制线程 |

```mermaid
flowchart TD
    A[encoder_thread_init] --> B[rt_mq_create motor_mq]
    B --> C{motor_mq != NULL?}
    C -->|false| D[rt_kprintf error return]
    C -->|true| E[rt_device_find pulse3]
    E --> F{encoder_dev != NULL?}
    F -->|false| G[rt_kprintf error return]
    F -->|true| H[rt_device_open encoder_dev]
    H --> I{res == RT_EOK?}
    I -->|false| J[rt_kprintf error return]
    I -->|true| K[rt_device_find pwm15]
    K --> L{motor_pwm_dev != NULL?}
    L -->|false| M[rt_kprintf error return]
    L -->|true| N[rt_pwm_set period=1000000 pulse=0]
    N --> O[rt_pwm_enable]
    O --> P[rt_pin_mode MOTOR_IN1/IN2 OUTPUT]
    P --> Q[rt_pin_mode HALL_PIN INPUT_PULLUP]
    Q --> R[motor_zero_out]
    R --> S[rt_thread_create motor]
    S --> T[rt_thread_startup]
```

**encodermotor_entry()** - 电机PID控制线程

| 项目 | 说明 |
|------|------|
| **函数签名** | `void encodermotor_entry(void *parameter)` |
| **输入参数** | `parameter` - 线程参数（未使用） |
| **输出/返回值** | 无 |
| **核心逻辑** | 接收目标位置，执行PID闭环控制，定位完成后发送事件标志 |

```mermaid
flowchart TD
    A[encodermotor_entry] --> B[初始化PID参数 Kp=0.35,Ki=0.05,Kd=0]
    B --> C[rt_mq_recv motor_mq]
    C --> D[target = pos * 290]
    D --> E[while true]
    E --> F[rt_device_read encoder]
    F --> G[rt_device_control clear count]
    G --> H[actual += count]
    H --> I[error_prev = error]
    I --> J[error = target - actual]
    J --> K{abs error < 5?}
    K -->|true| L[t++]
    L --> M{t == 10?}
    M -->|true| N[motor_set_pwm 0]
    N --> O[rt_event_send EVENT_POSITION_OK]
    O --> P[break PID循环]
    M -->|false| Q[continue]
    K -->|false| R[t = 0]
    R --> S{abs error < 30?}
    S -->|true| T[integral += error]
    S -->|false| U[integral = 0]
    T --> V[pwm = Kp*e + Ki*∫e + Kd*de]
    U --> V
    V --> W{pwm > 30?}
    W -->|true| X[pwm = 30]
    W -->|false| Y{pwm < -30?}
    Y -->|true| Z[pwm = -30]
    Y -->|false| AA[motor_set_pwm pwm]
    X --> AA
    Z --> AA
    AA --> AB[rt_thread_mdelay 20]
    AB --> E
    P --> AC[rt_event_recv EVENT_PICK_FINNISH]
    AC --> AD[rt_thread_delay 20]
    AD --> C
```

**motor_set_pwm()** - 设置电机PWM输出

| 项目 | 说明 |
|------|------|
| **函数签名** | `void motor_set_pwm(int32_t pwm_val)` |
| **输入参数** | `pwm_val` - PWM值（-100~100，正负表示方向） |
| **输出/返回值** | 无 |
| **核心逻辑** | 根据PWM值设置电机方向和脉冲宽度 |

```mermaid
flowchart TD
    A[motor_set_pwm pwm_val] --> B{pwm_val > 100?}
    B -->|true| C[pwm_val = 100]
    B -->|false| D{pwm_val < -100?}
    D -->|true| E[pwm_val = -100]
    D -->|false| F{pwm_val >= 0?}
    C --> F
    E --> F
    F -->|true| G[rt_pin_write IN1=1 IN2=0]
    F -->|false| H[rt_pin_write IN1=0 IN2=1]
    G --> I[pulse = pwm_val * 10000]
    H --> J[pulse = -pwm_val * 10000]
    I --> K[rt_pwm_set motor_pwm_dev]
    J --> K
```

**motor_zero_out()** - 电机归零

| 项目 | 说明 |
|------|------|
| **函数签名** | `void motor_zero_out(void)` |
| **输入参数** | 无 |
| **输出/返回值** | 无 |
| **核心逻辑** | 控制电机转动，检测霍尔传感器上升沿实现归零 |

```mermaid
flowchart TD
    A[motor_zero_out] --> B[motor_set_pwm 30]
    B --> C[while true]
    C --> D[pin_state_prev = pin_state]
    D --> E[pin_state = rt_pin_read HALL_PIN]
    E --> F{pin_state == HIGH && pin_state_prev == LOW?}
    F -->|true| G[rt_device_read encoder]
    G --> H[rt_device_control clear count]
    H --> I{count > 600?}
    I -->|true| J[motor_set_pwm 0]
    J --> K[break]
    I -->|false| L[rt_thread_mdelay 20]
    F -->|false| L
    L --> C
```

---

### 2.3.2.4 舵机控制模块（servo.c）

#### 模块概述

舵机控制模块负责控制3个SG90舵机，根据目标层数打开/关闭对应的取件门。

#### 模块流程图

```mermaid
flowchart TD
    A[servo_thread_init] --> B[创建servo_mq消息队列]
    B --> C[打开PWM4设备]
    C --> D[打开PWM2设备]
    D --> E[创建舵机控制线程]
    E --> F[线程循环]
    F --> G[初始化舵机关门]
    G --> H[rt_mq_recv servo_mq]
    H --> I[获取layer]
    I --> J[rt_event_recv EVENT_POSITION_OK]
    J --> K{layer == 0?}
    K -->|true| L[选择舵机1 PWM4-CH4]
    K -->|false| M{layer == 1?}
    M -->|true| N[选择舵机2 PWM4-CH3]
    M -->|false| O[选择舵机3 PWM2-CH2]
    L --> P[rt_pwm_set 开门]
    N --> P
    O --> P
    P --> Q[rt_event_recv EVENT_PICK_FINNISH]
    Q --> R[rt_pwm_set 关门]
    R --> S[rt_thread_delay 20]
    S --> H
```

#### 关键函数说明

**servo_thread_init()** - 舵机线程初始化

| 项目 | 说明 |
|------|------|
| **函数签名** | `void servo_thread_init(void)` |
| **输入参数** | 无 |
| **输出/返回值** | 无 |
| **核心逻辑** | 创建消息队列、打开PWM设备、创建控制线程 |

```mermaid
flowchart TD
    A[servo_thread_init] --> B[rt_mq_create servo_mq]
    B --> C{servo_mq != NULL?}
    C -->|false| D[rt_kprintf error return]
    C -->|true| E[rt_device_find pwm4]
    E --> F{servo_pwm_dev1 != NULL?}
    F -->|false| G[rt_kprintf error return]
    F -->|true| H[rt_device_find pwm2]
    H --> I{servo_pwm_dev3 != NULL?}
    I -->|false| J[rt_kprintf error return]
    I -->|true| K[rt_thread_create servo]
    K --> L[rt_thread_startup]
```

**servo_trhread_entry()** - 舵机控制线程

| 项目 | 说明 |
|------|------|
| **函数签名** | `void servo_trhread_entry(void *parameter)` |
| **输入参数** | `parameter` - 线程参数（未使用） |
| **输出/返回值** | 无 |
| **核心逻辑** | 接收层数信息，等待电机定位完成，控制对应舵机开门/关门 |

```mermaid
flowchart TD
    A[servo_thread_entry] --> B[初始化舵机关门]
    B --> C[rt_mq_recv servo_mq]
    C --> D[获取layer]
    D --> E[rt_event_recv EVENT_POSITION_OK]
    E --> F{layer == 0?}
    F -->|true| G[pwm_dev=pwm4, channel=4]
    F -->|false| H{layer == 1?}
    H -->|true| I[pwm_dev=pwm4, channel=3]
    H -->|false| J[pwm_dev=pwm2, channel=2]
    G --> K[rt_pwm_set 开门脉冲]
    I --> K
    J --> K
    K --> L[rt_event_recv EVENT_PICK_FINNISH]
    L --> M[rt_pwm_set 关门脉冲]
    M --> N[rt_thread_delay 20]
    N --> C
```

---

### 2.3.2.5 数据存储模块（database.c）

#### 模块概述

数据存储模块基于FlashDB实现键值存储，管理器件信息的添加、查找和更新。

#### 模块流程图

```mermaid
flowchart TD
    A[database_init] --> B[fdb_kvdb_init]
    B --> C{初始化成功?}
    C -->|false| D[返回-1]
    C -->|true| E[返回0]
    
    F[components_add_kv] --> G[分配内存 info]
    G --> H{fdb_kv_iterate 遍历}
    H --> I{kv值长度 == sizeof component_info?}
    I -->|false| J[continue]
    I -->|true| K[fdb_blob_read]
    K --> L{jsoninfo == info->para_info?}
    L -->|true| M[弹出重复提示]
    M --> N[返回1]
    L -->|false| O[continue]
    H --> P[遍历结束]
    P --> Q[查找空位 key=00-35]
    Q --> R{找到空位?}
    R -->|false| S[弹出满箱提示]
    S --> T[返回-1]
    R -->|true| U[fdb_kv_set_blob]
    U --> V[发送pos到motor_mq]
    V --> W[发送layer到servo_mq]
    W --> X[弹出等待提示]
    X --> Y[返回0]
```

#### 关键函数说明

**database_init()** - 数据库初始化

| 项目 | 说明 |
|------|------|
| **函数签名** | `int database_init(void)` |
| **输入参数** | 无 |
| **输出/返回值** | `int` - 0表示成功，-1表示失败 |
| **核心逻辑** | 初始化FlashDB键值数据库 |

```mermaid
flowchart TD
    A[database_init] --> B[fdb_kvdb_control UNLOCK]
    B --> C[fdb_kvdb_init components_kv_db]
    C --> D{res == FDB_NO_ERR?}
    D -->|false| E[FDB_INFO error]
    E --> F[return -1]
    D -->|true| G[return 0]
```

**components_add_kv()** - 添加器件

| 项目 | 说明 |
|------|------|
| **函数签名** | `int components_add_kv(char *jsoninfo, uint32_t quantity)` |
| **输入参数** | `jsoninfo` - JSON格式的器件信息，`quantity` - 器件数量 |
| **输出/返回值** | `int` - 0成功，1重复，-1失败 |
| **核心逻辑** | 遍历数据库检查重复，找到空位后存储器件信息 |

```mermaid
flowchart TD
    A[components_add_kv jsoninfo quantity] --> B[rt_malloc info]
    B --> C{fdb_kv_iterator_init}
    C --> D[fdb_kv_iterate]
    D --> E{cur_kv->value_len == sizeof component_info?}
    E -->|false| D
    E -->|true| F[fdb_blob_read]
    F --> G{strcmp jsoninfo info->para_info == 0?}
    G -->|true| H[lv_msgbox 重复提示]
    H --> I[rt_free info]
    I --> J[return 1]
    G -->|false| D
    D --> K[遍历结束]
    K --> L[for i=0 to 35]
    L --> M[strncpy key %02d]
    M --> N{fdb_kv_get_obj key == NULL?}
    N -->|true| O[info->quantity = quantity]
    O --> P[strncpy info->para_info]
    P --> Q[fdb_kv_set_blob]
    Q --> R[pos = atoi key % 12]
    R --> S[layer = atoi key / 12]
    S --> T[rt_mq_send motor_mq pos]
    T --> U[rt_mq_send servo_mq layer]
    U --> V[lv_msgbox 等待提示]
    V --> W[rt_free info]
    W --> X[return 0]
    N -->|false| L
    L --> Y[i >= 36?]
    Y -->|true| Z[lv_msgbox 满箱提示]
    Z --> AA[rt_free info]
    AA --> AB[return -1]
```

**components_find_kv()** - 查找器件

| 项目 | 说明 |
|------|------|
| **函数签名** | `int components_find_kv(filter_info_t filter)` |
| **输入参数** | `filter` - 筛选条件结构指针 |
| **输出/返回值** | `int` - 0表示成功 |
| **核心逻辑** | 根据筛选条件遍历数据库，匹配的器件添加到界面列表 |

```mermaid
flowchart TD
    A[components_find_kv filter] --> B[rt_malloc info]
    B --> C[fdb_kv_iterator_init]
    C --> D[fdb_kv_iterate]
    D --> E{cur_kv->value_len == sizeof component_info?}
    E -->|false| D
    E -->|true| F[fdb_blob_read]
    F --> G[cJSON_Parse info->para_info]
    G --> H{root != NULL?}
    H -->|false| D
    H -->|true| I[cJSON_GetObjectItem type/val/pkg/rated/tol/other]
    I --> J{Is_match type AND val AND pkg AND rated AND tol AND other?}
    J -->|true| K[add_component_to_list]
    K --> L[cJSON_Delete root]
    J -->|false| L
    L --> D
    D --> M[遍历结束]
    M --> N[rt_free info]
    N --> O[return 0]
```

---

### 2.3.2.6 蜂鸣器模块（buzzer.c）

#### 模块概述

蜂鸣器模块负责触摸屏幕时发出声音反馈，提升用户交互体验。

#### 模块流程图

```mermaid
flowchart TD
    A[buzzer_thread_init] --> B[rt_pin_mode PG11 OUTPUT]
    B --> C[rt_pin_write PG11 LOW]
    C --> D[rt_thread_create buzzer]
    D --> E[rt_thread_startup]
    E --> F[buzzer_thread_entry]
    F --> G[rt_event_recv EVENT_TOUCH_SCREEN]
    G --> H[rt_pin_write PG11 HIGH]
    H --> I[rt_thread_mdelay 80]
    I --> J[rt_pin_write PG11 LOW]
    J --> G
```

#### 关键函数说明

**buzzer_thread_init()** - 蜂鸣器线程初始化

| 项目 | 说明 |
|------|------|
| **函数签名** | `void buzzer_thread_init(void)` |
| **输入参数** | 无 |
| **输出/返回值** | 无 |
| **核心逻辑** | 设置蜂鸣器引脚为输出模式，创建蜂鸣器控制线程 |

**buzzer_thread_entry()** - 蜂鸣器控制线程

| 项目 | 说明 |
|------|------|
| **函数签名** | `void buzzer_thread_entry(void *parameter)` |
| **输入参数** | `parameter` - 线程参数（未使用） |
| **输出/返回值** | 无 |
| **核心逻辑** | 等待触摸事件，触发蜂鸣器发声80ms |

---

### 2.3.2.7 GUI事件处理模块（ui_events.c）

#### 模块概述

GUI事件处理模块负责处理LVGL界面的用户交互事件，包括添加器件、查找器件、取件操作等。

#### 模块流程图

```mermaid
flowchart TD
    A[用户触摸屏幕] --> B[触发LVGL事件]
    B --> C{事件类型}
    C -->|取件按钮点击| D[event_get_component_cb]
    C -->|添加按钮点击| E[addBTN_call_function]
    C -->|查找按钮点击| F[findBTN_call_function]
    C -->|文本框点击| G[ui_event_Textarea]
    C -->|完成按钮点击| H[event_finish_cb]
    
    D --> I[获取key/info/num]
    I --> J{num为空?}
    J -->|true| K[弹出警告]
    J -->|false| L{取件且num > rest_num?}
    L -->|true| M[弹出警告]
    L -->|false| N{添加模式?}
    N -->|true| O[更新数量+]
    N -->|false| P{num == rest_num?}
    P -->|true| Q[删除KV记录]
    P -->|false| R[更新数量-]
    O --> S[发送pos/layer到MQ]
    Q --> S
    R --> S
    
    E --> T[获取Tab类型]
    T --> U[读取参数]
    U --> V{参数完整?}
    V -->|false| W[弹出提示]
    V -->|true| X[cJSON组装]
    X --> Y[components_add_kv]
    
    F --> Z[获取Tab类型]
    Z --> AA[组装filter]
    AA --> AB[components_find_kv]
    
    H --> AC[rt_event_send EVENT_PICK_FINNISH]
```

#### 关键函数说明

**event_get_component_cb()** - 取件操作处理

| 项目 | 说明 |
|------|------|
| **函数签名** | `void event_get_component_cb(lv_event_t *e)` |
| **输入参数** | `e` - LVGL事件结构体指针 |
| **输出/返回值** | 无 |
| **核心逻辑** | 获取器件信息和数量，更新数据库，发送位置指令 |

```mermaid
flowchart TD
    A[event_get_component_cb] --> B[screen_touch_function]
    B --> C{event_code == CLICKED?}
    C -->|false| D[return]
    C -->|true| E[获取key_label/info_label/restnum_label]
    E --> F[获取num_textarea]
    F --> G{numtext[0] == '\0'?}
    G -->|true| H[lv_msgbox 警告]
    H --> I[return]
    G -->|false| J[num = atoi numtext]
    J --> K{!add_switch AND num > rest_num?}
    K -->|true| L[lv_msgbox 警告]
    L --> M[return]
    K -->|false| N{add_switch == CHECKED?}
    N -->|true| O[refreshinfo.quantity += num]
    N -->|false| P{num == rest_num?}
    P -->|true| Q[fdb_kv_del]
    Q --> R[lv_obj_del_async]
    P -->|false| S[refreshinfo.quantity -= num]
    O --> T[fdb_kv_set_blob]
    S --> T
    R --> U[pos = atoi key % 12]
    T --> U
    U --> V[layer = atoi key / 12]
    V --> W[rt_mq_send motor_mq pos]
    W --> X[rt_mq_send servo_mq layer]
    X --> Y[lv_msgbox 等待提示]
```

**addBTN_call_function()** - 添加器件处理

| 项目 | 说明 |
|------|------|
| **函数签名** | `void addBTN_call_function(lv_event_t *e)` |
| **输入参数** | `e` - LVGL事件结构体指针 |
| **输出/返回值** | 无 |
| **核心逻辑** | 获取Tab类型和参数，组装JSON数据，调用数据库添加函数 |

```mermaid
flowchart TD
    A[addBTN_call_function] --> B[current_tab = lv_tabview_get_tab_act]
    B --> C{cJSON_CreateObject}
    C --> D{current_tab == RESISTOR?}
    D -->|true| E[读取电阻参数]
    D -->|false| F{current_tab == CAPACITOR?}
    F -->|true| G[读取电容参数]
    F -->|false| H[读取电感参数]
    E --> I[cJSON_AddString type=Resistor]
    G --> J[cJSON_AddString type=Capacitor]
    H --> K[cJSON_AddString type=Inductor]
    I --> L{参数完整?}
    J --> L
    K --> L
    L -->|false| M[lv_msgbox 提示]
    M --> N[cJSON_Delete]
    N --> O[return]
    L -->|true| P[cJSON_AddString val/pkg/rated/accuracy/other]
    P --> Q[cJSON_PrintUnformatted]
    Q --> R[components_add_kv]
    R --> S[cJSON_free]
    S --> T[cJSON_Delete]
```

**findBTN_call_function()** - 查找器件处理

| 项目 | 说明 |
|------|------|
| **函数签名** | `void findBTN_call_function(lv_event_t *e)` |
| **输入参数** | `e` - LVGL事件结构体指针 |
| **输出/返回值** | 无 |
| **核心逻辑** | 获取Tab类型和筛选条件，调用数据库查找函数 |

```mermaid
flowchart TD
    A[findBTN_call_function] --> B[current_tab = lv_tabview_get_tab_act]
    B --> C[lv_obj_clean ui_refreshPanel]
    C --> D[rt_malloc filter]
    D --> E[memset filter 0]
    E --> F{current_tab == RESISTOR?}
    F -->|true| G[strcpy type=Resistor]
    F -->|false| H{current_tab == CAPACITOR?}
    H -->|true| I[strcpy type=Capacitor]
    H -->|false| J[strcpy type=Inductor]
    G --> K[filter->val/package/ratedval/tolerance/otherinfo = 控件文本]
    I --> K
    J --> K
    K --> L[components_find_kv]
    L --> M[rt_free filter]
```

**event_finish_cb()** - 完成按钮处理

| 项目 | 说明 |
|------|------|
| **函数签名** | `void event_finish_cb(lv_event_t *e)` |
| **输入参数** | `e` - LVGL事件结构体指针 |
| **输出/返回值** | 无 |
| **核心逻辑** | 用户点击完成按钮时，发送EVENT_PICK_FINNISH事件 |

```mermaid
flowchart TD
    A[event_finish_cb] --> B{event_code == VALUE_CHANGED?}
    B -->|false| C[return]
    B -->|true| D{lv_msgbox_get_active_btn == 0?}
    D -->|false| E[return]
    D -->|true| F[rt_event_send EVENT_PICK_FINNISH]
    F --> G[lv_msgbox_close]
```

---

## 附录：函数I/O变量汇总表

### 电机控制模块

| 函数名 | 输入参数 | 输出/返回值 | 关键变量 |
|--------|---------|------------|---------|
| `encoder_thread_init` | 无 | 无 | motor_mq, encoder_dev, motor_pwm_dev |
| `encodermotor_entry` | parameter (void*) | 无 | pos, target, error, pwm, Kp, Ki, Kd |
| `motor_set_pwm` | pwm_val (int32_t) | 无 | pulse, IN1, IN2 |
| `motor_zero_out` | 无 | 无 | count, pin_state |

### 舵机控制模块

| 函数名 | 输入参数 | 输出/返回值 | 关键变量 |
|--------|---------|------------|---------|
| `servo_thread_init` | 无 | 无 | servo_mq, servo_pwm_dev1, servo_pwm_dev3 |
| `servo_thread_entry` | parameter (void*) | 无 | layer, pwm_dev, channel, open_pwm, close_pwm |

### 数据存储模块

| 函数名 | 输入参数 | 输出/返回值 | 关键变量 |
|--------|---------|------------|---------|
| `database_init` | 无 | int (0/-1) | components_kv_db |
| `components_add_kv` | jsoninfo (char*), quantity (uint32_t) | int (0/1/-1) | info, key, pos, layer |
| `components_find_kv` | filter (filter_info_t) | int (0/-1) | info, cur_kv, cJSON对象 |

### GUI事件处理模块

| 函数名 | 输入参数 | 输出/返回值 | 关键变量 |
|--------|---------|------------|---------|
| `event_get_component_cb` | e (lv_event_t*) | 无 | key, info, num, pos, layer |
| `addBTN_call_function` | e (lv_event_t*) | 无 | current_tab, val, package, ratedval, cJSON对象 |
| `findBTN_call_function` | e (lv_event_t*) | 无 | current_tab, filter |
| `event_finish_cb` | e (lv_event_t*) | 无 | EVENT_PICK_FINNISH |

### 蜂鸣器模块

| 函数名 | 输入参数 | 输出/返回值 | 关键变量 |
|--------|---------|------------|---------|
| `buzzer_thread_init` | 无 | 无 | BUZZER_PIN |
| `buzzer_thread_entry` | parameter (void*) | 无 | EVENT_TOUCH_SCREEN |

---

**文档版本**：V1.0  
**创建日期**：2026-06-29  
**项目类型**：嵌入式智能存储系统  
**运行平台**：ART-PI (STM32H750) + RT-Thread