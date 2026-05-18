# Fun Engine — 3D 游戏引擎开发文档

## 1. 概述

### 1.1 项目目标

Fun Engine 是一款模块化、分层的 3D 游戏引擎。核心目标是**简单** — 用最少的概念、最少的代码、最少的依赖，让一个人或小团队能理解全部代码并快速做出游戏。

### 1.2 设计原则

- **简单优先** — 能用一种概念解决的不用两种，能删的代码不留着。引擎全部代码控制在可单人阅读范围内
- **分层架构** — 严格遵循自底向上的依赖：第三方库 → 平台层 → 核心层 → 资源层 → 功能层 → 工具层
- **库优先** — 非核心差异化部分全部使用第三方库，不重复造轮子
- **数据驱动** — 逻辑与数据分离，支持热加载和快速迭代
- **多平台** — 支持 Windows / Linux / macOS

---

## 2. 架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                       工具层 (Tool Layer)                    │
│            ImGui Editor / 场景编辑器 / 资源浏览器              │
├─────────────────────────────────────────────────────────────┤
│                      功能层 (Function Layer)                  │
│  渲染 │ 物理 │ 动画 │ 音频 │ 输入 │ 场景 │ 脚本(Lua) │
├─────────────────────────────────────────────────────────────┤
│                      资源层 (Resource Layer)                  │
│      资源管理  │  同步加载  │  资源引用                     │
├─────────────────────────────────────────────────────────────┤
│                      核心层 (Core Layer)                      │
│  数学 │ 内存 │ 序列化 │ 事件 │ GameObject+Component │
├─────────────────────────────────────────────────────────────┤
│                     平台层 (Platform Layer)                   │
│   窗口管理  │  文件系统  │  线程  │  平台抽象  │  输入(底层)     │
├─────────────────────────────────────────────────────────────┤
│                  第三方库 (全部 xrepo 管理)                    │
│  bgfx  │  Jolt  │  SDL3  │  ozz  │  ImGui  │  fmt  │  ...   │
└─────────────────────────────────────────────────────────────┘
```

**依赖规则：** 上层可依赖下层，下层不可依赖上层。同层模块间通过接口通信，避免直接耦合。

---

## 3. 第三方库选型

### 3.1 渲染 — bgfx

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/bkaradzic/bgfx |
| **xrepo** | `add_requires("bgfx")` |
| **职责** | 跨平台图形 API 抽象（DirectX / Vulkan / Metal / OpenGL） |

### 3.2 物理 — Jolt Physics

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/jrouwe/JoltPhysics |
| **xrepo** | `add_requires("joltphysics")` |
| **职责** | 刚体模拟、碰撞检测、约束求解、Ray cast / Shape cast |

### 3.3 窗口与输入 — SDL3

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/libsdl-org/SDL |
| **xrepo** | `add_requires("libsdl")` |
| **职责** | 窗口创建、键盘/鼠标/手柄输入、剪贴板 |

### 3.4 动画 — ozz-animation

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/guillaumeblanc/ozz-animation |
| **xrepo** | `add_requires("ozz-animation")` |
| **职责** | 骨骼动画采样、混合、IK |

### 3.5 音频 — SDL_mixer3

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/libsdl-org/SDL_mixer |
| **xrepo** | `add_requires("libsdl_mixer")` |
| **职责** | 音频播放 (WAV/MP3/OGG/FLAC)、多声道混音、3D 音效 |

### 3.6 编辑器UI — Dear ImGui

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/ocornut/imgui |
| **xrepo** | `add_requires("imgui")` |
| **职责** | 编辑器界面、调试面板、Inspector、控制台 |

### 3.7 数学 — GLM

| 项目 | 说明 |
|------|------|
| **仓库** | https://github.com/g-truc/glm |
| **xrepo** | `add_requires("glm")` |
| **职责** | 向量、矩阵、四元数、投影变换 |

### 3.8 其他依赖

| 库 | 用途 | xrepo |
|----|------|-------|
| **fmt** | 格式化字符串 | `add_requires("fmt")` |
| **spdlog** | 日志系统 | `add_requires("spdlog")` |
| **nlohmann/json** | JSON 解析/序列化 | `add_requires("nlohmann_json")` |
| **stb** | 图片加载 (png/jpg/hdr) | `add_requires("stb")` |
| **tinygltf** | glTF 模型加载 | `add_requires("tinygltf")` |
| **tracy** | 性能分析工具 | `add_requires("tracy")` |
| **sol2** | Lua C++ 绑定 | `add_requires("sol2")` |
| **lua** | 脚本运行时 | `add_requires("lua")` |

---

## 4. 各层详细设计

### 4.1 平台层 (Platform Layer)

**目录：** `Engine/Platform/`

```
Platform/
├── Platform.h              # 平台抽象接口
├── Window.h                # 窗口抽象
├── Input.h                 # 底层输入设备
├── FileSystem.h            # 文件系统抽象
├── Threading.h             # 线程/互斥/信号量
├── SDL3/
│   ├── SDL3Window.cpp      # SDL3 窗口实现
│   ├── SDL3Input.cpp       # SDL3 输入实现
│   └── SDL3Platform.cpp    # SDL3 平台初始化
```

**职责：**
- 窗口生命周期管理（创建、调整大小、全屏切换）
- 原始输入事件捕获（键鼠、手柄、触摸）→ 转发到功能层输入系统
- 文件路径抽象（不同平台的路径分隔符、沙盒路径）
- 线程池与任务调度基础设施
- 平台相关的原生 API 封装（如 Win32 COM、macOS NSApplication）

---

### 4.2 核心层 (Core Layer)

**目录：** `Engine/Core/`

```
Core/
├── Math/
│   ├── Vector2.h / Vector3.h / Vector4.h
│   ├── Matrix3.h / Matrix4.h
│   ├── Quaternion.h
│   ├── Transform.h           # 位置+旋转+缩放
│   └── AABB.h / Sphere.h / Plane.h
├── Memory/
│   └── LinearAllocator.h     # 线性分配器（帧内临时数据，每帧重置）
├── Serialization/
│   └── JsonArchive.h / .cpp   # JSON 序列化
├── Event/
│   ├── EventBus.h             # 事件总线（发布/订阅）
│   └── Event.h                # 事件基类
├── GameObject/
│   ├── GameObject.h          # 游戏对象 (Transform + Component 容器)
│   ├── Component.h           # 组件基类 (含序列化 / Inspector / Lua 绑定)
│   └── Transform.h           # 变换组件 (每个 GameObject 必有)
└── Core.h                    # 核心头文件汇总 + 引擎初始化
```

**关键设计决策：**

- **数学库：** 直接使用 GLM，不做二次封装。
- **属性注册不依赖注册表：** 不用 ClassDB 全局注册表。每个 Component 直接覆写 `OnSerialize()` / `OnInspector()` / `OnBindLua()` 三个虚函数，在函数体内手动写属性的读写逻辑。零宏、零注册、零间接层。
- **容器与字符串：** 直接使用 STL（`std::vector`, `std::unordered_map`, `std::string`），不做二次封装。
- **内存：** 只提供一个 `LinearAllocator`（帧分配器），用于渲染管线的临时数据分配，每帧重置。STL 容器默认使用全局 new/delete。
- **GameObject / Component 架构（Unity 风格）：** 场景由 GameObject 组成，每个 GameObject 挂载多个 Component。引擎功能（渲染、物理、动画、音频、脚本）全部以 Component 形式提供。不引入 ECS，保持简单的 OOP 模型。

**GameObject / Component 设计详解：**

```cpp
// ── Component.h ──
class GameObject;

