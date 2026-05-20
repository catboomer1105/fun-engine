#include <gtest/gtest.h>
#include <Core/Scene/Scene.h>
#include <Core/Serialization/JsonArchive.h>
#include <Core/GameObject/GameObject.h>
#include <Core/GameObject/Transform.h>

using namespace fun;

TEST(Scene, CreateGameObject) {
    Scene scene("Test");
    auto* obj = scene.CreateGameObject("Player");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->GetName(), "Player");
    EXPECT_EQ(scene.GetRootObjects().size(), 1u);
    EXPECT_EQ(scene.GetRootObjects()[0], obj);
}

TEST(Scene, FindByName) {
    Scene scene("Test");
    auto* obj = scene.CreateGameObject("Player");
    scene.CreateGameObject("Enemy");

    auto* found = scene.Find("Player");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found, obj);

    EXPECT_EQ(scene.Find("NonExistent"), nullptr);
}

TEST(Scene, FindByNameInChildren) {
    Scene scene("Test");
    auto* parent = scene.CreateGameObject("Parent");
    auto* child = new GameObject("Child");
    child->SetParent(parent);

    auto* found = scene.Find("Child");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found, child);
}

TEST(Scene, FindByTag) {
    Scene scene("Test");
    auto* obj1 = scene.CreateGameObject("Player1");
    obj1->SetTag("Player");
    auto* obj2 = scene.CreateGameObject("Player2");
    obj2->SetTag("Player");
    auto* obj3 = scene.CreateGameObject("Enemy");
    obj3->SetTag("Enemy");

    auto players = scene.FindByTag("Player");
    EXPECT_EQ(players.size(), 2u);

    auto enemies = scene.FindByTag("Enemy");
    EXPECT_EQ(enemies.size(), 1u);

    auto none = scene.FindByTag("None");
    EXPECT_EQ(none.size(), 0u);
}

TEST(Scene, ForEach) {
    Scene scene("Test");
    auto* root = scene.CreateGameObject("Root");
    auto* child = new GameObject("Child");
    child->SetParent(root);
    auto* grandchild = new GameObject("GrandChild");
    grandchild->SetParent(child);

    int count = 0;
    scene.ForEach([&count](GameObject*) { ++count; });
    EXPECT_EQ(count, 3);
}

TEST(Scene, SetParentRemovesFromRoot) {
    Scene scene("Test");
    auto* parent = scene.CreateGameObject("Parent");
    auto* child = scene.CreateGameObject("Child");

    EXPECT_EQ(scene.GetRootObjects().size(), 2u);

    child->SetParent(parent);

    EXPECT_EQ(scene.GetRootObjects().size(), 1u);
    EXPECT_EQ(scene.GetRootObjects()[0], parent);
}

TEST(Scene, DestroyGameObject) {
    Scene scene("Test");
    auto* obj = scene.CreateGameObject("Obj");
    scene.Destroy(obj);
    EXPECT_EQ(scene.GetRootObjects().size(), 0u);
    EXPECT_EQ(scene.Find("Obj"), nullptr);
}

TEST(Scene, SerializeDeserialize) {
    Scene scene("Arena");
    auto* obj = scene.CreateGameObject("Player");
    obj->SetTag("Player");
    obj->GetTransform()->SetLocalPosition(Vec3(1.0f, 2.0f, 3.0f));

    auto* child = new GameObject("Child");
    child->SetParent(obj);
    child->GetTransform()->SetLocalPosition(Vec3(4.0f, 5.0f, 6.0f));

    // 序列化
    JsonArchive writer;
    scene.Serialize(writer);
    std::string jsonStr = writer.ToString();
    EXPECT_FALSE(jsonStr.empty());

    // 反序列化到新场景
    Scene loaded("Arena");
    JsonArchive reader(jsonStr);
    loaded.Deserialize(reader);

    EXPECT_EQ(loaded.GetName(), "Arena");
    EXPECT_EQ(loaded.GetRootObjects().size(), 1u);

    auto* found = loaded.Find("Player");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetTag(), "Player");

    auto pos = found->GetTransform()->GetLocalPosition();
    EXPECT_NEAR(pos.x, 1.0f, 0.01f);
    EXPECT_NEAR(pos.y, 2.0f, 0.01f);
    EXPECT_NEAR(pos.z, 3.0f, 0.01f);

    EXPECT_EQ(found->GetChildren().size(), 1u);
    EXPECT_EQ(found->GetChildren()[0]->GetName(), "Child");
}

TEST(Scene, SaveAndLoadFromFile) {
    const std::string path = "test_scene_temp.scene";

    {
        Scene scene("TestScene");
        auto* obj = scene.CreateGameObject("Player");
        obj->SetTag("Player");
        obj->GetTransform()->SetLocalPosition(Vec3(1.0f, 2.0f, 3.0f));

        JsonArchive writer;
        scene.Serialize(writer);
        ASSERT_TRUE(writer.SaveToFile(path));
    }

    {
        Scene scene("TestScene");
        JsonArchive reader = JsonArchive::LoadFromFile(path);
        scene.Deserialize(reader);

        auto* found = scene.Find("Player");
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->GetTag(), "Player");
        auto pos = found->GetTransform()->GetLocalPosition();
        EXPECT_NEAR(pos.x, 1.0f, 0.01f);
        EXPECT_NEAR(pos.y, 2.0f, 0.01f);
        EXPECT_NEAR(pos.z, 3.0f, 0.01f);
    }

    std::remove(path.c_str());
}

TEST(Scene, OnUpdate) {
    Scene scene("Test");
    scene.CreateGameObject("Obj");

    // OnUpdate 不应崩溃
    scene.OnUpdate(0.016f);
}

TEST(Scene, SceneDestructorCleanup) {
    GameObject* ptr = nullptr;
    {
        Scene scene("Test");
        ptr = scene.CreateGameObject("Obj");
    }
    // scene 析构后 ptr 指向的内存已被释放（不要解引用，只验证不崩溃）
    (void)ptr;
}

TEST(Scene, SaveToFileAndLoadFromFile) {
    const std::string path = "test_scene_convenience.scene";

    {
        Scene scene("ConvenienceTest");
        auto* obj = scene.CreateGameObject("Hero");
        obj->SetTag("Player");
        obj->GetTransform()->SetLocalPosition(Vec3(10.0f, 20.0f, 30.0f));

        auto* child = new GameObject("Sword");
        child->SetParent(obj);
        child->GetTransform()->SetLocalPosition(Vec3(1.0f, 0.0f, 0.0f));

        ASSERT_TRUE(scene.SaveToFile(path));
    }

    {
        auto* scene = Scene::LoadFromFile(path);
        ASSERT_NE(scene, nullptr);
        EXPECT_EQ(scene->GetName(), "ConvenienceTest");
        EXPECT_TRUE(scene->IsLoaded());

        auto* hero = scene->Find("Hero");
        ASSERT_NE(hero, nullptr);
        EXPECT_EQ(hero->GetTag(), "Player");
        auto pos = hero->GetTransform()->GetLocalPosition();
        EXPECT_NEAR(pos.x, 10.0f, 0.01f);
        EXPECT_NEAR(pos.y, 20.0f, 0.01f);
        EXPECT_NEAR(pos.z, 30.0f, 0.01f);

        EXPECT_EQ(hero->GetChildren().size(), 1u);
        EXPECT_EQ(hero->GetChildren()[0]->GetName(), "Sword");

        delete scene;
    }

    std::remove(path.c_str());
}
