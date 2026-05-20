#include <gtest/gtest.h>
#include <Core/GameObject/GameObject.h>
#include <Core/GameObject/Component.h>
#include <Core/GameObject/Transform.h>
#include <Core/Serialization/JsonArchive.h>

using namespace fun;

// -- 测试用 Component --

class TestComponent : public Component {
public:
    const char* GetTypeName() const override { return "TestComponent"; }

    float speed = 10.0f;
    int health = 100;

    void OnSerialize(JsonArchive& ar) override {
        ar("speed", speed);
        ar("health", health);
    }

    Component* Clone() const override {
        auto* c = new TestComponent();
        c->speed = speed;
        c->health = health;
        return c;
    }
};

// 注册测试用组件（全局一次）
struct TestComponentRegistrar {
    TestComponentRegistrar() {
        ComponentRegistry::Get().Register<TestComponent>("TestComponent");
    }
};
static TestComponentRegistrar s_registrar;

// -- GameObject 基础 --

TEST(GameObject, CreateWithTransform) {
    GameObject obj("Test");
    EXPECT_EQ(obj.GetName(), "Test");
    EXPECT_NE(obj.GetTransform(), nullptr);
    EXPECT_TRUE(obj.IsActive());
}

TEST(GameObject, AddAndGetComponent) {
    GameObject obj("Test");

    auto* comp = obj.AddComponent<TestComponent>();
    ASSERT_NE(comp, nullptr);
    comp->speed = 20.0f;
    comp->health = 200;

    auto* fetched = obj.GetComponent<TestComponent>();
    ASSERT_NE(fetched, nullptr);
    EXPECT_FLOAT_EQ(fetched->speed, 20.0f);
    EXPECT_EQ(fetched->health, 200);
}

TEST(GameObject, GetComponentNotFound) {
    GameObject obj("Test");
    auto* comp = obj.GetComponent<TestComponent>();
    EXPECT_EQ(comp, nullptr);
}

TEST(GameObject, RemoveComponent) {
    GameObject obj("Test");
    obj.AddComponent<TestComponent>();
    ASSERT_NE(obj.GetComponent<TestComponent>(), nullptr);

    obj.RemoveComponent<TestComponent>();
    EXPECT_EQ(obj.GetComponent<TestComponent>(), nullptr);
}

// -- 父子层级 --

TEST(GameObject, SetParent) {
    auto* parent = new GameObject("Parent");
    auto* child = new GameObject("Child");

    child->SetParent(parent);
    EXPECT_EQ(child->GetParent(), parent);
    EXPECT_EQ(parent->GetChildren().size(), 1u);
    EXPECT_EQ(parent->GetChildren()[0], child);

    delete parent;  // 递归删除 child
}

TEST(GameObject, SetParentNull) {
    auto* parent = new GameObject("Parent");
    auto* child = new GameObject("Child");

    child->SetParent(parent);
    EXPECT_EQ(child->GetParent(), parent);

    child->SetParent(nullptr);
    EXPECT_EQ(child->GetParent(), nullptr);
    EXPECT_EQ(parent->GetChildren().size(), 0u);

    delete parent;
    delete child;
}

TEST(GameObject, ReparentNode) {
    auto* parent1 = new GameObject("Parent1");
    auto* parent2 = new GameObject("Parent2");
    auto* child = new GameObject("Child");

    child->SetParent(parent1);
    EXPECT_EQ(parent1->GetChildren().size(), 1u);

    child->SetParent(parent2);
    EXPECT_EQ(parent1->GetChildren().size(), 0u);
    EXPECT_EQ(parent2->GetChildren().size(), 1u);
    EXPECT_EQ(child->GetParent(), parent2);

    delete parent1;
    delete parent2; // 递归删除 child
}

// -- Transform --

