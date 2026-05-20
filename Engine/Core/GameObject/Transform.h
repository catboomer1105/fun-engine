#pragma once
#include "Core/GameObject/Component.h"
#include "Core/Math/MathTypes.h"
#include <glm/gtc/matrix_transform.hpp>

namespace fun {

class JsonArchive;

class Transform : public Component {
public:
    const char* GetTypeName() const override { return "Transform"; }

    // 本地属性
    const Vec3& GetLocalPosition() const { return m_localPosition; }
    void SetLocalPosition(const Vec3& v) { m_localPosition = v; m_dirty = true; }

    const Quat& GetLocalRotation() const { return m_localRotation; }
    void SetLocalRotation(const Quat& q) { m_localRotation = q; m_dirty = true; }

    const Vec3& GetLocalScale() const { return m_localScale; }
    void SetLocalScale(const Vec3& v) { m_localScale = v; m_dirty = true; }

    // 便捷 Setter -- 常用别名
    void SetPosition(const Vec3& v) { SetLocalPosition(v); }
    void SetRotation(const Quat& q) { SetLocalRotation(q); }
    void SetScale(const Vec3& v) { SetLocalScale(v); }

    Vec3 GetPosition() const { return m_localPosition; }
    Quat GetRotation() const { return m_localRotation; }
    Vec3 GetScale() const { return m_localScale; }

    // 矩阵
    const Mat4& GetLocalMatrix();
    const Mat4& GetWorldMatrix();

    // 方向向量
    Vec3 GetForward() const;
    Vec3 GetRight() const;
    Vec3 GetUp() const;

    // 标记 dirty（父级变化时由外部调用）
    void SetDirty() { m_dirty = true; }

    void OnSerialize(JsonArchive& ar) override;

    Component* Clone() const override;

private:
    Vec3 m_localPosition{0.0f, 0.0f, 0.0f};
    Quat m_localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 m_localScale{1.0f, 1.0f, 1.0f};

    Mat4 m_localMatrix{1.0f};
    Mat4 m_worldMatrix{1.0f};
    bool m_dirty = true;
};

} // namespace fun
