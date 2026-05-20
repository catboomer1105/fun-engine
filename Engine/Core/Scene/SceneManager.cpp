#include "Core/Scene/SceneManager.h"
#include "Core/Serialization/JsonArchive.h"
#include "Core/Engine.h"
#include "Core/Log.h"
#include <fstream>

namespace fun {

SceneManager::~SceneManager() {
    for (auto& [name, scene] : m_loadedScenes) {
        delete scene;
    }
    m_loadedScenes.clear();
    m_activeScene = nullptr;
}

Scene* SceneManager::LoadScene(const std::string& path) {
    // 先检查文件是否存在
    std::ifstream check(path);
    if (!check.is_open()) {
        FUN_WARN("Scene file not found: {}", path);
        return nullptr;
    }
    check.close();

    JsonArchive ar = JsonArchive::LoadFromFile(path);

    std::string sceneName;
    ar("name", sceneName);
    if (sceneName.empty()) {
        sceneName = path;
    }

    auto* scene = new Scene(sceneName);
    scene->Deserialize(ar);
    scene->OnStart();

    m_loadedScenes[sceneName] = scene;

    // 如果是第一个加载的场景，自动设为活动场景
    if (!m_activeScene) {
        m_activeScene = scene;
    }

    emitSceneEvent("SceneLoaded", scene);
    return scene;
}

void SceneManager::UnloadScene(const std::string& name) {
    auto it = m_loadedScenes.find(name);
    if (it == m_loadedScenes.end()) return;

    Scene* scene = it->second;

    if (m_activeScene == scene) {
        m_activeScene = nullptr;
    }

    emitSceneEvent("SceneUnloaded", scene);

    delete scene;
    m_loadedScenes.erase(it);
}

void SceneManager::SetActiveScene(const std::string& name) {
    auto it = m_loadedScenes.find(name);
    if (it != m_loadedScenes.end()) {
        m_activeScene = it->second;
    }
}

Scene* SceneManager::GetScene(const std::string& name) const {
    auto it = m_loadedScenes.find(name);
    if (it != m_loadedScenes.end()) {
        return it->second;
    }
    return nullptr;
}

void SceneManager::emitSceneEvent(const std::string& type, Scene* scene) {
    if (Engine::GetInstance()) {
        Event event(type);
        event.Set("scene", scene);
        Engine::GetInstance()->GetEventBus().Emit(event);
    }
}

} // namespace fun
