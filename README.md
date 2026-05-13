# Aura Edit - Real-time Face Retouching App

## Architecture
- **Backend:** C++20 with Qt 6.4
- **Graphics API:** OpenGL 3.3 Core Profile (GLSL)
- **Face Tracking:** MediaPipe Face Mesh C++ API Wrapper
- **Frontend UI:** QWebEngineView + QWebChannel (HTML5, TailwindCSS)

## Features
- **UI Integration:** Full Tailwind CSS mockup implemented. Transparent Chromium layer stacked on top of C++ OpenGL.
- **Zero-Copy Architecture:** Loads QImage directly to GPU via QOpenGLTexture.
- **Displacement Map Rendering:** FBO (Framebuffer Object) used to write landmark transformations into a displacement texture map in pass 1.
- **GPU Shaders:** All pixel manipulation (Bilateral Blur, Warp via Displacement map lookup) is run natively on the GPU in pass 2.
- **IPC UI Bridge:** Sliders in the Chromium layer modify GLSL uniforms via `AppController` WebChannel bridge.

## MediaPipe Integration
To build MediaPipe properly for your host environment:
1. Ensure Bazel is installed.
2. Checkout the Google MediaPipe repository and point `WORKSPACE` correctly.
3. Build the `mediapipe_bridge` static library.
4. Uncomment the real implementation in `MediaPipeBridge.cpp`.

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
