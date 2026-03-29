# 🚀 蓝桥杯嵌入式 STM32G4 极速开发模板 (Gemini 终极版)

欢迎来到 **Gemini\_Template**！本模板专为蓝桥杯嵌入式（STM32G431）及各类单片机裸机开发设计。

无论你是刚学会点亮 LED 的**新手**，还是备战国赛、追求代码极致优雅的**老鸟**，这套以**“中心化数据字典 + 纯非阻塞时间片轮询”**为核心的 MVC 架构，都能让你在 5 小时的考场上实现**“降维打击”**。

告别按键冲突、告别 LCD 刷屏闪烁、告别 I2C 延时卡死！考场上，你**只需要修改 2 个文件**！

---

## 📁 工程目录结构指南

根据标准工程设计，本模板的文件树极度清爽：

**Plaintext**

```
GEMINI_TEMPLATE/
├── APP/                    # 🌟 核心魔法区 (所有模板驱动与业务逻辑都在这里)
│   ├── Inc/                # 头文件目录 (含 global_system.h, exam_logic.h 等)
│   └── Src/                # 源文件目录 (含 exam_logic.c 及各类非阻塞驱动)
├── Core/                   # CubeMX 自动生成的核心文件 (main.c 等)
├── Drivers/                # STM32 HAL 库与 CMSIS 驱动
├── MDK-ARM/                # Keil MDK 工程文件输出目录
├── .mxproject              # CubeMX 工程缓存
├── Gemini_Template.ioc     # CubeMX 配置文件 (时钟树80M、外设引脚已配好)
├── 蓝桥杯嵌入式十五届.md     # 📝 核心数据建模表 (做题第一步，极其重要！)
└── clean.bat               # 一键清理编译垃圾脚本
```

---

## 🧠 核心架构哲学 (The Secret Sauce)

本模板强制推行 **MVC (Model-View-Controller)** 思想：

* **Model (模型)**：`global_system.h` 里的 `sys` 字典。它是全工程**唯一的真理之源**，严禁使用 `extern` 飞线。
* **View (视图)**：`exam_logic.c` 中的 `Logic_UI_Proc` 和 `Logic_LED_Proc`。它们是哑巴，只负责**读取** `sys` 字典并渲染硬件。
* **Controller (控制器)**：`exam_logic.c` 中的 `Logic_Ctrl_Proc` (按键/串口) 和 `Logic_Data_Proc` (传感器)。它们负责处理输入并**修改** `sys` 字典。

---

## 🛠️ 极速通关教程：以《第十五届省赛真题》为例

下面，我们将以第十五届省赛真题（双通道频率测量与突变报警系统）为例，演示如何用本模板在 1 小时内“秒杀”赛题。

### 🟢 基础篇：新手如何“填空”做题

对于新手，你不需要关心底层 I2C 怎么写、双缓冲显存怎么防闪。你只需要做**三步填空题**。

#### 第一步：阅读试卷，提取《数据字典表》

打开工程目录下的 `蓝桥杯嵌入式十五届.md`，边读试卷边提取变量。这是做题的灵魂！

| **数据含义 (UI展示或逻辑运算)** | **对应 sys 字典变量名** | **数据类型** | **默认值/作用域** | **备注说明 (题目要求)**    |
| ------------------------------------- | ----------------------------- | ------------------ | ----------------------- | -------------------------------- |
| 通道 A 原始脉冲频率                   | `freq_ch1`                  | `uint32_t`       | 0                       | 由底层自动更新                   |
| 通道 A 校准后频率                     | `f_a_cal`                   | `int32_t`        | 0                       | 公式:`freq_ch1 + PX`(可能为负) |
| 频率突变参数 (Delta)                  | `para_pd`                   | `uint32_t`       | 1000                    | 步进100, 范围: 100\~ 1000        |
| 频率超限参数 (High)                   | `para_ph`                   | `uint32_t`       | 5000                    | 步进100, 范围: 1000\~ 10000      |
| 频率校准参数 (X)                      | `para_px`                   | `int32_t`        | 0                       | 步进100, 范围: -1000\~ 1000      |
| A通道频率突变次数                     | `count_nda`                 | `uint32_t`       | 0                       | 3秒滑动窗口最大差值 > PD 加1     |
| A通道频率超限次数                     | `count_nha`                 | `uint32_t`       | 0                       | `f_a_cal > PH`(边缘触发加1)    |
| 频率/周期显示模式                     | `data_mode`                 | `uint8_t`        | 0                       | 0:频率(Hz), 1:周期(uS)           |

