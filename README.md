# BridgeEngine

## 鹊桥引擎

BridgeEngine 是一个跨平台 2D 图形引擎，提供简洁的 C API。桌面端使用 SDL3 + FFmpeg，XJ380 端使用原生 XAPI 后端。

---

## 功能特性

| 模块 | 说明 |
|------|------|
| **渲染** | 点、线、矩形、圆、三角形、多边形等基本图元绘制 |
| **文字** | 桌面端基于 SDL3_ttf，支持 TTF 字体 |
| **图像** | 加载和绘制 PNG/JPEG 等格式纹理 |
| **音频** | 桌面端 WAV 音效加载播放，支持循环和音量控制 |
| **视频** | 桌面端基于 FFmpeg 解码，支持 MP4/AVI/MKV 播放 |
| **场景管理** | 多场景创建/切换，生命周期回调 |
| **关卡管理** | 关卡加载/卸载，序列管理，上下级切换 |
| **XML 配置** | 场景和关卡的 XML 文件加载与保存 |
| **输入系统** | 键盘状态查询、鼠标位置获取 |
| **摄像机** | 2D 摄像机，支持平移、缩放、世界坐标转换 |
| **数学工具** | 向量运算、插值、钳制等工具函数 |

---

## 依赖

- SDL3 + SDL3_image + SDL3_ttf
- FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample)
- OpenGL (Linux) 或 opengl32 (Windows)
- CMake >= 3.20 或 GNU Make
- C11 编译器 (GCC/Clang/MSVC)

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

---

## 构建

### CMake

```bash
cmake -S . -B build -DBRIDGEENGINE_REQUIRE_DESKTOP_DEPS=ON
cmake --build build
```

CMake 默认生成 `build/compile_commands.json`，并构建：

- `bridgeengine_shared`
- `bridgeengine_static`
- `main`
- `text_example`
- `log_test`

如果桌面依赖不完整且没有打开 `BRIDGEENGINE_REQUIRE_DESKTOP_DEPS`，CMake 仍会生成 `compile_commands.json`，但桌面库和示例会从默认构建中排除。

### Makefile

```bash
make            # 构建动态库 + 示例程序
make lib        # 仅构建动态库
make staticlib  # 仅构建静态库
```

### XJ380

XJ380 SDK 是本地依赖，不进入 Git。需要在仓库根目录提供：

```text
XJ380_XACT_2026v4_xj380/depend/include
XJ380_XACT_2026v4_xj380/depend/obj-gui
```

构建目标：

```bash
cmake --build build --target xj380_staticlib
cmake --build build --target xj380_epf

# 或使用 Makefile
make xj380_staticlib
make xj380_epf
```

---

## 后端说明

桌面 SDL3 后端使用 SDL3_ttf 加载字体文件，并通过 FFmpeg 实现视频。

XJ380 后端使用 XAPI 内置文本绘制，当前不加载 TTF 文件。XJ380 用户态头文件里的 `WSTR` 实际是 `char *`，BridgeEngine 不把它当作 UTF-16 或 `wchar_t`。XJ380 音频和视频后端目前尚未实现。

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

```text
BridgeEngine/
├── include/            # 公共头文件
├── engine/             # 引擎实现
│   ├── platform/       # 平台层 (SDL3 / XJ380)
│   ├── render/         # 渲染实现
│   ├── video/          # 视频实现
│   ├── audio/          # 音频实现
│   ├── text/           # 文字实现
│   └── ...
├── cmake/              # CMake 模块
├── main.c              # 功能演示程序
├── CMakeLists.txt
├── Makefile
└── BAPI_README.md      # 详细 API 文档
```

完整 API 文档请查阅 [BAPI_README.md](BAPI_README.md)。

---

## License

[MIT](LICENSE)
