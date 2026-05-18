# Phase 1 — 基础骨架

**目标：** SDL3 窗口 + bgfx 渲染三角形 + spdlog 日志 + 帧循环  
**涉及层：** 平台层、核心层（Math 部分）

---

## 1. 目录结构

Phase 1 只需以下文件：

```
FunEngine/
├── xmake.lua
├── .gitignore
│
├── Engine/
│   ├── Core/
│   │   ├── Math/
│   │   │   └── MathTypes.h          # glm 类型别名
│   │   ├── Log.h                    # spdlog 封装
│   │   └── Engine.h                 # 引擎初始化 / 关闭 / 主循环入口
│   │
│   └── Platform/
│       └── SDL3/
│           └── SDL3Platform.cpp     # 窗口创建 + bgfx 初始化 + 事件轮询
│
├── Assets/
│   └── Shaders/
│       ├── vs_triangle.sc           # bgfx 顶点着色器
│       ├── fs_triangle.sc           # bgfx 片元着色器
│       └── varying.def.sc          # 着色器输入/输出定义
│
└── Samples/
    └── Sandbox/
        └── main.cpp                 # 程序入口
```

未在 Phase 1 创建的目录（`ClassDB/` `Serialization/` `Event/` `GameObject/` `Resource/` `Function/` `Editor/` `Tests/`）全部留空，不预先创建——按"能删的代码不留着"原则，用到时再建。

---

## 2. xmake.lua

```lua
-- xmake.lua
set_project("FunEngine")
set_version("0.1.0")
set_languages("c++20")

add_rules("mode.debug", "mode.release")

-- Phase 1 只需要 3 个库
add_requires("libsdl", "bgfx", "spdlog")

-- ── 引擎库 ────────────────────────────────────
target("FunEngine")
    set_kind("static")
    add_packages("libsdl", "bgfx", "spdlog")
    add_files("Engine/Core/**.cpp")
    add_files("Engine/Platform/**.cpp")
    add_includedirs("Engine", {public = true})

-- ── 沙盒 ──────────────────────────────────────
target("Sandbox")
    set_kind("binary")
    add_files("Samples/Sandbox/main.cpp")
    add_deps("FunEngine")
```

`xmake build Sandbox` 即可编译运行。

---

## 3. 实现顺序

### 步骤 1：MathTypes.h — glm 别名

**文件：** `Engine/Core/Math/MathTypes.h`

```cpp
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fun {

using Vec2  = glm::vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using Mat3  = glm::mat3;
using Mat4  = glm::mat4;
using Quat  = glm::quat;

} // namespace fun
```

就这些。不做任何包装函数——`glm::normalize(v)` 直接可用。

---

### 步骤 2：Log.h — 日志封装

**文件：** `Engine/Core/Log.h`

```cpp
#pragma once
#include <spdlog/spdlog.h>
#include <memory>

namespace fun {

class Log {
public:
    static void Init() {
        spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::debug);
    }

    static const auto& Get() { return *spdlog::default_logger_raw(); }
};

// 便捷宏
#define FUN_TRACE(...) spdlog::trace(__VA_ARGS__)
#define FUN_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define FUN_INFO(...)  spdlog::info(__VA_ARGS__)
#define FUN_WARN(...)  spdlog::warn(__VA_ARGS__)
#define FUN_ERROR(...) spdlog::error(__VA_ARGS__)

} // namespace fun
```

不需要花哨的分级 logger，一个全局 logger 足够。

---

### 步骤 3：Engine.h — 引擎骨架

**文件：** `Engine/Core/Engine.h`

```cpp
#pragma once
#include "Log.h"

namespace fun {

class Engine {
public:
    Engine(int argc, char** argv) {
        Log::Init();
        FUN_INFO("FunEngine v0.1.0 — Phase 1");
        m_platformInit();
        m_running = true;
    }

    ~Engine() {
        m_platformShutdown();
        FUN_INFO("Engine shutdown complete");
    }

    void Run() {
        while (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;

            m_platformPollEvents();
            m_platformRender(dt);
        }
    }

    bool IsRunning() const { return m_running; }
    void Quit() { m_running = false; }

private:
    bool m_running = false;
    std::chrono::high_resolution_clock::time_point m_lastFrame;

    // 平台层实现（见 SDL3Platform.cpp）
    void m_platformInit();
    void m_platformShutdown();
    void m_platformPollEvents();
    void m_platformRender(float dt);
};

} // namespace fun
```

命名带 `m_` 前缀表示"由平台层实现"，Phase 1 只有 SDL3 一个平台。

---

### 步骤 4：SDL3Platform.cpp — 窗口 + bgfx + 三角形

**文件：** `Engine/Platform/SDL3/SDL3Platform.cpp`

