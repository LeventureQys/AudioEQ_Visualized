# AudioEQ 第一阶段开发文档（详细版 v2）

## 项目概览

本项目是一个基于 **Qt6 + Vulkan** 的音视频可视化 EQ SDK，以**动态库**形式交付给外部使用。对外暴露名为 `AudioEQ` 的 **QWidget 子类**，该类自身负责界面显示 + 逻辑接口。构建系统使用 **CMake**，无单例模式，每个 AudioEQ 实例独立管理自己的所有资源（包括 Vulkan 设备），支持同一进程中多个实例并存。

---

## 第一章：整体架构

### 1.1 模块划分

```
┌──────────────────────────────────────────────────────────┐
│                       AudioEQ                            │
│  (QWidget subclass - 对外唯一入口)                        │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐ │
│  │EqualizerModel│  │CurveEngine   │  │ ViewEqualizer  │ │
│  │  数据模型     │  │ 曲线计算引擎  │  │ (内部 Widget)  │ │
│  └──────────────┘  │ (后台线程)    │  │                │ │
│                    └──────────────┘  │ ┌────────────┐ │ │
│                          │           │ │VulkanRend. │ │ │
│                    ┌─────┴─────┐     │ ├────────────┤ │ │
│                    │FilterAlgo │     │ │BandHandle×N│ │ │
│                    │ (抽象基类) │     │ │LpfHandle   │ │ │
│                    └───────────┘     │ │HpfHandle   │ │ │
│                                      │ └────────────┘ │ │
│                                      └────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

### 1.2 数据流

```
外部设置参数 → EqualizerModel (存储)
                    ↓  快照复制
              CurveEngine (后台线程异步计算)
                    ↓  ┌──────────────────────────────┐
                    ↓  │ FilterAlgorithm.evaluateAt() │ ← 纯函数, 线程安全
                    ↓  └──────────────────────────────┘
                    ↓ 跨线程信号返回结果
            QVector<QPointF> (视口坐标系点集)
                    ↓
            ViewEqualizer
              ├── VulkanRenderer (绘制)
              └── BandHandle × N (交互捕获)
                    ↓
              屏幕显示
```

用户拖拽 Band → BandHandle 发出 Qt 信号 → AudioEQ 槽函数 → CurveEngine 异步重算 → 结果信号返回 → VulkanRenderer 重绘

### 1.3 运行前 Vulkan 可用性检测

在 `AudioEQ` 构造函数或使用 `QVulkanInstance` 之前，调用检测函数：

```cpp
// AudioEQ 暴露的静态检测方法
static bool AudioEQ::isVulkanSupported();

// 内部实现：尝试创建 QVulkanInstance, 查询物理设备
// 如果 Vulkan 不可用 → 通过信号/返回值报告错误, 不降级
// 调用方在构造 AudioEQ 之前应调用此方法确认
```

如果 Vulkan 不可用，直接报错（日志 + 返回 false），不做降级渲染。后续如果需要，再考虑通过转译层（如 MoltenVK、DXVK 等）兜底。

---

## 第二章：数据结构定义

### 2.0 通用返回码

所有涉及状态变更的 API 统一使用以下返回码，不抛异常：

```cpp
enum class ResultCode {
    OK = 0,
    Failed,                 // 通用失败
    IndexOutOfRange,        // index 越界
    IndexConflict,          // addBand 时 index 冲突
    InvalidParameter,       // 参数非法 (e.g. min >= max)
    VulkanNotAvailable,     // Vulkan 不可用
};
```

### 2.1 枚举类型

```cpp
// 滤波器算法类型（可扩展）
enum class FilterAlgorithm {
    ButterworthIIR,   // 二阶IIR巴特沃斯
    // 未来扩展: LinkwitzRiley, Bessel, FIR, ...
};

// 滤波器类型
enum class FilterType {
    Peak,       // 峰值/钟形滤波器
    LowShelf,   // 低频搁架
    HighShelf,  // 高频搁架
    LowPass,    // 低通 (高切)
    HighPass,   // 高通 (低切)
    BandPass,   // 带通
};

// 采样率枚举
enum class SampleRate : int {
    SR_44100  = 44100,
    SR_48000  = 48000,
    SR_96000  = 96000,
    SR_192000 = 192000,
};
```

### 2.2 EQBand 结构

```cpp
struct EQBand {
    int     index       = -1;           // Band 序号 (0-based)
    double  frequency   = 1000.0;       // 中心频率 (Hz)
    double  gain        = 0.0;          // 增益 (dB)
    double  q           = 1.0;          // Q 值
    FilterType      type      = FilterType::Peak;
    FilterAlgorithm algorithm = FilterAlgorithm::ButterworthIIR;
    bool    bypass      = false;        // 是否旁路
};
```

### 2.3 Band 焦点 / Z-Order 机制

多个 Band 可以自由重叠。当前被选中（获得焦点）的 Band 在渲染和交互上置顶：

```cpp
class EqualizerModel : public QObject {
    // ...
    int  focusedBandIndex() const;          // -1 表示无焦点
    void setFocusedBandIndex(int index);

