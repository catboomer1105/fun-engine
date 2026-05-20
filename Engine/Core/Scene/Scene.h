#pragma once
#include <string>
#include <vector>
#include <functional>
#include "Core/GameObject/GameObject.h"

namespace fun {

class JsonArchive;

class Scene {
public:
    explicit Scene(const std::string& name);
    ~Scene();

    const std::string& GetName() const { return m_name; }
    bool IsLoaded() const { return m_loaded; }

    // GameObject 管理
    GameObject* CreateGameObject(const std::string& name);
    GameObject* Find(const std::string& name) const;
    std::vector<GameObject*> FindByTag(const std::string& tag) const;
    void Destroy(GameObject* obj);

    // 遍历所有 GameObject（递归，含子对象）
    void ForEach(const std::function<void(GameObject*)>& callback);

    // 获取根级对象列表
    const std::vector<GameObject*>& GetRootObjects() const { return m_rootObjects; }

    // 将对象从根级列表移除（由 GameObject::SetParent 触发）
    void RemoveFromRoot(GameObject* obj);

    // 序列化
    void Serialize(JsonArchive& ar);
    void Deserialize(JsonArchive& ar);

    // 便捷文件 I/O（编辑器工作流）
    bool SaveToFile(const std::string& path);
    static Scene* LoadFromFile(const std::string& path);

    // 生命周期
    void OnStart();
    void OnUpdate(float dt);
    void OnDestroy();

private:
    std::string m_name;
    std::vector<GameObject*> m_rootObjects;
    bool m_loaded = false;

    void destroyRecursive(GameObject* obj);
    void forEachRecursive(GameObject* obj, const std::function<void(GameObject*)>& callback);
    GameObject* findRecursive(GameObject* obj, const std::string& name) const;
    void findByTagRecursive(GameObject* obj, const std::string& tag, std::vector<GameObject*>& out) const;
};

} // namespace fun
