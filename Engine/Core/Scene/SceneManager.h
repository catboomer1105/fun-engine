#pragma once
#include <string>
#include <unordered_map>
#include "Scene.h"
#include "Core/Event/EventBus.h"

namespace fun {

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    Scene* GetActiveScene() const { return m_activeScene; }

    // 同步加载场景（从 .scene JSON 文件）
    Scene* LoadScene(const std::string& path);

    // 卸载场景（销毁所有 GameObject）
    void UnloadScene(const std::string& name);

    // 切换活动场景
    void SetActiveScene(const std::string& name);

    // 获取已加载的场景
    Scene* GetScene(const std::string& name) const;

private:
    Scene* m_activeScene = nullptr;
    std::unordered_map<std::string, Scene*> m_loadedScenes;

    void emitSceneEvent(const std::string& type, Scene* scene);
};

} // namespace fun
