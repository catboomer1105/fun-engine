#include "Core/Log.h"
#include <Core/Engine.h>
#include <Core/Scene/Scene.h>
#include <Core/Scene/SceneManager.h>
#include <Core/Scene/Prefab.h>
#include <Core/GameObject/GameObject.h>
#include <Core/GameObject/Transform.h>
#include <Core/GameObject/Component.h>
#include <Core/Serialization/JsonArchive.h>
#include <glm/gtc/quaternion.hpp>

using namespace fun;

// ── 自定义组件：让对象每帧绕 Y 轴旋转 ──
class Rotator : public Component {
public:
    const char* GetTypeName() const override { return "Rotator"; }

    float speed = 90.0f;         // 度/秒
    float logTimer = 0.0f;       // 日志输出间隔

    void OnUpdate(float dt) override {
        logTimer += dt;
        auto* t = GetTransform();
        if (!t) return;

        float angle = glm::radians(speed * dt);
        Quat rot = glm::angleAxis(angle, Vec3(0, 1, 0));
        t->SetLocalRotation(t->GetLocalRotation() * rot);

        if (logTimer >= 1.0f) {
            logTimer = 0.0f;
            auto euler = glm::eulerAngles(t->GetLocalRotation());
            FUN_INFO("  {}  y={:.1f}deg",
                gameObject->GetName(), glm::degrees(euler.y));
        }
    }

    void OnSerialize(JsonArchive& ar) override { ar("speed", speed); }
    Component* Clone() const override {
        auto* c = new Rotator();
        c->speed = speed;
        return c;
    }
};

// 注册组件
struct RotatorRegistrar {
    RotatorRegistrar() { ComponentRegistry::Get().Register<Rotator>("Rotator"); }
};
static RotatorRegistrar s_rot;

int main(int argc, char** argv) {
    Engine engine(argc, argv);

    // ── 创建场景并添加对象 ──
    Scene scene("RuntimeDemo");

    auto* staticObj = scene.CreateGameObject("StaticBlock");
    staticObj->GetTransform()->SetLocalPosition(Vec3(0, 0, 0));

    auto* spinning = scene.CreateGameObject("SpinningCube");
    spinning->GetTransform()->SetLocalPosition(Vec3(5, 0, 0));
    spinning->AddComponent<Rotator>()->speed = 45.0f;

    auto* fast = scene.CreateGameObject("FastSpinner");
    fast->GetTransform()->SetLocalPosition(Vec3(10, 0, 0));
    fast->AddComponent<Rotator>()->speed = 180.0f;

    FUN_INFO("=== {} objects ready ===", scene.GetRootObjects().size());

    scene.SaveToFile("Assets/RuntimeDemo.scene");
    engine.GetSceneManager().LoadScene("Assets/RuntimeDemo.scene");
    FUN_INFO("=== Scene loaded, starting update loop... ===");

    engine.Run();
    return 0;
}
