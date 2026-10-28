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
- **宠物形象**：不再用汉字/图片，改为程序化线条+像素轮廓；每个形态有独立轮廓
  （蛋、幼体、成长期、完全体、究极体、超究极体），并按心情显示眼睛/嘴巴表情。
  图片素材仍可通过 [`main/ui/pet_avatar.cpp`](main/ui/pet_avatar.cpp) 的
  `petAvatarSetImage()` 接入。
- **互动**：触摸宠物冒“爱心”并发出叫声；实体按钮有按键音；屏幕朝下约 1.5 秒会
  睡觉并显示 `Zzz`，摇一摇唤醒。
- **属性**：疫苗、数据、病毒三角克制；自由、未知为稀有属性，不参与三角。
- **战斗数值**：HP、SP、ATK、DEF、INT、SPI、SPD，全部按
  “物种基础值 + 等级成长 + 训练值” 计算；另外显示体重、战力和胜场。
- **网状进化 / 退化**：参考《数码宝贝物语 时空异客》的等级 / 能力值思路，共
  41 个形态、45 条进化边；从滚球兽开始分出 8 条成长期路线，再交叉到完全体 /
  究极体 / 超究极体。条件包含年龄、等级、能力值、体重区间和胜场。
  完整图见 [`docs/EVOLUTION.md`](docs/EVOLUTION.md)。
- **自由进化 / 退化**：满足条件后在「菜单 → 进化」里自由选择，也可以退化回上一形态
  （保留部分训练，经验/体重略降）。
- **对战（当前版本）**：简单剪刀石头布，攻击=剪刀、防御=石头、技能=布，对手随机出。
  胜利获得经验/能力点并减重，失败扣 HP/SP，平局只加少量经验；胜场会计入进化条件。
- **训练**：10 秒摇一摇或连续点击，随机提升战斗能力并增加体重。
- **锁屏**：大号时间 + 电池图标 + 百分比；按实际状态显示“充电中 / 已充满 / USB”，
  亮度跟随系统页设置；锁屏 20 秒后自动关屏省电，PWR 短按解锁。
- **系统页**：亮度滑条（掉电保存）、校时、电量、运行时间、设备信息、实体键说明、网页管理、重置存档。
- **网页管理 / OTA**：系统页进入网页模式后，手机/电脑连接设备 AP `Digimon-175C`
  （密码 `digimon123`），打开 `http://192.168.4.1` 即可导出/导入 JSON 存档、直接改数值、
  上传 `.bin` 固件做 OTA。网页模式会临时释放界面内存，设备屏幕保持静态；BOOT 短按退出，
  5 分钟无操作自动关闭，WiFi 只在网页模式开启。
- **省电**：锁屏时 CPU 降到 80MHz、开启 light sleep、关闭 IMU、暂停动画、降低 LVGL 刷新、
  20 秒后关屏；每次发声后关闭 ES8311/功放，避免待机音频耗电。
- **实体按键**：锁屏/唤醒、关机都走实体键；屏幕锁定后触摸不会唤醒。

> 已移除喂食 / 玩耍 / 清洁 / 睡觉等旧养成按钮，也移除了 TamaPoke 遗留模块；
> WiFi 不再常开，只用于按需进入的网页管理/OTA 模式。

## 按键

| 操作 | 效果 |
| --- | --- |
| BOOT 短按 | 主界面打开菜单；子界面返回（网页模式退出并关闭 WiFi）；**锁屏时不响应** |
| PWR 短按 | **锁屏 / 唤醒**（锁屏只能用它退出） |
| PWR 长按 | **关机**（AXP2101 软关机） |
| PWR 极长按 | AXP2101 硬件关机 |
| 触摸宠物 | 冒爱心 |
| 屏幕朝下 | 宠物睡觉 |
| 摇一摇 | 唤醒睡觉的宠物；训练时计数 |

所有实体按键都会发出短按键音；触摸宠物是上扬的叫声（ES8311/I2S）。
锁屏页使用系统页里设置的亮度；20 秒后自动关屏，PWR 短按直接解锁。不会自动唤醒。

## 目录结构