    void moveBandZOrder(int fromIndex, int toIndex); // 手动调整堆叠顺序
};
```

焦点规则：
- 鼠标点击某个 Band → 该 Band 获得焦点
- 焦点 Band 的渲染层级最高（圆圈绘制在其它 Band 上方）
- 点击空白区域 → 取消焦点
- `m_focusedBandIndex` 在 EqualizerModel 中维护，通过信号通知

### 2.4 LPF / HPF 特殊 Band

LPF 和 HPF 是**椭圆形**控件，中间显示 `LPF` / `HPF` 字样（而非数字）。特性：
- index 固定：HPF = -2，LPF = -1
- Gain 锁定为 0 dB，只允许水平拖动（改变频率）
- 默认不启用（enabled = false），未启用时颜色较暗
- LPF 位于界面最右侧，HPF 位于界面最左侧
- 在 EqualizerModel 中独立存储，不混入普通 band 列表

```cpp
struct ShelfBand {
    double          frequency   = 20000.0;      // LPF 默认~20kHz, HPF 默认~20Hz
    bool            enabled     = false;
    FilterAlgorithm algorithm   = FilterAlgorithm::ButterworthIIR; // 可配置算法
};
```

### 2.5 EqualizerModel (完整数据模型)

```cpp
class EqualizerModel : public QObject {
    Q_OBJECT
public:
    // ===== Band 管理 =====
    int  bandCount() const;
    ResultCode setBandCount(int count);         // 默认 5, 构造时自动调用

    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);   // 忽略传入 index, 自动分配
    ResultCode removeBand(int index);                                     // 删除指定 band

    const EQBand& bandAt(int index) const;
    ResultCode setBandParams(int index, const EQBand& params);
    QVector<EQBand> allBands() const;

    // ===== 焦点管理 =====
    int  focusedBandIndex() const;
    void setFocusedBandIndex(int index);
    ResultCode moveBandZOrder(int fromIndex, int toIndex);

    // ===== 采样率 =====
    SampleRate sampleRate() const;
    ResultCode setSampleRate(SampleRate rate);
    double nyquistFrequency() const;

    // ===== LPF / HPF =====
    ShelfBand lpf() const;
    ResultCode setLpf(const ShelfBand& lpf);
    ShelfBand hpf() const;
    ResultCode setHpf(const ShelfBand& hpf);

    // 便捷方法
    ResultCode setLpfEnabled(bool enabled);
    ResultCode setHpfEnabled(bool enabled);

signals:
    void bandChanged(int index);
    void bandAdded(int index);
    void bandRemoved(int index);
    void bandCountChanged(int newCount);
    void focusedBandChanged(int index);
    void sampleRateChanged(SampleRate rate);
    void lpfChanged();
    void hpfChanged();
    void modelReset();
};
```

---

## 第三章：滤波器算法抽象

### 3.1 设计原则

- **不暴露 biquad 系数**（TFZ_coefficients 删除）
- 外部只通过 `(频率, 增益, Q值, 类型)` 配置滤波器
- 曲线点计算对调用方透明
- `evaluateAt()` 是纯函数（const），天然线程安全

### 3.2 FilterAlgorithm 抽象基类

```cpp
class FilterAlgorithm {
public:
    virtual ~FilterAlgorithm() = default;

    /// @brief 计算滤波器在给定频率点上的增益 (dB) — 纯函数, 线程安全
    virtual double evaluateAt(double freqHz, double sampleRate, const EQBand& band) const = 0;

    /// @brief 返回算法名称标识
    virtual FilterAlgorithm type() const = 0;

    /// @brief 返回该算法支持的 Q 值范围 (min, max)
    /// @param filterType 滤波器类型
    virtual QPair<double,double> qRange(FilterType filterType) const = 0;
};
```

### 3.3 ButterworthIIR 实现

```cpp
class ButterworthIIR : public FilterAlgorithm {
public:
    FilterAlgorithm type() const override { return FilterAlgorithm::ButterworthIIR; }
    double evaluateAt(double freqHz, double sampleRate, const EQBand& band) const override;

    QPair<double,double> qRange(FilterType filterType) const override {
        switch (filterType) {
        case FilterType::Peak:       return {0.4, 128.0};
        case FilterType::LowShelf:
        case FilterType::HighShelf:  return {0.4, 1.6};
        case FilterType::LowPass:
        case FilterType::HighPass:
        case FilterType::BandPass:   return {0.4, 128.0};
        }
        return {0.4, 128.0};
    }

private:
    struct BiquadCoeff { double b0, b1, b2, a1, a2; };

