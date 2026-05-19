#pragma once
#include <string>
#include <functional>
#include <unordered_map>

namespace fun {

class GameObject;
class JsonArchive;

class Component {
public:
    virtual ~Component() = default;

    GameObject* gameObject = nullptr;
    bool enabled = true;

    GameObject* GetGameObject() const { return gameObject; }
    class Transform* GetTransform() const;

    virtual const char* GetTypeName() const = 0;

    // 生命周期
    virtual void OnStart() {}
    virtual void OnUpdate(float dt) { (void)dt; }
    virtual void OnDestroy() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}

    // 序列化 / Inspector / Lua 绑定
    virtual void OnSerialize(JsonArchive& ar) { (void)ar; }
    virtual void OnInspector() {}
    virtual void OnBindLua() {}
};

// Component 类型工厂：从类型名字符串创建 Component 实例
using ComponentFactory = std::function<Component*()>;

class ComponentRegistry {
public:
    static ComponentRegistry& Get();

    template<typename T>
    void Register(const std::string& name) {
        m_factories[name] = []() -> Component* { return new T(); };
    }

    Component* Create(const std::string& name) const {
        auto it = m_factories.find(name);
        if (it != m_factories.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, ComponentFactory> m_factories;
};

} // namespace fun
