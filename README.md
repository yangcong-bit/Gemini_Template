# 🚀 蓝桥杯嵌入式 STM32G4 极速开发模板 (Gemini 终极版)

欢迎来到 **Gemini_Template**！本模板专为蓝桥杯嵌入式（STM32G431）及各类单片机裸机开发量身定制。

无论你是刚学会点亮 LED 想要在考场上快速套用模板的**新手**，还是备战国赛乃至更高级别电赛、追求代码极致优雅和底层掌控力的**老鸟**，这套以**“中心化数据字典 + 纯非阻塞时间片轮询”**为核心的 MVC 架构，都能让你在 5 小时的紧张考场上实现**“降维打击”**。

告别按键冲突、告别 LCD 刷屏闪烁、告别 I2C 延时卡死！考场上，你**只需要修改 2 个文件**，将全部精力集中在业务逻辑的突破上。

---

## 🗂️ 源码架构解析 (清爽至极的工程树)

本模板将庞杂的代码严格划分为底层驱动、中间件与核心业务层，摒弃所有冗余：

```plaintext
GEMINI_TEMPLATE/
├── APP/                    # 🌟 标准全功能应用层 (非阻塞驱动与核心调度器)
│   ├── Inc/                # 头文件 (含 global_system.h, exam_logic.h 等)
│   └── Src/                # 源文件 (含 exam_logic.c 及各类非阻塞外设驱动)
├── APP_Lite/               # ⚡ 轻量级应用层 (为资源受限或精简业务场景保留的分支)
├── Core/                   # CubeMX 自动生成的系统内核与外设初始化代码
├── Drivers/                # STM32G4xx HAL 库与 CMSIS 底层驱动
├── MDK-ARM/                # Keil uVision5 工程目录
├── .gitea/workflows/       # 内置 keil-build.yaml，支持 CI/CD 自动化云端编译
├── 蓝桥杯嵌入式十五届.md     # 📝 核心数据建模表 (做题第一步，灵魂所在)
├── clean.bat               # Windows 一键清理编译临时文件脚本
└── clean_template.py       # Python 深度清理脚本，保持工程纯净
```

---

## 🧠 核心架构哲学 (MVC 模式与非阻塞驱动)

本模板在裸机上强制推行现代软件工程的 **MVC (Model-View-Controller)** 思想，彻底消除 `HAL_Delay()` 对系统调度的致命阻塞：

* **Model (模型)**：`global_system.h` 里的 `sys` 数据字典。它是全工程**唯一的真理之源**，严禁满天飞的 `extern` 变量。
* **View (视图)**：`exam_logic.c` 中的 `Logic_UI_Proc` (屏幕) 和 `Logic_LED_Proc` (指示灯)。它们是“哑巴”渲染器，只负责**读取** `sys` 字典并映射到硬件。
* **Controller (控制器)**：`exam_logic.c` 中的 `Logic_Ctrl_Proc` (按键交互) 和 `Logic_Data_Proc` (传感器采集运算)。它们负责处理外部输入并**修改** `sys` 字典。

### 🛠️ 高阶非设驱动库 (APP 目录一览)
* **`scheduler.c`**：时间片调度器，接管系统心跳，确保按键、屏幕、数据各司其职。
* **`key_app.c` + `ringbuffer.c`**：零延时按键驱动，自带消抖、长短按与双击，键值压入环形队列，系统高负载也绝不丢按键。
* **`eeprom_app.c`**：基于异步状态机的 EEPROM 读写，将写时序切片，彻底解决 I2C 延时卡死。
* **`freq_app.c` & `adc_app.c`**：基于定时器的高精度频率测量与多通道 ADC 软件滤波采集。

---

## 🏎️ 极速通关教程：以《第十五届省赛真题》为例

下面演示如何利用这套架构，在 1 小时内“傻瓜式”秒杀赛题。你不需要关心底层通信怎么写，只需要完成**三步填空题**。