class Component {
public:
    GameObject* gameObject = nullptr;
    GameObject* GetGameObject() const { return gameObject; }
    Transform* GetTransform() const;
    virtual const char* GetTypeName() const = 0;   // e.g. "MeshRenderer"

    // ── 生命周期 ──
    virtual void OnStart()    {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy()  {}
    virtual void OnEnable()   {}
    virtual void OnDisable()  {}

    // ── 序列化 / Inspector / Lua 绑定 — 子类手动覆写 ──
    virtual void OnSerialize(JsonArchive& ar) {}   // ar("speed", speed); ...
    virtual void OnInspector() {}                  // ImGui::DragFloat("Speed", &speed); ...
    virtual void OnBindLua(sol::table& t) {}       // t["speed"] = &speed; ...

    bool enabled = true;
};

// ── GameObject.h ──
class GameObject {
    std::string m_name;
    Transform* m_transform;                      // 必有组件
    std::vector<Component*> m_components;        // 其他组件
    GameObject* m_parent = nullptr;
    std::vector<GameObject*> m_children;
    bool m_active = true;

public:
    const std::string& GetName() const;
    Transform* GetTransform() const;
    GameObject* GetParent() const;
    const std::vector<GameObject*>& GetChildren() const;

    // 核心 API
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        T* comp = new T(std::forward<Args>(args)...);
        comp->gameObject = this;
        m_components.push_back(comp);
        if (m_inScene) comp->OnStart();
        return comp;
    }

    template<typename T>
    T* GetComponent() {
        for (auto* c : m_components)
            if (auto* t = dynamic_cast<T*>(c)) return t;
        return nullptr;
    }

    template<typename T>
    std::vector<T*> GetComponents() { /* ... */ }

    void SetParent(GameObject* parent);   // 自动维护层级
    void Destroy();                       // 递归销毁子对象
};

