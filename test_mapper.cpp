#include <iostream>
#include <cmath>
#include "src/CoordinateMapper.h"

int main() {
    CoordinateMapper mapper;
    mapper.setViewport(QRectF(0, 0, 800, 400));
    mapper.setSampleRate(SampleRate::SR_44100);
    mapper.setGainRange(-48.0, 48.0);

    double x_left   = mapper.freqToX(20.0);
    double x_right  = mapper.freqToX(22050.0);
    double y_top    = mapper.gainToY(48.0);
    double y_bottom = mapper.gainToY(-48.0);
    double y_mid    = mapper.gainToY(0.0);

    std::cout << "freqToX(20) = " << x_left   << " (expect ~0)"    << std::endl;
    std::cout << "freqToX(22050) = " << x_right << " (expect ~800)" << std::endl;
    std::cout << "gainToY(48) = " << y_top    << " (expect ~0)"    << std::endl;
    std::cout << "gainToY(-48) = " << y_bottom << " (expect ~400)" << std::endl;
    std::cout << "gainToY(0) = " << y_mid    << " (expect ~200)"  << std::endl;

    double roundtrip = mapper.xToFreq(mapper.freqToX(1000.0));
    std::cout << "Round-trip 1000Hz: " << roundtrip << " (expect ~1000)" << std::endl;

    int errors = 0;
    if (std::abs(x_left) > 0.001)     { std::cerr << "FAIL: freqToX(20)" << std::endl; errors++; }
    if (std::abs(x_right - 800) > 0.001) { std::cerr << "FAIL: freqToX(22050)" << std::endl; errors++; }
    if (std::abs(y_top) > 0.001)      { std::cerr << "FAIL: gainToY(48)" << std::endl; errors++; }
    if (std::abs(y_bottom - 400) > 0.001) { std::cerr << "FAIL: gainToY(-48)" << std::endl; errors++; }
    if (std::abs(roundtrip - 1000.0) > 0.01) { std::cerr << "FAIL: round-trip" << std::endl; errors++; }

    if (errors == 0) std::cout << "ALL TESTS PASSED" << std::endl;
    return errors;
}