### 第一步：阅读试卷，提取《数据字典表》
打开工程目录下的 `蓝桥杯嵌入式十五届.md`，边读试卷边提取需要运算和展示的变量。**这是做题的灵魂！**

| 数据含义 (UI展示或逻辑运算) | 对应 sys 字典变量名 | 数据类型 | 默认值 | 备注说明 (题目要求) |
| :--- | :--- | :--- | :--- | :--- |
| 通道 A 原始脉冲频率 | `freq_ch1` | `uint32_t` | 0 | 由底层自动更新 |
| 通道 A 校准后频率 | `f_a_cal` | `int32_t` | 0 | 公式: `freq_ch1 + PX` |
| 频率超限参数 (High) | `para_ph` | `uint32_t` | 5000 | 步进100, 范围: 1000~ 10000 |
| A通道频率超限次数 | `count_nha` | `uint32_t` | 0 | `f_a_cal > PH` (边缘触发加1) |
| 频率/周期显示模式 | `data_mode` | `uint8_t` | 0 | 0:频率(Hz), 1:周期(uS) |

将这些变量抄进 `global_system.h` 的 `SystemData_t` 结构体中，并在 `scheduler.c` 中赋予初始值。

### 第二步：在 `exam_logic.c` 中完成流水线填空
考场上，你只需要打开这**一个文件**，往下顺序写：

**1. 写按键控制 (Logic_Ctrl_Proc)**
看表办事：例如 B3 要求切换频率/周期模式。
```c
if (key_val == 3) { // B3 短按
    if (sys.current_page == PAGE_DATA) {
        sys.data_mode = !sys.data_mode; // 翻转 0/1
    }
}
```

**2. 写数据结算 (Logic_Data_Proc)**
看表办事：校准频率等于原始频率加参数；计算超限次数（边缘触发）。
```c
// 算校准值
sys.f_a_cal = (int32_t)sys.freq_ch1 + sys.para_px;

// 算超限次数 (边缘触发经典写法)
static bool is_a_over = false;
if (sys.f_a_cal > 0 && sys.f_a_cal > sys.para_ph) {
    if (!is_a_over) { sys.count_nha++; is_a_over = true; } // 刚超限瞬间加 1
} else {
    is_a_over = false; // 回落解锁
}
```

**3. 写屏幕与灯光 (Logic_UI_Proc & Logic_LED_Proc)**
看表办事：根据 `data_mode` 画屏幕，根据频率亮灯。
```c
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

## 🟡 进阶篇：了解底层，知其所以然

如果你想成为高手，请点开 `APP/` 目录下的其他文件，学习这些工业级设计：

1.  **VRAM 双缓冲防闪机制 (`lcd_app.c`)**：
    UI 任务先在内存数组 `lcd_vram[10][21]` 中“画好”整屏，然后底层利用 `strcmp` 与上一帧 `lcd_vram_bak` 进行比对。**只有内容发生真实变化的行，才会调用慢速的 SPI 发送给屏幕。** 彻底告别 `LCD_Clear()` 带来的瞎眼闪烁。
2.  **异步切片状态机 (`eeprom_app.c`)**：
    写 EEPROM 的传统 `HAL_Delay(5)` 会卡死整个单片机。本模板使用 `sys.log_queue` 队列接收保存请求，底层状态机每 5ms 苏醒一次，**切碎大块结构体，一次只写 1 个字节**。主循环始终保持满速运行。

---

## ⚙️ 编译与工程维护

* **本地开发**：使用 Keil MDK 打开 `MDK-ARM/Gemini_Template.uvprojx` 即可直接编译下载。
* **一键瘦身**：在提交代码或归档前，双击运行 `clean.bat`（或执行 `python clean_template.py`），自动清理百兆编译垃圾文件，保持工程极度轻量。
* **持续集成 (CI)**：内置 `.gitea/workflows/keil-build.yaml`，推送到支持 Actions 的 Git 远端后，可自动触发云端 Keil 编译校验。

祝你在蓝桥杯赛场上，代码一遍过，早日拿国一！🏆