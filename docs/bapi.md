# BridgeEngine API 文档

`include/BridgeEngine.h` 是唯一公开 API 和类型契约来源。本文不重复维护声明副本，而是描述每个
公开函数的行为语义、返回值、前置条件和资源所有权。签名以头文件为准，本文件描述的语义以当前
实现为准。发现不一致时以头文件 + 实现为准并回报。

## 全局约定

### 生命周期

1. 先调用 `bapi_engine_init(title, width, height)`。成功返回 `0`，失败返回非零。
2. 初始化成功后，应用可轮询事件、绘制、使用媒体和输入子系统。
3. 退出调用 `bapi_engine_quit()`。它可重复调用；第一次调用按 视频 → 音频 → 纹理 → 文字 → TTF →
   renderer → window → 平台 的顺序停止运行时，并关闭日志。

退出会释放引擎仍追踪的资源。所有 `bapi_texture_t`、`bapi_sound_t`、`bapi_video_t`，以及
`bapi_engine_get_window()` / `bapi_engine_get_renderer()` 返回的借用句柄，退出后全部失效，不得再
使用或释放。

初始化前和退出后：
- `bapi_poll_event()` 返回 `0`，若传入事件则写入 `BAPI_EVENT_UNKNOWN`
- `bapi_engine_get_window()` / `bapi_engine_get_renderer()` 返回 `NULL`
- 基础绘制、`bapi_delay()`、`bapi_get_ticks()` 安全返回（绘制为 no-op，delay 直接返回，ticks 为 0）
- 事件查询函数接受 `NULL` 并返回零值或 `BAPI_EVENT_UNKNOWN`

### 类型与返回约定

不透明资源类型：`bapi_window_t`、`bapi_renderer_t`、`bapi_texture_t`、`bapi_sound_t`、
`bapi_video_t`、`bapi_scene_t`、`bapi_scene_manager_t`、`bapi_level_t`、`bapi_level_manager_t`、
`bapi_file_t`。内部布局定义在 `src/internal/`，公开头不可见。

颜色、矩形、事件、摄像机、向量、圆、按钮、日志配置以及场景/关卡回调结构的精确布局都定义在
`BridgeEngine.h`。

- 创建/加载类函数：返回 `NULL` 表示失败
- 状态/操作类函数：通常 `0` 成功、非零失败
- 无返回值的函数是安全的命令式操作，任何状态下调用都不会崩溃
- 指针参数可为 `NULL` 的具体情况在各函数条目中说明

### 平台媒体能力

桌面 SDL3 后端支持 WAV 音频和 FFmpeg 视频。XJ380 保留窗口、基础绘制和内置文字，不支持音频或
视频：`bapi_audio_init()` 和 `bapi_video_init()` 返回非零，`bapi_sound_load()` 和 `bapi_video_load()`
返回 `NULL`，首次失败通过平台 warning 记录原因。无返回值的媒体控制与清理函数可安全调用。公开
API 不提供 capability 查询；调用方检查既有返回值即可。

### 资源所有权

调用方可在运行时用 `bapi_texture_destroy`、`bapi_sound_free`、`bapi_video_free` 提前释放资源；
不释放则引擎退出统一回收。场景、关卡及其 manager 由调用方创建并持有，`*_manager_destroy` 会
级联释放其管理的 scene/level。

## 引擎、事件与渲染

### 引擎生命周期

```c
int bapi_engine_init(const char *title, int width, int height);
```
创建窗口、renderer 并初始化文字后端。`title` 可为 `NULL`。成功返回 0，任一环节失败返回非零，
失败时内部已做回滚。重复调用（已初始化）直接返回 0，不重建。

```c
void bapi_engine_quit(void);
```
停止运行时并关闭日志。可重复调用，幂等。

```c
bapi_window_t bapi_engine_get_window(void);
bapi_renderer_t bapi_engine_get_renderer(void);
```
返回借用句柄，仅初始化期间有效，退出后返回 `NULL`。调用方不拥有，无对应销毁函数。

### 事件

```c
int bapi_poll_event(bapi_event_t *event);
```
从队列取一个事件。取到返回非零并填充 `event`；无事件返回 0。`event` 传 `NULL` 时不消费事件，
只返回"是否有事件可读"。

```c
int bapi_event_get_type(const bapi_event_t *event);
```
返回 `event->type`。`event` 为 `NULL` 时返回 `BAPI_EVENT_UNKNOWN`。

```c
uint8_t bapi_event_get_key_code(const bapi_event_t *event);
int bapi_event_get_mouse_x(const bapi_event_t *event);
int bapi_event_get_mouse_y(const bapi_event_t *event);
int bapi_event_get_mouse_button(const bapi_event_t *event);
int bapi_event_get_motion_x(const bapi_event_t *event);
int bapi_event_get_motion_y(const bapi_event_t *event);
```
各 getter 从 `event` 中取对应字段；`event` 为 `NULL` 时返回 0。鼠标位置返回整型像素坐标（内部
为 float，取整）。仅当事件类型匹配对应 union 分支时字段才有意义。

```c
int bapi_event_is_mouse_button_down(const bapi_event_t *event);
int bapi_event_is_mouse_button_up(const bapi_event_t *event);
int bapi_event_is_mouse_motion(const bapi_event_t *event);
```
类型断言：`event` 为 `NULL` 或类型不匹配时返回 0。

