# BridgeEngine 内部架构

BridgeEngine 保持公开 BAPI 稳定，并将平台相关实现隔离在 `src/internal/platform/platform.h` 之后。

## 平台层

平台适配器按能力分组：

- `core`：初始化、关闭、计时和日志。
- `window`：窗口、事件和鼠标状态。
- `renderer`：渲染命令。
- `texture`：纹理与图像加载。
- `text`：字体与文字渲染。
- `audio`：音频设备、流和 WAV 加载。
- `sync`：互斥锁。

`plat_get()` 是唯一的平台内部入口。引擎 module 直接使用相应能力组，`plat_interface_t` 不再提供扁平的兼容函数指针。

`plat_interface_t.capabilities` 是内部能力位图，`plat_supports()` 用于查询。SDL3 声明音频和视频能力；XJ380 两者均不声明。运行时始终请求窗口/图形初始化，仅在声明音频能力时追加 `PLAT_INIT_AUDIO`。XJ380 的音频 vtable 保持为 `NULL` 槽位，渲染和内置文字能力不受影响。

不支持的媒体通过既有 BAPI 返回值显式失败：音频/视频初始化返回非零，声音/视频加载返回 `NULL`；每种媒体首次失败写入一条平台 warning。没有返回值的媒体控制和清理函数仍为安全无操作。这是内部 adapter 契约，不新增公开 capability 查询接口。

## 文字后端

桌面 SDL3 后端通过 SDL3_ttf 和 BAPI 文字函数加载、绘制字体。运行时优先从 `assets/text/font.ttf` 加载字体；从源码树直接运行示例时，会回退到 `examples/assets/text/font.ttf`，最后兼容旧项目的 `text/font.ttf`。

XJ380 后端使用 XAPI 内置文字绘制。当前 XJ380 头文件中的 `WSTR` 实际为 `char *`，因此 BAPI 将文字按字节串传给 XAPI，不做 UTF-16 或 `wchar_t` 转换。

XJ380 尚未实现字体文件加载；若 XAPI 要求非 UTF-8 编码，应在独立适配器中处理编码兼容性。

## 引擎生命周期

单实例运行时状态由 `src/core/render_context.c` 管理。该 module 负责平台、窗口、renderer 和 TTF 的启动、失败回滚与关闭顺序。

关闭时先释放视频、音频、纹理和文字资源，再关闭 TTF、renderer、窗口和平台。这不会增加多窗口或多引擎支持，但能避免生命周期状态分散在多个文件级全局变量中。

## 事件

BAPI 自己定义事件枚举和事件结构。平台适配器产生 `plat_event_t`，`src/core/init.c` 将其转换为 `bapi_event_t` 后再由 `bapi_poll_event()` 返回。

## 后续 2.5D

不要在 `src/draw.c` 中加入 2.5D 或世界坐标渲染；基础渲染 module 应保持为轻量的 2D 绘制层。

未来的 2.5D 工作应位于独立 module，例如 `world25d` 或 `render25d`，并通过窄接口连接到现有 renderer 能力。
