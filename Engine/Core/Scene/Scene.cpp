#include "Core/Scene/Scene.h"
#include "Core/Serialization/JsonArchive.h"
#include "Core/Log.h"
#include <algorithm>

namespace fun {

Scene::Scene(const std::string& name)
    : m_name(name) {
}

Scene::~Scene() {
    OnDestroy();
    for (auto* obj : m_rootObjects) {
        delete obj;
    }
    m_rootObjects.clear();
}

GameObject* Scene::CreateGameObject(const std::string& name) {
    auto* obj = new GameObject(name);
    obj->m_scene = this;
    obj->SetInScene(true);
    m_rootObjects.push_back(obj);
    return obj;
}

GameObject* Scene::Find(const std::string& name) const {
    for (auto* root : m_rootObjects) {
        if (auto* found = findRecursive(root, name)) {
            return found;
        }
    }
    return nullptr;
}

std::vector<GameObject*> Scene::FindByTag(const std::string& tag) const {
    std::vector<GameObject*> result;
    for (auto* root : m_rootObjects) {
        findByTagRecursive(root, tag, result);
    }
    return result;
}

void Scene::Destroy(GameObject* obj) {
    if (!obj) return;

    // 从根级列表移除
    auto it = std::find(m_rootObjects.begin(), m_rootObjects.end(), obj);
    if (it != m_rootObjects.end()) {
        m_rootObjects.erase(it);
    }

    // 从父级移除（如果有）
    if (obj->GetParent()) {
        auto& siblings = const_cast<std::vector<GameObject*>&>(obj->GetParent()->GetChildren());
        siblings.erase(std::remove(siblings.begin(), siblings.end(), obj), siblings.end());
    }

    delete obj;
}

void Scene::ForEach(const std::function<void(GameObject*)>& callback) {
    for (auto* root : m_rootObjects) {
        forEachRecursive(root, callback);
    }
}

void Scene::RemoveFromRoot(GameObject* obj) {
    auto it = std::find(m_rootObjects.begin(), m_rootObjects.end(), obj);
    if (it != m_rootObjects.end()) {
        m_rootObjects.erase(it);
    }
}

void Scene::Serialize(JsonArchive& ar) {
    ar("name", m_name);
    ar.BeginArray("gameObjects");
    if (ar.IsWriting()) {
        for (auto* obj : m_rootObjects) {
            ar.PushArrayElement();
            obj->Serialize(ar);
            ar.Pop();
        }
    }
    ar.EndArray();
}

void Scene::Deserialize(JsonArchive& ar) {
    ar("name", m_name);

    // 清理现有对象
    for (auto* obj : m_rootObjects) {
        delete obj;
    }
    m_rootObjects.clear();

    ar.BeginArray("gameObjects");
    size_t count = ar.ArraySize();
    for (size_t i = 0; i < count; ++i) {
        ar.PushArrayElement(i);
        GameObject* obj = GameObject::Deserialize(ar);
        if (obj) {
            obj->m_scene = this;
            obj->SetInScene(true);
            m_rootObjects.push_back(obj);
        }
        ar.Pop();
    }
    ar.EndArray();

    m_loaded = true;
}

void Scene::OnStart() {
    for (auto* root : m_rootObjects) {
        forEachRecursive(root, [](GameObject* obj) {
            obj->SetInScene(true);
        });
    }
    m_loaded = true;
}

void Scene::OnUpdate(float dt) {
    for (auto* root : m_rootObjects) {
        root->Update(dt);
    }
}

bool Scene::SaveToFile(const std::string& path) {
    JsonArchive ar;
    Serialize(ar);
    return ar.SaveToFile(path);
}

Scene* Scene::LoadFromFile(const std::string& path) {
    JsonArchive ar = JsonArchive::LoadFromFile(path);
    if (!ar.IsReading()) {
        FUN_WARN("Failed to load scene from: {}", path);
        return nullptr;
    }

    std::string sceneName;
    ar("name", sceneName);
    if (sceneName.empty()) {
        sceneName = path;
    }

    auto* scene = new Scene(sceneName);
    scene->Deserialize(ar);
    scene->OnStart();
    return scene;
}

void Scene::OnDestroy() {
    for (auto* root : m_rootObjects) {
        forEachRecursive(root, [](GameObject* obj) {
            obj->Destroy();
        });
    }
}

void Scene::destroyRecursive(GameObject* obj) {
    for (auto* child : obj->GetChildren()) {
        destroyRecursive(child);
    }
    delete obj;
}

void Scene::forEachRecursive(GameObject* obj, const std::function<void(GameObject*)>& callback) {
    callback(obj);
    for (auto* child : obj->GetChildren()) {
        forEachRecursive(child, callback);
    }
}

GameObject* Scene::findRecursive(GameObject* obj, const std::string& name) const {
    if (obj->GetName() == name) {
        return obj;
    }
    for (auto* child : obj->GetChildren()) {
        if (auto* found = findRecursive(child, name)) {
            return found;
        }
    }
    return nullptr;
}

void Scene::findByTagRecursive(GameObject* obj, const std::string& tag, std::vector<GameObject*>& out) const {
    if (obj->GetTag() == tag) {
        out.push_back(obj);
    }
    for (auto* child : obj->GetChildren()) {
        findByTagRecursive(child, tag, out);
    }
}

} // namespace fun