事件类型 `bapi_event_type_t`：`BAPI_EVENT_QUIT`、`BAPI_EVENT_KEY_DOWN`、`BAPI_EVENT_KEY_UP`、
`BAPI_EVENT_MOUSE_BUTTON_DOWN`、`BAPI_EVENT_MOUSE_BUTTON_UP`、`BAPI_EVENT_MOUSE_MOTION`、
`BAPI_EVENT_UNKNOWN`。按键码 0-127 为 ASCII 码，128 起为 `KEY_*` 特殊键（ESC、F1-F12 等）。
`BAPI_BUTTON_LEFT` 为左键（值为 1）。

### 渲染控制

```c
void bapi_render_clear(void);
```
将 renderer 清为黑色（0,0,0,255）。

```c
void bapi_render_present(void);
```
把当前帧提交到屏幕。绘制调用必须介于一次 `bapi_render_clear()` 和一次 `bapi_render_present()`
之间。

```c
void bapi_set_render_color(bapi_color_t color);
```
设置后续绘制使用的颜色。带颜色参数的绘制函数会覆盖此设置。

```c
void bapi_delay(uint32_t ms);
```
睡眠 `ms` 毫秒。用于简单帧率控制。

### 图元绘制

所有绘制函数需在初始化后调用，坐标以像素为单位，颜色为 `bapi_color_t`。圆和多边形用线段逼近。

```c
void bapi_draw_pixel(float x, float y, bapi_color_t color);
void bapi_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color);
void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color);
void bapi_fill_rect(float x, float y, float w, float h, bapi_color_t color);
void bapi_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, bapi_color_t color);
void bapi_fill_circle(float cx, float cy, float radius, bapi_color_t color);
```
`fill_rect`/`fill_circle` 填充实心区域；三角形和圆形只有描边版本（`draw_triangle`、
`draw_circle` 为描边，`fill_circle` 用中心到圆周的三角扇形填充）。

```c
void bapi_draw_circle(float cx, float cy, float radius, bapi_color_t color);
void bapi_draw_polygon(float cx, float cy, float radius, int sides, bapi_color_t color);
void bapi_fill_polygon(float cx, float cy, float radius, int sides, bapi_color_t color);
```
正多边形，中心 `(cx,cy)`、外接圆半径 `radius`、`sides` 条边。`sides < 3` 时直接返回。第一个顶点
在正上方，顺时针排列。

```c
void bapi_draw_image(const char *filepath, float x, float y, float w, float h);
```
一次性加载图片并绘制到 `(x,y,w,h)`，绘制完立即释放纹理。不缓存，反复调用会产生重复 IO。需要
复用纹理时用 `bapi_texture_from_file()` 先加载。`filepath` 为 `NULL` 或加载失败时 no-op。

### 颜色与计时

```c
bapi_color_t bapi_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
bapi_color_t bapi_color_from_hex(uint32_t hex_color);
```
`bapi_color` 直接构造颜色。`bapi_color_from_hex` 按 `0xAARRGGBB` 字节序解析（高位 alpha）。
`bapi_color_t` 为 `{r,g,b,a}` 四字节结构。

```c
uint32_t bapi_get_ticks(void);
```
自初始化以来的毫秒数。用于帧耗时测量与计时。

## 文字与鼠标绘图

### 文字

文字基于字体文件，运行时按优先级在 `assets/text/font.ttf`、`examples/assets/text/font.ttf`、
`text/font.ttf` 中查找，按 `size` 缓存最多 8 个字号。

```c
void bapi_text_init(void);
void bapi_text_cleanup(void);
```
`bapi_text_init` 重置文字状态；`bapi_draw_text` / `bapi_get_text_size` 内部会自动调用，一般无需
手动调用。`bapi_text_cleanup` 释放缓存的字体，由引擎退出时调用。

```c
void bapi_draw_text(const char *text, float x, float y, float size, bapi_color_t color);
```
以 `size` 字号绘制文字，左上角在 `(x,y)`。`text` 为 `NULL` 或空串时 no-op。字体加载失败时 no-op。

```c
void bapi_get_text_size(const char *text, float size, float *width, float *height);
```
测量文字在 `size` 字号下的像素尺寸，写入输出参数。输出参数可为 `NULL`（跳过该项）。`text` 为空
或测量失败时输出 0。

### 鼠标绘图

轻量画板：按住左键拖动画线，内部记录最近最多 1000 条线。

```c
void bapi_mouse_init(void);
void bapi_mouse_handle_event(const bapi_event_t *event);
void bapi_mouse_render(void);
void bapi_mouse_clear(void);
void bapi_mouse_cleanup(void);
```
把轮询到的事件喂给 `bapi_mouse_handle_event` 以开始/继续/结束画线；每帧在 `bapi_render_present`
前调用 `bapi_mouse_render` 重绘已记录的线条；`bapi_mouse_clear` 清空记录；`bapi_mouse_cleanup`
由引擎退出时调用。

```c
void bapi_mouse_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color);
```
直接绘制一条线，等价于 `bapi_draw_line`。

## 媒体

### 纹理

```c
bapi_texture_t bapi_texture_load(const char *filepath);
```
从文件加载纹理，返回新句柄（引用计数 1）。失败返回 `NULL`。**不经过缓存**，每次都是新对象。

```c
void bapi_texture_destroy(bapi_texture_t texture);
```
释放一次引用；引用计数归零时销毁底层纹理。`texture` 为 `NULL` 时 no-op。

