#pragma once

#if defined(AUDIOEQ_LIBRARY)
#  define AUDIOEQ_EXPORT Q_DECL_EXPORT
#else
#  define AUDIOEQ_EXPORT Q_DECL_IMPORT
#endif

enum class FilterType {
    Peak,
    LowShelf,
    HighShelf,
    LowPass,
    HighPass,
    BandPass,
};

enum class FilterAlgorithmType {
    ButterworthIIR,
};

enum class SampleRate : int {
    SR_44100  = 44100,
    SR_48000  = 48000,
    SR_96000  = 96000,
    SR_192000 = 192000,
};

enum class ResultCode {
    OK = 0,
    Failed,
    IndexOutOfRange,
    IndexConflict,
    InvalidParameter,
    VulkanNotAvailable,
};

struct EQBand {
    int     index       = -1;
    double  frequency   = 1000.0;
    double  gain        = 0.0;
    double  q           = 1.0;
    FilterType      type      = FilterType::Peak;
    FilterAlgorithmType algorithm = FilterAlgorithmType::ButterworthIIR;
    bool    bypass      = false;
};

struct ShelfBand {
    double          frequency   = 20000.0;
    bool            enabled     = false;
    FilterAlgorithmType algorithm   = FilterAlgorithmType::ButterworthIIR;
};