```
Digimon-175C/
  CMakeLists.txt
  sdkconfig.defaults
  partitions.csv
  HARDWARE.md                 # 板卡规格 / 引脚 / 电源与按键
  components/
    waveshare__esp32_s3_touch_amoled_1_75c/  # 本地 BSP（LVGL 部分刷新 4 行，兼容 WiFi 内存）
  main/
    CMakeLists.txt
    main.cpp                  # app_main: NVS -> 配置 -> BSP 显示 -> UI
    board.h                   # 1.75C 硬件数据（来自 claude-desktop-buddy）
    app/
      digimon_types.h         # 属性三角 / 战斗属性枚举
      digimon.{h,cpp}         # 蛋孵化、等级、HP/SP、能力值、体重、进化/退化
      digimon_cfg.{h,cpp}     # 41 个形态 / 45 条网状进化边（内嵌 JSON）
      battle.{h,cpp}          # 剪刀石头布对战、胜场与奖励
      compat/                 # Arduino millis/random + Preferences(NVS)
    ui/
      ui.{h,cpp}              # 主界面、菜单、状态、属性、系统、训练、战斗、锁屏
      pet_avatar.{h,cpp}      # 每形态独立线条/像素轮廓 + 表情（预留图片替换）
      clock.{c,h}             # 软件时钟（无 RTC）+ 校时 + NVS 保存
      audio.{c,h}             # ES8311 叫声 / 按键音
      power.{c,h}             # BOOT / AXP2101 按键、电量、关机
      imu.{c,h}               # QMI8658 摇一摇
      web.{cpp,h}             # 本地 AP + 网页存档/改数值/OTA
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
> 完整 41 个形态和 45 条进化边见 [`docs/EVOLUTION.md`](docs/EVOLUTION.md)。

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
3. 改成 `nullptr` 即可恢复程序化线条/像素模式。

也可以直接调用 `petAvatarSetImage(avatar, &yourImageDsc)` 做更细粒度的控制。

游戏逻辑、数值和界面都不需要改。

## 网页管理 / OTA

1. 设备进入「系统 → 网页」；屏幕会显示 AP 信息并保持静态。
2. 手机/电脑连接 WiFi：
   - SSID：`Digimon-175C`
   - 密码：`digimon123`
   - 地址：`http://192.168.4.1`
3. 网页内可以：
   - **导出 JSON**：下载完整存档（形态、年龄、经验、体重、HP/SP、训练值、胜场、时间）。
   - **导入 JSON**：用导出的文件覆盖当前存档。
   - **改数值**：表单直接修改形态/等级相关数值/训练值/时间，保存后写入 NVS。
   - **OTA 固件**：选择 `build/digimon_pet.bin`（应用固件，不是合并镜像）上传；
     写入完成后设备自动重启，仍保留 NVS 存档。
4. 退出：按 **BOOT 短按**，或等待 5 分钟无操作自动关闭；退出后 WiFi 关闭、界面重建。

> 网页模式为了给 WiFi 腾出内部 RAM，会临时删除并释放其它 LVGL 界面，屏幕保持
> 静态 AP 信息（不再刷新）；退出网页模式后自动重建界面。这样 WiFi 和 AMOLED
> 的 SPI DMA 不会同时抢内部 RAM。

## 编译与烧录

本机 ESP-IDF v6.1 / EIM 环境：

```powershell
chcp 65001
& "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"
idf.py set-target esp32s3
idf.py build
idf.py -p COM8 flash monitor
```

BSP 已放在 `components/waveshare__esp32_s3_touch_amoled_1_75c`（本地组件，LVGL
部分刷新高度改为 4 行，避免 WiFi 与 AMOLED SPI DMA 抢内部 RAM）；其余组件首次编译时
仍会从组件仓库拉取。

分区表已改为 OTA A/B（`ota_0` / `ota_1`，各 4MB）。**第一次升级到本版本必须用 USB
串口烧录一次**，之后就可以在网页里上传 `build/digimon_pet.bin` 做 OTA 升级。

## 生成资源

```powershell
python tools\gen_font.py
```

生成：

- `digifont.c` 30px 主界面 / 菜单
- `digifont_small.c` 20px 次要文字
- `digifont_big.c` 64px 数字（电量 / 时间 / 倒计时）

## 锁屏 / 时钟 / 待机

- 1.75C 没有外部 RTC，固件使用软件时钟：首次用编译时间初始化，之后从 NVS 恢复上次保存时间；
  断电期间的时间无法自动补回，可在系统页「校时」调整。
- 锁屏显示 `HH:MM`、电池图标/百分比和充电状态；只有真正充电时显示“充电中”，
  插着 USB 但未充电显示“USB”，满电显示“已充满”。时间每分钟变化时才重绘。
- 锁屏 20 秒后自动关闭屏幕（仍是锁定状态），避免屏幕长期耗电；PWR 短按解锁。
- 锁屏时 CPU 最高降到 80MHz、打开 light sleep、关闭 IMU、暂停宠物动画，并把 LVGL 刷新周期
  放慢到 500ms（关屏后 1000ms）；解锁后恢复 240MHz 和正常刷新。
- 宠物和时钟每 2 分钟保存一次，锁屏时也会立即保存；电量低于 5% 时自动保存并关机。
- 想进一步延长待机，可在系统页把亮度调低；AMOLED 的黑色区域本身不发光。

## 说明

- 1.75C 没有外部 RTC，也没有联网对时；时间可在系统页校时。断电再上电会恢复到
  最后一次保存的时间，关机期间经过的时间无法自动补回。
- 宠物/时钟每 2 分钟保存，锁屏时立即保存；低电量会自动保存并关机，避免断电丢存档。
- 训练会回满 HP/SP，对战中残血时可以靠训练恢复。
- 当前对战是剪刀石头布占位版本，属性/等级/训练值暂不影响胜负；属性克制表已保留，
  后续可接回伤害计算。
- 体重影响进化分支（重 → 亚古兽系，轻 → 加布兽系）。