```c
void bapi_texture_render(bapi_texture_t texture, float x, float y);
void bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h);
```
绘制纹理。前者按原始尺寸绘制到 `(x,y)`，后者拉伸到 `(w,h)`。`texture` 为 `NULL` 时 no-op。

```c
void bapi_texture_get_size(bapi_texture_t texture, int *w, int *h);
```
返回纹理像素宽高。输出参数可为 `NULL`。`texture` 为 `NULL` 时输出 0。

```c
bapi_texture_t bapi_texture_from_file(const char *filepath, int *out_w, int *out_h);
```
从文件加载纹理并**按文件路径缓存**（最多 `BAPI_MAX_CACHED_TEXTURES` 个槽位）。命中缓存时返回
同一句柄并增加引用计数；未命中则加载并写入缓存。`out_w`/`out_h` 可为 `NULL`，返回纹理尺寸。
`filepath` 为空或加载失败返回 `NULL`。

```c
void bapi_texture_cache_clear(void);
```
释放缓存中所有纹理引用。只移除缓存引用，不销毁仍被调用方持有的纹理（引用计数保护）。

### 音频

```c
int bapi_audio_init(void);
void bapi_audio_cleanup(void);
```
`bapi_audio_init` 打开音频设备（双声道、44100 Hz）。成功返回 0；平台不支持或设备打开失败返回
非零。`bapi_audio_cleanup` 释放全部声音并关闭设备，引擎退出时调用。

```c
bapi_sound_t bapi_sound_load(const char *filepath);
```
加载 WAV 文件。成功返回句柄（初始音量 1.0、不循环、未播放）；失败或平台不支持返回 `NULL`。

```c
int bapi_sound_play(bapi_sound_t sound);
```
播放（或重播）声音。需要先成功 `bapi_audio_init`。成功返回 0，否则非零。重复调用会重新从头播放。

```c
void bapi_sound_set_volume(bapi_sound_t sound, float volume);
void bapi_sound_set_loop(bapi_sound_t sound, int loop);
void bapi_sound_stop(bapi_sound_t sound);
```
音量钳制在 0.0-1.0。`loop` 非零时循环。`stop` 停止播放并清空已排队数据。`sound` 为 `NULL` 时
no-op。

```c
void bapi_sound_update(void);
```
推进所有播放中声音的状态（检测播放结束、非循环声音播完自动停止、循环声音补充数据）。**必须每
帧调用**，否则声音不会正常结束/循环。

```c
void bapi_sound_free(bapi_sound_t sound);
```
停止并释放声音及其资源。`sound` 为 `NULL` 时 no-op。

### 视频

```c
int bapi_video_init(void);
void bapi_video_cleanup(void);
```
`bapi_video_init` 检查平台能力，成功返回 0，不支持返回非零。`bapi_video_cleanup` 释放全部视频，
引擎退出时调用。

```c
bapi_video_t bapi_video_load(const char *filepath);
```
加载 MP4/AVI/MKV 等 FFmpeg 支持的容器。成功返回句柄（未播放、音量 1.0、不循环）；失败或平台不
支持返回 `NULL`。加载时若音轨可解码则同时初始化音频流。

```c
void bapi_video_free(bapi_video_t video);
```
停止并释放视频及其解码器、纹理和音频流。`video` 为 `NULL` 时 no-op。退出后句柄失效，不应再次
调用。

```c
int bapi_video_play(bapi_video_t video);
void bapi_video_pause(bapi_video_t video);
void bapi_video_stop(bapi_video_t video);
```
`play` 开始播放（若已停止则从头），成功返回 0，`video` 为 `NULL` 返回 1。`pause` 切换暂停/继续。
`stop` 停止并回到开头，清空音频数据。

```c
void bapi_video_render(bapi_video_t video, int x, int y, int w, int h);
void bapi_video_render_fit(bapi_video_t video, int area_x, int area_y, int area_w, int area_h);
void bapi_video_render_center(bapi_video_t video, int window_w, int window_h);
```
`render` 拉伸到 `(x,y,w,h)`。`render_fit` 在指定区域内等比缩放并居中（letterbox）。`render_center`
在整个窗口中等比缩放并居中。

```c
void bapi_video_set_loop(bapi_video_t video, int loop);
void bapi_video_set_volume(bapi_video_t video, float volume);
int bapi_video_is_playing(bapi_video_t video);
void bapi_video_get_size(bapi_video_t video, int *w, int *h);
```
`loop` 非零循环播放。音量钳制 0.0-1.0。`is_playing` 返回是否正在播放（非暂停）。`get_size` 返回
源视频像素尺寸，输出参数可为 `NULL`。

```c
void bapi_video_update(void);
```
按视频帧率推进当前播放的视频：解码下一帧、更新纹理、播放关联音频。**必须每帧调用**，否则视频
不会前进。播放到结尾时自动停止；`loop` 开启则从头重播。

## UI、输入与摄像机

### 按钮

`bapi_button_t` 是公开结构体，字段在头文件可见。

```c
bapi_button_t *bapi_create_button(float x, float y, float w, float h, const char *text,
                                  bapi_color_t normal_color, bapi_color_t hover_color,
                                  bapi_color_t click_color, bapi_color_t text_color,
                                  float text_size);
void bapi_destroy_button(bapi_button_t *button);
```
创建/销毁按钮。`text` 可为 `NULL`（纯色按钮）。创建时用当前文字后端测量文本尺寸用于居中。

