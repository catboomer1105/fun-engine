#include <gtest/gtest.h>
#include <Core/Scene/SceneManager.h>
#include <Core/Scene/Scene.h>
#include <Core/Serialization/JsonArchive.h>
#include <Core/Engine.h>
#include <Core/Event/EventBus.h>
#include <Core/GameObject/GameObject.h>
#include <Core/GameObject/Transform.h>

using namespace fun;

class SceneManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 确保 Engine 实例存在（SceneManager 通过 EventBus 发事件需要 Engine）
        if (!Engine::GetInstance()) {
            m_engine = new Engine(0, nullptr);
        }
    }

    void TearDown() override {
        if (m_engine) {
            delete m_engine;
            m_engine = nullptr;
        }
    }

    Engine* m_engine = nullptr;
};

TEST_F(SceneManagerTest, LoadSceneFromFile) {
    const std::string path = "test_manager_scene.scene";

    {
        Scene scene("ManagerTest");
        auto* obj = scene.CreateGameObject("Hero");
        obj->GetTransform()->SetLocalPosition(Vec3(10.0f, 0.0f, 0.0f));

        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    SceneManager mgr;
    Scene* loaded = mgr.LoadScene(path);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetName(), "ManagerTest");

    auto* hero = loaded->Find("Hero");
    ASSERT_NE(hero, nullptr);
    EXPECT_NEAR(hero->GetTransform()->GetLocalPosition().x, 10.0f, 0.01f);

    mgr.UnloadScene("ManagerTest");
    std::remove(path.c_str());
}

TEST_F(SceneManagerTest, SetActiveScene) {
    const std::string path1 = "test_active_scene1.scene";
    const std::string path2 = "test_active_scene2.scene";

    {
        Scene scene("Scene1");
        scene.CreateGameObject("Obj1");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path1);
    }
    {
        Scene scene("Scene2");
        scene.CreateGameObject("Obj2");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path2);
    }

    SceneManager mgr;
    mgr.LoadScene(path1);
    mgr.LoadScene(path2);

    EXPECT_EQ(mgr.GetActiveScene()->GetName(), "Scene1"); // 第一个加载的自动设为活动

    mgr.SetActiveScene("Scene2");
    EXPECT_EQ(mgr.GetActiveScene()->GetName(), "Scene2");

    mgr.SetActiveScene("Scene1");
    EXPECT_EQ(mgr.GetActiveScene()->GetName(), "Scene1");

    std::remove(path1.c_str());
    std::remove(path2.c_str());
}

TEST_F(SceneManagerTest, UnloadScene) {
    const std::string path = "test_unload_scene.scene";

    {
        Scene scene("UnloadTest");
        scene.CreateGameObject("Obj");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    SceneManager mgr;
    mgr.LoadScene(path);
    ASSERT_NE(mgr.GetScene("UnloadTest"), nullptr);

    mgr.UnloadScene("UnloadTest");
    EXPECT_EQ(mgr.GetScene("UnloadTest"), nullptr);
    EXPECT_EQ(mgr.GetActiveScene(), nullptr);

    std::remove(path.c_str());
}

TEST_F(SceneManagerTest, LoadSceneNotFound) {
    SceneManager mgr;
    Scene* loaded = mgr.LoadScene("__nonexistent__.scene");
    EXPECT_EQ(loaded, nullptr);
}

TEST_F(SceneManagerTest, SceneLoadedEvent) {
    const std::string path = "test_event_scene.scene";

    {
        Scene scene("EventTest");
        scene.CreateGameObject("Obj");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    Scene* receivedScene = nullptr;
    auto subId = Engine::GetInstance()->GetEventBus().Subscribe("SceneLoaded",
        [&receivedScene](const Event& e) {
            auto opt = e.Get<Scene*>("scene");
            if (opt.has_value()) {
                receivedScene = opt.value();
            }
        });

    SceneManager mgr;
    Scene* loaded = mgr.LoadScene(path);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(receivedScene, loaded);

    Engine::GetInstance()->GetEventBus().Unsubscribe(subId);
    std::remove(path.c_str());
}

TEST_F(SceneManagerTest, SceneUnloadedEvent) {
    const std::string path = "test_unload_event_scene.scene";

    {
        Scene scene("UnloadEventTest");
        scene.CreateGameObject("Obj");
        JsonArchive writer;
        scene.Serialize(writer);
        writer.SaveToFile(path);
    }

    SceneManager mgr;
    mgr.LoadScene(path);

    Scene* receivedScene = nullptr;
    auto subId = Engine::GetInstance()->GetEventBus().Subscribe("SceneUnloaded",
        [&receivedScene](const Event& e) {
            auto opt = e.Get<Scene*>("scene");
            if (opt.has_value()) {
                receivedScene = opt.value();
            }
        });

    mgr.UnloadScene("UnloadEventTest");
    EXPECT_NE(receivedScene, nullptr);

    Engine::GetInstance()->GetEventBus().Unsubscribe(subId);
    std::remove(path.c_str());
}
