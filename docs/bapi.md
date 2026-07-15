# BridgeEngine API 文档

## 契约来源

`include/BridgeEngine.h` 是唯一的公开 API、类型和 ABI 契约来源。本文不重复维护会漂移的声明副本；调用方应
直接包含 `<BridgeEngine.h>`，并以该头文件中的函数签名和枚举值为准。

当前聚合头对 master 文档化 BAPI 保持重新编译兼容。旧拆分头路径（例如 `audio/audio.h`）、事件的平台内部字段和
旧二进制 ABI 不属于兼容承诺；旧项目应只包含 `<BridgeEngine.h>` 并重新链接当前库。

## 生命周期

先调用 `bapi_engine_init(title, width, height)`。成功返回 `0`，失败返回非零。初始化成功后，应用可轮询事件、
绘制并使用媒体子系统。`bapi_engine_quit()` 可重复调用；第一次调用按视频、音频、纹理、文字、TTF、renderer、
window、平台的顺序停止运行时。

退出会释放尚未显式释放的引擎资源。所有 `bapi_texture_t`、`bapi_sound_t`、`bapi_video_t`、以及从
`bapi_engine_get_window()` 和 `bapi_engine_get_renderer()` 获得的借用句柄，都会在退出后失效。不要在退出后继续
使用或释放它们。

`bapi_engine_get_window()` 和 `bapi_engine_get_renderer()` 只在运行时返回借用句柄；初始化前和退出后返回 `NULL`。
调用方不拥有它们，也没有对应的销毁函数。

初始化前和退出后，`bapi_poll_event()` 返回 `0`，若传入事件则写入 `BAPI_EVENT_UNKNOWN`。基础绘制、延迟和计时
API 安全返回且不调用后端；事件查询函数接受 `NULL` 并返回零值或 `BAPI_EVENT_UNKNOWN`。

## 类型与返回约定

不透明资源类型为 `bapi_window_t`、`bapi_renderer_t`、`bapi_texture_t`、`bapi_sound_t`、`bapi_video_t`、
`bapi_scene_t`、`bapi_scene_manager_t`、`bapi_level_t` 和 `bapi_level_manager_t`。颜色、矩形、事件、摄像机、向量、
圆、按钮、日志和场景/关卡回调结构的精确布局均定义在公开头文件中。

创建或加载函数以 `NULL` 表示失败。状态或操作函数通常以 `0` 表示成功、非零表示失败；没有返回值的函数为安全的
命令式操作。输出参数可为 `NULL` 的具体情况以头文件声明及对应实现为准。

## 几何查询

`bapi_rect_intersects()` 与 `bapi_collision_aabb()` 仅在两个矩形存在正面积重叠时返回非零；边或角刚好接触时返回 `0`。
`bapi_collision_aabb()` 是 `bapi_rect_intersects()` 的别名，二者结果始终一致。

`bapi_circle_intersects_circle()` 与 `bapi_circle_intersects_rect()` 在圆形与对象接触或重叠时返回非零。所有几何查询只接受有限坐标、非负圆半径和非负矩形宽高；违反这些前置条件时，结果不作承诺。

## 平台媒体能力

桌面 SDL3 支持既有的 WAV 音频和 FFmpeg 视频路径。XJ380 保留窗口、基础绘制和内置文字，但不支持音频或视频：
`bapi_audio_init()` 和 `bapi_video_init()` 返回非零，`bapi_sound_load()` 和 `bapi_video_load()` 返回 `NULL`。首次失败会
通过平台 warning 记录原因；其余没有返回值的媒体控制与清理函数可安全调用。调用方必须检查这些既有返回值；公开 BAPI
不提供 capability 查询函数。

## API 分类

- **引擎、事件与渲染：** 引擎、事件、渲染控制、绘图、延迟、计时和颜色函数。
- **文字与鼠标绘图：** 文字测量与绘制，以及鼠标绘图函数。
- **媒体：** 纹理、音频、声音和视频函数。纹理缓存通过
  `bapi_texture_from_file` 取得；调用 `bapi_texture_cache_clear` 仅移除缓存引用，不销毁仍由调用方持有的纹理。
- **UI、输入与摄像机：** 按钮、输入状态、鼠标位置和摄像机函数。
- **场景与关卡：** 场景、场景管理器、关卡和关卡管理器函数。
- **数学、日志与版本：** 向量、矩形、圆、碰撞、插值、日志和版本函数，包括 `bapi_collision_aabb`、`bapi_clamp`、
	`bapi_lerp`、`bapi_deg_to_rad` 和 `bapi_rad_to_deg`。
- **XML 持久化：** `bapi_scene_manager_load_from_xml`、`bapi_level_manager_load_from_xml`、
  `bapi_scene_manager_save_to_xml`、`bapi_level_manager_save_to_xml`。

## 资源所有权

调用方可在运行时使用 `bapi_texture_destroy`、`bapi_sound_free` 和 `bapi_video_free` 提前释放相应资源。若不释放，
引擎退出会统一回收。场景、关卡及其 manager 的创建、添加和销毁语义以 `BridgeEngine.h` 和当前实现为准；本版本未
为 manager 添加额外的所有权保证。

`bridgeengine.resource_shutdown` 回归测试使用真实 MP4 的 FFmpeg 容器/流初始化路径，并使用测试平台的图像和 WAV
替身验证受追踪资源退出释放。它不宣称覆盖 PNG/WAV 加载或视频帧解码。