// ── Transform.h ──
class Transform : public Component {
    Vec3 m_localPosition, m_localScale{1,1,1};
    Quat m_localRotation;
    Mat4 m_localMatrix;    // 缓存
    Mat4 m_worldMatrix;    // 缓存
    bool m_dirty = true;

public:
    Vec3 GetPosition() const;
    void SetPosition(const Vec3& v);
    Vec3 GetForward() const;
    // ...
};
```

**组件类型一览：**

```
Component (基类)
├── Transform        # 每个 GameObject 必有，位置/旋转/缩放
├── MeshRenderer     # 网格渲染 (引用 Mesh + Material)
├── RigidBody        # 刚体 (Jolt)
├── BoxCollider       # 盒碰撞体
├── SphereCollider    # 球碰撞体
├── CapsuleCollider   # 胶囊碰撞体
├── Animator         # 动画控制器 (ozz)
├── AudioSource      # 音频源
├── AudioListener    # 音频监听器
├── Camera           # 摄像机
├── Light            # 光源
├── LuaScript        # Lua 脚本组件 (每个实例对应一个 .lua 文件)
└── (用户 Lua 扩展)  # 通过 LuaScript 挂载任意 Lua 行为
```

**Lua 自定义组件：**

```lua
-- player_controller.lua
local self = {}

function self:OnStart()
    self.speed = 10.0
end

function self:OnUpdate(dt)
    local t = self.gameObject.transform
    if Engine.Input:GetKey("W") then
        t.position = t.position + t.forward * self.speed * dt
    end
end

return self
```

`LuaScript` 是引擎内置的 Component 子类，加载 .lua 文件后返回的表就是组件实例。`self.gameObject` 和 `self.transform` 由 C++ 侧自动注入。多个 LuaScript 可挂到同一个 GameObject 上，互不干扰。

**属性注册 — 纯虚函数方案（不用注册表）：**

不用全局 ClassDB 查表。每个 Component 直接在三个虚函数里手动写属性代码：

```cpp
class MyComponent : public Component {
    float speed = 10.0f;
    int health = 100;
    std::string name;

    // 序列化 — 只写属性名和变量，JsonArchive 负责 JSON 读写
    void OnSerialize(JsonArchive& ar) override {
        ar("speed",  speed);
        ar("health", health);
        ar("name",   name);
    }

    // Inspector — 直接调 ImGui，编译期类型安全
    void OnInspector() override {
        ImGui::DragFloat("Speed",  &speed,  0.1f, 0.0f, 100.0f);
        ImGui::DragInt  ("Health", &health, 1,    0,    999);
        ImGui::InputText("Name",   &name);
    }

    // Lua 绑定 — sol2 直接暴露成员指针
    void OnBindLua(sol::table& t) override {
        t["speed"]  = &speed;
        t["health"] = &health;
        t["name"]   = &name;
    }
};
```

**为什么不用注册表：**
- 引擎总共 10-20 种 Component，全局注册表（~250 行基础设施）比它服务的业务代码还多
- 纯虚函数是 C++ 原语，零学习成本——不需要理解 Variant/PropertyInfo/getter/setter
- `OnInspector()` 直接写 ImGui 调用，可以有自定义布局、条件显示、分组——不受 VariantType 枚举限制
- 属性名重复两三次（序列化 + Inspector + Lua），换个名字是 IDE 一键重命名，实际代价为零

---

### 4.3 资源层 (Resource Layer)

**目录：** `Engine/Resource/`

```
Resource/
├── Resource.h              # 资源基类（引用计数 + 加载状态）
├── ResourceHandle.h        # 资源句柄
├── ResourceManager.h       # 资源管理器（注册、获取、卸载）
├── ResourceLoader.h        # 加载器接口
├── Mesh/
│   ├── Mesh.h              # 网格数据 (vertices, indices, submeshes)
│   └── MeshLoader.cpp       # tinygltf 解析 → Mesh
├── Texture/
│   ├── Texture.h           # 纹理 (width, height, format, mips)
│   └── TextureLoader.cpp   # stb_image + bgfx 纹理创建
├── Material/
│   └── Material.h          # 材质 (shader + 参数表)
├── Shader/
│   ├── Shader.h            # bgfx 着色器封装
│   └── ShaderCompiler.cpp  # shaderc 编译工具
├── Skeleton/
│   ├── Skeleton.h          # 骨骼定义（骨骼名+层级+绑定姿势）
│   └── SkeletonLoader.cpp  # ozz 骨架导入
├── Animation/
│   ├── AnimationClip.h     # 动画片段
│   └── AnimationLoader.cpp # ozz 动画导入
└── Audio/
    ├── AudioClip.h          # 音频片段
    └── AudioLoader.cpp
```

**资源状态机：**

```
Unloaded → Loading → Loaded (包含 GPU 资源)
                ↓
              Failed
