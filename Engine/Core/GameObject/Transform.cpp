#include "Core/GameObject/Transform.h"
#include "Core/GameObject/GameObject.h"
#include "Core/Serialization/JsonArchive.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace fun {

const Mat4& Transform::GetLocalMatrix() {
    if (m_dirty) {
        Mat4 t = glm::translate(Mat4(1.0f), m_localPosition);
        Mat4 r = glm::toMat4(m_localRotation);
        Mat4 s = glm::scale(Mat4(1.0f), m_localScale);
        m_localMatrix = t * r * s;
    }
    return m_localMatrix;
}

const Mat4& Transform::GetWorldMatrix() {
    if (m_dirty) {
        GetLocalMatrix();
        if (gameObject && gameObject->GetParent()) {
            m_worldMatrix = gameObject->GetParent()->GetTransform()->GetWorldMatrix()
                          * m_localMatrix;
        } else {
            m_worldMatrix = m_localMatrix;
        }
        m_dirty = false;
    }
    return m_worldMatrix;
}

Vec3 Transform::GetForward() const {
    return m_localRotation * Vec3(0.0f, 0.0f, -1.0f);
}

Vec3 Transform::GetRight() const {
    return m_localRotation * Vec3(1.0f, 0.0f, 0.0f);
}

Vec3 Transform::GetUp() const {
    return m_localRotation * Vec3(0.0f, 1.0f, 0.0f);
}

Component* Transform::Clone() const {
    auto* t = new Transform();
    t->m_localPosition = m_localPosition;
    t->m_localRotation = m_localRotation;
    t->m_localScale = m_localScale;
    t->m_dirty = true;
    t->enabled = enabled;
    return t;
}

void Transform::OnSerialize(JsonArchive& ar) {
    float px = m_localPosition.x, py = m_localPosition.y, pz = m_localPosition.z;
    float rx = m_localRotation.x, ry = m_localRotation.y, rz = m_localRotation.z, rw = m_localRotation.w;
    float sx = m_localScale.x, sy = m_localScale.y, sz = m_localScale.z;

    float posData[3] = {px, py, pz};
    float rotData[4] = {rx, ry, rz, rw};
    float scaleData[3] = {sx, sy, sz};

    ar.Vec3Serialize("position", posData);
    ar.QuatSerialize("rotation", rotData);
    ar.Vec3Serialize("scale", scaleData);

    if (ar.IsReading()) {
        m_localPosition = Vec3(posData[0], posData[1], posData[2]);
        m_localRotation = Quat(rotData[3], rotData[0], rotData[1], rotData[2]); // w, x, y, z
        m_localScale = Vec3(scaleData[0], scaleData[1], scaleData[2]);
        m_dirty = true;
    }
}

} // namespace fun
