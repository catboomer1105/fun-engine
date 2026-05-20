# Phase 3 — 场景系统（核心层）

**目标：** Scene / SceneManager / Prefab — 场景即 GameObject 层级树的容器
**涉及层：** 核心层（Scene）
**前置依赖：** Phase 2（GameObject / Component / JsonArchive / EventBus）

---

## 1. 目录结构

Phase 3 在现有 Phase 2 基础上新增以下文件：

```
Engine/
├── Core/
│   ├── Math/
│   │   └── MathTypes.h              # [已有]
│   ├── Log.h                        # [已有]
│   ├── Engine.h                     # [已有→修改] 集成 SceneManager
│   ├── Memory/
│   │   └── LinearAllocator.h        # [已有]
│   ├── Serialization/
│   │   ├── JsonArchive.h            # [已有]
│   │   └── JsonArchive.cpp          # [已有]
│   ├── Event/
│   │   ├── EventBus.h               # [已有]
│   │   └── Event.h                  # [已有]
│   ├── GameObject/
│   │   ├── GameObject.h / .cpp      # [已有]
│   │   ├── Component.h              # [已有]
│   │   └── Transform.h / .cpp       # [已有]
│   │
│   └── Scene/
│       ├── Scene.h                  # [新增] 场景定义
│       ├── Scene.cpp                # [新增] 场景实现
│       ├── SceneManager.h           # [新增] 场景管理器
│       ├── SceneManager.cpp         # [新增] 场景管理器实现
│       ├── Prefab.h                 # [新增] 预制体定义
│       └── Prefab.cpp               # [新增] 预制体实现
```

不新增第三方库依赖。场景序列化直接复用 Phase 2 的 JsonArchive。

---

## 2. xmake.lua 变更

无需新增第三方库。场景序列化复用 `nlohmann_json`（Phase 2 已引入）。

JsonArchive 新增文件 I/O 方法（`SaveToFile` / `LoadFromFile`），需要添加 `<fstream>` / `<sstream>` include（无新增链接依赖）。

新增源文件：`Engine/Core/Scene/**.cpp` 被 `add_files("Engine/Core/**.cpp")` 自动覆盖，无需修改 xmake.lua。

---

## 3. 模块设计

### 3.1 Scene — 场景

**文件：** `Engine/Core/Scene/Scene.h`

```cpp
#pragma once
#include <string>
#include <vector>
#include <functional>
#include "Core/GameObject/GameObject.h"

namespace fun {

class JsonArchive;

class Scene {
public:
    explicit Scene(const std::string& name);
    ~Scene();

    const std::string& GetName() const { return m_name; }
    bool IsLoaded() const { return m_loaded; }

    // ── GameObject 管理 ──
    GameObject* CreateGameObject(const std::string& name);
    GameObject* Find(const std::string& name) const;
    std::vector<GameObject*> FindByTag(const std::string& tag) const;
    void Destroy(GameObject* obj);

    // 遍历所有 GameObject（递归，含子对象）
    void ForEach(const std::function<void(GameObject*)>& callback);

    // 获取根级对象列表
    const std::vector<GameObject*>& GetRootObjects() const { return m_rootObjects; }

    // 将对象从根级列表移除（由 GameObject::SetParent 触发）
    void RemoveFromRoot(GameObject* obj);

    // ── 序列化 ──
    void Serialize(JsonArchive& ar);
    void Deserialize(JsonArchive& ar);

    // ── 生命周期 ──
    void OnStart();              // 场景加载后调用所有 Component::OnStart
    void OnUpdate(float dt);     // 每帧调用所有 Component::OnUpdate
    void OnDestroy();            // 场景卸载前调用所有 Component::OnDestroy

private:
    std::string m_name;
    std::vector<GameObject*> m_rootObjects;
    bool m_loaded = false;

    void destroyRecursive(GameObject* obj);
    void forEachRecursive(GameObject* obj, const std::function<void(GameObject*)>& callback);
};

} // namespace fun
```

**关键设计：**
- 只持有根级 GameObject 列表，子对象通过 `GameObject::GetChildren()` 间接访问
- `CreateGameObject` 创建的对象自动加入根级列表；若通过 `SetParent` 挂到其他对象下，GameObject 通过内部 `m_scene` 指针回调 `Scene::RemoveFromRoot` 从根级列表移除（详见 5.1 节）
- `Destroy` 递归销毁对象及其子对象和 Component
- `ForEach` 递归遍历整棵树
- 序列化/反序列化依赖 JsonArchive，格式即 .scene JSON

---

### 3.2 SceneManager — 场景管理器

**文件：** `Engine/Core/Scene/SceneManager.h`