```

**加载流程（Phase 3 采用同步加载）：**
1. `ResourceManager::Load<Mesh>("path/to/mesh.glb")` 返回 `ResourceHandle<Mesh>`
2. 主线程执行 IO + 解析 + GPU 上传，一步到位
3. 异步加载推迟到后续版本需要时再加入

**资源引用：**
- 资源间使用 GUID 引用，通过 AssetRegistry 维护 GUID → 路径映射

---

### 4.4 功能层 (Function Layer)

**目录：** `Engine/Function/`

```
Function/
├── Render/
│   ├── RenderSystem.h       # 渲染系统入口
│   ├── RenderContext.h      # 帧渲染上下文
│   ├── Camera.h             # 摄像机
│   ├── Light.h              # 光源 (Directional/Point/Spot)
│   ├── MeshRenderer.h       # 网格渲染组件
│   ├── Skybox.h             # 天空盒
│   ├── PostProcess.h        # 后处理栈 (Bloom/SSAO/ToneMapping)
│   ├── DebugDraw.h          # 调试绘制 (线框、箭头、包围盒)
│   └── bgfx/
│       ├── BgfxManager.cpp  # bgfx 初始化 / 帧提交
│       └── BgfxUtils.cpp    # bgfx ↔ 引擎类型转换
├── Physics/
│   ├── PhysicsSystem.h      # 物理系统入口
│   ├── RigidBody.h          # 刚体组件
│   ├── Collider.h           # 碰撞体 (Box/Sphere/Capsule/Mesh)
│   ├── CharacterController.h # 角色控制器
│   ├── PhysicsMaterial.h    # 物理材质 (摩擦/反弹)
│   └── Jolt/
│       ├── JoltManager.cpp  # Jolt 初始化 / 模拟步进
│       └── JoltUtils.cpp    # Jolt ↔ 引擎类型转换
├── Animation/
│   ├── AnimationSystem.h    # 动画系统入口
│   ├── Animator.h           # 动画控制器（播放/混合/过渡）
│   ├── SkeletonComponent.h  # 骨骼组件
│   └── ozz/
│       ├── OzzManager.cpp   # ozz 运行时管理
│       └── OzzUtils.cpp     # ozz ↔ 引擎类型转换
├── Scene/
│   ├── Scene.h              # 场景（GameObject 层级树的容器）
│   ├── SceneManager.h       # 场景管理器（加载/卸载/切换）
│   └── Prefab.h             # 预制体（GameObject 模板，支持嵌套）
├── Input/
│   ├── InputSystem.h        # 输入系统（动作映射）
│   ├── InputAction.h        # 输入动作 (Jump, Fire, Move...)
│   └── InputMapping.h       # 键位绑定 (支持 Ctrl+C 等组合键)
├── Audio/
│   ├── AudioSystem.h        # 音频系统入口
│   ├── AudioSource.h        # 音频源组件
│   ├── AudioListener.h      # 音频监听器
│   └── SDLMixer/
│       ├── SDLMixerManager.cpp  # SDL_mixer3 初始化 / 混音管理
│       └── SDLMixerUtils.cpp    # SDL_mixer ↔ 引擎类型转换
└── Script/
    ├── ScriptSystem.h       # 脚本系统入口
    ├── LuaScript.h          # Lua 脚本组件
    └── Lua/
        ├── LuaManager.cpp   # Lua 虚拟机管理 (sol2)
        ├── LuaBindings.cpp  # C++ API 绑定到 Lua
        └── LuaDebugger.cpp  # Lua 调试支持
```

**渲染管线（简单前向渲染，Phase 1）：**

```
1. 收集可见对象 (Frustum Culling)
2. 深度Pass (可选，用于 Z-Prepass)
3. 不透明Pass → 按材质排序渲染
4. 天空盒Pass
5. 半透明Pass → 按距离排序渲染
6. 后处理Pass (Bloom → Tonemap → FXAA)
7. ImGui Pass
8. bgfx::frame() 提交
```

**场景系统：**

```
Scene
├── GameObject "Player"
│   ├── Transform          (position, rotation, scale)
│   ├── MeshRenderer        (→ mesh + material)
│   ├── RigidBody           (mass: 80, drag: 0.1)
│   ├── CapsuleCollider     (height: 1.8, radius: 0.4)
│   ├── Animator            (→ animation graph)
│   ├── AudioSource         (→ footstep clips)
│   └── LuaScript           (→ player_controller.lua)
│
├── GameObject "Main Camera"
│   ├── Transform
│   ├── Camera              (fov: 60, near: 0.1, far: 1000)
│   └── AudioListener
│
└── GameObject "Directional Light"
    ├── Transform
    └── Light               (type: Directional, color, intensity)
```

场景即 GameObject 层级树。每个 Component 在 `OnUpdate(dt)` 中被调用。GameObject 通过 `SetParent()` 构建父子关系，Transform 自动继承父级变换。

**脚本系统：**

`LuaScript` 是引擎内置的 Component 子类，将 .lua 文件作为组件附加到 GameObject。

```
C++ 侧：                        Lua 侧 (.lua 文件):
───────────────────────────────────────────
LuaManager                      ┌─ player_controller.lua
  ├── lua_State* (主VM)         │   local self = {}
  ├── 绑定 Engine API           │   function self:OnStart()
  └── 加载/热重载脚本            │     self.speed = 10.0
                                │   end
LuaScript : Component           │   function self:OnUpdate(dt)
  ├── 持有 lua 表引用           │     local t = self.transform
  ├── self.gameObject 自动注入  │     if Input:GetKey("W") then
  └── self.transform 自动注入   │       t.position = t.position + t.forward * self.speed * dt
                                │     end
                                │   end
                                │   return self
                                └─ (挂载到任意 GameObject)