```c
int bapi_button_update(bapi_button_t *button, const bapi_event_t *event);
```
每帧传入事件更新按钮状态。返回非零表示本次发生了一次完整的点击（按下+在按钮内松开）；
`button` 或 `event` 为 `NULL` 返回 0。悬停由鼠标位置决定。

```c
void bapi_button_render(bapi_button_t *button);
```
绘制按钮：按状态（点击 > 悬停 > 常态）填充颜色，画黑色边框，文本居中。

```c
int bapi_button_is_clicked(bapi_button_t *button);
int bapi_button_is_hovered(bapi_button_t *button);
```
状态查询，`button` 为 `NULL` 返回 0。

### 输入状态

```c
void bapi_input_init(void);
void bapi_input_cleanup(void);
```
`bapi_input_init` 清零输入状态并标记初始化。`bapi_input_cleanup` 清零，引擎退出时调用。

```c
void bapi_input_handle_event(const bapi_event_t *event);
void bapi_input_update(void);
```
把轮询到的事件喂给 `handle_event` 更新按键/鼠标状态。`bapi_input_update` **每帧调用一次**：保存
上一帧状态（供 pressed/released 判断）并刷新鼠标位置。

```c
int bapi_is_key_down(uint8_t key);
int bapi_is_key_pressed(uint8_t key);
int bapi_is_key_released(uint8_t key);
```
`down`：当前按住。`pressed`：本帧新按下（上一帧未按下）。`released`：本帧松开。`key` 为 ASCII 码
或 `KEY_*` 特殊键。需先 `bapi_input_handle_event` + `bapi_input_update`。

```c
int bapi_is_mouse_button_down(int button);
int bapi_is_mouse_button_pressed(int button);
int bapi_is_mouse_button_released(int button);
```
与按键语义一致。`button` 用 `BAPI_BUTTON_LEFT`（值为 1）等。

```c
float bapi_get_mouse_x(void);
float bapi_get_mouse_y(void);
void bapi_get_mouse_position(float *x, float *y);
```
当前鼠标位置。`get_mouse_position` 输出参数可为 `NULL`。

### 摄像机

`bapi_camera_t` 是公开结构体，保存位置、缩放、旋转和视口尺寸。

```c
void bapi_camera_init(bapi_camera_t *cam, float viewport_w, float viewport_h);
void bapi_camera_set_position(bapi_camera_t *cam, float x, float y);
void bapi_camera_move(bapi_camera_t *cam, float dx, float dy);
void bapi_camera_set_zoom(bapi_camera_t *cam, float zoom);
void bapi_camera_set_rotation(bapi_camera_t *cam, float angle_rad);
void bapi_camera_set_viewport(bapi_camera_t *cam, float w, float h);
```
`init` 把摄像机放在原点、缩放 1、无旋转。`move` 是相对移动，`set_position` 是绝对定位。旋转单位
为弧度。所有函数要求 `cam` 非 `NULL`（不检查）。

```c
void bapi_camera_world_to_screen(bapi_camera_t *cam, float wx, float wy, float *sx, float *sy);
void bapi_camera_screen_to_world(bapi_camera_t *cam, float sx, float sy, float *wx, float *wy);
bapi_vec2_t bapi_camera_world_to_screen_v(bapi_camera_t *cam, bapi_vec2_t world);
bapi_vec2_t bapi_camera_screen_to_world_v(bapi_camera_t *cam, bapi_vec2_t screen);
void bapi_camera_get_view_rect(bapi_camera_t *cam, bapi_rect_t *out_rect);
```
坐标转换：世界到屏幕 = 世界坐标减去摄像机位置 → 绕摄像机旋转 → 乘缩放 → 平移视口中心。
`screen_to_world` 为其逆变换。`*_v` 版本返回向量。`get_view_rect` 输出摄像机可见的世界范围
矩形（`out_rect` 不得为 `NULL`）。

## 场景与关卡

### 场景

```c
bapi_scene_t bapi_scene_create(const char *name, bapi_scene_callbacks_t callbacks);
void bapi_scene_destroy(bapi_scene_t scene);
```
创建/销毁场景。`name` 不能为 `NULL`，用于按名字查找和 XML 持久化。`callbacks` 含
`on_enter`/`on_exit`/`on_update`/`on_render` 四个可选回调和一个 `user_data`（非空时会被写入场景）。
`bapi_scene_destroy` 只释放场景本身，不释放 manager。

```c
const char *bapi_scene_get_name(bapi_scene_t scene);
void *bapi_scene_get_user_data(bapi_scene_t scene);
void bapi_scene_set_user_data(bapi_scene_t scene, void *user_data);
```
查询/设置场景的用户数据。`scene` 为 `NULL` 时 `get_*` 返回 `NULL`，`set_*` 为 no-op。

```c
bapi_scene_manager_t bapi_scene_manager_create(void);
void bapi_scene_manager_destroy(bapi_scene_manager_t manager);
```
创建空 manager。`destroy` 级联销毁全部已添加场景（不重复销毁，添加后归 manager 所有）。最大
`MAX_SCENES`（64）个场景。

```c
int bapi_scene_manager_add_scene(bapi_scene_manager_t manager, bapi_scene_t scene);
```
添加场景。返回 0 成功；`-1` 参数非法；`-2` 已满。