```cpp
#pragma once
#include <string>
#include <unordered_map>
#include "Scene.h"
#include "Core/Event/EventBus.h"

namespace fun {

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager();

    Scene* GetActiveScene() const { return m_activeScene; }

    // 同步加载场景（从 .scene JSON 文件）
    Scene* LoadScene(const std::string& path);

    // 卸载场景（销毁所有 GameObject）
    void UnloadScene(const std::string& name);

    // 切换活动场景
    void SetActiveScene(const std::string& name);

    // 获取已加载的场景
    Scene* GetScene(const std::string& name) const;

    // 事件通过 EventBus 发送：
    //   "SceneLoaded"   event.Set("scene", scenePtr)
    //   "SceneUnloaded" event.Set("scene", scenePtr)
    // 订阅示例：
    //   Engine::GetInstance()->GetEventBus().Subscribe("SceneLoaded", [](const Event& e) {
    //       auto* scene = e.Get<Scene*>("scene").value_or(nullptr);
    //   });

private:
    Scene* m_activeScene = nullptr;
    std::unordered_map<std::string, Scene*> m_loadedScenes;

    void emitSceneEvent(const std::string& type, Scene* scene);
};

} // namespace fun
```

**加载流程：**
1. `JsonArchive::LoadFromFile` 读取 .scene JSON 文件
2. JsonArchive 反序列化 → 递归创建 GameObject 树
3. 调用 `OnStart()` 初始化所有 Component
4. 通过 EventBus 发送 `"SceneLoaded"` 事件（携带 Scene*）

---

### 3.3 Prefab — 预制体

**文件：** `Engine/Core/Scene/Prefab.h`

```cpp
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

private:
    std::string m_name;
    GameObject* m_template = nullptr;  // 模板对象，不参与场景更新

    explicit Prefab(const std::string& name);
    GameObject* deepCopy(GameObject* src);
};

} // namespace fun
```

**设计要点：**
- .prefab 文件本质是只有一个根 GameObject 的 .scene 文件
- `Instantiate()` 深拷贝模板，返回独立实例
- 模板对象不参与场景更新循环，仅供拷贝
- v1 不做嵌套 Prefab 同步——修改模板后已生成的实例不自动更新

---

### 3.4 Tag 系统

GameObject 添加 `m_tag` 字段和 `SetTag`/`GetTag` 方法：

```cpp
// GameObject.h 新增
std::string m_tag;

void SetTag(const std::string& tag) { m_tag = tag; }
const std::string& GetTag() const { return m_tag; }
```

Scene::FindByTag 遍历所有 GameObject 收集匹配 tag 的对象。

---

## 4. .scene / .prefab 文件格式

与主文档一致，使用 JSON：

```json
{
    "name": "Arena",
    "gameObjects": [
        {
            "name": "Player",
            "tag": "Player",
            "transform": {
                "position": [0, 0, 0],
                "rotation": [0, 0, 0, 1],
                "scale": [1, 1, 1]
            },
            "components": [
                {
                    "type": "MeshRenderer",
                    "properties": {
                        "mesh": "guid://abc123",
                        "material": "guid://def456"
                    }
                }
            ],
            "children": []
        }
    ]
}
```

**Component 序列化策略：**
- 每个 Component 覆写 `OnSerialize(JsonArchive& ar)`，JsonArchive 内部按 Component type 名字段读写属性
- 加载时：解析 type 字段 → 工厂创建对应 Component → 调 `OnSerialize` 读回属性
- 未知 type 跳过并打 warning 日志（向前兼容）

---

## 5. 实现顺序

1. **JsonArchive 扩展** — 添加 `SaveToFile` / `LoadFromFile` 方法
2. **Scene.h / Scene.cpp** — GameObject 容器 + 遍历 + 查找
3. **Scene 序列化** — Serialize/Deserialize 与 JsonArchive 对接
4. **SceneManager.h / SceneManager.cpp** — 加载/卸载/切换 + EventBus 事件
5. **Prefab.h / Prefab.cpp** — 模板加载 + Instantiate 深拷贝
6. **Tag 系统** — GameObject 添加 tag 字段 + SetTag/GetTag，Scene::FindByTag
7. **GameObject::SetParent 联动** — SetParent 时自动从 Scene 根级列表移除（见下方说明）
8. **Engine.h 集成** — Engine 持有 SceneManager，替代临时 m_rootObjects（见下方代码）

### 5.1 GameObject → Scene 同步机制

`CreateGameObject` 将新对象加入 Scene 根级列表。若后续调用 `SetParent` 将对象挂到另一个 GameObject 下，需要从 Scene 根级列表中移除。  
实现方式：在 `GameObject` 中添加 `Scene* m_scene` 指针，`SetParent` 中当新 parent 非空时调用 `m_scene->RemoveFromRoot(this)`。

```cpp
// GameObject.h 变更
class GameObject {
    // ... 现有成员 ...
    Scene* m_scene = nullptr;  // 所属 Scene（Phase 3 新增）
    
    friend class Scene;  // 允许 Scene 设置 m_scene
};

// SetParent 实现片段（GameObject.cpp）
void GameObject::SetParent(GameObject* parent) {
    // ... 现有层级操作 ...
    if (parent != nullptr && m_scene != nullptr) {
        m_scene->RemoveFromRoot(this);
    }
    m_parent = parent;
}
```

### 5.2 Engine.h 集成

Engine 持有 SceneManager，主循环中调用活动场景的 OnUpdate，替代临时的 `m_rootObjects` 管理：

