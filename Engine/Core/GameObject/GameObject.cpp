#include "Core/GameObject/GameObject.h"
#include "Core/GameObject/Component.h"

namespace fun {

// Component::GetTransform 实现（需要 GameObject 完整定义）
Transform* Component::GetTransform() const {
    return gameObject ? gameObject->GetTransform() : nullptr;
}

// ComponentRegistry 单例
ComponentRegistry& ComponentRegistry::Get() {
    static ComponentRegistry s_instance;
    return s_instance;
}

GameObject::GameObject(const std::string& name)
    : m_name(name) {
    m_transform = new Transform();
    m_transform->gameObject = this;
}

GameObject::~GameObject() {
    // 子对象的析构函数不应再反向修改父级的 children 列表
    // 先清空子对象的 parent 指针，防止子析构时访问已毁的父
    for (auto* child : m_children) {
        child->m_parent = nullptr;
    }

    // 销毁子对象
    for (auto* child : m_children) {
        delete child;
    }
    m_children.clear();

    // 销毁组件（Transform 最后）
    for (auto* comp : m_components) {
        comp->OnDestroy();
        delete comp;
    }
    m_components.clear();

    // Transform 不在 m_components 中，单独删除
    if (m_transform) {
        m_transform->OnDestroy();
        delete m_transform;
        m_transform = nullptr;
    }

    // 从父级移除
    if (m_parent) {
        auto& siblings = m_parent->m_children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }
}

void GameObject::SetActive(bool active) {
    if (m_active != active) {
        m_active = active;
        for (auto* comp : m_components) {
            if (active) comp->OnEnable();
            else comp->OnDisable();
        }
        if (m_transform) {
            if (active) m_transform->OnEnable();
            else m_transform->OnDisable();
        }
    }
}

void GameObject::SetInScene(bool inScene) {
    if (m_inScene == inScene) return;
    m_inScene = inScene;
    if (inScene) {
        if (m_transform) m_transform->OnStart();
        for (auto* comp : m_components) {
            comp->OnStart();
        }
    }
}

void GameObject::SetParent(GameObject* parent) {
    // 从旧父级移除
    if (m_parent) {
        auto& siblings = m_parent->m_children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    m_parent = parent;

    // 加入新父级
    if (m_parent) {
        m_parent->m_children.push_back(this);
    }

    // 标记 Transform dirty
    if (m_transform) {
        m_transform->SetDirty();
    }
}

void GameObject::Destroy() {
    // 递归销毁子对象
    for (auto* child : m_children) {
        child->Destroy();
    }

    // 通知所有组件
    for (auto* comp : m_components) {
        comp->OnDestroy();
    }
    if (m_transform) {
        m_transform->OnDestroy();
    }
}

void GameObject::Update(float dt) {
    if (!m_active) return;

    // 更新 Transform
    if (m_transform && m_transform->enabled) {
        m_transform->OnUpdate(dt);
    }

    // 更新所有组件
    for (auto* comp : m_components) {
        if (comp->enabled) {
            comp->OnUpdate(dt);
        }
    }

    // 递归更新子对象
    for (auto* child : m_children) {
        child->Update(dt);
    }
}

void GameObject::Serialize(JsonArchive& ar) {
    ar("name", m_name);
    ar("active", m_active);

    // 序列化 Transform
    ar.Push("transform");
    m_transform->OnSerialize(ar);
    ar.Pop();

    // 序列化组件
    ar.BeginArray("components");
    if (ar.IsWriting()) {
        for (auto* comp : m_components) {
            ar.PushArrayElement();
            std::string typeName = comp->GetTypeName();
            ar("type", typeName);
            comp->OnSerialize(ar);
            ar.Pop();
        }
    }
    ar.EndArray();

    // 序列化子对象
    ar.BeginArray("children");
    if (ar.IsWriting()) {
        for (auto* child : m_children) {
            ar.PushArrayElement();
            child->Serialize(ar);
            ar.Pop();
        }
    }
    ar.EndArray();
}

GameObject* GameObject::Deserialize(JsonArchive& ar) {
    std::string name;
    ar("name", name);

    auto* obj = new GameObject(name);

    bool active = true;
    ar("active", active);
    obj->SetActive(active);

    // 反序列化 Transform
    ar.Push("transform");
    obj->m_transform->OnSerialize(ar);
    ar.Pop();

    // 反序列化组件
    ar.BeginArray("components");
    size_t compCount = ar.ArraySize();
    for (size_t i = 0; i < compCount; ++i) {
        ar.PushArrayElement(i);
        std::string typeName;
        ar("type", typeName);
        Component* comp = ComponentRegistry::Get().Create(typeName);
        if (comp) {
            comp->gameObject = obj;
            comp->OnSerialize(ar);
            obj->m_components.push_back(comp);
        }
        ar.Pop();
    }
    ar.EndArray();

    // 反序列化子对象
    ar.BeginArray("children");
    size_t childCount = ar.ArraySize();
    for (size_t i = 0; i < childCount; ++i) {
        ar.PushArrayElement(i);
        GameObject* child = Deserialize(ar);
        if (child) {
            child->SetParent(obj);
        }
        ar.Pop();
    }
    ar.EndArray();

    return obj;
}

} // namespace fun
