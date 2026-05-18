#pragma once
#include "Log.h"
#include <chrono>

namespace fun {

class Engine {
public:
    Engine(int argc, char** argv) {
        Log::Init();
        FUN_INFO("FunEngine v0.1.0 Phase 1");
        m_platformInit();
        m_running = true;
        m_lastFrame = std::chrono::high_resolution_clock::now();
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
