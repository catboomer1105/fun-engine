#pragma once
#include <string>
#include "Core/GameObject/GameObject.h"

namespace fun {

class Prefab {
public:
    static Prefab* Load(const std::string& path);

    const std::string& GetName() const { return m_name; }

    // 实例化：深拷贝模板，返回独立 GameObject（根级）
    GameObject* Instantiate();

    // 保存为 .prefab 文件（编辑器工作流）
    bool SaveToFile(const std::string& path);

    ~Prefab();

private:
    std::string m_name;
    GameObject* m_template = nullptr;

    explicit Prefab(const std::string& name);
};

} // namespace fun
