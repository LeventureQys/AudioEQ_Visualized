#pragma once

// ══════════════════════════════════════════════
// AudioEQ Shared Types — v1.0.0
// ══════════════════════════════════════════════

// ─── API Return Codes ─────────────────────────
enum class ResultCode {
    OK = 0,
    Failed,
    InvalidParameter,
    IndexOutOfRange,
    VulkanNotAvailable,
};

// ─── Filter Type ──────────────────────────────
enum class FilterType {
    Peak,           // Parametric peak (bell)
    LowShelf,       // Low shelf
    HighShelf,      // High shelf
    LowPass,        // Low pass (used by LPF)
    HighPass,       // High pass (used by HPF)
};

// ─── Filter Algorithm Type ────────────────────
enum class FilterAlgorithmType {
    ButterworthIIR,
};

// ─── Sample Rate ──────────────────────────────
enum class SampleRate : int {
    SR_44100  = 44100,
    SR_48000  = 48000,
    SR_96000  = 96000,
    SR_192000 = 192000,
};

// ─── EQ Band Parameters ───────────────────────
struct EQBand {
    double              freqHz   = 1000.0;
    double              gainDb   = 0.0;
    double              q        = 1.0;
    FilterType          type     = FilterType::Peak;
    FilterAlgorithmType algo     = FilterAlgorithmType::ButterworthIIR;
    bool                bypass   = false;
};

// ─── Shelf Band (LPF / HPF) Parameters ────────
struct ShelfBand {
    double              freqHz  = 20000.0;
    FilterAlgorithmType algo    = FilterAlgorithmType::ButterworthIIR;
    bool                enabled = false;
    bool                bypass  = false;
};
