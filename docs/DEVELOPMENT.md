# BridgeEngine 开发准则

这些是写代码时必须遵守的规范。改哪都可以，别破这些。

## 公有 API

### 追加速度
BridgeEngine 2.0 起公开 API 只能追加，不能改删。下一条是合法的：

```c
// 先在 src/ 的实现里写好，再来 BridgeEngine.h 声明
int bapi_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3);
```

下面这些是非法的：
- 改了现有函数（签名变了、返回值类型变了、参数多了或少了）
- 改了枚举值的数字（删掉中间一个值导致后面的值往前挪也不行）
- 删了旧的类型或宏

要废掉一个函数也留给下个大版本再说，v2 里不改它。

### 类型体系
公开类型只分两类：
- **不透明指针**：`bapi_window_t`、`bapi_texture_t` 这些，结构体定义在 `src/internal/` 里，外部永远看不到
- **公开结构体**：`bapi_rect_t`、`bapi_color_t`、`bapi_event_t` 这些，结构体定义在 `BridgeEngine.h` 公开暴露，字段一旦定了就不能改

新加类型先想清楚归哪类。不透明类型永远好处更大，除非真的需要外部直接访问字段。

### 返回值约定
- 创建/加载类返回 NULL 表示失败
- 操作/状态类返回 0 成功、非零失败
- 没返回值的函数是 safe-command（什么时候调都不会崩）
- 允许传 NULL 给指针参数的情况要在头文件注释里写清楚

## 平台层

### 适配器分隔

`src/internal/platform/platform.h` 里的 `plat_interface_t` 是平台层的唯一进出口。按能力分组建，不要搞扁平函数指针：

```
core    → 初始化、关闭、计时、日志
window  → 窗口管理、事件轮询、鼠标
renderer→ 渲染命令
texture → 纹理加载/创建
text    → 字体、文字渲染
audio   → 音频设备、WAV
sync    → 互斥锁
io      → 文件读写
```

新加平台能力时按以下顺序来：
1. 在 `platform_types.h` 里声明类型
2. 在 `platform.h` 里扩 vtable 结构体
3. SDL3 后端实现
4. XJ380 后端实现（哪怕只是 no-op）
5. 业务代码通过 `plat_get()` 调

### XJ380 侧的铁律

XJ380 是 freestanding 环境：
- 没有 `<math.h>`（sinf/cosf/sqrtf 是来自 XAPI 的 baremetal 实现）
- 没有标准 `<time.h>`（时间用 `xapi_GetTime()`）
- 没有 C 标准库的文件 I/O
- 不支持音频和视频

XJ380 代码路径里：
- 别引标准 C 的 `<math.h>`
- `bapi_audio_init()` 和 `bapi_video_init()` 返回非零
- 不支持的媒体加载返回 NULL，加一条 platform warning 说明原因
- 值定义写进 `CMakeLists.txt` 的 `XJ380_COMMON_FLAGS` 里，别硬编码到每个 .c 文件

### capability 的设计原则

平台能力通过 `plat_interface_t.capabilities` 位图声明，`plat_supports()` 查询。不要去公开头里暴露 capability 查询 API——调用方检查已有返回值就能知道平台支不支持音视频。

## 内存与资源

### 分配规则
- `malloc` 之后立刻判 NULL
- `free` 之前允许传 NULL（C 标准保证了 free(NULL) 是安全的）
- 不要用 VLA，它在 C11 里是 optional 的，MSVC 就不支持
- 静态数组长度酌情用 `#define`，别写魔法数字

### 资源生命周期

引擎退出释放顺序（写死在 `render_context.c` 的 `bapi_runtime_stop()` 里）：

```
视频资源 cleanup
→ 音频资源 cleanup  
→ 纹理资源 cleanup
→ 文字资源 cleanup
→ TTF 库关闭
→ renderer 销毁
→ window 销毁
→ 平台关闭
```

不能改这个顺序。`lifecycle_test.c` 会在 mock 环境下验证。退出的清理包括引擎追踪的资源，外部如果已经自己释放了就不会再释放（比如 audio 里从 allocated_sounds 链表摘掉了）。

### 资源追踪

内部有追踪的资源要通过内部链表管理（参考 `texture.c` 的 cache、`audio.c` 的 allocated/active）。增删节点时：
```c
// 从链表移除后把 next 置 NULL
sound->next_active = NULL;
```
忘记置 NULL 会导致二次释放或悬空引用。

## 编译与平台

### 编译器旗帜
- GCC/Clang：`-Wall -Wextra`（CMakeLists 里统一设置，别在源码或子目录里重复加）
- MSVC：`/W4` + `_CRT_SECURE_NO_WARNINGS`
- ASAN：通过 `-DBRIDGEENGINE_ENABLE_ASAN=ON` 开启，不是默认的

### MSVC 特有注意
- `/W4` 会在隐式转换上炸：float 不能接收 int 值不警告，Uint64 赋值给 uint32_t 必须显式 cast
- 平台 Windows 系统库（opengl32、gdi32、user32、kernel32、shell32）只在 WIN32 分支链接

### XJ380 编译
- 新增 C 源文件必须同步更新 `XJ380_C_SOURCES`，不然 XJ380 打包会缺符号
- C 文件用 `xxcc`（Clang 前端），C++ 文件用 `xxc++`
- 编译参数写在 `XJ380_COMMON_FLAGS` 里，统一管理

## 测试

### 测试结构
- 每个 test 是一个独立可执行文件，编译自己的 `tests/xxx.c` + 需要的 `src/xxx.c`
- 不链接完整库，这叫 self-contained 测试
- 在 CMakeLists.txt 的 `BRIDGEENGINE_BUILD_TESTS` 分支里用 `add_executable` + `add_test` 注册

### Mock 方法
参考 `tests/lifecycle_test.c`：
- 在 test 文件里定义假的平台 struct（`struct plat_window { int unused; }`）
- 定义假的计数器和 flag
- 用自己的函数指针覆盖平台借口的字段
- 框架用 `expect(condition, "message")` 不要用 assert（assert 在 Release 下会被编译掉）

### 命名和运行
- test name 格式：`bridgeengine.<topic>`（如 `bridgeengine.lifecycle`）
- CMake target 和 ctest name 必须一致
- 主函数返回 0 → 通过，1 → 失败

## 文档

- API 变更更新 `docs/bapi.md`
- 架构变更更新 `docs/architecture.md`
- 纯内部重构不用更新文档
- 只记录"做了什么"和"为什么这样做"，不重复 API 签名（会漂移）

## 版本与引用

### 分支
- `master`：稳定版本，总可直接发布
- `dev_xxx`：开发者自己的分支，PR 合入 master 前先自己验证

### 引用内容
- 写代码引用的实际上是头文件的具体位置和函数签名，记得写 `file:line`
- 改的是内部逻辑，不用在全代码库搜引用；改的是公开 API，要把所有公开引用位置找出来

## 附加约束

- 不做公开 ABI 兼容——recompile 就行
- 不要为了"整洁"把 C 代码改成 C++ 的写法（`#pragma once` 除外，那已经是惯例了）
- 平台接口只走 vtable，不要直接 `extern` 声明平台函数
- C 没有 namespace，内部函数加 `static`，不要只写 `extern` 偷懒
- 别在 CI 里加依赖外部网络步骤到关键路径上——SDK/FFmpeg 的下载要做 caching