```c
int bapi_scene_manager_switch_scene(bapi_scene_manager_t manager, const char *name);
```
切换到指定名字的场景。若当前有场景则触发其 `on_exit`，再触发新场景 `on_enter`。返回 0 成功；
`-1` 参数非法；`-2` 未找到。

```c
bapi_scene_t bapi_scene_manager_get_current_scene(bapi_scene_manager_t manager);
bapi_scene_t bapi_scene_manager_get_scene(bapi_scene_manager_t manager, const char *name);
```
查询当前场景 / 按名字查找。未找到返回 `NULL`。

```c
void bapi_scene_manager_update(bapi_scene_manager_t manager, float delta_time);
void bapi_scene_manager_render(bapi_scene_manager_t manager);
```
把更新/渲染派发给当前场景的回调。无当前场景或回调未设置时 no-op。

### 关卡

```c
bapi_level_t bapi_level_create(const char *name, int index, bapi_level_callbacks_t callbacks);
void bapi_level_destroy(bapi_level_t level);
```
创建/销毁关卡。`name` 不能为 `NULL`。`index` 是逻辑关卡序号，用于按序号导航（0-255 内有效）。
`callbacks` 结构与场景一致（`on_load`/`on_unload`/`on_update`/`on_render` + `user_data`）。

```c
const char *bapi_level_get_name(bapi_level_t level);
int bapi_level_get_index(bapi_level_t level);
void *bapi_level_get_user_data(bapi_level_t level);
void bapi_level_set_user_data(bapi_level_t level, void *user_data);
```
查询/设置。`level` 为 `NULL` 时 `get_name`/`get_user_data` 返回 `NULL`，`get_index` 返回 -1。

```c
bapi_level_manager_t bapi_level_manager_create(void);
void bapi_level_manager_destroy(bapi_level_manager_t manager);
```
创建空 manager；`destroy` 级联销毁全部关卡。最大 `MAX_LEVELS`（64）个。

```c
int bapi_level_manager_add_level(bapi_level_manager_t manager, bapi_level_t level);
```
添加关卡，同时登记其 `index` 到序号映射。返回 0 成功；`-1` 参数非法；`-2` 已满。

```c
int bapi_level_manager_load_level(bapi_level_manager_t manager, const char *name);
int bapi_level_manager_load_level_by_index(bapi_level_manager_t manager, int index);
int bapi_level_manager_next_level(bapi_level_manager_t manager);
int bapi_level_manager_previous_level(bapi_level_manager_t manager);
```
加载关卡：卸载当前关卡（触发 `on_unload`）并加载目标关卡（触发 `on_load`）。按名字/序号/下一关/
上一关。返回 0 成功；`-1` 参数非法；`-2` 目标不存在（按名字未找到、序号未登记、已到边界）。

```c
bapi_level_t bapi_level_manager_get_current_level(bapi_level_manager_t manager);
bapi_level_t bapi_level_manager_get_level(bapi_level_manager_t manager, const char *name);
bapi_level_t bapi_level_manager_get_level_by_index(bapi_level_manager_t manager, int index);
int bapi_level_manager_get_level_count(bapi_level_manager_t manager);
```
查询。未找到返回 `NULL`/0。

```c
void bapi_level_manager_update(bapi_level_manager_t manager, float delta_time);
void bapi_level_manager_render(bapi_level_manager_t manager);
```
把更新/渲染派发给当前关卡的回调。

### XML 持久化

受限的 scene/level 格式，不是通用 XML parser。

```c
bapi_scene_manager_t bapi_scene_manager_load_from_xml(const char *filepath);
bapi_level_manager_t bapi_level_manager_load_from_xml(const char *filepath);
```
从 XML 文件加载并返回 manager。只还原名字（和关卡的 index），回调为空；文件打不开或创建失败
返回 `NULL`。

```c
int bapi_scene_manager_save_to_xml(bapi_scene_manager_t manager, const char *filepath);
int bapi_level_manager_save_to_xml(bapi_level_manager_t manager, const char *filepath);
```
把 manager 的全部场景/关卡写到 XML。返回 0 成功；`-1` 参数非法或文件写入失败。

格式：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<scenes>
  <scene name="menu" />
  <scene name="game" />
</scenes>
```

```xml
<?xml version="1.0" encoding="UTF-8"?>
<levels>
  <level name="level_1" index="1" />
</levels>
```

### UI XML

UI XML 是独立于 scene/level 持久化的受限界面描述格式，不是通用 XML parser。

```c
typedef struct bapi_ui_internal *bapi_ui_t;
typedef struct bapi_ui_component_internal *bapi_ui_component_t;