```cpp
// Engine.h 变更
#pragma once
#include "Core/Log.h"
#include "Core/Memory/LinearAllocator.h"
#include "Core/Event/EventBus.h"
#include "Core/Scene/SceneManager.h"

namespace fun {

class Engine {
public:
    Engine(int argc, char** argv)
        : m_frameAllocator(1024 * 1024) {
        s_instance = this;
        Log::Init();
        FUN_INFO("FunEngine v0.1.0 -- Phase 3");
        m_platformInit();
        m_running = true;
        m_lastFrame = std::chrono::high_resolution_clock::now();
    }

    ~Engine() {
        // SceneManager 析构会卸载所有场景，销毁所有 GameObject
        m_platformShutdown();
        s_instance = nullptr;
        FUN_INFO("Engine shutdown complete");
    }

    void Run() {
        while (m_running) {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - m_lastFrame).count();
            m_lastFrame = now;

            m_frameAllocator.Reset();
            m_platformPollEvents();

            // 更新活动场景
            if (auto* scene = m_sceneManager.GetActiveScene()) {
                scene->OnUpdate(dt);
            }

            m_platformRender(dt);
        }
    }

    bool IsRunning() const { return m_running; }
    void Quit() { m_running = false; }

    LinearAllocator& GetFrameAllocator() { return m_frameAllocator; }
    EventBus& GetEventBus() { return m_eventBus; }
    SceneManager& GetSceneManager() { return m_sceneManager; }

    static Engine* GetInstance() { return s_instance; }

private:
    bool m_running = false;
    std::chrono::high_resolution_clock::time_point m_lastFrame;
    LinearAllocator m_frameAllocator;
    EventBus m_eventBus;
    SceneManager m_sceneManager;

    static inline Engine* s_instance = nullptr;

    void m_platformInit();
    void m_platformShutdown();
    void m_platformPollEvents();
    void m_platformRender(float dt);
};

} // namespace fun
```

**注意：** 原有 `m_rootObjects` / `AddRootObject` / `RemoveRootObject` 全部移除，GameObject 管理完全由 SceneManager 接管。

---

## 6. Sandbox 验证

### 6.1 手动创建场景

```cpp
// Samples/Sandbox/main.cpp
#include "Core/Engine.h"

int main(int argc, char** argv) {
    fun::Engine engine(argc, argv);

    // 加载 .scene 文件（SceneManager 通过 Engine::GetSceneManager 访问）
    engine.GetSceneManager().LoadScene("Assets/test.scene");

    engine.Run();
    return 0;
}
```

### 6.2 验证序列化往返

```cpp
// 创建场景 → 保存 → 清空 → 加载 → 验证一致
Scene scene("Test");
auto* obj = scene.CreateGameObject("Player");
obj->SetTag("Player");
obj->GetTransform()->SetPosition({1, 2, 3});

// 保存（JsonArchive::SaveToFile 在 Phase 3 中新增）
JsonArchive ar;
scene.Serialize(ar);
ar.SaveToFile("test_output.scene");

// 加载（JsonArchive::LoadFromFile 在 Phase 3 中新增）
Scene loaded("Test");
JsonArchive ar2 = JsonArchive::LoadFromFile("test_output.scene");
loaded.Deserialize(ar2);

auto* found = loaded.Find("Player");
assert(found != nullptr);
assert(found->GetTransform()->GetPosition() == Vec3{1, 2, 3});
```

### 6.3 验证 Prefab

```cpp
auto* prefab = Prefab::Load("Assets/enemy.prefab");
auto* instance1 = prefab->Instantiate();
auto* instance2 = prefab->Instantiate();
// instance1 和 instance2 是不同对象
assert(instance1 != instance2);
assert(instance1->GetName() == instance2->GetName());
```

---

## 7. 构建与运行

```bash
xmake build Sandbox
xmake run Sandbox
```

---

## 8. 验收标准

| 检查项 | 标准 |
|--------|------|
| Scene 创建 | CreateGameObject 返回有效对象，Find 能找到 |
| 层级遍历 | ForEach 递归访问所有子对象 |
| 序列化往返 | 场景保存为 JSON → 加载还原，GameObject 名/层级/Transform 完全一致 |
| SceneManager 加载 | LoadScene 从 .scene 文件加载场景，通过 EventBus 发送 "SceneLoaded" |
| SceneManager 切换 | SetActiveScene 切换当前活动场景 |
| Prefab 加载 | Load 后 Instantiate 生成独立副本 |
| Tag 查找 | FindByTag 返回所有匹配对象 |
| 文件数 | 新增 ≤ 6 个源文件（Scene.h/.cpp, SceneManager.h/.cpp, Prefab.h/.cpp） |

---

## 9. Phase 3 不做什么

- ❌ 不创建具体功能 Component（MeshRenderer, RigidBody 等 — 留给 Phase 5-8）
- ❌ 不做场景编辑器可视化 — 场景只存在于内存和 JSON 文件
- ❌ 不做异步场景加载 — 同步加载足够
- ❌ 不做嵌套 Prefab 自动同步 — v1 Instantiate 是纯拷贝
- ❌ 不依赖渲染/物理/动画等上层系统
- ❌ 不新增第三方库
