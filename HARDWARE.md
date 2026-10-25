# 硬件说明 — Waveshare ESP32-S3-Touch-AMOLED-1.75C

本项目运行在 **Waveshare ESP32-S3-Touch-AMOLED-1.75C**（V1.0）上。
引脚与能力数据整理在 [`main/board.h`](main/board.h)，显示/触摸/I2C 初始化使用
Waveshare 官方 BSP：`waveshare/esp32_s3_touch_amoled_1_75c`。

## 板卡规格

| 项目 | 规格 |
| --- | --- |
| 型号 | Waveshare ESP32-S3-Touch-AMOLED-1.75C |
| MCU | ESP32-S3R8，双核 Xtensa LX7，最高 240 MHz |
| PSRAM | 8 MB Octal (OPI) PSRAM，80 MHz |
| Flash | 32 MB（本机实测；部分批次为 8/16 MB，以实际板卡为准） |
| 显示屏 | 1.75 英寸 **圆形** AMOLED，466×466，CO5300，QSPI |
| 触摸 | CST9217 电容触摸，I2C 地址 `0x5A`，分辨率 466×466 |
| 电源管理 | AXP2101，I2C 地址 `0x34`，电池 + USB-C，PWR/PEK 按键 |
| 传感器 | QMI8658 六轴 IMU（加速度 + 陀螺仪），I2C 地址 `0x6B` |
| 音频 | ES8311 音频 Codec + 功放 + 扬声器 |
| RTC | 无外部 RTC（没有 PCF85063；本固件用运行时间） |
| SD 卡 | 当前 BSP 未启用，本固件不使用 |
| 按键 | BOOT（GPIO0）+ PWR（AXP2101 PEK） |
| 调试/烧录 | USB-C，USB Serial/JTAG（开发机枚举为 COM8） |

> 注意：1.75C 是圆形屏。四周物理圆角/边缘会裁掉画面，因此 UI 需要把内容
> 收在圆心附近，避免把按钮放到四角。

## 引脚分配

### 显示（CO5300，QSPI）

| 信号 | GPIO |
| --- | --- |
| LCD_CS | 12 |
| LCD_PCLK / SCLK | 38 |
| LCD_SDIO0 | 4 |
| LCD_SDIO1 | 5 |
| LCD_SDIO2 | 6 |
| LCD_SDIO3 | 7 |
| LCD_RST | 1 |
| LCD_TE | 13（预留，未使用） |

BSP 初始化时设置了面板列偏移 `0x06`（圆形屏 RAM 窗口）。

### 触摸（CST9217，I2C）

| 信号 | GPIO |
| --- | --- |
| TP_RST | 2 |
| TP_INT | 11 |

### I2C 共享总线

| 信号 | GPIO |
| --- | --- |
| I2C_SDA | 15 |
| I2C_SCL | 14 |

总线上挂载：AXP2101（`0x34`）、CST9217（`0x5A`）、QMI8658（`0x6B`）、
ES8311（音频 Codec）。

### 音频（ES8311 + PA）

| 信号 | GPIO |
| --- | --- |
| I2S_MCLK | 16 |
| I2S_BCLK | 9 |
| I2S_WS / LRCK | 45 |
| I2S_DIN | 10 |
| I2S_DOUT | 8 |
| PA_CTRL | 46 |

### 按键

| 按键 | 连接 | 说明 |
| --- | --- | --- |
| BOOT / Key1 | GPIO0，低有效 | 短按打开菜单 / 返回；锁屏时唤醒并打开菜单 |
| PWR / Key3 | AXP2101 PEK | 短按锁屏/唤醒；长按关机；极长按 AXP 硬件关机 |

AXP2101 的 IRQ 没有引到 ESP32 GPIO，因此 PEK 事件通过 I2C 轮询
`INTSTS2` 寄存器检测。

## 固件如何使用这些硬件

- `bsp_display_start()`：初始化 CO5300 QSPI 屏、CST9217 触摸、LVGL 任务。
- `main/ui/power.c`：通过 BSP I2C 句柄访问 AXP2101，读取电池电压/电量，
  轮询 PWR 键事件，并调用软关机。
- `main/ui/imu.c`：通过 BSP I2C 句柄配置 QMI8658，读取加速度并做摇一摇检测。
- `main/ui/ui.cpp`：LVGL 全屏 UI；`pet-ui: display 466x466` 可在串口日志确认。
- `main/board.h`：本文件的引脚/能力常量，来源于参考工程
  [`claude-desktop-buddy-esp32`](https://github.com/vthinkxie/claude-desktop-buddy-esp32)
  的 `src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h`。

## 电源与烧录

- 电池通过 AXP2101 管理；USB-C 插入时日志中电量会显示 `USB`。
- 烧录命令：

```powershell
& "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"
idf.py -p COM8 flash monitor
```

- 首次编译会从组件仓库拉取 `waveshare/esp32_s3_touch_amoled_1_75c` 和 LVGL。

## 参考

- [Waveshare ESP32-S3-Touch-AMOLED-1.75C 文档](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C)
- [claude-desktop-buddy-esp32](https://github.com/vthinkxie/claude-desktop-buddy-esp32)
- [ESP-IDF 文档](https://docs.espressif.com/projects/esp-idf/)
- [LVGL 文档](https://docs.lvgl.io/)
