<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

<h1 align="center">塔罗占卜</h1>

<p align="center">
  完整的韦特塔罗——78 张牌、三个按键、一副口袋牌库。<br>
  <a href="docs/README.zh_CN.md">FoloToy AI Passport</a> 开源可穿戴平台应用。
</p>

<p align="center">
  <img src="assets/images/home.jpg" alt="FoloToy AI Passport 可穿戴设备正面、侧面与背面。" width="100%">
</p>

## 这是什么

塔罗占卜把 AI Passport 变成一台独立的占卜牌。固件开机直达应用——无菜单、无演示——并内置完整 78 张 Rider-Waite-Smith 牌库(1909 年，公有领域)与经典牌面。

- **78 张完整牌库**——22 张大阿卡纳 + 56 张小阿卡纳，每张牌配正/逆位关键词与中文短解读。
- **单张抽牌**——洗牌动画，一张牌问当下，随即进入解读页。
- **三牌阵**——过去、现在与未来，逐次按键翻开，之后可逐张浏览。
- **真实逆位**——约一半的抽取会翻出逆位：牌面真实上下颠倒显示，配朱砂描边与「逆位」徽记。
- **经典牌面**——78 张扫描图以两档分辨率(120 × 180 与 64 × 96)编译进固件，约占 8MB Flash 的 4.2MB。
- **口袋级的电量关怀**——右上角实时电量百分比。

## 怎么玩

三个按键驱动全部功能；任何页面长按确定键回到封面。

| 页面 | 上 / 下 | 确定键(短按) | 确定键(长按) |
| --- | --- | --- | --- |
| 封面 | 选择 单张 / 三牌阵 | 开始抽牌 | — |
| 洗牌 | — | 洗牌并翻开下一张 | 回封面 |
| 解读 | 滚动解读文本 | 三牌阵依次切换牌面 | 回封面 |

## 安装

用 [esptool](https://docs.espressif.com/projects/esptool/) 把经过验证的合并镜像从 `0x0` 烧入(任何等价的 ESP32-C3 烧录工具皆可)。合并烧录可能重置设备上的存储数据，参见[烧录与数据策略](docs/development/engineering/firmware-layout.zh_CN.md)。运行下方门禁后，最新验证过的构建位于 `build/FoloToy-AI-Passport-full.bin`。

## 从源码构建

前置条件：Windows / Linux / macOS 上的 [ESP-IDF 5.5.3](docs/development/engineering/environment-setup.zh_CN.md)。

```bash
./tools/validate.sh --static    # 仓库检查 + 主机测试(不需要工具链)
./tools/validate.sh --firmware  # ESP-IDF 构建 + 合并镜像校验
./tools/validate.sh             # 完整门禁
```

门禁产出 `build/FoloToy-AI-Passport-full.bin`(合并镜像,从 `0x0` 烧录)以及 `build/firmware/` 下的带哈希归档。详见[构建与测试](docs/development/engineering/build-and-test.zh_CN.md)。

## 再生成资产

UI 文案、字库与牌面均为生成式、确定性、已入库：

```bash
python tools/gen_tarot_font_symbols.py   # 从源码提取字形清单
./tools/gen_tarot_fonts.sh               # Noto Sans SC 子集(需要 node/npx)
python tools/gen_tarot_art.py --fetch    # 下载牌面扫描图(公有领域)并转换
python tools/gen_tarot_art.py --check    # 校验已入库牌面是否最新
```

来源与许可记录见[资产文档](assets/README.zh_CN.md)。

## 开发

- `main/` 承载全部应用逻辑;`components/bsp` 是未改动的板级支持层(显示、按键、音频、电池、共享 I2C)。
- 牌库数据、抽牌引擎、导航与符号几何均为不依赖 IDF/LVGL 的纯 C,由仓库门禁中的主机测试覆盖。
- AI 辅助开发从 [AGENTS.zh_CN.md](AGENTS.zh_CN.md) 开始;Fork 专属工作流见 [Fork 指南](docs/fork-guide.zh_CN.md)。
- 应用档案与可复用工程要点:[docs/reference/mutalisk999/tarot-divination](docs/reference/mutalisk999/tarot-divination/README.zh_CN.md)。

## 硬件

| | |
| --- | --- |
| 目标 | ESP32-C3,8MB Flash,无 PSRAM |
| 显示 | 240 × 320 竖屏,ST7789P3,SPI 接口 |
| 输入 | 三按键(上 / 下 / 确定),单 ADC 分压 |
| 其余占用 | 电量计(CW2017),共享 I2C |

板级事实与完整能力契约:[硬件开发指南](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md)。

## 署名与许可

- 牌面：Rider-Waite-Smith 塔罗，1909 年，Pamela Colman Smith 绘——公有领域;扫描件来自 [searge/tarot](https://github.com/searge/tarot)。
- 中文字体：Noto Sans SC,SIL Open Font License 1.1。
- 其余内容：MIT——见 [LICENSE](LICENSE)。贡献流程遵循 [CONTRIBUTING](.github/CONTRIBUTING.md)。
