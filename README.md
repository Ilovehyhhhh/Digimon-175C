# 数码宝贝随身宠物 — ESP-IDF (Waveshare ESP32-S3-Touch-AMOLED-1.75C)

基于 ESP-IDF + LVGL 的数码宝贝养成 / 对战电子宠物，运行在
Waveshare **ESP32-S3-Touch-AMOLED-1.75C** 圆屏开发板上。

硬件引脚与能力取自
[`claude-desktop-buddy-esp32`](https://github.com/vthinkxie/claude-desktop-buddy-esp32) 的
`src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h`，整理在
[`main/board.h`](main/board.h)。显示、触摸和 I2C 总线由 Waveshare BSP 初始化。
完整的板卡规格、引脚表和电源/按键说明见 [`HARDWARE.md`](HARDWARE.md)。

## 功能

- **从蛋开始**：新存档是数码蛋，约 3 分钟孵化成黑球兽；8 分钟后可进化为滚球兽。
- **宠物形象**：用形态名的第一个汉字（96px）作为宠物，蛋形态显示“蛋”；
  图片素材通过 [`main/ui/pet_avatar.cpp`](main/ui/pet_avatar.cpp) 的
  `petAvatarSetImage()` 接入，不需要改游戏逻辑。
- **属性**：疫苗、数据、病毒三角克制；自由、未知为稀有属性，不参与三角。
- **战斗数值**：HP、SP、ATK、DEF、INT、SPI、SPD，全部按
  “物种基础值 + 等级成长 + 训练值” 计算；另外显示体重、战力和胜场。
- **网状进化 / 退化**：参考《数码宝贝物语 时空异客》的等级 / 能力值思路，共
  41 个形态、49 条进化边；从滚球兽开始分出 8 条成长期路线，再交叉到完全体 /
  究极体 / 超究极体。条件包含年龄、等级、能力值、体重区间和胜场。
  完整图见 [`docs/EVOLUTION.md`](docs/EVOLUTION.md)。
- **自由进化 / 退化**：满足条件后在「菜单 → 进化」里自由选择，也可以退化回上一形态
  （保留部分训练，经验/体重略降）。
- **对战（当前版本）**：简单剪刀石头布，攻击=剪刀、防御=石头、技能=布，对手随机出。
  胜利获得经验/能力点并减重，失败扣 HP/SP，平局只加少量经验；胜场会计入进化条件。
- **训练**：10 秒摇一摇或连续点击，随机提升战斗能力并增加体重。
- **锁屏**：大号时间 + 电池图标 + 百分比，充电时显示“充电中”；亮度跟随系统页设置。
- **系统页**：亮度滑条（掉电保存）、校时、电量、运行时间、设备信息、实体键说明、重置存档。
- **省电**：锁屏时 CPU 降到 80MHz、开启 light sleep、关闭 IMU、暂停动画并降低 LVGL 刷新频率。
- **实体按键**：锁屏/唤醒、关机都走实体键；屏幕锁定后触摸不会唤醒。

> 已移除喂食 / 玩耍 / 清洁 / 睡觉等旧养成按钮，也移除了 WiFi/NTP 与 TamaPoke 遗留模块。

## 按键

| 操作 | 效果 |
| --- | --- |
| BOOT 短按 | 主界面打开菜单；子界面返回；锁屏时唤醒并打开菜单 |
| PWR 短按 | **锁屏 / 唤醒**（仅实体键，触摸不唤醒） |
| PWR 长按 | **关机**（AXP2101 软关机） |
| PWR 极长按 | AXP2101 硬件关机 |
| 摇一摇 | 主界面开心 / 训练计数 |

锁屏页使用系统页里设置的亮度（可在系统页调低以进一步省电）；不会自动熄灭或自动唤醒，必须按实体键。

## 目录结构

```
Digimon-175C/
  CMakeLists.txt
  sdkconfig.defaults
  partitions.csv
  HARDWARE.md                 # 板卡规格 / 引脚 / 电源与按键
  main/
    CMakeLists.txt
    main.cpp                  # app_main: NVS -> 配置 -> BSP 显示 -> UI
    board.h                   # 1.75C 硬件数据（来自 claude-desktop-buddy）
    app/
      digimon_types.h         # 属性三角 / 战斗属性枚举
      digimon.{h,cpp}         # 蛋孵化、等级、HP/SP、能力值、体重、进化/退化
      digimon_cfg.{h,cpp}     # 41 个形态 / 49 条网状进化边（内嵌 JSON）
      battle.{h,cpp}          # 剪刀石头布对战、胜场与奖励
      compat/                 # Arduino millis/random + Preferences(NVS)
    ui/
      ui.{h,cpp}              # 主界面、菜单、状态、属性、系统、训练、战斗、锁屏
      pet_avatar.{h,cpp}      # 宠物形象（汉字；预留图片替换接口）
      clock.{c,h}             # 软件时钟（无 RTC）+ 校时 + NVS 保存
      power.{c,h}             # BOOT / AXP2101 按键、电量、关机
      imu.{c,h}               # QMI8658 摇一摇
      digifont*.c             # 生成的中文字库
  docs/
    EVOLUTION.md              # 完整进化网 + 条件表
  tools/
    gen_font.py               # 从源码生成 LVGL 中文字库 + 宠物大字
```

## 配置：物种 / 属性 / 进化

编辑 [`main/app/digimon_cfg.cpp`](main/app/digimon_cfg.cpp) 里的 `CFG_JSON`：

```json
{ "name": "亚古兽", "type": "病毒", "weight": 35,
  "stats": [120, 38, 34, 22, 14, 16, 26] }
```

- `type`：`疫苗` / `数据` / `病毒` / `自由` / `未知`
- `stats`：`[HP, SP, ATK, DEF, INT, SPI, SPD]` 基础值
- `weight`：基础体重（kg）

进化条件（真实示例：滚球兽 → 亚古兽）：

```json
{ "from": 2, "to": 3, "age": 15, "atk": 20, "wmin": 10 }
```

`age`(分钟) / `lvl` / `hp` / `atk` / `def` / `intel` / `spi` / `spd` /
`wmin` / `wmax` / `wins` 都可以组合，为 0 或省略即不检查。
`wins` 是剪刀石头布胜场，用来限制高阶段和奥米加兽等特殊路线。

> 形态 0 是数码蛋；孵化/进化条件参考《数码宝贝物语 时空异客》的等级 / 能力值思路，
> 并额外用体重、胜场做本固件的分支限制；时间压缩成了分钟，数值按本固件重新缩放。
> 完整 41 个形态和 49 条进化边见 [`docs/EVOLUTION.md`](docs/EVOLUTION.md)。

改完重新生成字库（新名字/新文本需要字库支持）：

```powershell
python tools\gen_font.py
```

## 属性克制

| 攻击方 | 克制 | 被克制 |
| --- | --- | --- |
| 疫苗 | 病毒 | 数据 |
| 病毒 | 数据 | 疫苗 |
| 数据 | 疫苗 | 病毒 |
| 自由 / 未知 | — | — |

效果：克制 150%，被克制 50%，其余 100%。

## 宠物素材替换

所有宠物绘制集中在 `main/ui/pet_avatar.cpp`。有图片后最简单的接入方式是：

1. 把图片转换成 LVGL `lv_image_dsc_t`（或使用 LVGL 文件系统）。
2. 在 `pet_avatar.cpp` 里把 `g_petImage` 指向你的图片资源。
3. 改成 `nullptr` 即可恢复汉字模式。

也可以直接调用 `petAvatarSetImage(avatar, &yourImageDsc)` 做更细粒度的控制。

游戏逻辑、数值和界面都不需要改。

## 编译与烧录

本机 ESP-IDF v6.1 / EIM 环境：

```powershell
chcp 65001
& "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"
idf.py set-target esp32s3
idf.py build
idf.py -p COM8 flash monitor
```

首次编译会从组件仓库拉取 Waveshare BSP
(`waveshare/esp32_s3_touch_amoled_1_75c`) 与 LVGL。

## 生成资源

```powershell
python tools\gen_font.py
```

生成：

- `digifont.c` 30px 主界面 / 菜单
- `digifont_small.c` 20px 次要文字
- `digifont_big.c` 64px 数字（电量 / 倒计时）
- `digifont_pet.c` 96px 宠物大字（自动取物种名首字）

## 锁屏 / 时钟 / 待机

- 1.75C 没有外部 RTC，固件使用软件时钟：首次用编译时间初始化，之后从 NVS 恢复上次保存时间；
  断电期间的时间无法自动补回，可在系统页「校时」调整。
- 锁屏显示 `HH:MM`、电池图标/百分比和充电状态；时间每分钟变化时才重绘，电池每 30 秒读取一次。
- 锁屏时 CPU 最高降到 80MHz、打开 light sleep、关闭 IMU、暂停宠物动画，并把 LVGL 刷新周期
  放慢到 500ms；解锁后恢复 240MHz 和正常刷新。
- 想进一步延长待机，可在系统页把亮度调低；AMOLED 的黑色区域本身不发光。

## 说明

- 1.75C 没有外部 RTC，也没有联网对时；年龄/经验只在运行时增长，时间可在系统页校时。
- 训练会回满 HP/SP，对战中残血时可以靠训练恢复。
- 当前对战是剪刀石头布占位版本，属性/等级/训练值暂不影响胜负；属性克制表已保留，
  后续可接回伤害计算。
- 体重影响进化分支（重 → 亚古兽系，轻 → 加布兽系）。
