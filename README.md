# Aura Edit - Real-time Face Retouching App

## Architecture
- **Backend:** C++20 with Qt 6.4
- **Graphics API:** OpenGL 3.3 Core Profile (GLSL)
- **Face Tracking:** MediaPipe Face Mesh C++ API Wrapper
- **Frontend UI:** QWebEngineView + QWebChannel (HTML5, TailwindCSS)

## Features
- **Zero-Copy Architecture:** Loads QImage directly to GPU via QOpenGLTexture.
- **GPU Shaders:** All pixel manipulation (Bilateral Blur, Warp) is run natively on the GPU without CPU fallback.
- **IPC UI Bridge:** Sliders in the Chromium layer directly modify GLSL uniforms via `AppController` WebChannel bridge.

## Build Requirements
- CMake 3.16+
- Qt 6.4+ (WebEngine, OpenGLWidgets, WebChannel)
- Supported C++20 Compiler (GCC 10+, Clang 10+, MSVC 19.29+)
- OpenGL 3.3+ capable hardware

## Compilation
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Running
```bash
./AuraEdit
```