    BiquadCoeff makePeakFilter(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeLowShelf(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeHighShelf(double freq, double q, double gain, double sampleRate) const;
    BiquadCoeff makeLowPass(double freq, double sampleRate) const;
    BiquadCoeff makeHighPass(double freq, double sampleRate) const;

    double freqResponseGain(const BiquadCoeff& coeff, double freq, double sampleRate) const;
};
```

### 3.4 算法工厂（支持扩展）

```cpp
class FilterAlgorithmFactory {
public:
    static std::unique_ptr<FilterAlgorithm> create(FilterAlgorithm algoType);
    static void registerAlgorithm(FilterAlgorithm algoType,
                                  std::function<std::unique_ptr<FilterAlgorithm>()> creator);
};
```

---

## 第四章：曲线计算引擎 (CurveEngine) — 多线程设计

### 4.1 线程模型

CurveEngine 在**独立后台线程**上运行，不阻塞主线程（UI）:

```
主线程                         后台工作线程
  │                               │
  ├─ requestTotalCurve(model) ──→ │
  │  (信号/槽 跨线程队列)         ├─ 复制 model 快照
  │                               ├─ 生成对数频点
  │                               ├─ 逐频点累加所有 band 增益
  │                               ├─ 坐标变换
  │                               └─ totalCurveReady(points) ──→ 主线程
  │                                                              │
  │                                                       VulkanRenderer 更新 VBO
```

关键设计：
- 使用 `QObject::moveToThread()` 将 CurveEngine 工作对象移至 `QThread`
- 主线程通过 queued signal 发起计算请求
- 工作线程完成后通过 queued signal 将结果发回主线程
- `FilterAlgorithm::evaluateAt()` 是 const 纯函数，所有线程共享同一个 FilterAlgorithm 实例，无锁
- 如果新的计算请求到达而旧的计算尚未完成，**取消旧计算**（通过原子标志位），只处理最新请求

### 4.2 CurveEngine 接口

```cpp
class CurveEngine : public QObject {
    Q_OBJECT
public:
    explicit CurveEngine(FilterAlgorithm* algo, QObject* parent = nullptr);
    ~CurveEngine();

    void setPointCount(int count);
    int  pointCount() const;

    void setFreqRange(double minHz, double maxHz);
    void setGainRange(double minDb, double maxDb);

    // 异步请求 (主线程调用, 非阻塞)
    void requestTotalCurve(const EqualizerModel& modelSnapshot);
    void requestSingleBandCurve(int bandIndex, const EqualizerModel& modelSnapshot);
    void cancelPending();

signals:
    // 结果回到主线程
    void totalCurveReady(QVector<QPointF> points);
    void singleBandCurveReady(int bandIndex, QVector<QPointF> points);

private:
    QThread m_workerThread;
    FilterAlgorithm* m_filterAlgo;          // 非拥有, 纯函数, 共享使用
    std::atomic<bool> m_cancelPending{false};
    int m_pointCount = 500;                 // 默认值, 可通过 AudioEQ SDK 配置
    double m_freqMin = 1.0, m_freqMax = 24000.0;
    double m_gainMin = -48.0, m_gainMax = 48.0;
};
```

### 4.3 频点生成策略（对数分布）

**设计缘由**：如果使用线性分布生成频率点，低频段（20Hz~200Hz）只有约 9% 的采样点，而高频段（10kHz~20kHz）占 50%，导致低频段曲线稀疏、锯齿明显。使用对数分布可保证每个倍频程（octave）拥有相同密度的采样点，曲线在低高频段都平滑。

```cpp
// freq[i] = freqMin * (freqMax / freqMin) ^ (i / (pointCount - 1))
// 每个倍频程 (octave) 的采样点数 = pointCount / log2(freqMax / freqMin)
QVector<double> generateLogFrequencyPoints(double freqMin, double freqMax, int pointCount);
```

### 4.4 总频响曲线计算（运行在工作线程）

```cpp
QVector<QPointF> computeTotalCurve(const EqualizerModel& model, int pointCount);
// 1. 生成对数频率点列表 (1Hz ~ nyquistFrequency)
// 2. 对每个频率点, 累加所有 band 的增益
// 3. 叠加 LPF/HPF 增益 (if enabled)
// 4. 坐标变换 (频率→x, 增益→y)
// 5. 裁剪到增益范围 [-48, +48] dB（默认, 可配置）
```

### 4.5 坐标变换

```cpp
class CoordinateMapper {
public:
    CoordinateMapper(QRect viewport, double freqMin, double freqMax, double gainMin, double gainMax);

    double freqToX(double freqHz) const;    // 频率 → X (对数映射)
    double gainToY(double gainDb) const;    // 增益 → Y (线性映射)
    double xToFreq(double x) const;         // 反向映射 (鼠标交互)
    double yToGain(double y) const;

    void setViewport(QRect viewport);       // resize 时仅更新视口, 不重新计算曲线
    void setFreqRange(double min, double max);
    void setGainRange(double min, double max);
};
```

### 4.6 Band 默认频率分配

当 `setBandCount(N)` 被调用时，默认频点按对数均匀分布，频率范围为 `[20Hz, 20000Hz]`：

```cpp
// 示例: N=5 → { 20, 158, 1000, 6300, 20000 } Hz (近似)
//       N=10 → { 20, 46, 106, 245, 562, 1290, 2960, 6800, 13850, 20000 }
// 算法: freq[i] = freqMin * (freqMax / freqMin) ^ (i / (N - 1))
```

### 4.7 Q 值范围变更时的 Clamp 行为

当通过 `setQRange(type, min, max)` 修改某滤波器类型的 Q 值范围后，所有使用该类型的已有 band 的 Q 值**自动 clamp** 到新范围：

```cpp
// 例: band[0].q = 5.0, 类型 Peak
eq->setQRange(FilterType::Peak, 0.4, 2.0);
// → band[0].q 自动 clamp 为 2.0
```
`EqualizerModel` 内部遍历所有 band，对匹配类型的 band 的 Q 值执行 `std::clamp`。修改后的值通过 `bandChanged` 信号通知。

---

## 第五章：频率坐标轴刻度值

### 5.1 横轴刻度 (对数频率, 始终显示到 Nyquist)

显示范围从 **10Hz** 到 **当前 Nyquist 频率**，Nyquist 根据采样率动态变化：

| 采样率 | Nyquist | X 轴最大刻度 |
|--------|---------|------------|
| 44100  | 22050   | 22k        |
| 48000  | 24000   | 24k        |
| 96000  | 48000   | 48k        |
| 192000 | 96000   | 96k        |

栅格刻度线（参考常见专业 EQ 产品）：

```
20Hz, 30Hz, 40Hz, 50Hz, 60Hz, 80Hz,
100Hz, 200Hz, 300Hz, 400Hz, 500Hz, 800Hz,
1kHz, 2kHz, 3kHz, 4kHz, 5kHz, 8kHz,
10kHz, 15kHz, 20kHz, [Nyquist]
```

显示规则：< 1000Hz 显示 `xx Hz`，>= 1000Hz 显示 `x.x kHz` 或 `xx kHz`。

### 5.2 纵轴刻度 (增益)

默认范围 **[-48dB, +48dB]**，可通过 `AudioEQ::setGainRange()` 配置：

```
-48dB, -42dB, -36dB, -30dB, -24dB, -18dB, -12dB, -6dB,
0dB,
+6dB, +12dB, +18dB, +24dB, +30dB, +36dB, +42dB, +48dB
```

---

## 第六章：Vulkan 渲染引擎 (VulkanRenderer)

### 6.1 总体设计

- **原生 Vulkan API**，每个 AudioEQ 实例拥有独立的 Vulkan 上下文
- 同一进程中多个实例各自创建自己的 Instance/Device/Swapchain
- 跨平台 (Windows / Linux / macOS via MoltenVK)
- 启动前通过 `AudioEQ::isVulkanSupported()` 检测可用性，不可用则报错

### 6.2 依赖

| 库 | 用途 |
|---|---|
| Vulkan SDK | Vulkan 核心 API |
| volk | Vulkan 加载器 (推荐，简化函数加载) |
| GLM | 数学库 (矩阵/向量运算) |
| stb_truetype | 轻量级 TrueType 字体光栅化 |

### 6.3 渲染管线模块

```
VulkanRenderer
├── VulkanContext      -- 实例、设备、队列 (每实例独立)
├── VulkanSwapchain    -- 交换链、图像视图、MSAA resolve
├── VulkanPipeline     -- 管线池管理 (4条管线)
├── VulkanBufferPool   -- 顶点/索引/Uniform 缓冲区管理
├── VulkanFontAtlas    -- 字体纹理图集 (stb_truetype + 嵌入字体)
└── VulkanFrameSync    -- 帧同步 (Fences / Semaphores)
```

### 6.4 四条渲染管线

| 管线 | 用途 | 图元类型 |
|---|---|---|
| **Grid** | 坐标轴 + 网格线 | LineStrip |
| **Curve** | 频响曲线 (总曲线 + 单band曲线) | TriangleStrip (Catmull-Rom) |
| **Fill** | 单band曲线下方的半透明填充 | TriangleStrip |
| **Glyph** | 文字标签 + Band 圆形/椭圆控件 | TriangleStrip |

### 6.5 曲线平滑策略

使用 **Catmull-Rom 样条**将离散点插值为平滑曲线：
1. CurveEngine 输出 ~N 个点 (默认 500，可配置)
2. Catmull-Rom 插值密度自适应视口宽度（约每像素 1-2 细分点）
3. 生成 TriangleStrip 送入 Vulkan
4. 线宽：主曲线 2px / 单band曲线 1.5px

### 6.6 文字渲染方案

使用 **嵌入开源字体 + stb_truetype + 纹理图集**：
1. 嵌入一个开源 TrueType 字体文件（见附录：需求清单）
2. 将 ASCII 可打印字符 + 数字 + 常用符号光栅化为单通道纹理图集
3. 每个字符 = 一个带 UV 的 Quad，shader 做纹理采样 + alpha 混合
4. 静态文字（坐标轴标签）初始化时一次性生成 VBO
5. 动态文字（Band 数字 / LPF/HPF 文字）在状态变化时更新对应 VBO 区域

### 6.7 抗锯齿

- **MSAA**：Swapchain 创建时启用 `VK_SAMPLE_COUNT_4_BIT`
- **曲线边缘**：Fragment Shader 中 `smoothstep` 距离衰减

### 6.8 高 DPI 适配

- `devicePixelRatioF()` 获取缩放因子
- Swapchain extent × devicePixelRatio
- 字体渲染 fontSize × devicePixelRatio
- 线宽、坐标轴标签等均按比例缩放

### 6.9 颜色方案

| 元素 | 颜色 |
|---|---|
| 背景 | `#1A1A1A` (暗灰) |
| 坐标轴线 | `rgba(255,255,255,0.85)` |
| 网格线 | `rgba(255,255,255,0.12)` |
| 主频响曲线 | `#00FF00` (绿, 硬编码, 所有 band 共用) |
| 单band曲线 | `#00FF00` (绿, 更细) |
| 单band填充 | `rgba(0,255,0,0.15)` |
| Band 圆形 | 填充 `#1A1A1A`，描边 `rgba(255,255,255,0.9)` 1px |
| Band 选中态(焦点) | 圆形填充变亮 `#3A3A3A` |
| Band 内部数字 | `rgba(255,255,255,0.9)` |
| LPF/HPF 椭圆(未开启) | 填充 `#1A1A1A`，描边 `rgba(255,255,255,0.6)` |
| LPF/HPF 椭圆(已开启) | 填充 `#1A1A1A`，描边 `#00FF00` |
| LPF/HPF 内部文字 | `rgba(255,255,255,0.9)` (未开启) / `#00FF00` (已开启) |

### 6.10 LPF / HPF 视觉形态

- **椭圆形**（水平拉长的椭圆，区别于 Band 的圆形）
- 内部显示 `LPF` / `HPF` 文字（不是数字）
- LPF 固定在界面最右侧（频率轴高段），HPF 固定在最左侧（频率轴低段）
- 仅水平拖动，垂直方向锁定在 0dB 位置

### 6.11 帧率控制

- **空闲态**：`VK_PRESENT_MODE_FIFO` (VSync)，跟随显示器刷新率
- **拖拽态**：QTimer 限制重算 + 提交频率 ≤ **60Hz** (16.67ms)

```cpp
void BandHandle::onMouseMove(QMouseEvent* event) {
    m_pendingPosition = event->pos();
    if (!m_dragTimer.isActive()) {
        m_dragTimer.start(16);      // ~60Hz
        processDrag();              // 立即处理首帧
    }
}
```

---

## 第七章：Band 交互设计 (BandHandle)

### 7.1 Widget 层级

```
ViewEqualizer (内部 QWidget)
├── VulkanRenderer        (Vulkan 渲染区域)
├── BandHandle × N        (透明 QWidget 覆盖层, 捕获鼠标事件)
├── LpfHandle             (透明 QWidget 覆盖层, 仅水平移动)
└── HpfHandle             (透明 QWidget 覆盖层, 仅水平移动)
```

`ViewEqualizer` 是 `AudioEQ` 内部的组合子控件，负责协调 Vulkan 渲染和 Band 交互覆盖层。`AudioEQ` 不直接管理这些细节。

### 7.2 焦点与 Z-Order

- 鼠标点击某个 BandHandle → 该 Band 获得焦点 → 圆圈绘制层级提升，视觉上置顶
- 点击空白区域（无 BandHandle 区域）→ 取消焦点
- 多个 Band 自由重叠，焦点 Band 始终渲染在最上层
- `EqualizerModel` 维护 `focusedBandIndex`

### 7.3 拖拽行为

1. 该 Band 获得焦点
2. 渲染单 band 曲线（比主曲线细）+ 下方半透明填充
3. 拖拽位置通过信号发给 AudioEQ → 触发 CurveEngine 异步重算 → VulkanRenderer 重绘
4. 帧率上限 60Hz（QTimer 节流）

### 7.4 信号定义

```cpp
class BandHandle : public QWidget {
    Q_OBJECT
signals:
    void bandDragged(int index, double newFreq, double newGain);
    void bandPressed(int index);       // 获得焦点
    void bandDeselected();
};
```

---

## 第八章：AudioEQ 公共 API

### 8.1 静态检测方法

```cpp
// 在构造 AudioEQ 之前调用, 检测 Vulkan 是否可用
static bool AudioEQ::isVulkanSupported();
```

### 8.2 类定义

```cpp
class AudioEQ : public QWidget {
    Q_OBJECT
public:
    explicit AudioEQ(QWidget* parent = nullptr); // 构造时自动 setBandCount(5)
    ~AudioEQ();

    // ===== Band 配置 =====
    int  bandCount() const;
    ResultCode setBandCount(int count);

    ResultCode addBand(const EQBand& band, int* outIndex = nullptr);
    ResultCode removeBand(int index);

    EQBand       bandParams(int index) const;
    ResultCode   setBandParams(int index, const EQBand& params);
    ResultCode   setBandFrequency(int index, double freqHz);
    ResultCode   setBandGain(int index, double gainDb);
    ResultCode   setBandQ(int index, double q);
    ResultCode   setBandType(int index, FilterType type);
    ResultCode   setBandAlgorithm(int index, FilterAlgorithm algo);
    ResultCode   setBandBypass(int index, bool bypass);

    // ===== 焦点 =====
    int  focusedBandIndex() const;
    void setFocusedBandIndex(int index);

    // ===== 采样率 =====
    SampleRate  sampleRate() const;
    ResultCode  setSampleRate(SampleRate rate);

    // ===== LPF / HPF =====
    ResultCode setLpfEnabled(bool enabled);
    bool       isLpfEnabled() const;
    ResultCode setLpfFrequency(double freqHz);
    ResultCode setLpfAlgorithm(FilterAlgorithm algo);

    ResultCode setHpfEnabled(bool enabled);
    bool       isHpfEnabled() const;
    ResultCode setHpfFrequency(double freqHz);
    ResultCode setHpfAlgorithm(FilterAlgorithm algo);

    // ===== Q 值范围 =====
    ResultCode setQRange(FilterType type, double min, double max);
    QPair<double,double> qRange(FilterType type) const;

    // ===== 增益范围 =====
    ResultCode setGainRange(double minDb, double maxDb);     // 默认 [-48, +48] dB
    double gainMin() const;
    double gainMax() const;

    // ===== 曲线外观 =====
    ResultCode setCurvePointCount(int count);                 // 默认 500
    int  curvePointCount() const;
    ResultCode setCurveColor(const QColor& color);
    ResultCode setBackgroundColor(const QColor& color);
    ResultCode setFrequencyVisibleRange(double minHz, double maxHz);

signals:
    void bandChanged(int index);
    void bandAdded(int index);
    void bandRemoved(int index);
    void bandSelected(int index);
    void bandDeselected();
    void focusedBandChanged(int index);
    void sampleRateChanged(SampleRate rate);
    void lpfChanged();
    void hpfChanged();
};
```

### 8.3 使用示例

```cpp
// 启动前检测
if (!AudioEQ::isVulkanSupported()) {
    qCritical("Vulkan not available");
    return;
}

auto* eq = new AudioEQ(parent);                     // 构造时默认 5 个 band
eq->setSampleRate(SampleRate::SR_48000);
eq->setBandParams(0, {.frequency=100.0, .gain=3.0, .q=1.0, .type=FilterType::Peak});

// 动态添加 band (自动分配 index)
EQBand newBand{};
newBand.frequency = 5000.0;
newBand.gain = -2.0;
newBand.q = 2.0;
newBand.type = FilterType::Peak;
int newIndex;
ResultCode rc = eq->addBand(newBand, &newIndex);
if (rc == ResultCode::OK) {
    qDebug() << "Band added at index" << newIndex;
}

// 删除 band
eq->removeBand(2);

// 连接信号
connect(eq, &AudioEQ::bandChanged, [](int idx) {
    // 外部响应 band 参数变化
});
```

---

## 第九章：CMake 构建配置

### 9.1 项目结构

```
AudioEQ_Visualized/
├── CMakeLists.txt
├── src/
│   ├── AudioEQ.h / .cpp
│   ├── ViewEqualizer.h / .cpp        # 内部渲染+交互组合控件
│   ├── EqualizerModel.h / .cpp
│   ├── CurveEngine.h / .cpp
│   ├── CoordinateMapper.h / .cpp
│   ├── BandHandle.h / .cpp
│   ├── filter/
│   │   ├── FilterAlgorithm.h
│   │   ├── ButterworthIIR.h / .cpp
│   │   └── FilterAlgorithmFactory.h / .cpp
│   └── vulkan/
│       ├── VulkanRenderer.h / .cpp
│       ├── VulkanContext.h / .cpp
│       ├── VulkanSwapchain.h / .cpp
│       ├── VulkanPipeline.h / .cpp
│       ├── VulkanBufferPool.h / .cpp
│       ├── VulkanFontAtlas.h / .cpp
│       ├── VulkanFrameSync.h / .cpp
│       └── shaders/
│           ├── grid.vert / .frag
│           ├── curve.vert / .frag
│           ├── fill.vert / .frag
│           └── glyph.vert / .frag
├── thirdparty/
│   ├── volk/
│   ├── stb/
│   └── glm/
├── resources/
│   └── fonts/
│       └── NotoSans-Regular.ttf          # 嵌入字体
├── tests/
│   ├── TestCurveEngine.cpp
│   ├── TestCoordinateMapper.cpp
│   └── TestButterworthIIR.cpp
└── examples/
    └── SimpleDemo/
        ├── CMakeLists.txt
        └── main.cpp
```

### 9.2 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(AudioEQ VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Vulkan REQUIRED)

add_subdirectory(thirdparty)   # volk, glm, stb

# 嵌入字体文件为 Qt 资源
qt_add_resources(AudioEQ_RESOURCES resources/resources.qrc)

add_library(AudioEQ SHARED
    src/AudioEQ.cpp
    src/ViewEqualizer.cpp
    src/EqualizerModel.cpp
    src/CurveEngine.cpp
    src/CoordinateMapper.cpp
    src/BandHandle.cpp
    src/filter/ButterworthIIR.cpp
    src/filter/FilterAlgorithmFactory.cpp
    src/vulkan/VulkanRenderer.cpp
    src/vulkan/VulkanContext.cpp
    src/vulkan/VulkanSwapchain.cpp
    src/vulkan/VulkanPipeline.cpp
    src/vulkan/VulkanBufferPool.cpp
    src/vulkan/VulkanFontAtlas.cpp
    src/vulkan/VulkanFrameSync.cpp
    ${AudioEQ_RESOURCES}
)

target_include_directories(AudioEQ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm
)

target_link_libraries(AudioEQ PUBLIC
    Qt6::Widgets
    Vulkan::Vulkan
)
```

### 9.3 Shader 编译策略

- **开发/Debug**：运行时通过 `glslang` 编译 GLSL → SPIR-V（灵活，方便迭代）
- **Release**：构建时通过 CMake 调用 `glslc` 预编译为 `.spv` 文件，嵌入 Qt 资源

```cmake
# Release 预编译:
find_program(GLSLC glslc)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/grid.vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/src/vulkan/shaders/grid.vert
            -o ${CMAKE_BINARY_DIR}/shaders/grid.vert.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/src/vulkan/shaders/grid.vert
)
```

---

## 第十章：实施任务分解

### Phase 1-A: 基础架构搭建
| # | 任务 | 依赖 |
|---|---|---|
| A1 | 搭建 CMake 项目骨架 (volk/glm/stb/Qt6/Vulkan) | — |
| A2 | 定义基础枚举和结构体 (`FilterType`, `FilterAlgorithm`, `SampleRate`, `EQBand`, `ShelfBand`) | — |
| A3 | 实现 `FilterAlgorithm` 抽象基类 + `FilterAlgorithmFactory` | A2 |
| A4 | 实现 `ButterworthIIR` (`evaluateAt`, `qRange`, 内部 biquad 计算) | A3 |
| A5 | 实现 `CoordinateMapper` (频率/增益 ↔ 视口坐标, 对数+线性映射) | — |
| A6 | 实现 `EqualizerModel` (含焦点/Z-Order、addBand、removeBand 等) | A2 |
| A7 | 实现 `CurveEngine` (后台线程、对数频点生成、总/单曲线、取消机制) | A2,A3,A5,A6 |

### Phase 1-B: Vulkan 渲染
| # | 任务 | 依赖 |
|---|---|---|
| B1 | 实现 `VulkanContext` + `AudioEQ::isVulkanSupported()` | A1 |
| B2 | 实现 `VulkanSwapchain` (MSAA + HDPI) | B1 |
| B3 | 实现 `VulkanPipeline` (Grid/Curve/Fill/Glyph 四条管线) | B1 |
| B4 | 实现 `VulkanBufferPool` (VBO/IBO/UBO) | B1 |
| B5 | 编写 4 组 Shader (GLSL, 含 Catmull-Rom spline 逻辑) | — |
| B6 | 实现 `VulkanFrameSync` (Fences/Semaphores) | B2 |
| B7 | 实现 `VulkanFontAtlas` (stb_truetype + 嵌入字体 + 纹理图集) | B1 |
| B8 | 实现 `VulkanRenderer` (整合所有模块, 暴露 draw 接口) | B2-B7 |
| B9 | 集成到 Qt Widget (`QVulkanInstance` + `QWidget::createWindowContainer`) | B8 |

### Phase 1-C: 交互与集成
| # | 任务 | 依赖 |
|---|---|---|
| C1 | 实现 `BandHandle` (鼠标交互/拖拽/焦点/60Hz节流) | — |
| C2 | 实现 LPF/HPF 椭圆形手柄 (水平仅拖拽/文字"LPF""HPF") | C1 |
| C3 | 实现 `AudioEQ` 集成类 (组合 Model + Engine + Renderer + Handles, Vulkan检测) | A7,B9,C1 |
| C4 | 实现 Band 焦点态交互 (单band曲线 + 填充 + 圆形高亮) | B9,C1 |
| C5 | 采样率切换 (切换Nyquist, 保持band位置不变, X轴刻度动态更新) | A7,B9 |
| C6 | 实现 `SimpleDemo` 示例程序 | C5 |

### Phase 1-D: 测试
| # | 任务 | 依赖 |
|---|---|---|
| D1 | `ButterworthIIR` 单元测试 (对比已知理论频响值) | A4 |
| D2 | `CoordinateMapper` 单元测试 (边界值/对数映射精度) | A5 |
| D3 | `CurveEngine` 多线程安全性测试 | A7 |
| D4 | 多分辨率/多DPI/Vulkan 设备兼容性测试 | C3 |

---

## 附录 A：原始代码迁移参考

### 需要保留的算法 (迁移到 `ButterworthIIR` 内部)

| 原始函数 | 新位置 | 说明 |
|---|---|---|
| `MakeCoeffPeakFilter` | `ButterworthIIR::makePeakFilter` | — |
| `MakeCoeffLowShelf` | `ButterworthIIR::makeLowShelf` | — |
| `MakeCoeffHighShelf` | `ButterworthIIR::makeHighShelf` | — |
| `MakeCoeffLowPass` | `ButterworthIIR::makeLowPass` | — |
| `MakeCoeffHighPass` | `ButterworthIIR::makeHighPass` | — |
| `FreqResponse` 中的 gain 公式 | `ButterworthIIR::freqResponseGain` | 内部实现 |
| `Actual_to_View` | `CoordinateMapper::freqToX` | 对数坐标映射 |

### 必须删除

- `CalculateCore::ins()` — 单例
- `TFZ_coefficients` — 不对外暴露
- `PublicVar` — 单例, 删除
- 所有 `MakeCoeff*List` — 不再预计算多采样率

---

## 附录 B：需求清单（待你确认并提供）

以下是我需要你提供的**第三方依赖和资源**：

### B.1 第三方库
| 库 | 用途 | License | 获取方式 |
|---|---|---|---|
| **volk** | Vulkan 函数加载器 | MIT | GitHub: zeux/volk |
| **GLM** | 数学库 (矩阵/向量) | MIT/Happy Bunny | GitHub: g-truc/glm |
| **stb_truetype** | TrueType 字体光栅化 | MIT/Public Domain | GitHub: nothings/stb |

### B.2 嵌入字体
需要一个**开源 TrueType 字体文件** (.ttf)，要求：
- 开源许可证 (SIL OFL / Apache 2.0), 可嵌入动态库
- 包含完整的 ASCII 可打印字符 + 数字
- 在 10-14px 小字号下清晰可读（坐标轴标签）
- 推荐候选：**Noto Sans**、**Inter**、**Roboto**

> 如果你没有偏好，建议用 **Noto Sans Regular** (Apache 2.0)，我可以直接下载。

### B.3 开发环境
- **Vulkan SDK** (v1.3+) — 需要 `glslc` 编译器和 `vulkan.h`
- **Qt 6.5+** (Widgets 模块)
- **CMake 3.20+**
- **C++17** 编译器 (MSVC 2022 / GCC 11+ / Clang 14+)

---

## 附录 C：所有已确认的设计决策

| # | 决策项 | 结论 |
|---|---|---|
| 1 | Vulkan 不可用降级 | 不降级, 启动前检测并报错。后续需要再考虑 |
| 2 | 多实例支持 | 允许, 每个实例独立 VulkanContext, 无单例 |
| 3 | Qt 版本 | Qt6 Only (需要 HDPI 能力) |
| 4 | Band 数量上限 | 无硬上限, 典型使用 ≤10, 暂不测试极端性能 |
| 5 | 默认频率分配 | 对数均匀分布 [20Hz ~ 20kHz] |
| 6 | Band 重叠行为 | 自由重叠, 焦点 Band 置顶渲染 |
| 7 | Band 动态增删 | 支持 `addBand()` / `removeBand()`, 但非主要使用场景 |
| 8 | LPF/HPF 形态 | 椭圆 + 显示 "LPF"/"HPF" 文字 |
| 9 | Band 颜色 | 所有 band 共用同一种颜色, 硬编码 |
| 10 | 字体方案 | 嵌入开源字体 (.ttf) |
| 11 | Q 值范围 | 可配置, 不同 FilterType 独立范围, SDK 提供接口 |
| 12 | 增益范围 | 默认 [-48, +48] dB, SDK 接口可修改 |
| 13 | 曲线采样点数 | 可配置, 默认 500, SDK 接口可修改 |
| 14 | Shader 编译 | Debug 运行时编译 / Release 预编译 SPIR-V |
| 15 | 线程模型 | CurveEngine 在后台线程运行, 不阻塞主线程 |
| 16 | GPU 选择 | 自动, 无需用户配置 |
| 17 | Vulkan Validation | Debug 构建默认开启 |
| 18 | Widget resize | 仅重映射坐标, 不重算曲线 |
| 19 | X 轴频率上限 | 始终显示到当前 Nyquist 频率 |
| 20 | 创建 band 方式 | 通过 `addBand(EQBand)` 接口, 非点击曲线空白处 |
| 21 | 默认 Band 数量 | 构造时自动 `setBandCount(5)` |
| 22 | 内部组件拆分 | AudioEQ 组合 `ViewEqualizer` (内含 VulkanRenderer + BandHandles) |
| 23 | API 返回值 | 统一使用 `ResultCode` 枚举 (OK / Failed / IndexOutOfRange / ...) |
| 24 | LPF/HPF 算法 | 使用 `FilterAlgorithm` 抽象, ShelfBand 含 `FilterAlgorithm` 字段 |
| 25 | Q 值范围修改 | 修改后自动 clamp 所有匹配类型的已有 band 的 Q 值 |
| 26 | LPF/HPF 默认频率 | LPF 默认 20kHz, HPF 默认 20Hz, gain=0 锁定 |
| 27 | 对数频点分布 | 解决线性分布的疏密不均问题 (低频稀疏/高频过密) |