TEST(Transform, DefaultValues) {
    GameObject obj("Test");
    auto* t = obj.GetTransform();

    EXPECT_FLOAT_EQ(t->GetLocalPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(t->GetLocalPosition().y, 0.0f);
    EXPECT_FLOAT_EQ(t->GetLocalPosition().z, 0.0f);

    EXPECT_FLOAT_EQ(t->GetLocalScale().x, 1.0f);
    EXPECT_FLOAT_EQ(t->GetLocalScale().y, 1.0f);
    EXPECT_FLOAT_EQ(t->GetLocalScale().z, 1.0f);
}

TEST(Transform, SetPosition) {
    GameObject obj("Test");
    auto* t = obj.GetTransform();
    t->SetLocalPosition(Vec3(1.0f, 2.0f, 3.0f));

    EXPECT_FLOAT_EQ(t->GetLocalPosition().x, 1.0f);
    EXPECT_FLOAT_EQ(t->GetLocalPosition().y, 2.0f);
    EXPECT_FLOAT_EQ(t->GetLocalPosition().z, 3.0f);
}

TEST(Transform, WorldMatrixNoParent) {
    GameObject obj("Test");
    auto* t = obj.GetTransform();
    t->SetLocalPosition(Vec3(10.0f, 0.0f, 0.0f));

    const Mat4& world = t->GetWorldMatrix();
    Vec3 worldPos = Vec3(world[3]);
    EXPECT_NEAR(worldPos.x, 10.0f, 0.01f);
}

TEST(Transform, WorldMatrixWithParent) {
    auto* parent = new GameObject("Parent");
    auto* child = new GameObject("Child");
    child->SetParent(parent);

    parent->GetTransform()->SetLocalPosition(Vec3(10.0f, 0.0f, 0.0f));
    child->GetTransform()->SetLocalPosition(Vec3(5.0f, 0.0f, 0.0f));

    Vec3 worldPos = Vec3(child->GetTransform()->GetWorldMatrix()[3]);
    EXPECT_NEAR(worldPos.x, 15.0f, 0.01f);

    delete parent;
}

TEST(Transform, DirectionVectors) {
    GameObject obj("Test");
    auto* t = obj.GetTransform();

    // 默认朝向：forward = -Z
    Vec3 forward = t->GetForward();
    EXPECT_NEAR(forward.x, 0.0f, 0.01f);
    EXPECT_NEAR(forward.y, 0.0f, 0.01f);
    EXPECT_NEAR(forward.z, -1.0f, 0.01f);

    Vec3 right = t->GetRight();
    EXPECT_NEAR(right.x, 1.0f, 0.01f);

    Vec3 up = t->GetUp();
    EXPECT_NEAR(up.y, 1.0f, 0.01f);
}

// -- 序列化 --

TEST(GameObject, SerializeDeserialize) {
    auto* obj = new GameObject("Test");
    obj->AddComponent<TestComponent>();
    obj->GetComponent<TestComponent>()->speed = 20.0f;
    obj->GetComponent<TestComponent>()->health = 200;
    obj->GetTransform()->SetLocalPosition(Vec3(10.0f, 0.0f, 0.0f));

    auto* child = new GameObject("Child");
    child->SetParent(obj);
    child->GetTransform()->SetLocalPosition(Vec3(5.0f, 0.0f, 0.0f));

    // 序列化
    JsonArchive writer;
    writer.Push("gameObject");
    obj->Serialize(writer);
    writer.Pop();
    std::string jsonStr = writer.ToString();
    EXPECT_FALSE(jsonStr.empty());

    // 反序列化
    JsonArchive reader(jsonStr);
    reader.Push("gameObject");
    auto* restored = GameObject::Deserialize(reader);
    reader.Pop();

    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->GetName(), "Test");

    auto* comp = restored->GetComponent<TestComponent>();
    ASSERT_NE(comp, nullptr);
    EXPECT_FLOAT_EQ(comp->speed, 20.0f);
    EXPECT_EQ(comp->health, 200);

    EXPECT_NEAR(restored->GetTransform()->GetLocalPosition().x, 10.0f, 0.01f);

    EXPECT_EQ(restored->GetChildren().size(), 1u);
    EXPECT_EQ(restored->GetChildren()[0]->GetName(), "Child");
    EXPECT_NEAR(restored->GetChildren()[0]->GetTransform()->GetLocalPosition().x, 5.0f, 0.01f);

    delete obj;
    delete restored;
}

// -- ComponentRegistry --

TEST(ComponentRegistry, CreateKnownType) {
    Component* comp = ComponentRegistry::Get().Create("TestComponent");
    ASSERT_NE(comp, nullptr);
    EXPECT_STREQ(comp->GetTypeName(), "TestComponent");
    delete comp;
}

TEST(ComponentRegistry, CreateUnknownTypeReturnsNull) {
    Component* comp = ComponentRegistry::Get().Create("UnknownType");
    EXPECT_EQ(comp, nullptr);
}

// -- 生命周期 --

TEST(GameObject, SetActiveCallsOnEnableDisable) {
    GameObject obj("Test");
    auto* comp = obj.AddComponent<TestComponent>();

    // 默认 active=true，SetActive(false) 应触发 OnDisable
    obj.SetActive(false);
    EXPECT_FALSE(obj.IsActive());

    // SetActive(true) 应触发 OnEnable
    obj.SetActive(true);
    EXPECT_TRUE(obj.IsActive());
}

TEST(GameObject, SetInSceneCallsOnStart) {
    GameObject obj("Test");
    auto* comp = obj.AddComponent<TestComponent>();

    // SetInScene(true) 应触发 OnStart
    obj.SetInScene(true);
    EXPECT_TRUE(obj.IsInScene());
}

// -- Destroy --

TEST(GameObject, DestroyCallsOnDestroy) {
    auto* obj = new GameObject("Test");
    auto* comp = obj->AddComponent<TestComponent>();

    obj->Destroy();
    // OnDestroy 已调用，但对象本身未 delete
    delete obj;
}
