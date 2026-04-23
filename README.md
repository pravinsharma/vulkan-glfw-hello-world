# Vulkan Hello World

A simple Vulkan application that displays "Hello World!" text rendered using a TrueType font (FreeType) on a full-screen quad. Uses GLFW for window creation and Vulkan for rendering.

## Tech Stack

- **C++** (C++17)
- **CMake** (minimum 3.20)
- **Vulkan** graphics API
- **GLFW** window/input (preferred over SDL3 for being lightweight and minimal)
- **FreeType** font rasterization

## Project Structure

```
.
├── CMakeLists.txt
├── AGENTS.md          # Project guide for AI assistants
├── include/
│   ├── Window.hpp
│   ├── VulkanContext.hpp
│   └── FontRenderer.hpp
├── src/
│   ├── main.cpp
│   ├── Window.cpp
│   ├── VulkanContext.cpp
│   └── FontRenderer.cpp
├── shaders/
│   ├── text.vert       # GLSL vertex shader
│   └── text.frag       # GLSL fragment shader
└── fonts/
    └── Roboto-Regular.ttf (optional; falls back to system font)
```

## Build Requirements

### Environment Variables

- **VCPKG_ROOT**: Path to vcpkg installation (e.g., `C:\dev\vcpkg` on Windows). Used for GLFW and FreeType packages.
- **VULKAN_SDK**: Path to Vulkan SDK (e.g., `C:\VulkanSDK\1.4.341.1`). Provides Vulkan headers, libraries, and `glslc` shader compiler.

### Dependencies (via vcpkg)

- `glfw3`
- `freetype`

Vulkan SDK provides the Vulkan loader and headers.

### Operating System

Primarily developed for Windows. Linux/macOS support possible but may need path adjustments for environment variables and system font fallbacks.

## Build Instructions

```bash
# Configure (using vcpkg toolchain)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build

# Run (from build directory)
./VulkanHelloWorld.exe  # Windows
./VulkanHelloWorld      # Linux/macOS
```

**Note:** On Linux/macOS, replace `%VCPKG_ROOT%` with the appropriate path syntax for vcpkg if using it.

## Runtime Details

The executable expects to find:
- Compiled shaders in `shaders/` subdirectory relative to the executable
- Font in `fonts/Roboto-Regular.ttf`

If the font file is missing, the application will attempt to load a system font:
- **Windows**: Arial
- **Linux**: DejaVuSans
- **macOS**: Helvetica

## Code Architecture

- **Window**: Thin wrapper around GLFW window handling and Vulkan surface creation.
- **VulkanContext**: Encapsulates Vulkan instance, device, swapchain, render pass, graphics pipeline, command pool, synchronization objects, and a fullscreen quad vertex buffer. Provides helper methods for buffer/image creation, single-time command submission, and rendering.
- **FontRenderer**: Uses FreeType to load a TrueType font and render a text string into a single-channel (R8) Vulkan texture. Computes glyph placement for accurate kerning/advances. Returns a VkImageView for use with descriptor set.

## Important Implementation Details

- **Shader Integration**: Vertex shader passes position and UV; fragment shader samples the font texture (R8) and outputs orange color with alpha.
- **Descriptor Set**: One combined image sampler descriptor bound at set 0.
- **Command Recording**: One-time command buffer for texture upload; per-frame command buffers recorded each loop iteration.
- **Synchronization**: Uses one fence (in-flight) and two semaphores (image available, render finished).
- **Vertex Buffer**: Contains 6 vertices for two textured triangles covering NDC [-1,1] square.
- **Texture Format**: `VK_FORMAT_R8_UNORM` (single channel 8-bit) representing font alpha mask.
- **Font Fallback**: If the bundled Roboto-Regular.ttf is absent, tries common system font paths.

## Common Issues & Fixes

### Shaders not compiled
Make sure `glslc` from Vulkan SDK is available on PATH or `VULKAN_SDK` is set correctly. CMake will fail if not found.

### Linking errors with Vulkan/GLFW/FreeType
Ensure vcpkg toolchain path is correct and packages are installed:
```bash
vcpkg install glfw3 freetype
```

### Text appears invisible
Check that the generated text texture has non-zero alpha; font may not be loading correctly. The program logs font loading fallbacks to stdout/stderr.

### Validation layer errors
Debug build uses VK_LAYER_KHRONOS_validation; ensure Vulkan SDK validation layers are installed.

### Windows Smart App Control
If the application fails to run on Windows with no error message, **Smart App Control** may be blocking the unsigned executable. Disable Smart App Control in Windows Security settings:
- Open **Windows Security** → **App & browser control**
- Under **Smart App Control**, set to **Off** (or at least **Warn**)
- Re-run the application

Note: If using vcpkg, the executable depends on several DLLs (glfw3.dll, freetype.dll, etc.) which may also be flagged. The application should work once Smart App Control is disabled or the DLLs are properly installed.

## Extending the Project

- Add support for dynamic text updates by re-creating texture each frame.
- Implement signed distance fields (SDF) for scalable high-quality text.
- Add push constants or uniform buffers for dynamic color/transforms.
- Handle window resize by recreating swapchain and related resources.
- Integrate ImGui for UI overlay.

## Quick Reference

### CMake Key Variables
- `CMAKE_TOOLCHAIN_FILE`: Must point to `$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`.
- `VULKAN_SDK`: Used by CMake to locate `glslc`.

### Shader Compilation
GLSL files are compiled to SPIR-V during build and placed into `compiled_shaders/` and then copied to executable's `shaders/` directory.

### Important Resource Paths
- Shaders relative to executable: `./shaders/text.vert.spv`, `./shaders/text.frag.spv`
- Font relative to executable: `./fonts/Roboto-Regular.ttf`

## License

(Add license information here if applicable)