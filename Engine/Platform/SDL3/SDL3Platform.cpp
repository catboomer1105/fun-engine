#include "Core/Engine.h"
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

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
    g_window = SDL_CreateWindow("FunEngine -- Phase 1", 1280, 720, 0);
    FUN_INFO("SDL3 window created: 1280x720");

    // bgfx 初始化
    bgfx::Init init;
    init.type     = bgfx::RendererType::Count;  // 自动选择 (D3D11 > D3D12 > Vulkan > OpenGL)
    init.resolution.width  = 1280;
    init.resolution.height = 720;
    init.resolution.reset  = BGFX_RESET_VSYNC;

    init.platformData.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(g_window),
                                                    SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    FUN_ASSERT(init.platformData.nwh != nullptr, "Failed to get native window handle (HWND) from SDL");

    FUN_ASSERT(bgfx::init(init), "bgfx::init() failed");
    FUN_INFO("bgfx initialized: {}", bgfx::getRendererName(bgfx::getRendererType()));

    bgfx::setDebug(BGFX_DEBUG_TEXT);

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

    // 创建顶点缓冲（每帧重建，简单但低效 -- Phase 4 再优化）
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, 3, s_layout);
    bx::memCopy(tvb.data, s_triangleVertices, sizeof(s_triangleVertices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::submit(0, g_program);
    bgfx::frame();
}
