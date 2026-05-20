#include "Core/Scene/Prefab.h"
#include "Core/Serialization/JsonArchive.h"
#include "Core/Log.h"
#include <fstream>

namespace fun {

Prefab::Prefab(const std::string& name)
    : m_name(name) {
}

Prefab::~Prefab() {
    delete m_template;
}

Prefab* Prefab::Load(const std::string& path) {
    std::ifstream check(path);
    if (!check.is_open()) {
        FUN_WARN("Prefab file not found: {}", path);
        return nullptr;
    }
    check.close();

    JsonArchive ar = JsonArchive::LoadFromFile(path);

    std::string prefabName;
    ar("name", prefabName);
    if (prefabName.empty()) {
        prefabName = path;
    }

    auto* prefab = new Prefab(prefabName);

    // .prefab 格式与 .scene 相同，但只包含一个根 GameObject
    ar.BeginArray("gameObjects");
    size_t count = ar.ArraySize();
    if (count > 0) {
        ar.PushArrayElement(0);
        prefab->m_template = GameObject::Deserialize(ar);
        ar.Pop();
    }
    ar.EndArray();

    if (!prefab->m_template) {
        FUN_WARN("Failed to deserialize prefab template from: {}", path);
        delete prefab;
        return nullptr;
    }

    return prefab;
}

GameObject* Prefab::Instantiate() {
    if (!m_template) return nullptr;
    return m_template->Clone();
}

bool Prefab::SaveToFile(const std::string& path) {
    if (!m_template) return false;

    JsonArchive ar;
    ar("name", m_name);
    ar.BeginArray("gameObjects");
    ar.PushArrayElement();
    m_template->Serialize(ar);
    ar.Pop();
    ar.EndArray();
    return ar.SaveToFile(path);
}

} // namespace fun
