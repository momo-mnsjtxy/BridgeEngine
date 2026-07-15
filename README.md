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
- CMake >= 3.20
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
cmake --preset default
cmake --build --preset default
```

预设使用 Ninja，并将默认构建目录固定为 `build/`。若此前用其他生成器配置过该目录，先删除 `build/CMakeCache.txt` 和 `build/CMakeFiles/` 再重新配置。

CMake 默认生成 `build/compile_commands.json`，并构建：

- `bridgeengine_shared`
- `bridgeengine_static`
- `bridgeengine_desktop_example`
- `bridgeengine_text_example`
- `bridgeengine_log_example`

请求桌面库或示例时，CMake 会在配置阶段检查 SDL3、SDL3_image、SDL3_ttf 和 FFmpeg；依赖缺失会直接失败。

### XJ380

XJ380 SDK 是本地依赖，不进入 Git。CMake 会按以下顺序自动查找可用 SDK：

1. `XJ380_XACT_2026v4_xj380/depend`
2. `XJ380_XACT_2026v4_linux/depend`
3. `XJ380_XACT_2026v4_Windows/depend`

每个候选目录都应包含：

```text
XJ380_XACT_2026v4_xj380/depend/include
XJ380_XACT_2026v4_xj380/depend/obj-gui
```

构建目标：

```bash
cmake --preset xj380
cmake --build --preset xj380 --target xj380_staticlib
cmake --build --preset xj380 --target xj380_package
cmake --build --preset xj380 --target xj380_epf
```

若 SDK 位于其他位置，可在配置时指定：

```bash
cmake --preset xj380 -DXJ380_SDK=/实际路径/depend
```

外部 XJ380 项目应先构建 `xj380_package`，再将 `build/xj380/package/include/` 和
`build/xj380/package/lib/` 复制到项目中，或作为 xxcc 的头文件和库搜索路径。包内提供
`BridgeEngine.h`、生成的 `bridgeengine_version.h` 与 `libbridgeengine.a`；项目仍需自行提供 XJ380 SDK 的
系统头、链接脚本和运行时对象。

---

## 后端说明

桌面 SDL3 后端使用 SDL3_ttf 加载字体文件，并通过 FFmpeg 实现视频。

XJ380 后端使用 XAPI 内置文本绘制，当前不加载 TTF 文件。XJ380 用户态头文件里的 `WSTR` 实际是 `char *`，BridgeEngine 不把它当作 UTF-16 或 `wchar_t`。XJ380 不支持音频和视频：`bapi_audio_init()` 与 `bapi_video_init()` 返回非零，声音和视频加载返回 `NULL`，并通过平台 warning 说明原因。调用方应检查这些既有返回值；不会新增媒体 capability 查询 BAPI。

---

## 最简单的例子

```c
#include <BridgeEngine.h>

int main(void)
{
    if (bapi_engine_init("Hello BridgeEngine", 800, 600) != 0) return 1;

    bapi_event_t event;
    int running = 1;
    while (running) {
        while (bapi_poll_event(&event)) {
            if (bapi_event_get_type(&event) == BAPI_EVENT_QUIT) running = 0;
        }
        bapi_render_clear();
        bapi_draw_circle(400, 300, 80, bapi_color(255, 255, 255, 255));
        bapi_render_present();
    }

    bapi_engine_quit();
    return 0;
}
```

`bapi_engine_quit()` 会先释放引擎仍追踪的纹理、音频和视频资源，再销毁 renderer、窗口和平台。退出后所有
资源句柄及借用的 window/renderer 句柄均失效，不应再使用或释放。

---

## 项目结构

```text
BridgeEngine/
├── include/            # 唯一公共头文件 BridgeEngine.h
├── src/                # 引擎实现
│   ├── core/           # 初始化、渲染上下文与版本
│   ├── internal/       # 平台及其他私有实现头
│   ├── platform/       # 平台层 (SDL3 / XJ380)
│   └── ...
├── examples/           # 桌面、文字、日志和 XJ380 示例及资源
├── tests/              # 回归测试
├── docs/               # 架构与 API 文档
├── cmake/              # CMake 模块
├── CMakeLists.txt
└── README.md
```

完整 API 文档请查阅 [docs/bapi.md](docs/bapi.md)。

## 源码兼容性

v2 的公开接口仅为 `BridgeEngine.h`。旧项目若仍包含 `audio/audio.h`、`render/draw.h` 等拆分头路径，可额外将包内的 `include/legacy/` 加入头文件搜索路径；其中的兼容头会转发到当前聚合头。
这不包含 `bapi_internal.h`、`scene/scene_internal.h`、`bapi_event_t` 的旧平台内部字段、`bapi_texture_internal` 的旧内部字段，或已编译二进制的 ABI 兼容性。

---

## License

[MIT](LICENSE)
