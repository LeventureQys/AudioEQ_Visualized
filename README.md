# AudioEQ_Visualized

可视化的参量均衡器（Parametric Equalizer）界面工具，允许用户通过修改参数和拖动控件来实时调整 EQ 频响曲线。

目前项目处于早期开发阶段，仅完成了 DSP 运算核心。

## 技术栈

- **语言**: C++ (C++11+)
- **框架**: Qt (Qt5/Qt6)
- **算法**: RBJ (Robert Bristow-Johnson) Audio EQ Cookbook 双二阶滤波器

## 架构概览

```
┌──────────┐     ┌──────────────┐     ┌──────────────┐
│  UI 层    │ ──▶ │ CalculateCore│ ──▶ │  可视化绘图   │
│ (待开发)  │     │  (DSP 核心)  │     │  (Qt 控件)    │
└──────────┘     └──────────────┘     └──────────────┘
```

### 核心类: `CalculateCore`

单例 DSP 运算核心，负责所有 EQ 频响曲线的计算。

| 功能 | 说明 |
|---|---|
| **滤波器类型** | Peaking (峰值)、Low Shelf (低频搁架)、High Shelf (高频搁架)、Low-Pass (低通)、High-Pass (高通) |
| **多采样率支持** | 44.1kHz / 48kHz / 96kHz / 192kHz，一次计算预生成四组系数 |
| **频响计算** | 基于双二阶 IIR Z 传递函数，逐频点计算幅度响应 (dB) |
| **整合曲线** | 将所有频段的频响 + LPF + HPF 叠加，输出最终 EQ 曲线 |
| **频率吸附** | 对数刻度频率查找表 (10Hz ~ 48kHz, 446 个点)，支持吸附到最近有效值 |
| **Q 值吸附** | 对数刻度 Q 值查找表 (0.4 ~ 128 或 0.4 ~ 1.6, 102 个点) |
| **默认频点** | 15 个 ISO 近似中心频率 (31Hz ~ 19.9kHz) |

### 数据流

```
Point 列表 (type, freq, Q, gain)
        │
        ▼
MakeCoeffBand() ──▶ TFZ_coefficients (b0, b1, b2, a1, a2)
        │
        ▼
FreqResponse() ──▶ {频率 → 增益(dB)} 映射
        │
        ▼
QTotalFreqResponse() ──▶ QVector<double> (Qt 绘图用坐标)
```

### TFZ_coefficients 结构

```cpp
struct TFZ_coefficients {
    double b0, b1, b2;  // 分子系数 (零点)
    double a1, a2;       // 分母系数 (极点)
    bool blnBypass;      // 旁路标志
};
```

### 坐标映射 (`Actual_to_View`)

将实际频率 (Hz) 通过对数-线性插值映射到视图坐标 [0, 11]:

| 视图坐标 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---------|---|---|---|---|---|---|---|---|---|----|----|----|----|-----|
| 频率(Hz) | 1 | 20 | 50 | 100 | 200 | 500 | 1k | 2k | 5k | 10k | 20k | 50k |

## 文件结构

```
AudioEQ_Visualized/
├── exapmle/
│   ├── CalculateCore.h      # DSP 运算核心头文件 (229 lines)
│   └── CalculateCore.cpp    # DSP 运算核心实现 (748 lines)
└── README.md
```

## 依赖

- **Qt** (Qt5 或 Qt6): `QVector`, `QMap`, `QList`, `QJsonDocument`, `qDebug`
- 依赖项目内部文件（未提交至仓库）:
  - `DataClass.h` — 定义 `Point` 结构体、`BandType` 枚举
  - `PublicVar.h` — 定义全局参数 `sample_rate`、`nyquist_pattern`

## 待开发功能

- [ ] Qt UI 界面（频谱显示、频点拖动控件）
- [ ] 音频输入/输出流水线
- [ ] 实时音频播放与处理
- [ ] 预设管理
- [ ] 构建系统 (CMakeLists.txt 或 .pro)
- [ ] 主程序入口 (`main.cpp`)

## 许可

待定