```

- 一个 GameObject 可挂多个 LuaScript，每个加载不同的 .lua 文件
- `self.gameObject` 和 `self.transform` 由 C++ 侧在创建时自动注入到 Lua 表
- `LuaBindings.cpp` 将引擎核心类型（Transform, Input, Scene, Physics 等）暴露给 Lua
- 支持热重载：检测文件变更 → 重新加载 → 保留数据表
- 调试：集成 Lua Debug API，支持断点和调用栈查看

**物理管线：**

```
1. PhysicsSystem::Update(dt)
2.   同步 Transform 到 Jolt Body
3.   Jolt Step (collision detection + constraint solving)
4.   同步 Jolt Body 到 Transform
5.   发送碰撞事件到 EventBus
```

---

### 4.5 工具层 (Tool Layer) — `Engine/Editor/`

工具层代码放在 `Engine/Editor/` 下，与引擎其他层同目录。通过 `FUN_EDITOR` 宏条件编译，不会进入独立 Runtime。

```
Engine/Editor/
├── Editor.cpp               # 编辑器入口 main()
├── Panels/
│   ├── ViewportPanel.cpp    # 3D 视口
│   ├── HierarchyPanel.cpp   # 场景层级树
│   ├── InspectorPanel.cpp   # 属性检视器（调 Component::OnInspector）
│   ├── ConsolePanel.cpp     # 日志控制台
│   ├── ContentBrowser.cpp   # 资源浏览器
│   └── ProfilerPanel.cpp    # 性能面板（集成 tracy）
├── MaterialEditor.cpp       # 材质编辑器（属性面板，非节点式）
├── SceneEditor.cpp          # 场景编辑器 (Gizmo / 摆放 / 属性编辑)
└── AssetPipeline/
    ├── AssetImporter.h      # 资源导入器
    ├── TextureImporter.cpp  # 纹理导入
    ├── MeshImporter.cpp     # 模型导入 (tinygltf)
    └── AnimationImporter.cpp # 动画导入 (tinygltf → ozz)
```

**编辑器架构：**
- 基于 ImGui 的 Docking 多窗口布局
- 运行时与编辑器共享同一引擎，编辑器作为额外系统挂载
- `Edit Mode` → 引擎运行在编辑器循环中
- `Play Mode` → 引擎独立 Tick，编辑器暂停更新

**Inspector 面板核心思路：**
```cpp
void InspectorPanel::DrawComponents(GameObject* obj) {
    for (auto* comp : obj->GetComponents()) {
        if (ImGui::CollapsingHeader(comp->GetTypeName())) {
            comp->OnInspector();   // 每个 Component 自己画控件
        }
    }
}
```

不需要`Variant` / `PropertyInfo` / 类型分发——每个 Component 在 `OnInspector()` 里直接用 ImGui 画自己的属性。

---

## 5. 项目目录结构

```
FunEngine/                       # 仓库根目录
├── README.md
├── DEVELOPEMENT.md
├── xmake.lua                    # 构建配置 (xmake)
├── .gitignore
│
├── Engine/                      # ====== 引擎静态库 ======
│   │
│   ├── Core/                    # 核心层
│   │   ├── Math/                #   glm 类型别名
│   │   ├── Memory/              #   LinearAllocator
│   │   ├── Serialization/       #   JsonArchive
│   │   ├── Event/               #   EventBus
│   │   └── GameObject/          #   GameObject, Component, Transform
│   │
│   ├── Platform/                # 平台层
│   │   └── SDL3/                #   窗口, 输入, 文件系统
│   │
│   ├── Resource/                # 资源层
│   │   ├── Mesh/                #   Mesh + tinygltf 加载
│   │   ├── Texture/             #   Texture + stb_image 加载
│   │   ├── Material/            #   Material (shader + 参数)
│   │   ├── Shader/              #   bgfx shader 封装
│   │   ├── Skeleton/            #   骨骼定义
│   │   ├── Animation/           #   动画片段
│   │   └── Audio/               #   音频片段
│   │
│   ├── Function/                # 功能层
│   │   ├── Render/              #   渲染系统 + bgfx 后端
│   │   ├── Physics/             #   物理系统 + Jolt 后端
│   │   ├── Animation/           #   动画系统 + ozz 后端
│   │   ├── Scene/               #   场景管理
│   │   ├── Input/               #   输入动作映射
│   │   ├── Audio/               #   音频系统 + SDL_mixer 后端
│   │   └── Script/              #   Lua 脚本系统
│   │
│   └── Editor/                  # 工具层 (#ifdef FUN_EDITOR)
│       ├── Editor.cpp           #   编辑器入口 main()
│       ├── Panels/              #   视口 / 层级 / 检视 / 控制台 / 资源浏览器
│       ├── MaterialEditor.cpp   #   材质属性编辑
│       ├── SceneEditor.cpp      #   场景编辑 (Gizmo)
│       └── AssetPipeline/       #   纹理 / 模型 / 动画导入
│
├── Assets/                      # ====== 引擎内置资源 ======
│   ├── Shaders/                 #   内置 Shader 源文件
│   ├── Textures/                #   默认纹理 (白/灰/黑/法线)
│   ├── Meshes/                  #   基础几何体 .glb
│   └── Materials/               #   默认材质
│
├── Runtime/                     # ====== 独立游戏启动器 (Phase 6) ======
│   └── Runtime.cpp              #   一个 main(), 链接 FunEngine
│
├── Tests/                       # ====== 测试 ======
│   ├── TestMath.cpp
│   ├── TestGameObject.cpp
│   └── ...
│
└── Samples/                     # ====== 示例 ======
    └── Sandbox/                 #   开发测试沙盒
        └── main.cpp