#### 第二步：将表格翻译进 `global_system.h`

把上面的表，一行不落地抄进 `SystemData_t` 结构体中，并在 `scheduler.c` 的 `sys = {...}` 中赋予初始值。

#### 第三步：在 `exam_logic.c` 中完成流水线填空

考场上，你只需要打开这一个文件，往下顺序写：

**1. 写按键 (Logic\_Ctrl\_Proc)**

看表办事：B3 要求切换频率/周期模式。

**C**

```
if (key_val == 3) { // B3 短按
    if (sys.current_page == PAGE_DATA) {
        sys.data_mode = !sys.data_mode; // 翻转 0/1
    }
}
```

**2. 写结算 (Logic\_Data\_Proc)**

看表办事：校准频率等于原始频率加上 PX 参数。

**C**

```
// 算校准值
sys.f_a_cal = (int32_t)sys.freq_ch1 + sys.para_px;

// 算超限次数 (边缘触发经典写法)
static bool is_a_over = false;
if (sys.f_a_cal > 0 && sys.f_a_cal > sys.para_ph) {
    if (!is_a_over) { sys.count_nha++; is_a_over = true; } // 刚超限的瞬间加 1
} else {
    is_a_over = false; // 回落解锁
}
```

**3. 写屏幕与灯光 (Logic\_UI\_Proc & Logic\_LED\_Proc)**

看表办事：如果模式为 0，画频率；如果 A通道频率 > PH，亮 LD2。

**C**

```
// 在 Logic_UI_Proc 中：
if (sys.data_mode == 0) {
    sprintf(temp, "     A=%dHz", (int)sys.f_a_cal);
} else {
    sprintf(temp, "     A=%duS", 1000000 / (int)sys.f_a_cal);
}
sprintf(lcd_vram[3], "%-20s", temp); 

// 在 Logic_LED_Proc 中：
if (sys.f_a_cal > sys.para_ph) sys.led_ctrl[1] = 1; // 亮 LD2
```

---

### 🟡 进阶篇：了解底层，知其所以然

如果你想成为高手，请点开 `APP/` 目录下的其他文件，学习这些工业级设计：

1. **VRAM 双缓冲防闪机制 (`lcd_app.c`)**：
   UI 任务先在内存数组 `lcd_vram[10][21]` 中“画好”整屏，然后底层利用 `strcmp` 与上一帧 `lcd_vram_bak` 对比。**只有内容发生变化的行，才会调用慢速的 SPI 发送给屏幕。** 彻底告别 `LCD_Clear()` 带来的瞎眼闪烁。
2. **异步切片状态机 (`eeprom_app.c`)**：
   传统写 EEPROM 的 `HAL_Delay(5)` 会卡死整个单片机。本模板使用 `sys.log_queue` 队列接收你的保存请求，底层状态机每 5ms 苏醒一次，**切碎结构体，一次只写 1 个字节**。主循环依然满速运行。
3. **零延时按键队列 (`key_app.c`)**：
   自带 10ms 消抖、长短按、以及**双击检测**。产生按键后压入环形队列 `sys.key_queue`，UI 模块想什么时候读就什么时候读，永远不丢按键。

---

### 🔴 高级篇：致备战国赛与求职的你

这套协作式时间片轮询架构（Time-slice Polling）是小型嵌入式系统的极致，但它依然不是硬实时（Hard Real-Time）的。

当你吃透了这套模板后，你可以尝试：

1. **引入 RTOS**：将 `scheduler.c` 替换为 FreeRTOS。将 `sys` 字典替换为 RTOS 的**消息队列 (Message Queue)** 与 **互斥锁 (Mutex)**，防止并发读写冲突。
2. **扩展板状态机化**：国赛若遇 DS18B20 或矩阵键盘，严禁直接复制官方带 delay 的驱动。请参考 `adc_proc` 的写法，用状态机将其改造为非阻塞驱动，并挂载到调度器中。

祝你在蓝桥杯赛场上，代码一遍过，早日拿国一！🏆
