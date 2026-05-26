#pragma once
#include <string>
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>

namespace fun {

class JsonArchive {
public:
    // 写模式
    JsonArchive();

    // 读模式：从 JSON 字符串构造
    explicit JsonArchive(const std::string& jsonStr);

    bool IsReading() const { return m_reading; }
    bool IsWriting() const { return !m_reading; }

    // 核心操作：ar("key", value)
    void operator()(const std::string& key, int& value);
    void operator()(const std::string& key, float& value);
    void operator()(const std::string& key, double& value);
    void operator()(const std::string& key, bool& value);
    void operator()(const std::string& key, std::string& value);

    // GLM 类型序列化为 JSON 数组
    void operator()(const std::string& key, float* data, int count);
    void Vec3Serialize(const std::string& key, float* data);
    void QuatSerialize(const std::string& key, float* data);

    // 嵌套对象（按 key 进入子对象）
    void Push(const std::string& key);
    void Pop();

    // 数组
    void BeginArray(const std::string& key);
    void EndArray();
    size_t ArraySize() const;

    // 在数组上下文中：写模式追加新对象，读模式按 index 进入
    void PushArrayElement();
    void PushArrayElement(size_t index);

    // 数组元素访问（读模式循环中使用）
    void ArrayElement(size_t index, int& value);
    void ArrayElement(size_t index, float& value);
    void ArrayElement(size_t index, double& value);
    void ArrayElement(size_t index, bool& value);
    void ArrayElement(size_t index, std::string& value);

    // 输出
    std::string ToString() const;
    bool SaveToFile(const std::string& path) const;
    static JsonArchive LoadFromFile(const std::string& path);

private:
    nlohmann::ordered_json* CurrentNode();

    bool m_reading;
    nlohmann::ordered_json m_root;
    std::vector<nlohmann::ordered_json*> m_stack;
};

} // namespace fun