```

**阅读顺序（从上到下 = 从底到顶）：**

```
xrepo/  →  Platform/  →  Core/  →  Resource/  →  Function/  →  Editor/
第三方      平台层        核心层      资源层         功能层         工具层
```

15 个第三方库全部由 xrepo 管理，不出现在仓库目录中。

只看 `Engine/` 目录就能看到完整的 5 层架构，每层一个子目录，层之间不会跨目录引用。`Editor/` 通过 `#ifdef FUN_EDITOR` 条件编译，不会进入 Runtime。

---

## 6. xmake 构建系统

```lua
-- xmake.lua (仓库根目录)
set_project("FunEngine")
set_version("0.1.0")
set_languages("c++20")

add_rules("mode.debug", "mode.release")

-- ── 第三方库 (全部由 xrepo 管理) ─────────────
add_requires(
    "libsdl",          -- SDL3
    "libsdl_mixer",    -- SDL_mixer3
    "bgfx",            -- 渲染 (自动带 bimg + bx)
    "joltphysics",     -- 物理
    "ozz-animation",   -- 动画
    "imgui",           -- 编辑器 UI
    "glm",             -- 数学
    "fmt",             -- 格式化
    "spdlog",          -- 日志
    "nlohmann_json",   -- JSON
    "stb",             -- 图片加载
    "tinygltf",        -- glTF 加载
    "lua",             -- 脚本运行时
    "sol2",            -- Lua 绑定
    "tracy")           -- 性能分析

-- ── 引擎库 ────────────────────────────────────
target("FunEngine")
    set_kind("static")
    add_defines("FUN_EDITOR")  -- 开发期间默认开启编辑器

    add_packages(
        "libsdl", "libsdl_mixer", "bgfx",
        "joltphysics", "ozz-animation",
        "imgui", "glm", "fmt", "spdlog",
        "nlohmann_json", "stb", "tinygltf",
        "lua", "sol2", "tracy")

    add_files("Engine/Core/**.cpp")
    add_files("Engine/Platform/**.cpp")
    add_files("Engine/Resource/**.cpp")
    add_files("Engine/Function/**.cpp")
    -- Editor 层 (排除入口 main)
    add_files("Engine/Editor/**.cpp", {excludes = "Engine/Editor/Editor.cpp"})
    add_includedirs("Engine", {public = true})

-- ── 编辑器 ────────────────────────────────────
target("Editor")
    set_kind("binary")
    add_files("Engine/Editor/Editor.cpp")
    add_deps("FunEngine")

-- ── 沙盒 ──────────────────────────────────────
target("Sandbox")
    set_kind("binary")
    add_files("Samples/Sandbox/main.cpp")
    add_deps("FunEngine")

-- ── 测试 ──────────────────────────────────────
for _, file in ipairs(os.files("Tests/Test*.cpp")) do
    local name = path.basename(file)
    target(name)
        set_kind("binary")
        add_files(file)
        add_deps("FunEngine")
end
```

---

## 7. 开发阶段规划

### Phase 1 — 基础骨架

- [x] 项目目录 & xmake 构建
- [ ] SDL3 窗口 + bgfx 初始化三角形
- [ ] 核心数学库 (GLM 直接使用)
- [ ] spdlog 日志集成
- [ ] 帧循环与 deltaTime

**验收：** 窗口 1280×720，灰色背景，彩色三角形可见。按 Esc 退出。控制台输出带时间戳的日志。全部源文件 ≤ 10 个。`xmake build Sandbox` 一键编译。

---

### Phase 2 — 核心层

- [ ] 帧分配器 (LinearAllocator)
- [ ] JSON 序列化 (JsonArchive)
- [ ] 事件总线
- [ ] GameObject / Component 系统（含 OnSerialize / OnInspector / OnBindLua 虚函数）

**验收：** 能 `new GameObject("Test")`，`AddComponent<MyComponent>()`，`GetComponent<T>()`。一个 Component 覆写 `OnInspector()` 后能在 ImGui 窗口画自己的属性。GameObject 树能序列化为 JSON 再反序列化还原。事件总线能 `Emit("Damage", {target, 25})` 被订阅者收到。无需任何 ClassDB/PropertyInfo/Variant 代码。

---

### Phase 3 — 资源层

- [ ] 资源基类与 ResourceHandle
- [ ] ResourceManager（同步加载）
- [ ] 模型加载 (tinygltf: glTF 2.0)
- [ ] 纹理加载 (stb_image)
- [ ] Shader 管理 (bgfx shaderc)
- [ ] Material 系统
- [ ] Skeleton + Animation 加载 (ozz)