bapi_ui_t bapi_ui_load_from_xml(const char *filepath);
void bapi_ui_destroy(bapi_ui_t ui);
void bapi_ui_update(bapi_ui_t ui, const bapi_event_t *event);
void bapi_ui_render(bapi_ui_t ui);
void bapi_ui_layout(bapi_ui_t ui);
int bapi_ui_was_clicked(bapi_ui_t ui, const char *id);
bapi_ui_component_t bapi_ui_find(bapi_ui_t ui, const char *id);
bapi_ui_component_type_t bapi_ui_component_get_type(bapi_ui_component_t component);
void bapi_ui_component_get_rect(bapi_ui_component_t component, bapi_rect_t *out_rect);
int bapi_ui_component_is_visible(bapi_ui_component_t component);
void bapi_ui_component_set_visible(bapi_ui_component_t component, int visible);
int bapi_ui_component_is_enabled(bapi_ui_component_t component);
void bapi_ui_component_set_enabled(bapi_ui_component_t component, int enabled);
const char *bapi_ui_component_get_text(bapi_ui_component_t component);
int bapi_ui_component_set_text(bapi_ui_component_t component, const char *text);
float bapi_ui_component_get_value(bapi_ui_component_t component);
int bapi_ui_component_set_value(bapi_ui_component_t component, float value);
int bapi_ui_component_is_checked(bapi_ui_component_t component);
int bapi_ui_component_is_focused(bapi_ui_component_t component);
int bapi_ui_component_get_selected_index(bapi_ui_component_t component);
int bapi_ui_component_set_selected_index(bapi_ui_component_t component, int index);
float bapi_ui_component_get_scroll_offset(bapi_ui_component_t component);
int bapi_ui_component_set_scroll_offset(bapi_ui_component_t component, float offset);
```

`bapi_ui_load_from_xml` 加载一个 `<ui>` 根节点及其自闭合组件。UI XML 只描述界面的静态结构和
视觉属性，不在 XML 里绑定 C 回调；业务逻辑应通过组件 `id` 在 C 代码中处理。组件按照 XML
出现的顺序渲染，后面的组件会覆盖前面的组件。UI 对象拥有组件树、文本和纹理/视频资源，
必须使用 `bapi_ui_destroy` 释放。文件打不开、组件属性非法、未知组件、重复 `id` 或资源加载
失败时返回 `NULL`。

`bapi_ui_update` 将事件派发给当前最上层的可交互组件。重叠组件按 XML 顺序决定层级，后出现的
组件优先命中；鼠标必须在同一组件内按下和释放才算点击。`bapi_ui_was_clicked(ui, id)` 返回
1；查询非按钮或未知 `id` 返回 0。点击状态在下一次 `bapi_ui_update` 时更新。

文件格式约束：

- 根节点必须是 `<ui>`。根节点可以为空：`<ui></ui>` 或 `<ui />`。
- 叶子组件必须是自闭合标签，例如 `<button ... />`；容器组件可以使用成对标签包含子组件。
- 属性必须使用双引号。支持属性跨行。
- 支持 XML 声明 `<?xml version="1.0" encoding="UTF-8"?>` 和 `<!-- comment -->` 注释。
- 支持基础 entity：`&amp;`、`&lt;`、`&gt;`、`&quot;`、`&apos;`。
- 不支持百分比坐标、样式继承、脚本、回调名或通用 XML 解析能力。

通用属性：

- `id`：所有组件必填，必须非空，且在同一个 UI 文件内唯一。C 代码通过它查询按钮状态。
- `x` / `y`：组件左上角位置，单位是当前渲染坐标中的像素。
- `w` / `h`：组件宽高。`rect`、`button`、`image` 必填。
- `visible` / `enabled`：可选布尔值，接受 `true`、`false`、`1`、`0`、`yes`。
- `relative`：可选布尔值。为 `true` 时，组件坐标相对于父容器；默认为绝对坐标。
- `text`：文本组件或交互组件的初始文本。文本中的 `&amp;` 等基础 entity 会被解码。
- 颜色写成 `#RRGGBB` 或 `#RRGGBBAA`。省略 alpha 时按 `255` 处理。

查询 API 返回的是 UI 内部借用句柄，不需要也不能单独释放。`bapi_ui_find` 按 `id` 在整棵
组件树中查找；`bapi_ui_component_get_type` 返回组件类型；`get_rect`、`get_text`、`get_value`
和状态查询用于读取组件当前状态。`set_visible`、`set_enabled`、`set_text`、`set_value`
用于运行时修改组件。组件被 `bapi_ui_destroy` 销毁后，所有借用句柄失效。修改布局组件尺寸、
子组件坐标或滚动偏移后调用 `bapi_ui_layout` 重新计算位置。`Tab` 在可交互组件之间移动焦点，
`Enter` 激活焦点组件，`Esc` 清除焦点；`input` 支持可打印字符和 `KEY_BACKSPACE`，`max_length`
限制文本长度。

组件属性：

```xml
<rect id="panel" x="20" y="16" w="400" h="180" color="#20242CFF" />
```

`rect` 绘制一个实心矩形。必填属性：`id`、`x`、`y`、`w`、`h`、`color`。

```xml
<label id="title" x="40" y="24" text="Main Menu" size="28" color="#FFFFFFFF" />
```

`label` 绘制一段文本。必填属性：`id`、`x`、`y`、`text`、`size`、`color`。文本使用当前引擎
文字系统，字体加载失败时不会绘制文本。

```xml
<button id="start" x="40" y="80" w="180" h="48" text="Start"
        text_size="20" normal="#2F80EDFF" hover="#56CCF2FF"
        click="#1C5DB8FF" text_color="#FFFFFFFF" />
```

`button` 使用内置按钮样式和鼠标状态。必填属性：`id`、`x`、`y`、`w`、`h`、`text`、
`text_size`、`normal`、`hover`、`click`、`text_color`。`normal` 是默认颜色，`hover` 是鼠标
悬停颜色，`click` 是按下状态颜色，`text_color` 是按钮文字颜色。

```xml
<image id="logo" x="260" y="24" w="128" h="128" src="assets/logo.png" />
```

