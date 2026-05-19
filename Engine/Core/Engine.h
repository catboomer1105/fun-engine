#pragma once
#include "Core/Log.h"
#include "Core/Memory/LinearAllocator.h"
#include "Core/Event/EventBus.h"
#include "Core/GameObject/GameObject.h"
#include <chrono>
#include <vector>

namespace fun {

class Engine {
public:
    Engine(int argc, char** argv)
        : m_frameAllocator(1024 * 1024) { // 1MB 帧分配器
        s_instance = this;
        Log::Init();
        FUN_INFO("FunEngine v0.1.0 -- Phase 2");
        m_platformInit();
        m_running = true;
        m_lastFrame = std::chrono::high_resolution_clock::now();
    }

    ~Engine() {
        // 清理根级 GameObject
        for (auto* obj : m_rootObjects) {
            delete obj;
        }
        m_rootObjects.clear();

        m_platformShutdown();
        s_instance = nullptr;
        FUN_INFO("Engine shutdown complete");
    }

    void Run() {
        while (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;

            // 帧分配器每帧重置
            m_frameAllocator.Reset();

            m_platformPollEvents();

            // 更新所有根级 GameObject
            for (auto* obj : m_rootObjects) {
                obj->Update(dt);
            }

            m_platformRender(dt);
        }
    }

    bool IsRunning() const { return m_running; }
    void Quit() { m_running = false; }

    // 核心层访问器
    LinearAllocator& GetFrameAllocator() { return m_frameAllocator; }
    EventBus& GetEventBus() { return m_eventBus; }

    // GameObject 管理（临时方案，Phase 4 由 Scene 替代）
    void AddRootObject(GameObject* obj) {
        m_rootObjects.push_back(obj);
        obj->SetInScene(true);
    }

    void RemoveRootObject(GameObject* obj) {
        auto it = std::find(m_rootObjects.begin(), m_rootObjects.end(), obj);
        if (it != m_rootObjects.end()) {
            (*it)->SetInScene(false);
            m_rootObjects.erase(it);
        }
    }

    // 全局实例访问
    static Engine* GetInstance() { return s_instance; }

private:
    bool m_running = false;
    std::chrono::high_resolution_clock::time_point m_lastFrame;
    LinearAllocator m_frameAllocator;
    EventBus m_eventBus;
    std::vector<GameObject*> m_rootObjects;

    static inline Engine* s_instance = nullptr;

    // 平台层实现（见 SDL3Platform.cpp）
    void m_platformInit();
    void m_platformShutdown();
    void m_platformPollEvents();
    void m_platformRender(float dt);
};

} // namespace fun
