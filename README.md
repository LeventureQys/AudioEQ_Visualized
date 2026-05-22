# AudioEQ_Visualized

可视化的参量均衡器（Parametric Equalizer）界面工具，允许用户通过修改参数和拖动控件来实时调整 EQ 频响曲线。

目前项目处于早期开发阶段，DSP 运算核心已完成，UI 层待开发。

## 技术栈

- **语言**: C++ (C++11+)
- **框架**: Qt (Qt5/Qt6)
- **算法**: RBJ (Robert Bristow-Johnson) Audio EQ Cookbook 双二阶滤波器 + Butterworth 滤波器

## 架构

```
                    ┌─────────────────────┐
                    │    CalculateCore     │
                    │       (协调器)        │
                    ├─────────────────────┤
                    │  频率/Q值查找表       │
                    │  频响计算 & 叠加      │
                    │  坐标映射            │
                    └───────┬─────────────┘
                            │ dispatches to
            ┌───────────────┼───────────────┐
            │               │               │
    ┌───────▼───────┐ ┌─────▼─────┐ ┌───────▼───────┐
    │  BiquadFilter │ │ BiquadFilter│ │  BiquadFilter │
    │   Peaking     │ │ LowShelf   │ │  HighShelf    │
    └───────────────┘ └─────────────┘ └───────────────┘
    ┌───────────────┐ ┌─────────────┐
    │  BiquadFilter │ │ BiquadFilter│
    │ ButterworthLPF│ │ButterworthHPF│
    └───────────────┘ └─────────────┘
```

### 类层级

```
BiquadFilter (抽象基类)
├── PeakingFilter       — RBJ 峰值均衡
├── LowShelfFilter      — RBJ 低频搁架
├── HighShelfFilter     — RBJ 高频搁架
├── ButterworthLPF      — 2 阶 Butterworth 低通
└── ButterworthHPF      — 2 阶 Butterworth 高通
```

### 数据流

```
Point 列表 (type, freq, Q, gain)
        │
        ▼
CalculateCore::MakeCoeffBand()  ──▶  dispatch to BiquadFilter::MakeCoeff()
        │
        ▼
TFZ_coefficients (b0, b1, b2, a1, a2)
        │
        ▼
CalculateCore::FreqResponse() ──▶ {频率 → 增益(dB)} 映射
        │
        ▼
CalculateCore::QTotalFreqResponse() ──▶ QVector<double> (Qt 绘图用坐标)
```

### BiquadFilter 抽象基类

```cpp
class BiquadFilter {
public:
    virtual ~BiquadFilter() = default;
    // 纯虚函数：计算双二阶滤波器系数
    virtual TFZ_coefficients MakeCoeff(double freq, double Q, double gain, double fs) = 0;
    // 模板方法：一次性计算 44.1k/48k/96k/192k 四种采样率的系数
    QMap<TFZType, TFZ_coefficients> MakeCoeffList(double freq, double Q, double gain, bool bypass = false);
};
```

### 添加新滤波器类型

只需继承 `BiquadFilter` 并实现 `MakeCoeff()`：

```cpp
class MyFilter : public BiquadFilter {
public:
    TFZ_coefficients MakeCoeff(double freq, double Q, double gain, double fs) override {
        // 实现你的双二阶滤波算法
    }
};
```

### 坐标映射 (`Actual_to_View`)

| 视图坐标 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---------|---|---|---|---|---|---|---|---|---|----|----|----|----|-----|
| 频率(Hz) | 1 | 20 | 50 | 100 | 200 | 500 | 1k | 2k | 5k | 10k | 20k | 50k |

## 文件结构

```
AudioEQ_Visualized/
├── calculator_core/            # 原始代码（保留作为参考）
│   ├── CalculateCore.h
│   └── CalculateCore.cpp
├── AudioEQ/                    # 重构后的代码
│   ├── DataClass.h             # Point 结构体、BandType 枚举
│   ├── PublicVar.h             # 全局参数单例 (采样率)
│   ├── PublicVar.cpp
│   ├── FilterBase.h            # BiquadFilter 抽象基类 + TFZ_coefficients + TFZType
│   ├── FilterBase.cpp
│   ├── PeakingFilter.h/cpp     # RBJ 峰值滤波器
│   ├── LowShelfFilter.h/cpp    # RBJ 低频搁架滤波器
│   ├── HighShelfFilter.h/cpp   # RBJ 高频搁架滤波器
│   ├── ButterworthLPF.h/cpp    # Butterworth 低通滤波器
│   ├── ButterworthHPF.h/cpp    # Butterworth 高通滤波器
│   ├── CalculateCore.h         # 协调器（查找表、频响计算、滤波器调度）
│   └── CalculateCore.cpp
└── README.md
```

## 依赖

- **Qt** (Qt5 或 Qt6): `QVector`, `QMap`, `QList`

## 待开发功能

- [ ] Qt UI 界面（频谱显示、频点拖动控件）
- [ ] 音频输入/输出流水线
- [ ] 实时音频播放与处理
- [ ] 预设管理
- [ ] 构建系统 (CMakeLists.txt 或 .pro)
- [ ] 主程序入口 (`main.cpp`)

## 许可

待定
