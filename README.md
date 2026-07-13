# BridgeEngine

## 鹊桥引擎

BridgeEngine 是一个基于 SDL3 + FFmpeg 的跨平台 2D 图形引擎，提供简洁的 C API，支持 Windows (PC) 和 XJ380 嵌入式平台。

---

## 功能特性

| 模块 | 说明 |
|------|------|
| **渲染** | 点、线、矩形、圆、三角形、多边形等基本图元绘制 |
| **文字** | 基于 SDL3_ttf，支持 TTF 字体、UTF-8 编码、中英文混排 |
| **图像** | 加载和绘制 PNG/JPEG 等格式纹理 |
| **音频** | WAV 音效加载播放，支持循环和音量控制 |
| **视频** | 基于 FFmpeg 解码，支持 MP4/AVI/MKV 播放，自适应缩放 |
| **场景管理** | 多场景创建/切换，生命周期回调 |
| **关卡管理** | 关卡加载/卸载，序列管理，上下级切换 |
| **XML 配置** | 场景和关卡的 XML 文件加载与保存 |
| **输入系统** | 键盘状态查询（按下/按住/释放），鼠标位置获取 |
| **摄像机** | 2D 摄像机，支持平移、缩放、世界坐标转换 |
| **碰撞检测** | 圆与矩形相交检测 |
| **数学工具** | 向量运算、插值、钳制等工具函数 |

---

## 依赖

- SDL3 + SDL3_image + SDL3_ttf
- FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample)
- OpenGL (Linux) 或 opengl32 (Windows)
- CMake >= 3.20 或 GNU Make
- C11 编译器 (GCC/Clang/MSVC)

---

## 快速开始

### 安装依赖

```bash
# Windows (vcpkg)
vcpkg install sdl3 sdl3-image sdl3-ttf ffmpeg

# macOS (Homebrew)
brew install sdl3 sdl3_image sdl3_ttf ffmpeg

# Linux (apt)
sudo apt install libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev \
                 libavcodec-dev libavformat-dev libavutil-dev \
                 libswscale-dev libswresample-dev
```

### 构建 (CMake)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 构建 (Makefile)

```bash
make          # 构建动态库 + 示例程序
make lib      # 仅构建动态库 (libbridgeengine.dll / .so)
make staticlib # 仅构建静态库 (libbridgeengine.a)
```

### 构建 XJ380 嵌入式目标

```bash
make xj380_staticlib  # XJ380 静态库
make xj380_epf        # XJ380 EPF 可执行文件
```

---

## 最简单的例子

```c
#include <BridgeEngine.h>

int main(void) {
    bapi_engine_init("Hello BridgeEngine", 800, 600);

    bapi_texture_t text = bapi_render_text("Hello World!",
        bapi_color(255, 255, 255, 255));

    bool running = true;
    bapi_event_t event;
    while (running) {
        while (bapi_poll_event(&event)) {
            if (bapi_event_get_type(&event) == BAPI_EVENT_QUIT)
                running = false;
        }
        bapi_render_clear();
        bapi_draw_text(text, 50, 50, 300, 40);
        bapi_render_present();
    }

    bapi_destroy_text(text);
    bapi_text_cleanup();
    bapi_engine_quit();
    return 0;
}
```

---

## 项目结构

```
BridgeEngine/
├── include/            # 公共头文件
│   ├── BridgeEngine.h  # 统一入口
│   ├── bapi.h          # API 声明
│   ├── bapi_types.h    # 类型定义
│   ├── audio/          # 音频 API
│   ├── video/          # 视频 API
│   ├── text/           # 文字 API
│   ├── render/         # 渲染 API
│   ├── input/          # 输入 API
│   ├── camera/         # 摄像机 API
│   ├── math/           # 数学 API
│   ├── texture/        # 纹理 API
│   ├── scene/          # 场景 API
│   ├── level/          # 关卡 API
│   ├── button/         # 按钮 API
│   └── ...
├── engine/             # 引擎实现
│   ├── platform/       # 平台层 (SDL3 / XJ380)
│   ├── render/         # 渲染实现
│   ├── video/          # 视频实现 (FFmpeg)
│   ├── audio/          # 音频实现
│   ├── text/           # 文字实现
│   └── ...
├── cmake/              # CMake 模块
│   └── FindFFmpeg.cmake
├── main.c              # 功能演示程序
├── CMakeLists.txt
├── Makefile
└── BAPI_README.md      # 详细 API 文档
```

---

## API 文档

完整的 API 文档请查阅 [BAPI_README.md](BAPI_README.md)。

---

## 平台支持

| 平台 | 后端 | 状态 |
|------|------|------|
| Windows (x64) | SDL3 + OpenGL | 支持 |
| Linux | SDL3 + OpenGL | 支持 |
| macOS | SDL3 + OpenGL | 支持 |
| XJ380 嵌入式 | 原生 XAPI | 支持 (交叉编译) |

---

## License

[MIT](LICENSE)
