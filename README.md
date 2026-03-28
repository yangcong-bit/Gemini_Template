🏆 蓝桥杯嵌入式 STM32G4 极速开发模板 (The Ultimate Template)
欢迎来到蓝桥杯嵌入式（STM32G431）极速开发模板！本模板基于时间片轮询架构设计，彻底告别底层通信阻塞、LCD 刷屏闪烁、按键冲突和逻辑死锁。
“在考场的 5 个小时里，你只需要打开 exam_logic.c，把复杂的大题当成填空题来做。”
✨ 核心特性 (Features)
 * 中心化数据字典 (global_system.h)：全工程 零 extern。所有底层采集的数据（ADC、RTC、测频）自动上报至 sys 字典，UI 和控制逻辑只从 sys 读取。彻底解耦！
 * 绝对非阻塞 (Non-blocking)：
   * EEPROM 异步切片：后台 5ms 切片写入单字节，完美解决 AT24C02 连续写入卡死主循环的痛点。
   * UART IDLE + DMA：不定长数据全自动后台接收，主循环只负责解析。
   * LCD VRAM 双缓冲：自带显存比对算法，局部防闪刷新，告别 LCD_Clear() 带来的瞎眼闪烁。
 * 高内聚考场专用文件 (exam_logic.c)：底层驱动全黑盒化。考试时只需关注 exam_logic.c 中的 4 个函数（数据联动、LED 控制、串口解析、UI 渲染）。
 * 工业级按键状态机：自带 10ms 消抖、长按防连发、完美双击检测（配置数组一键开关双击功能）。
⚙️ STM32CubeMX HAL 库配置指南
为了让底层驱动正常工作，请在 CubeMX 中严格按照以下配置您的底层硬件（时钟树配置为主频 80MHz）：
1. 系统与调度基准
 * SYS: Debug 选 Serial Wire。Timebase Source 选择 SysTick（默认 1ms，调度器基准）。
2. ADC (模拟采集)
 * ADC1: 开启 IN11 (PB12，对应 R38)。
 * ADC2: 开启 IN15 (PB15，对应 R37)。
 * 配置: 开启 DMA Continuous Requests。在 DMA Settings 中添加 ADC1 和 ADC2 的 DMA 通道，Mode 选 Circular，Data Width 选 Half Word。
3. TIM (定时器配置)
 * PWM 输出 (按需配置)：模板默认配置 TIM1_CH1 (PA8)。若遇真题要求 PA7 输出 PWM，请在 CubeMX 开启 TIM3_CH2 (PA7) 的 PWM Generation，并在代码中修改句柄。
 * 输入捕获 (测频/占空比)：开启 TIM2 / TIM3 的 Input Capture。
4. USART (串口通信)
 * USART1: 波特率按需设置。开启全局中断 (NVIC)。在 DMA Settings 中添加 RX 通道，Mode 选 Normal，Data Width 选 Byte。
5. GPIO (引脚分配)
 * LED (PC8-PC15, PD2): 配置为推挽输出 (Output Push Pull)。
 * KEY (PB0-PB2, PA0): 配置为输入，上拉 (Pull-up)。
 * I2C 模拟 (PB6, PB7): 配置为开漏输出 (Output Open Drain)，必须上拉，速度 Very High。
🚀 实战演练：第十六届省赛真题降维打击
我们以**第十六届省赛真题（脉冲与 PWM 输出系统）**为例，演示如何只修改 2 个文件完成复杂逻辑。
题目核心要求简述：
 * 输入/输出：采集 R37/R38 调节 PA7 的 PWM 占空比和频率；PA15 采集外部脉冲频率。
 * 阶梯调节算法：占空比步长为 DS，范围 10%~DR；频率步长为 FS，范围 1000Hz~FR。
 * 锁定与异常判定：支持按键锁定；捕获频率与输出频率误差 > 1000Hz 时判定为异常。
 * UI与按键：三个界面（监控、统计、参数）；B1切换界面，B2切换参数/锁定/重置时间，B3/B4加减参数。
步骤 1：去 global_system.h 登记新增变量 (The Model)
根据题目要求的参数界面和统计界面，我们在 sys 字典里追加考题特有变量：
typedef struct {
    // ... [保留原有模板的基础变量] ...
    
    // 【第十六届真题新增参数】
    uint8_t  ds_step;       // 占空比步长 (1%)
    uint8_t  dr_max;        // 占空比范围上限 (80%)
    uint32_t fs_step;       // 频率步长 (100Hz)
    uint32_t fr_max;        // 频率范围上限 (2000Hz)
    
    // 【系统状态标志】
    bool     is_locked;     // 是否处于锁定状态
    bool     is_anomaly;    // 是否触发异常 (>1000Hz偏差)
    
    // 【统计锁定数据】
    uint32_t rec_cf;        // 最近一次异常时的输出频率
    uint8_t  rec_cd;        // 最近一次异常时的占空比
    uint32_t rec_df;        // 最近一次异常时的捕获频率
    uint32_t rec_xf;        // 最近一次异常时的频率差值
    
    uint32_t run_time_sec;  // 系统运行总秒数 (xxHyyMzzS)
} SystemData_t;