`image` 加载并绘制一张纹理。必填属性：`id`、`x`、`y`、`w`、`h`、`src`。`src` 可以是绝对路径，
也可以是相对路径；相对路径基于 XML 文件所在目录，而不是进程当前工作目录。图片加载失败时，
整个 `bapi_ui_load_from_xml` 返回 `NULL`。

`progress` 和 `slider` 使用 `value`、`min`、`max` 属性。内部绘制会把数值归一化到 `[0, 1]`，
因此 `min="20" max="100" value="50"` 会绘制为 37.5% 的进度。`slider` 点击轨道后更新
`value`，`bapi_ui_component_set_value` 也会自动限制到 `[min, max]`。

`checkbox`、`toggle` 和 `radio` 通过 `checked` 查询当前状态；同一父容器中的 `radio` 互斥。
`select`、`list` 和 `tab` 使用 `items` 和 selected index 作为基础选择模型：
`bapi_ui_component_get_selected_index` 查询索引，`bapi_ui_component_set_selected_index` 修改索引。
`scroll` 使用 `bapi_ui_component_get_scroll_offset` / `bapi_ui_component_set_scroll_offset` 管理
垂直偏移，修改后调用 `bapi_ui_layout`；滚动容器外的子组件不会参与命中和绘制。
XJ380 后端会把 `video` 组件降级为不可见、禁用节点，使同一份 UI XML 可以继续加载。

绘制和布局组件：

- `line`：使用 `x`、`y`、`w`、`h` 表示起点和终点偏移，使用 `color` 绘制线段。
- `circle`：使用 `x`、`y`、`radius` 和 `color`。`checked="true"` 时绘制实心圆，否则绘制圆框。
- `polygon`：使用 `x`、`y`、`radius`、`sides` 和 `color` 绘制多边形。
- `border`：使用 `x`、`y`、`w`、`h` 和 `color` 绘制矩形边框。
- `progress`：使用 `x`、`y`、`w`、`h`、`value`、`min`、`max`、`color` 和可选 `color2` 绘制进度条。
- `separator`：使用矩形属性绘制分隔线。
- `panel` / `container`：绘制背景并可包含子组件。
- `row` / `column`：按 `step` 间距沿横向/纵向排列子组件。
- `grid`：按 `columns` 和 `step` 将子组件排列为网格。

交互组件：

- `checkbox`、`radio`、`toggle`：点击后修改 `checked` 状态；同一容器下的 `radio` 互斥。
- `slider`：点击轨道修改 `value`，使用 `min` 和 `max` 限制范围。
- `input`：点击后获得焦点；键盘输入追加到 `text`，`KEY_BACKSPACE` 删除末尾字符。
- `select`、`list`、`tab`：当前提供可查询、可点击的文本组件基础行为，选项数据和弹出菜单由调用方管理。
- `scroll`、`canvas`：作为可嵌套的布局/扩展节点，目前不提供自动滚动或自定义绘制回调。
- `video`：使用 `src` 加载视频并按组件矩形渲染；视频后端加载失败会使整个 UI 加载失败。
- `nine_patch`、`animation`、`tooltip`、`modal`、`popup`：提供 XML 节点和基础渲染/容器行为；高级资源切片、
  动画时间轴、弹层管理和自动定位由调用方通过组件属性或业务代码控制。

典型使用方式是在场景初始化时加载 UI，在事件循环中更新 UI，在场景渲染回调中渲染 UI：

```c
static bapi_ui_t menu_ui;

static void menu_render(bapi_scene_t scene)
{
	(void)scene;
	bapi_ui_render(menu_ui);
}

/* 初始化阶段 */
menu_ui = bapi_ui_load_from_xml("assets/ui/menu.xml");

/* 事件循环中，仅在对应场景活跃时更新 */
bapi_ui_update(menu_ui, &event);
if (bapi_ui_was_clicked(menu_ui, "start")) {
	bapi_scene_manager_switch_scene(scene_manager, "game");
}

/* 退出阶段 */
bapi_ui_destroy(menu_ui);
```

示例：

```xml
<ui>
  <rect id="panel" x="20" y="16" w="400" h="180" color="#20242CFF" />
  <label id="title" x="40" y="24" text="Main Menu" size="28" color="#FFFFFFFF" />
  <button id="start" x="40" y="80" w="180" h="48" text="Start"
          text_size="20" normal="#2F80EDFF" hover="#56CCF2FF"
          click="#1C5DB8FF" text_color="#FFFFFFFF" />
  <image id="logo" x="260" y="24" w="128" h="128" src="assets/logo.png" />
</ui>
```

## 数学、日志与版本

### 向量

```c
bapi_vec2_t bapi_vec2(float x, float y);
bapi_vec2_t bapi_vec2_add(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t bapi_vec2_sub(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t bapi_vec2_scale(bapi_vec2_t vector, float scalar);
bapi_vec2_t bapi_vec2_negate(bapi_vec2_t vector);
```
构造与基本运算。`bapi_vec2_t` 为 `{x,y}`。

```c
float bapi_vec2_dot(bapi_vec2_t a, bapi_vec2_t b);
float bapi_vec2_length(bapi_vec2_t vector);
float bapi_vec2_length_sq(bapi_vec2_t vector);
float bapi_vec2_distance(bapi_vec2_t a, bapi_vec2_t b);
```
点积、长度、长度平方（免开方比较）、两点距离。

```c
bapi_vec2_t bapi_vec2_normalize(bapi_vec2_t vector);
```
单位化。长度小于 `0.00001` 时返回零向量（不做除零）。