```cpp
#include "Engine/Core/Engine.h"
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

// bgfx 需要嵌入的着色器二进制
#include "vs_triangle.h"    // shaderc 编译 .sc → .h
#include "fs_triangle.h"

namespace {

SDL_Window*   g_window   = nullptr;
bgfx::ProgramHandle g_program = BGFX_INVALID_HANDLE;

// ── 顶点结构 ──
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

PosColorVertex s_triangleVertices[] = {
    {  0.0f,  0.5f, 0.0f, 0xff0000ff },  // 红
    { -0.5f, -0.5f, 0.0f, 0xff00ff00 },  // 绿
    {  0.5f, -0.5f, 0.0f, 0xffff0000 },  // 蓝
};

bgfx::VertexLayout s_layout;

} // anonymous namespace

// ── Engine 的平台实现 ──

void fun::Engine::m_platformInit() {
    // SDL3 窗口
    SDL_Init(SDL_INIT_VIDEO);
    g_window = SDL_CreateWindow("FunEngine — Phase 1", 1280, 720, 0);
    FUN_INFO("SDL3 window created: 1280x720");

    // bgfx 初始化
    bgfx::PlatformData pd = {};
    pd.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(g_window),
                                     SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    bgfx::setPlatformData(pd);
    bgfx::init(bgfx::RendererType::Direct3D11);
    bgfx::reset(1280, 720, BGFX_RESET_VSYNC);
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    FUN_INFO("bgfx initialized: Direct3D11");

    // 顶点布局
    s_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
        .end();

    // 编译着色器程序
    auto vs = bgfx::createShader(bgfx::makeRef(vs_triangle, sizeof(vs_triangle)));
    auto fs = bgfx::createShader(bgfx::makeRef(fs_triangle, sizeof(fs_triangle)));
    g_program = bgfx::createProgram(vs, fs, true);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
}

void fun::Engine::m_platformShutdown() {
    if (bgfx::isValid(g_program))
        bgfx::destroy(g_program);
    bgfx::shutdown();
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void fun::Engine::m_platformPollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            m_running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE)
                m_running = false;
            break;
        }
    }
}

void fun::Engine::m_platformRender(float dt) {
    bgfx::setViewRect(0, 0, 0, 1280, 720);

    // 创建顶点缓冲（每帧重建，简单但低效 — Phase 4 再优化）
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, 3, s_layout);
    bx::memCopy(tvb.data, s_triangleVertices, sizeof(s_triangleVertices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::submit(0, g_program);
    bgfx::frame();
}
```

**关于 bgfx 着色器：** `.sc` 文件用 bgfx 自带的 `shaderc` 编译为 C 头文件：

```bash
shaderc -f vs_triangle.sc -o vs_triangle.h --type vertex --platform windows
shaderc -f fs_triangle.sc -o fs_triangle.h --type fragment --platform windows
```

在 Phase 1 中手动执行一次，将生成的 `.h` 放到 `Assets/Shaders/`。

---

### 步骤 5：bgfx 着色器

**文件：** `Assets/Shaders/varying.def.sc`

```
vec4 v_color0 : COLOR0;

vec3 a_position  : POSITION;
vec4 a_color0    : COLOR0;
```

**文件：** `Assets/Shaders/vs_triangle.sc`

```
$input a_position, a_color0
$output v_color0
#include <bgfx_shader.sh>

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_color0 = a_color0;
}
```

**文件：** `Assets/Shaders/fs_triangle.sc`

```
$input v_color0
#include <bgfx_shader.sh>

void main() {
    gl_FragColor = v_color0;
}
```

---

### 步骤 6：Sandbox main.cpp

**文件：** `Samples/Sandbox/main.cpp`

```cpp
#include <Engine/Core/Engine.h>

int main(int argc, char** argv) {
    fun::Engine engine(argc, argv);
    engine.Run();
    return 0;
}
```

---

## 4. 构建与运行

```bash
# 首次：安装依赖
xmake f -m debug

# 编译着色器（手动，只需要做一次）
cd Assets/Shaders
shaderc -f vs_triangle.sc -o vs_triangle.h --type vertex --platform windows
shaderc -f fs_triangle.sc -o fs_triangle.h --type fragment --platform windows
cd ../..

# 构建
xmake build Sandbox

# 运行
xmake run Sandbox
```

**预期结果：**
- 窗口 1280×720，标题 "FunEngine — Phase 1"
- 灰色背景，中间显示一个彩色三角形
- 按 Esc 或关闭窗口退出
- 控制台输出 `[HH:MM:SS] [info] FunEngine v0.1.0 — Phase 1`

---

## 5. 验收标准

| 检查项 | 标准 |
|--------|------|
| 窗口创建 | 1280×720，标题正确 |
| bgfx 初始化 | 使用 Direct3D11 或 Vulkan 后端 |
| 三角形渲染 | 红绿蓝三色三角形可见 |
| 帧循环 | `dt` 正确计算，控制台可打印验证 |
| 退出 | Esc 或关闭按钮正常退出，无泄漏 |
| 日志 | spdlog 输出格式正确，时间戳 + 级别 |
| 文件数 | 不超过 10 个源文件（不含生成的头文件） |

---

## 6. Phase 1 不做什么

这些是主文档提到的，但 Phase 1 **不做**：

- ❌ 序列化
- ❌ 事件系统
- ❌ GameObject / Component
- ❌ 资源管理
- ❌ 物理 / 动画 / 音频
- ❌ Lua
- ❌ ImGui 编辑器
- ❌ 单元测试

Phase 1 只证明"SDL3 + bgfx + spdlog 能一起工作"。后续每个 Phase 只加一个子系统。