**验收：** `ResourceManager::Load<Mesh>("cube.glb")` 返回可用 Mesh，包含顶点和索引数据。纹理从 PNG 加载后显示在 bgfx 中。Material 能绑定 Shader + 参数。ozz 骨架和动画能从 glTF 提取后正确采样。所有资源通过 GUID 引用，换路径不影响引用。

---

### Phase 4 — 功能层

- [ ] 渲染系统 (MeshRenderer, Camera, Light, Skybox)
- [ ] 物理系统 (Jolt 集成, RigidBody, Colliders)
- [ ] 动画系统 (ozz 采样 + 混合)
- [ ] 输入系统 (动作映射)
- [ ] 场景管理 (Scene / SceneManager)
- [ ] 音频系统 (SDL_mixer3 集成)
- [ ] Lua 脚本系统 (sol2 绑定 + 热重载)

**验收：** Sandbox 能用 Lua 脚本控制角色移动、播放动画、触发射击。按下跳跃键角色跳起并受重力回落。子弹碰撞墙体产生碰撞事件。修改 .lua 文件保存后自动重载，不重启程序。`SceneManager::LoadScene("a.scene")` 切换到另一个场景。

---

### Phase 5 — 编辑器

- [ ] ImGui 初始化 + Docking 布局
- [ ] Viewport + Hierarchy + Inspector
- [ ] 资产浏览器 + 控制台
- [ ] Scene Editor (Transform Gizmo / 属性编辑)
- [ ] Material Editor (属性面板，非节点式)
- [ ] Play Mode / Edit Mode 切换

**验收：** 编辑器能摆放 GameObject、拖拽赋值资源、在 Inspector 中修改属性。Ctrl+Z 撤销。Play Mode 点击后编辑器暂停更新，引擎独立运行，再次点击回到 Edit Mode 状态还原。所有操作不需要写代码。

---

### Phase 6 — 打磨

- [ ] 后处理栈
- [ ] 性能分析 (tracy 集成)
- [ ] 资源分发工具（复制 Data/ + engine.conf → 输出目录）
- [ ] 独立 Runtime 可执行文件
- [ ] Demo 项目
- [ ] 文档与示例

**验收：** `xmake build Runtime` 生成无编辑器版本。打包成 zip 发给朋友能双击运行。Demo 是一个完整的 FPS 关卡（场景 + 敌人 + 计分）。tracy 能抓帧并显示 CPU/GPU 耗时分布。

---

## 8. 打包与分发

引擎不依赖"资源烘焙"步骤——开发时用的 glTF/PNG/WAV/Lua 源文件即最终分发格式。只做三件事：构建 Runtime、整理资源目录、打包。

### 8.1 目录结构

```
MyGame/
├── Runtime.exe              # 无编辑器版本 (xmake build Runtime)
├── SDL3.dll                 # 动态库 (仅 Windows，静态链接可省)
├── Data/                    # 游戏数据 (源文件即运行时格式)
│   ├── arena.scene          #   场景 (JSON)
│   ├── player.prefab        #   预制体 (JSON)
│   ├── enemy.prefab
│   ├── meshes/              #   .glb
│   ├── textures/            #   .png / .jpg
│   ├── sounds/              #   .wav / .ogg
│   └── scripts/             #   .lua
└── engine.conf              # 启动配置 (JSON)
```

资源不需要 Cooking（v1 不做）。首次启动时引擎自动在 `Data/.cache/` 生成着色器二进制和动画预处理数据，后续启动直接读缓存。

### 8.2 构建 Runtime

```bash
xmake f -m release
xmake build Runtime
```

`Runtime` 和 `Editor` 链接同一引擎库，唯一区别是不定义 `FUN_EDITOR` 宏——ImGui、AssetPipeline、SceneEditor 等编辑器代码全部被 `#ifdef` 排除，产物体积极小。

```lua
-- xmake.lua
target("Runtime")
    set_kind("binary")
    add_files("Runtime/Runtime.cpp")
    add_deps("FunEngine")
```

### 8.3 Runtime.cpp

```cpp
#include <FunEngine/Core.h>
#include <FunEngine/Function/Scene/SceneManager.h>

int main(int argc, char** argv) {
    fun::Engine engine(argc, argv);            // 读 engine.conf, 初始化所有子系统
    engine.GetSceneManager().LoadScene(        // 加载启动场景
        engine.GetConfig().startupScene);
    engine.Run();                              // 主循环
    return 0;
}
```

### 8.4 engine.conf

```json
{
    "window": {
        "title": "My Game",
        "width": 1920,
        "height": 1080,
        "fullscreen": false
    },
    "startup_scene": "Data/arena.scene",
    "data_path": "Data/"
}
```

### 8.5 分发

Windows 上把整个 `MyGame/` 打成 zip 分发给玩家。双击 `Runtime.exe` 即可运行。

### 8.6 不做 Cooking 的理由

