#include "Core/Serialization/JsonArchive.h"
#include "Core/Math/MathTypes.h"

namespace fun {

JsonArchive::JsonArchive()
    : m_reading(false) {
    m_root = nlohmann::json::object();
    m_stack.push_back(&m_root);
}

JsonArchive::JsonArchive(const std::string& jsonStr)
    : m_reading(true) {
    m_root = nlohmann::json::parse(jsonStr, nullptr, false);
    if (m_root.is_discarded()) {
        m_root = nlohmann::json::object();
    }
    m_stack.push_back(&m_root);
}

nlohmann::json* JsonArchive::CurrentNode() {
    return m_stack.back();
}

// ── 基本类型 ──────────────────────────────────────

void JsonArchive::operator()(const std::string& key, int& value) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_number_integer()) {
                value = it->get<int>();
            }
        }
    } else {
        (*CurrentNode())[key] = value;
    }
}

void JsonArchive::operator()(const std::string& key, float& value) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_number()) {
                value = it->get<float>();
            }
        }
    } else {
        (*CurrentNode())[key] = value;
    }
}

void JsonArchive::operator()(const std::string& key, double& value) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_number()) {
                value = it->get<double>();
            }
        }
    } else {
        (*CurrentNode())[key] = value;
    }
}

void JsonArchive::operator()(const std::string& key, bool& value) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_boolean()) {
                value = it->get<bool>();
            }
        }
    } else {
        (*CurrentNode())[key] = value;
    }
}

void JsonArchive::operator()(const std::string& key, std::string& value) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_string()) {
                value = it->get<std::string>();
            }
        }
    } else {
        (*CurrentNode())[key] = value;
    }
}

// ── GLM 类型序列化 ────────────────────────────────

void JsonArchive::operator()(const std::string& key, float* data, int count) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_array() && it->size() == static_cast<size_t>(count)) {
                for (int i = 0; i < count; ++i) {
                    data[i] = (*it)[i].get<float>();
                }
            }
        }
    } else {
        nlohmann::json arr = nlohmann::json::array();
        for (int i = 0; i < count; ++i) {
            arr.push_back(data[i]);
        }
        (*CurrentNode())[key] = arr;
    }
}

void JsonArchive::Vec3Serialize(const std::string& key, float* data) {
    operator()(key, data, 3);
}

void JsonArchive::QuatSerialize(const std::string& key, float* data) {
    operator()(key, data, 4);
}

// ── 嵌套对象 ──────────────────────────────────────

void JsonArchive::Push(const std::string& key) {
    if (m_reading) {
        auto it = CurrentNode()->find(key);
        if (it != CurrentNode()->end() && it->is_object()) {
            m_stack.push_back(&(*it));
        } else {
            static nlohmann::json s_empty = nlohmann::json::object();
            m_stack.push_back(&s_empty);
        }
    } else {
        auto& node = (*CurrentNode())[key];
        if (!node.is_object()) {
            node = nlohmann::json::object();
        }
        m_stack.push_back(&node);
    }
}

void JsonArchive::Pop() {
    if (m_stack.size() > 1) {
        m_stack.pop_back();
    }
}

// ── 数组 ──────────────────────────────────────────

void JsonArchive::BeginArray(const std::string& key) {
    if (m_reading) {
        if (CurrentNode()->is_object()) {
            auto it = CurrentNode()->find(key);
            if (it != CurrentNode()->end() && it->is_array()) {
                m_stack.push_back(&(*it));
            } else {
                static nlohmann::json s_emptyArray = nlohmann::json::array();
                m_stack.push_back(&s_emptyArray);
            }
        }
    } else {
        auto& node = (*CurrentNode())[key];
        if (!node.is_array()) {
            node = nlohmann::json::array();
        }
        m_stack.push_back(&node);
    }
}

void JsonArchive::EndArray() {
    if (m_stack.size() > 1) {
        m_stack.pop_back();
    }
}

size_t JsonArchive::ArraySize() const {
    auto* node = m_stack.back();
    return node->is_array() ? node->size() : 0;
}

// 写模式：在当前数组中追加一个新对象并压栈
// 读模式：不支持（用 PushArrayElement(index) 代替）
void JsonArchive::PushArrayElement() {
    if (m_reading) {
        // 读模式不应该调用这个
        static nlohmann::json s_empty = nlohmann::json::object();
        m_stack.push_back(&s_empty);
    } else {
        CurrentNode()->push_back(nlohmann::json::object());
        m_stack.push_back(&CurrentNode()->back());
    }
}

// 读模式：进入数组的第 index 个元素（对象）
void JsonArchive::PushArrayElement(size_t index) {
    if (m_reading) {
        auto* arr = CurrentNode();
        if (arr->is_array() && index < arr->size() && (*arr)[index].is_object()) {
            m_stack.push_back(&(*arr)[index]);
        } else {
            static nlohmann::json s_empty = nlohmann::json::object();
            m_stack.push_back(&s_empty);
        }
    } else {
        // 写模式不应该调用这个
        CurrentNode()->push_back(nlohmann::json::object());
        m_stack.push_back(&CurrentNode()->back());
    }
}

void JsonArchive::ArrayElement(size_t index, int& value) {
    auto* node = m_stack.back();
    if (node->is_array() && index < node->size() && (*node)[index].is_number_integer()) {
        value = (*node)[index].get<int>();
    }
}

void JsonArchive::ArrayElement(size_t index, float& value) {
    auto* node = m_stack.back();
    if (node->is_array() && index < node->size() && (*node)[index].is_number()) {
        value = (*node)[index].get<float>();
    }
}

void JsonArchive::ArrayElement(size_t index, double& value) {
    auto* node = m_stack.back();
    if (node->is_array() && index < node->size() && (*node)[index].is_number()) {
        value = (*node)[index].get<double>();
    }
}

void JsonArchive::ArrayElement(size_t index, bool& value) {
    auto* node = m_stack.back();
    if (node->is_array() && index < node->size() && (*node)[index].is_boolean()) {
        value = (*node)[index].get<bool>();
    }
}

void JsonArchive::ArrayElement(size_t index, std::string& value) {
    auto* node = m_stack.back();
    if (node->is_array() && index < node->size() && (*node)[index].is_string()) {
        value = (*node)[index].get<std::string>();
    }
}

// ── 输出 ──────────────────────────────────────────

std::string JsonArchive::ToString() const {
    return m_root.dump(2);
}

} // namespace fun
