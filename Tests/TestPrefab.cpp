#include <gtest/gtest.h>
#include <Core/Scene/Prefab.h>
#include <Core/Scene/Scene.h>
#include <Core/Serialization/JsonArchive.h>
#include <Core/GameObject/GameObject.h>
#include <Core/GameObject/Transform.h>

using namespace fun;

TEST(Prefab, LoadFromFile) {
    const std::string path = "test_prefab_temp.prefab";

    {
        // 创建一个 .prefab 文件（格式与 .scene 相同，但只有一个根对象）
        Scene scene("EnemyPrefab");
        auto* obj = scene.CreateGameObject("Enemy");
        obj->SetTag("Enemy");
        obj->GetTransform()->SetLocalPosition(Vec3(1.0f, 2.0f, 3.0f));

        auto* child = new GameObject("Weapon");
        child->SetParent(obj);
        child->GetTransform()->SetLocalPosition(Vec3(0.5f, 0.0f, 0.0f));

        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    auto* prefab = Prefab::Load(path);
    ASSERT_NE(prefab, nullptr);
    EXPECT_EQ(prefab->GetName(), "EnemyPrefab");

    auto* instance = prefab->Instantiate();
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->GetName(), "Enemy");
    EXPECT_EQ(instance->GetTag(), "Enemy");
    EXPECT_EQ(instance->GetParent(), nullptr); // 根级对象

    auto pos = instance->GetTransform()->GetLocalPosition();
    EXPECT_NEAR(pos.x, 1.0f, 0.01f);
    EXPECT_NEAR(pos.y, 2.0f, 0.01f);
    EXPECT_NEAR(pos.z, 3.0f, 0.01f);

    EXPECT_EQ(instance->GetChildren().size(), 1u);
    EXPECT_EQ(instance->GetChildren()[0]->GetName(), "Weapon");

    delete prefab;
    delete instance;
    std::remove(path.c_str());
}

TEST(Prefab, InstantiateCreatesIndependentCopies) {
    const std::string path = "test_prefab_independent.prefab";

    {
        Scene scene("TestPrefab");
        auto* obj = scene.CreateGameObject("Template");
        obj->GetTransform()->SetLocalPosition(Vec3(1.0f, 0.0f, 0.0f));

        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    auto* prefab = Prefab::Load(path);
    ASSERT_NE(prefab, nullptr);

    auto* instance1 = prefab->Instantiate();
    auto* instance2 = prefab->Instantiate();

    ASSERT_NE(instance1, nullptr);
    ASSERT_NE(instance2, nullptr);
    EXPECT_NE(instance1, instance2); // 不同对象
    EXPECT_EQ(instance1->GetName(), instance2->GetName());

    // 修改 instance1 不应影响 instance2
    instance1->GetTransform()->SetLocalPosition(Vec3(99.0f, 0.0f, 0.0f));
    EXPECT_NEAR(instance2->GetTransform()->GetLocalPosition().x, 1.0f, 0.01f);

    delete prefab;
    delete instance1;
    delete instance2;
    std::remove(path.c_str());
}

TEST(Prefab, LoadNotFound) {
    auto* prefab = Prefab::Load("__nonexistent__.prefab");
    EXPECT_EQ(prefab, nullptr);
}

TEST(Prefab, TagSystem) {
    const std::string path = "test_prefab_tag.prefab";

    {
        Scene scene("TagPrefab");
        auto* obj = scene.CreateGameObject("TaggedObj");
        obj->SetTag("MyTag");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    auto* prefab = Prefab::Load(path);
    ASSERT_NE(prefab, nullptr);

    auto* instance = prefab->Instantiate();
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->GetTag(), "MyTag");

    delete prefab;
    delete instance;
    std::remove(path.c_str());
}

TEST(Prefab, SaveToFile) {
    const std::string path = "test_prefab_save.prefab";

    // 先创建一个 .prefab 文件
    {
        Scene scene("Original");
        auto* obj = scene.CreateGameObject("Template");
        obj->SetTag("OriginalTag");
        obj->GetTransform()->SetLocalPosition(Vec3(1.0f, 2.0f, 3.0f));

        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    // 加载 Prefab，调用 SaveToFile，再加载验证
    auto* prefab = Prefab::Load(path);
    ASSERT_NE(prefab, nullptr);

    ASSERT_TRUE(prefab->SaveToFile(path));

    auto* reloaded = Prefab::Load(path);
    ASSERT_NE(reloaded, nullptr);

    auto* instance = reloaded->Instantiate();
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->GetName(), "Template");
    EXPECT_EQ(instance->GetTag(), "OriginalTag");
    auto pos = instance->GetTransform()->GetLocalPosition();
    EXPECT_NEAR(pos.x, 1.0f, 0.01f);

    delete prefab;
    delete reloaded;
    delete instance;
    std::remove(path.c_str());
}
