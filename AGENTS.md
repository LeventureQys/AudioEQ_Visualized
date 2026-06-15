# AudioEQ — Agent Instructions

## Project

Qt6 + Vulkan audio EQ visualization SDK, delivered as a shared library (`AudioEQ.dll`/`.so`).
- **Language**: C++17
- **Build**: CMake 3.20+, MSVC 2022 on Windows
- **Qt**: 6.8.3, installed at `D:/Devtools/Qt/6.8.3/msvc2022_64`
- **Vulkan**: loaded via **volk** (not system SDK); headers in `thirdparty/code/vulkan-headers/include/`
- **Tests**: googletest (submodule at `thirdparty/code/googletest/`)
- **Shaders**: GLSL precompiled to `.spv` files in `src/vulkan/shaders/spv/`; no shader compiler needed at build time

## Build Commands

```powershell
# Configure (from repo root, in Developer PowerShell / VS prompt)
cmake -B build -DCMAKE_PREFIX_PATH="D:/Devtools/Qt/6.8.3/msvc2022_64"

# Build
cmake --build build --config Debug

# Run tests
ctest --test-dir build -C Debug

# Run SimpleDemo (requires build first)
.\build\examples\SimpleDemo\Debug\SimpleDemo.exe
```

## Architecture

Entry point: `src/AudioEQ.h` — the only public class (`QWidget` subclass).

| Layer | Key Files | Role |
|-------|-----------|------|
| Public API | `src/AudioEQ.h`, `src/AudioEQTypes.h` | All enums, structs, ResultCode |
| Model | `src/EqualizerModel.h` | Band state, sample rate, focus, ranges |
| DSP | `src/CurveEngine.h` | Background thread: async frequency response computation |
| Filter | `src/filter/FilterAlgorithm.h`, `src/filter/ButterworthIIR.h` | Stateless filter algorithms |
| Qt UI | `src/ViewEqualizer.h`, `src/BandHandle.h`, `src/LpfHandle.h`, `src/HpfHandle.h` | Widget layer for mouse interaction |
| Vulkan | `src/vulkan/VulkanRenderer.h`, `src/vulkan/VulkanContext.h`, `src/vulkan/VulkanPipeline.h`, `src/vulkan/VulkanSwapchain.h`, `src/vulkan/VulkanBufferPool.h`, `src/vulkan/VulkanFontAtlas.h`, `src/vulkan/VulkanFrameSync.h`, `src/vulkan/VulkanQtIntegration.h` | Rendering pipeline |
| Coordinate | `src/CoordinateMapper.h` | Pixel ↔ frequency/gain mapping |
| Example | `examples/SimpleDemo/main.cpp` | Standalone executable |

## Key Conventions

- **Export macro**: All public symbols use `AUDIOEQ_EXPORT` (defined via CMake: `dllexport` for library build, `dllimport` for consumers)
- **Vulkan includes**: Always `#include "volk.h"` (never `<vulkan/vulkan.h>` directly); call `volkInitialize()` at startup, then `volkLoadInstance()` after Qt creates the instance
- **Math**: GLM only in Vulkan layer; public API uses Qt types (QPointF, QRectF). Convert at boundary.
- **Shaders**: Precompiled `.spv` files in `src/vulkan/shaders/spv/`; CMake macro `embed_spirv()` converts to C arrays at build time
- **Font**: `thirdparty/font/Noto_Sans/static/NotoSans-Regular.ttf` embedded into binary via `embed_font()` CMake macro; no Qt resource system, no disk path
- **No comments**: Codebase convention — do not add code comments unless asked
- **Tests**: 18 gtest cases across 3 test executables: `TestButterworthIIR`, `TestCoordinateMapper`, `TestCurveEngine`
- **git tag**: `v1.0.0` not yet tagged (user confirmation pending)

## Current Status

Stages 1–8 are complete and tested. Stages 9, 10, 11 are pending:
- **Stage9**: SimpleDemo full UI panel (band inspector, table, global controls)
- **Stage10**: Margins and axis label layout changes
- **Stage11**: Axis visual polish (color scheme, grid steps, text layout, defaults)

Design docs live in `Document/v1.0.0/Stage{N}/设计文档.md`. Read the relevant one before working.

## opencode.json Caveat

The agent prompts in `opencode.json` still reference "TactileSense-old" (a different Python project). When using subagents via `@` mentions, the system prompt may be misaligned with this C++ codebase. Prefer direct tool use over subagent dispatch unless the task clearly maps.
