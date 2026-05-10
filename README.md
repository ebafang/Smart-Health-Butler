# 🏥 智能健康管家 (Smart Health Butler)

> 基于 ARM+STM32 双处理器架构的嵌入式语音交互健康监测系统

![Platform](https://img.shields.io/badge/platform-ARM%20Linux%20%7C%20STM32F407-blue)
![Language](https://img.shields.io/badge/language-C-green)
![License](https://img.shields.io/badge/license-MIT-orange)

---

## 📖 项目简介

**智能健康管家** 是一个嵌入式健康监测系统，通过语音交互即可完成血压、心率、血氧、体温等生命体征的测量。系统采用 **ARM GEC210 + STM32F407** 双处理器架构，配合**科大讯飞语音识别引擎**实现自然语言交互。

**核心亮点：**
- 🎙️ **语音控制** — "量血压"、"测心率"、"健康报告"，说话就能测
- 🖥️ **图形界面** — 800×480 LCD 触摸屏，中文字体渲染
- 📡 **多种传感器** — MKS141（血压/心率/血氧）+ AS6221（体温）
- 🔗 **双机协作** — ARM 负责UI/语音，STM32 负责传感器采集
- 📝 **中文文档** — 完整开发文档，快速上手二次开发

---

## 🏗️ 系统架构

```
┌──────────────────────────────────────────────────────────────┐
│                     系统架构图                               │
│                                                              │
│  [用户语音] ──→ [麦克风] ──→ [ARM GEC210] ──→ [x86 Ubuntu]  │
│                      ▲             │              │          │
│                      │             │ 讯飞语音引擎  │          │
│                      │             │ (端口54321)   │          │
│                      │             │              │          │
│                    [喇叭] ←── WAV播放            │          │
│                                   │              ▼          │
│                             /dev/ttySAC2       返回XML       │
│                             串口(115200)       (cmd id)     │
│                                   │                          │
│                                   ▼                          │
│                             [STM32F407]                     │
│                              │        │                     │
│                         ┌────┘        └────┐                │
│                         ▼                  ▼                 │
│                    [MKS141]           [AS6221]              │
│                 血压/心率/血氧        体温传感器            │
│                   (串口2,38400)        (I2C)                │
└──────────────────────────────────────────────────────────────┘
```

| 设备 | 角色 | 操作系统 | 关键接口 |
|------|------|----------|----------|
| **ARM GEC210** | 主控：UI / 语音录制 / 命令分发 | Linux | LCD, 触摸屏, 麦克风, 扬声器, 串口 |
| **x86 Ubuntu** | 语音识别服务器 | Ubuntu | 网络端口 54321 (讯飞 MSC) |
| **STM32F407** | 传感器数据采集 | 裸机 | USART3 (与ARM通信), MKS141, AS6221 |

---

## 📁 目录结构

```
Smart-Health-Butler/
├── README.md                         # 项目说明
├── 开发文档_智能健康管家.md           # 详细开发文档
├── .gitignore
│
├── arm/                              # ARM GEC210 端代码
│   ├── voicectl.c                    # ★ 主程序：UI交互 + 命令分发
│   ├── common.c                      # ★ 通用库：网络/串口/XML/显示
│   ├── lcd.c                         # LCD 显示驱动
│   ├── ts.c                          # 触摸屏驱动
│   ├── font.c                        # 字体管理
│   ├── truetype.c                    # TrueType 字体渲染
│   ├── Makefile                      # ARM 交叉编译脚本
│   ├── inc/                          # 头文件目录
│   │   ├── common.h
│   │   ├── lcd.h / ts.h / font.h
│   │   ├── truetype.h / bitmap.h
│   │   └── alsa/ / libxml/           # 第三方库头文件
│   ├── lib/                          # ARM 动态库 (.so)
│   ├── ai_bmp/                       # 界面图片 (BMP)
│   │   ├── start.bmp
│   │   └── voice.bmp
│   ├── ai_wav/                       # 语音提示文件 (WAV)
│   │   ├── welcome.wav               # 欢迎语
│   │   ├── help.wav                  # 帮助菜单
│   │   ├── nihao.wav                 # 打招呼
│   │   ├── dengdai.wav               # 请等待
│   │   ├── please_wait.wav           # 综合检测中
│   │   ├── report_done.wav           # 报告完成
│   │   ├── result_normal/high/low    # 血压结果
│   │   ├── hr_ok/high/low            # 心率结果
│   │   ├── spo2_ok.wav               # 血氧结果
│   │   ├── temp_ok.wav               # 体温结果
│   │   ├── start_measure.wav         # 开始测量
│   │   ├── thank_you.wav             # 感谢使用
│   │   └── who.wav / xueya.wav       # 自我介绍/血压
│   └── simfang.ttf                   # 中文字体文件
│
├── x86/                              # x86 Ubuntu 语音识别引擎
│   ├── bin/
│   │   ├── cmd.bnf                   # ★ 讯飞 BNF 语法文件
│   │   ├── main                      # 语音识别引擎程序
│   │   └── msc/                      # 讯飞配置文件
│   ├── lib/
│   │   └── libmsc.so                 # 讯飞 SDK
│   └── examples/                     # 示例代码
│       └── asr_record_demo/
│
└── stm32/                            # STM32F407 端代码
    └── project(1)/
        ├── USER/
        │   └── main.c                # ★ 主程序：传感器采集 + 串口应答
        ├── HARDWARE/
        │   ├── pressure/mks141.*     # MKS141 血压模块驱动
        │   ├── temperature/as6221.*  # AS6221 温度传感器驱动
        │   ├── oled/                 # OLED 显示驱动
        │   ├── esp8266/              # ESP8266 WiFi 驱动
        │   ├── PulseSensor/          # 脉搏传感器备选
        │   └── arm/                  # 机械臂控制（扩展）
        ├── CMSIS/                    # ARM Cortex-M4 核心支持
        ├── DEVICE_LIB/               # STM32F4 标准外设库
        └── OBJ/pro.hex               # 编译产物（烧录文件）
```

---

## 🚀 快速开始

### 硬件需求

| 硬件 | 说明 |
|------|------|
| ARM GEC210 开发板 | 主控板，运行 Linux |
| 7寸 LCD 触摸屏 | 800×480 分辨率 |
| STM32F407 开发板 | 传感器采集板 |
| MKS141 模块 | 血压、心率、血氧三合一传感器 |
| AS6221 模块 | I2C 高精度温度传感器 |
| USB 麦克风 | 语音输入 |
| 扬声器 | 语音提示输出 |
| x86 Ubuntu PC | 运行讯飞语音识别引擎 |

### 硬件连接

```
ARM GEC210                    STM32F407
  串口2 ──────────────────── 串口3 (PB10/PB11)
  /dev/ttySAC2                 USART3
  波特率: 115200, 8N1

STM32F407                    传感器
  串口2 (PA2/PA3)  ──────── MKS141 (38400 baud)
  I2C (PB6/PB7)     ──────── AS6221
```

### 编译与部署

#### 1. 编译 ARM 端程序

```bash
# 确保已安装 ARM 交叉编译工具链
# arm-none-linux-gnueabi-gcc

cd arm
make clean
make
# 生成 voicectl 可执行文件
```

#### 2. 编译 STM32 端程序

```bash
# 使用 Keil MDK 打开工程
# 打开 stm32/project(1)/USER/ 下的 .uvprojx 文件
# 点击 Build (F7) → 生成 pro.hex
# 用 ST-Link / J-Link 烧录到 STM32F407
```

#### 3. 部署到 ARM 开发板

```bash
# 将编译产物拷贝到 ARM 开发板
scp arm/voicectl root@<arm-ip>:/root/
scp -r arm/ai_bmp arm/ai_wav arm/simfang.ttf arm/lib/ root@<arm-ip>:/root/
```

#### 4. 启动 x86 语音识别引擎

```bash
# 在 Ubuntu 上
cd x86/bin
# 确认 cmd.bnf 语法文件在同目录下
./main
# 引擎开始监听端口 54321
```

#### 5. 运行系统

```bash
# 在 ARM 开发板上
cd /root
./voicectl
```

---

## 🎤 语音命令表

| 语音指令 | 功能 | 耗时 |
|---------|------|------|
| "你好" | 打招呼 | 即显 |
| "你是谁" | 自我介绍 | 即显 |
| "帮助" | 显示帮助菜单 | 即显 |
| "量血压" | 测量血压（收缩压+舒张压） | ~80s |
| "测心率" | 测量心率 | ~5s |
| "测血氧" | 测量血氧 | ~5s |
| "体温" | 测量体温 | ~35s |
| "健康报告" | 综合检测（血压+心率+血氧+体温） | ~80s |
| "再见" | 退出系统 | — |

添加新语音指令：编辑 `x86/bin/cmd.bnf`，格式为 `语音文本!id(数字)`，然后在 `arm/voicectl.c` 中处理对应 `id`。

---

## 📡 ARM ↔ STM32 通信协议

| ARM 发送 | STM32 返回 | 说明 |
|----------|-----------|------|
| `mks-all\n` | `HR,SBP,DBP,SPO2\n` | 一次充气获全部（~80s） |
| `mks-hr\n` | `HR\n` | 仅心率（~5s） |
| `mks-spo2\n` | `SPO2\n` | 仅血氧（~5s） |
| `asm-temp\n` | `TEMP\n` | 体温，单位℃（~35s） |

---

## 📖 开发文档

完整开发文档见：[开发文档_智能健康管家.md](开发文档_智能健康管家.md)

文档涵盖：
- 系统架构详解
- BNF 语法文件说明
- ARM 端代码函数说明
- `font_show()` 文本框布局参数速查
- STM32 端传感器驱动说明
- 如何添加新传感器/新语音指令
- 常见问题排查

---

## ⚠️ 注意事项

1. **讯飞 SDK 授权**：本项目使用科大讯飞 MSC SDK，请自行申请 AppID 并配置 `x86/bin/msc/msc.cfg`
2. **IP 地址配置**：ARM 端通过 `common.c` 中的 `init_sock()` 连接 x86 Ubuntu，默认 IP 为 `192.168.110.131`，请根据实际网络环境修改
3. **ALSA 声音**：播放 WAV 前请确保 ALSA 音频设备正常（`aplay -l`）
4. **串口权限**：ARM 端需要 `/dev/ttySAC2` 的读写权限
5. **AS6221 温度补偿**：传感器读数偏低约 3℃，STM32 代码已做 +3℃ 补偿

---

## 🔧 常见问题

### Q: 系统不启动？
```bash
# ARM 端检查设备节点
ls /dev/fb0 /dev/input/event0 /dev/ttySAC2
dmesg | grep -i "fb0\|input\|ttySAC2"
```

### Q: 语音识别没反应？
- 检查 x86 Ubuntu 上 `main` 是否在运行（监听端口 54321）
- 检查 ARM 和 Ubuntu 能否 ping 通
- 检查 `cmd.bnf` 是否正确部署

### Q: 传感器返回全 0？
- MKS141 需要手指/手腕正确放入传感器
- 检查 STM32 串口接线（PB10/PB11 ↔ ARM 串口2）
- ARM 端 `printf` 会打印 STM32 原始回复

---

## 🛤️ 扩展路线图

- [ ] 历史记录存储与趋势图表
- [ ] WiFi 联网上传健康数据至云端
- [ ] 微信小程序远程查看
- [ ] 异常数据短信/电话报警
- [ ] 多语言语音支持（英语/粤语）
- [ ] 血压/心率异常自动分级预警
- [ ] 摄像头人脸识别+用户切换

---

## 📄 开源协议

本项目基于 **MIT 协议** 开源，详见 [LICENSE](LICENSE) 文件。

第三方库：
- 科大讯飞 MSC SDK — 遵循讯飞许可协议
- zlib / libjpeg / libxml2 — 各遵循其原始许可
- ALSA — LGPL

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

---

## ⭐ 致谢

- [科大讯飞](https://www.iflytek.com/) — 语音识别引擎
- [中科康芯](http://www.zkkxjh.cn/) — 神舟号 STM32F407 开发板
- ARM GEC210 开发板

---

*如果这个项目对你有帮助，请给一个 ⭐ Star！*
