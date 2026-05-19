#pragma once
#include "Core/GameObject/Component.h"
#include "Core/GameObject/Transform.h"
#include "Core/Serialization/JsonArchive.h"
#include <string>
#include <vector>
#include <algorithm>

namespace fun {

class GameObject {
public:
    explicit GameObject(const std::string& name);
    ~GameObject();

    // 禁止拷贝
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    // 基本属性
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    Transform* GetTransform() const { return m_transform; }
    GameObject* GetParent() const { return m_parent; }
    const std::vector<GameObject*>& GetChildren() const { return m_children; }

    bool IsActive() const { return m_active; }
    void SetActive(bool active);

    bool IsInScene() const { return m_inScene; }
    void SetInScene(bool inScene);

    // 组件操作
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        T* comp = new T(std::forward<Args>(args)...);
        comp->gameObject = this;
        m_components.push_back(comp);
        if (m_inScene) {
            comp->OnStart();
        }
        return comp;
    }

    template<typename T>
    T* GetComponent() {
        for (auto* c : m_components) {
            if (auto* t = dynamic_cast<T*>(c)) {
                return t;
            }
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() {
        std::vector<T*> result;
        for (auto* c : m_components) {
            if (auto* t = dynamic_cast<T*>(c)) {
                result.push_back(t);
            }
        }
        return result;
    }

    template<typename T>
    void RemoveComponent() {
        auto it = std::find_if(m_components.begin(), m_components.end(),
            [](Component* c) { return dynamic_cast<T*>(c) != nullptr; });
        if (it != m_components.end()) {
            (*it)->OnDestroy();
            delete *it;
            m_components.erase(it);
        }
    }

    const std::vector<Component*>& GetComponents() const { return m_components; }

    // 层级
    void SetParent(GameObject* parent);

    // 销毁
    void Destroy();

    // 序列化
    void Serialize(JsonArchive& ar);
    static GameObject* Deserialize(JsonArchive& ar);

    // 更新
    void Update(float dt);

private:
    std::string m_name;
    Transform* m_transform;
    std::vector<Component*> m_components;
    GameObject* m_parent = nullptr;
    std::vector<GameObject*> m_children;
    bool m_active = true;
    bool m_inScene = false;
};

} // namespace fun