| 不做 Cooking | 做了 Cooking |
|-------------|-------------|
| glTF/PNG/WAV/Lua 源文件即运行时格式 | 需要离线工具转私有格式 |
| 首次启动慢 2-3 秒（解析 + GPU 上传） | 首次启动快 |
| 资源可直接修改（mod 友好） | 资源被混淆 |
| 零额外工具 | 需要开发和维护 Cooker |
| 索引文件在首次运行后缓存到 `.cache/` | 索引预先打包 |

对于个人项目和小团队分发，`Data/` 总共几百 MB，加载多等几秒完全可以接受。Cooking 可在商业发布需要时再做。

---

## 9. 编码规范

- **语言：** C++20
- **命名：**
  - 类/结构体：`PascalCase`
  - 函数/方法：`PascalCase`
  - 变量：`camelCase`（成员变量加 `m_` 前缀，静态变量加 `s_`）
  - 常量/枚举：`kPascalCase` 或 `UPPER_SNAKE_CASE`
  - 宏：`UPPER_SNAKE_CASE`
- **头文件：** 使用 `#pragma once`
- **命名空间：** 所有引擎代码在 `fun::` 下
- **错误处理：** 使用异常处理致命错误，`std::optional<T>` 处理可恢复的失败
- **智能指针：** 优先级 — 值语义 > `std::unique_ptr` > `std::shared_ptr`

---

## 10. 架构决策记录 (ADR)

### ADR-001: 直接使用 STL 容器

**决定：** 引擎所有代码直接使用 `std::vector`, `std::unordered_map`, `std::string` 等 STL 容器。

**理由：**
- 零维护成本：不需要实现和测试自己的容器库
- 每个 C++ 开发者都已熟悉 STL，学习成本为零
- 与第三方库无缝对接（bgfx、Jolt、sol2 等都使用 STL）
- 自定义分配器可在需要时通过 `std::vector<T, CustomAllocator>` 注入

### ADR-002: 单一渲染后端抽象

**决定：** 使用 bgfx 作为唯一渲染后端抽象，不在引擎中封装另一层。

**理由：**
- bgfx 已经是一个成熟的渲染抽象层，支持所有主流图形 API
- 再封装一层只会增加维护成本和性能开销
- 如果未来需要替换渲染后端，可以用模块内部适配器，不影响上层接口

### ADR-003: Unity 风格 GameObject / Component 架构

**决定：** 场景使用 GameObject + Component 模型，不使用 ECS。

**理由：**
- 简单直观：每帧遍历 GameObject 树，调用 Component 生命周期（OnStart / OnUpdate / OnDestroy）
- 编辑器友好：Hierarchy 面板天然对应 GameObject 层级树，Inspector 面板直接展示 Component 属性
- Lua 扩展自然：`LuaScript` 就是一个 Component，脚本开发者只需理解 GameObject/Component 模型
- 小型团队最优：没有数千实体的性能瓶颈时，ECS 的复杂度收益比很低
- 后续可演进：如果未来遇到极端实体数量的场景（如大量子弹、粒子），可在内部对特定 Component 使用 SoA 优化，不改变上层 API

### ADR-004: 编辑器与运行时共享引擎

**决定：** 编辑器和独立游戏运行时链接同一引擎库，通过编译宏区分 `FUN_EDITOR`。

**理由：**
- 所见即所得，编辑器内效果与运行时完全一致
- 减少代码重复
- `#ifdef FUN_EDITOR` 仅包含编辑器专属代码（ImGui、Inspector 等）

### ADR-005: Lua 作为脚本语言

**决定：** 使用 Lua 5.4 + sol2 作为游戏逻辑脚本层，替代 C# (Mono)。

**理由：**
- Lua 运行时仅 ~200KB，Mono 约 5MB，对引擎体积影响极小
- sol2 提供 modern C++ 风格的零开销绑定，API 简洁
- Lua 天然支持热重载 — 修改脚本无需重启引擎
- 学习成本低，美术/策划可直接编写简单逻辑
- Lua 没有 GC 暂停问题（增量 GC），适合实时游戏
- 可通过 LuaJIT 替换官方 Lua 获得 10x+ 性能提升，接口完全兼容

### ADR-006: 属性注册使用纯虚函数，不用注册表

**决定：** Component 基类提供 `OnSerialize()` / `OnInspector()` / `OnBindLua()` 三个虚函数。子类手动覆写，不经过任何全局注册表或中间数据结构。

**理由：**
- 零基础设施：不需要 ClassDB、PropertyInfo、Variant 等辅助类型，总共省掉 ~250 行框架代码
- 纯 C++ 原语：任何一个 C++ 开发者不读引擎文档就能看懂 `ImGui::DragFloat("Speed", &speed)`
- 编译期类型安全：Inspector 直接调 `ImGui::DragFloat`，不存在 Variant 运行时类型转换
- 布局自由：组件可以按需分组、条件显示、加分隔线和工具提示，不受统一分发逻辑限制
- 引擎总共 10-20 种组件，全局注册表的复杂度收益比为零