步骤 2：在 exam_logic.c 编写业务逻辑 (The Controller)
考场上，你完全不需要碰底层，直接在 Logic_Data_Proc 中实现核心阶梯调节算法：
void Logic_Data_Proc(void) {
    adc_proc(); // 采集 R37(对应sys.r37_voltage) 和 R38
    
    // 1. 系统运行时长统计 (通过 1Hz 软定时触发)
    static uint32_t last_sec_tick = 0;
    if (HAL_GetTick() - last_sec_tick >= 1000) {
        sys.run_time_sec++;
        last_sec_tick = HAL_GetTick();
    }

    // 2. 阶梯调节算法 (未锁定且不在参数界面时更新)
    if (!sys.is_locked && sys.current_page != PAGE_PARA) {
        // A. 占空比调节 (R37 0~3.3V)
        uint8_t max_n_d = (sys.dr_max - 10) / sys.ds_step; // 计算最大阶梯数
        uint8_t cur_n_d = (uint8_t)((sys.r37_voltage / 3.3f) * max_n_d);
        sys.pwm_duty = (10 + cur_n_d * sys.ds_step) / 100.0f;

        // B. 频率调节 (R38 0~3.3V)
        uint32_t max_n_f = (sys.fr_max - 1000) / sys.fs_step;
        uint32_t cur_n_f = (uint32_t)((sys.r38_voltage / 3.3f) * max_n_f);
        sys.pwm_freq = 1000 + cur_n_f * sys.fs_step;
    }

    // 3. 异常判定逻辑
    uint32_t freq_diff = (sys.freq_ch1 > sys.pwm_freq) ? (sys.freq_ch1 - sys.pwm_freq) : (sys.pwm_freq - sys.freq_ch1);
    if (freq_diff > 1000) {
        sys.is_anomaly = true;
        // 持续处于异常时不更新统计数据
        if (sys.rec_xf == 0) { 
            sys.rec_cf = sys.pwm_freq;
            sys.rec_cd = (uint8_t)(sys.pwm_duty * 100);
            sys.rec_df = sys.freq_ch1; // 假设 CH1 测 PA15
            sys.rec_xf = freq_diff;
        }
    } else {
        sys.is_anomaly = false;
        sys.rec_xf = 0; // 恢复正常后重置统计标记
    }
}

步骤 3：在 exam_logic.c 映射 LED (The View)
void Logic_LED_Proc(void) {
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0; // 擦除
    
    // LD1: 监控界面点亮
    if (sys.current_page == PAGE_DATA) sys.led_ctrl[0] = 1; 
    
    // LD2: 锁定状态点亮
    if (sys.is_locked) sys.led_ctrl[1] = 1; 
    
    // LD3: 异常状态点亮
    if (sys.is_anomaly) sys.led_ctrl[2] = 1; 
    
    LED_Disp();
}

步骤 4：在 exam_logic.c 搞定 UI 和按键 (The View & Controller)
void Logic_UI_Proc(void) {
    uint8_t key_val = 0;
    static uint8_t para_focus = 0; // 0:DS, 1:DR, 2:FS, 3:FR

    // 处理积压按键
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 1: // B1短按：切换 监控->统计->参数
                sys.current_page = (sys.current_page + 1) % 3; 
                para_focus = 0; // 退出参数界面自动切回 DS
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 清屏
                break;
            case 2: // B2短按：监控页切换锁定，参数页切换焦点
                if(sys.current_page == PAGE_DATA) sys.is_locked = !sys.is_locked;
                else if(sys.current_page == PAGE_PARA) para_focus = (para_focus + 1) % 4;
                break;
            case 12: // B2长按 (键值2+长按10)：清零时间
                if(sys.current_page == PAGE_DATA) sys.run_time_sec = 0;
                break;
            case 3: // B3加
            case 4: // B4减
                // ... 配合 para_focus 对 sys.ds_step 等进行加减与边界校验即可 ...
                break;
        }
    }
    
    // ... 下方的 sprintf 画屏逻辑严格按照题目图例填空即可，VRAM会自动防闪 ...
}

👨‍💻 总结
使用本模板，你不再需要纠结 HAL_Delay() 放在哪里会不会卡死按键，也不用担心刷屏导致串口漏接数据。
定义变量 -> 写计算公式 -> 绑定按键 -> 填空 sprintf。
祝各位在蓝桥杯赛场上，代码一次编译通过，顺利拿下国一！🥇