```c
bapi_vec2_t bapi_vec2_lerp(bapi_vec2_t a, bapi_vec2_t b, float t);
bapi_vec2_t bapi_vec2_rotate(bapi_vec2_t vector, float angle);
int bapi_vec2_equals(bapi_vec2_t a, bapi_vec2_t b);
```
线性插值、绕原点旋转（`angle` 弧度）、近似相等（容差 `0.00001`）。

### 几何

```c
bapi_circle_t bapi_circle(float x, float y, float radius);
```
构造圆，`bapi_circle_t` 为 `{x,y,r}`。

```c
int bapi_rect_contains_point(bapi_rect_t rect, bapi_vec2_t point);
int bapi_circle_contains_point(bapi_circle_t circle, bapi_vec2_t point);
```
点在矩形/圆内（边界算包含，`<=` / `>=` 比较）。

```c
int bapi_rect_intersects(bapi_rect_t a, bapi_rect_t b);
int bapi_circle_intersects_circle(bapi_circle_t a, bapi_circle_t b);
int bapi_circle_intersects_rect(bapi_circle_t circle, bapi_rect_t rect);
```
矩形相交要求**正面积重叠**（边/角恰好接触算不相交）。圆的相交把相切算作相交。几何查询要求
有限坐标、非负半径、非负矩形宽高，违反时结果不作承诺。

```c
bapi_rect_t bapi_rect_intersection(bapi_rect_t a, bapi_rect_t b);
bapi_rect_t bapi_rect_union(bapi_rect_t a, bapi_rect_t b);
bapi_vec2_t bapi_rect_center(bapi_rect_t rect);
```
相交矩形（无重叠时宽高钳制为 0）、包围矩形、矩形中心。

```c
int bapi_collision_aabb(bapi_rect_t a, bapi_rect_t b);
```
`bapi_rect_intersects` 的别名，语义完全一致。

### 标量工具

```c
float bapi_clamp(float value, float min, float max);
float bapi_lerp(float a, float b, float t);
float bapi_deg_to_rad(float degrees);
float bapi_rad_to_deg(float radians);
```
钳制、线性插值（`a + (b-a)*t`）、角度换算。`bapi_lerp` 不钳制 `t`。

### 日志

```c
bool bapi_log_init(const bapi_log_config_t *config);
void bapi_log_shutdown(void);
```
初始化日志。`config` 为 `NULL` 时用默认值（级别 INFO、着色开、不写文件）。`config` 含 `min_level`、
`use_colors`、`use_file`、`log_file_path`。重复调用返回 `true`（幂等）。`bapi_log_shutdown` 关闭
日志文件与互斥锁。`bapi_engine_quit()` 会自动调用。

```c
void bapi_log_set_level(bapi_log_level_t level);
```
调整最低输出级别，低于该级别的消息被丢弃。

```c
void bapi_log_message(bapi_log_level_t level, const char *file, int line, const char *func,
                      const char *format, ...);
```
底层记录函数，一般用下面的宏代替。日志级别见 `bapi_log_level_t`：
`DEBUG` < `INFO` < `WARN` < `ERROR` < `CRITICAL`（`NONE` 用于 `min_level` 屏蔽全部）。

```c
BAPI_LOG_DEBUG(fmt, ...);
BAPI_LOG_INFO(fmt, ...);
BAPI_LOG_WARN(fmt, ...);
BAPI_LOG_ERROR(fmt, ...);
BAPI_LOG_CRITICAL(fmt, ...);
BAPI_LOG_ASSERT(condition, fmt, ...);
BAPI_LOG_INIT_DEFAULT();
```
带 `__FILE__`/`__LINE__`/`__func__` 的便捷宏。仅在编译时定义 `BAPI_LOG_ENABLED` 时展开为实际
输出，否则为 no-op（`BAPI_LOG_ASSERT` 求值条件但不输出）。`BAPI_LOG_INIT_DEFAULT()` 用默认配置
初始化日志。

### 文件 I/O

跨平台只读文件访问，走平台 `io` 能力组。可在 XJ380 等无标准 C 文件 IO 的环境使用。

```c
bapi_file_t bapi_file_open(const char *path);
```
以只读方式打开文件，返回句柄。失败或 `path` 为 `NULL` 返回 `NULL`。

```c
size_t bapi_file_read(bapi_file_t file, void *buffer, size_t size);
```
从当前位置读取最多 `size` 字节，返回实际读取字节数；参数非法返回 0。

```c
int64_t bapi_file_seek(bapi_file_t file, int64_t offset, int origin);
int64_t bapi_file_tell(bapi_file_t file);
int64_t bapi_file_size(bapi_file_t file);
```
`seek` 的 `origin` 与标准 `fseek` 一致（`SEEK_SET`/`SEEK_CUR`/`SEEK_END`）。失败或参数非法返回
`-1`。`tell` 返回当前偏移，`size` 返回文件字节数。

```c
void bapi_file_close(bapi_file_t file);
```
关闭并释放文件句柄。`file` 为 `NULL` 时 no-op。关闭后不得再使用该句柄。

### 版本

```c
const char *bridgeengine_get_version(void);
int bridgeengine_get_version_number(void);
```
`get_version` 返回形如 `"2.0.0"` 的版本字符串。`get_version_number` 返回
`MAJOR*10000 + MINOR*100 + PATCH` 的整数（`2.0.0` → `20000`）。
