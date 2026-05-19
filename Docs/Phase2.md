# Phase 2 — 核心层

**目标：** 帧分配器 + JSON 序列化 + 事件总线 + GameObject/Component 系统
**涉及层：** 核心层（Memory / Serialization / Event / GameObject）

---

## 1. 目录结构

Phase 2 在现有 Phase 1 基础上新增以下文件：

```
Engine/
├── Core/
│   ├── Math/
│   │   └── MathTypes.h              # [已有] glm 类型别名
│   ├── Log.h                        # [已有] spdlog 封装
│   ├── Engine.h                     # [已有→修改] 引擎主循环集成核心层
│   │
│   ├── Memory/
│   │   └── LinearAllocator.h        # [新增] 线性/帧分配器
│   │
│   ├── Serialization/
│   │   ├── JsonArchive.h            # [新增] JSON 序列化归档器声明
│   │   └── JsonArchive.cpp          # [新增] JSON 序列化实现
│   │
│   ├── Event/
│   │   ├── EventBus.h               # [新增] 事件总线（发布/订阅）
│   │   └── Event.h                  # [新增] 事件数据定义
│   │
│   └── GameObject/
│       ├── GameObject.h             # [新增] 游戏对象
│       ├── GameObject.cpp           # [新增] 游戏对象实现
│       ├── Component.h              # [新增] 组件基类
│       ├── Transform.h              # [新增] 变换组件（每个 GameObject 必有）
│       └── Transform.cpp            # [新增] 变换组件实现
```

未在 Phase 2 创建的目录（`Resource/` `Function/` `Editor/` `Tests/`）仍不创建——用到时再建。

---

## 2. xmake.lua 变更

Phase 2 新增 1 个第三方库依赖：

- **nlohmann_json** — JSON 解析/序列化

`add_requires` 行增加 `"nlohmann_json"`，`add_packages` 行同步增加。其余不变。

---

## 3. 模块设计

### 3.1 LinearAllocator — 帧分配器

**文件：** `Engine/Core/Memory/LinearAllocator.h`

**设计意图：**

为渲染管线和其他子系统提供一种"每帧重置"的快速临时内存分配方式。不替代全局 `new/delete`，STL 容器仍使用默认分配器。

**接口要点：**

- 构造时传入固定容量（字节数），内部分配一块连续内存
- `Allocate(size, alignment)` → 从当前偏移处对齐分配，返回 `void*`；空间不足返回 `nullptr`
- `Reset()` → 将偏移归零，整块内存可重用（不调用析构函数）
- `GetUsed()` / `GetCapacity()` → 查询使用量和容量
- 禁止拷贝，允许移动
- 纯头文件实现（模板无关，但足够简单，inline 即可）

**使用场景：**

- Engine 主循环每帧开始时调用 `Reset()`，帧内所有临时数据从 LinearAllocator 分配
- 渲染管线的临时顶点/uniform 缓冲
- 事件系统的临时事件数据（可选，也可直接用栈/堆）

**注意事项：**

- 从 LinearAllocator 分配的对象**不调用析构函数**——只放 POD 或 trivially destructible 类型
- 线程不安全——单线程使用（主线程帧循环内）
- Phase 2 先提供一个全局 `LinearAllocator` 实例（如 `Engine` 持有），后续可按需增加实例

---

### 3.2 JsonArchive — JSON 序列化

**文件：** `Engine/Core/Serialization/JsonArchive.h` + `JsonArchive.cpp`

**设计意图：**

提供统一的 JSON 读写接口。序列化与反序列化共用一个 `JsonArchive` 类，通过 `IsReading()` / `IsWriting()` 区分方向。底层使用 nlohmann/json。

**接口要点：**

- 构造：
  - 写模式：`JsonArchive()` 默认写模式，内部创建空 JSON 对象
  - 读模式：`JsonArchive(const std::string& jsonStr)` 从 JSON 字符串构造
- `IsReading()` / `IsWriting()` → 判断当前方向
- 核心操作符：`ar("key", value)` — 写模式时写入 key-value，读模式时读取 key 到 value
  - 支持基本类型：`int`, `float`, `double`, `bool`, `std::string`
  - 支持类型别名：`Vec2`, `Vec3`, `Vec4`, `Quat`（序列化为 JSON 数组）
  - 支持嵌套：`ar.Push("subobject")` / `ar.Pop()` 进入/退出子对象
  - 支持数组：`ar.BeginArray("key")` / `ar.EndArray()` + `ar.Size()` + 循环内 `ar(i, element)`
- `ToString()` → 将写模式的结果输出为 JSON 字符串（带缩进，可读）

**设计约束：**

- `ar("key", value)` 的读/写方向在运行时判断，不需要编译期两套代码
- 不使用宏，不用代码生成——就是普通函数调用
- 不做版本号/向前兼容——Phase 2 的场景保存/加载是同版本，不需要迁移

**与 Component 的协作：**

Component 的 `OnSerialize(JsonArchive& ar)` 虚函数接收一个 JsonArchive 引用，在函数体内用 `ar("speed", speed)` 逐属性读写。写模式时序列化到 JSON，读模式时从 JSON 反序列化。Component 不需要知道自己在"存"还是"读"。

---

### 3.3 EventBus — 事件总线

**文件：** `Engine/Core/Event/EventBus.h` + `Event.h`

**设计意图：**

提供发布/订阅模式的全局事件系统，让模块间松耦合通信。例如物理系统发送"碰撞事件"，音频系统订阅并播放撞击音效——两者互不知道对方存在。

**Event.h 设计要点：**

- 不定义严格的事件基类层级——使用 `std::unordered_map<std::string, std::any>` 作为事件数据容器
- `Event` 结构体：
  - `std::string type` — 事件类型名（如 `"Collision"`, `"Damage"`）
  - `std::unordered_map<std::string, std::any> data` — 事件附带数据
- 便捷访问：`event.Get<int>("damage")` / `event.Set("damage", 25)` — 内部做 `std::any_cast`，类型不匹配时返回 `std::nullopt`

**EventBus.h 设计要点：**

- 单例或由 Engine 持有，全局唯一
- `Subscribe(eventType, callback)` — 订阅某类事件，callback 签名为 `void(const Event&)`，返回订阅 ID（用于取消订阅）
- `Unsubscribe(id)` — 取消订阅
- `Emit(const Event& event)` — 同步发布事件，所有订阅者立即被调用
- 订阅者存储：`std::unordered_map<std::string, std::vector<Subscriber>>`，按事件类型分组

**设计约束：**

- Phase 2 只做同步 Emit——事件在发布者线程立即递归处理，不做队列、不做延迟
- `std::any` 带来少量运行时开销，但事件数据量小、频率低，不影响性能
- 不做事件优先级——先注册先响应
- 回调内可以安全地 Emit 新事件（递归），但应避免无限循环

**使用示例（概念性）：**

```
// 订阅
bus.Subscribe("Damage", [](const Event& e) {
    int dmg = e.Get<int>("amount").value_or(0);
    // 处理伤害...
});

// 发布
Event e("Damage");
e.Set("target", std::string("Player"));
e.Set("amount", 25);
bus.Emit(e);
```

---

### 3.4 GameObject / Component / Transform

**文件：** `Engine/Core/GameObject/GameObject.h` + `.cpp`, `Component.h`, `Transform.h` + `.cpp`

**设计意图：**

实现 Unity 风格的 GameObject + Component 模型。这是引擎最核心的数据结构——后续所有功能（渲染、物理、动画、脚本）都挂载为 Component。

---

#### 3.4.1 Component — 组件基类

**文件：** `Engine/Core/GameObject/Component.h`

**接口要点：**

- `gameObject` 指针 — 反向引用所属 GameObject，由 `AddComponent` 时自动设置
- `GetTypeName()` 纯虚函数 — 返回组件类型名字符串（如 `"Transform"`, `"MeshRenderer"`），用于 Inspector 显示和序列化类型识别
- `enabled` 布尔 — 控制组件是否参与 `OnUpdate` 和 `OnInspector`
- 生命周期虚函数（默认空实现）：
  - `OnStart()` — 组件首次加入场景时调用一次
  - `OnUpdate(float dt)` — 每帧调用
  - `OnDestroy()` — 组件销毁前调用
  - `OnEnable()` / `OnDisable()` — enabled 状态切换时调用
- 属性虚函数（默认空实现）：
  - `OnSerialize(JsonArchive& ar)` — 序列化/反序列化组件属性
  - `OnInspector()` — 在 ImGui 窗口绘制属性控件（Phase 5 编辑器时才真正使用，Phase 2 先预留接口）
  - `OnBindLua(sol::table& t)` — 暴露属性到 Lua（Phase 4 脚本系统时才使用，Phase 2 先预留接口）
- 析构函数为虚函数

**设计约束：**

- 不用 ClassDB / PropertyInfo / Variant——参见 ADR-006
- Component 的内存由 GameObject 管理（`AddComponent` 中 `new`，`RemoveComponent` 或 `Destroy` 中 `delete`）
- Phase 2 中 `OnInspector()` 和 `OnBindLua()` 只声明不实现任何功能——这两个接口是为 Phase 4/5 预留的

---

#### 3.4.2 Transform — 变换组件

**文件：** `Engine/Core/GameObject/Transform.h` + `Transform.cpp`

**设计意图：**

Transform 是每个 GameObject **必有**的组件，表示对象在空间中的位置、旋转、缩放。支持父子层级，子对象继承父级变换。

**接口要点：**

- 继承自 `Component`，`GetTypeName()` 返回 `"Transform"`
- 本地属性：
  - `m_localPosition` (Vec3) — 本地位置，默认 `{0,0,0}`
  - `m_localRotation` (Quat) — 本地旋转，默认单位四元数
  - `m_localScale` (Vec3) — 本地缩放，默认 `{1,1,1}`
- 缓存矩阵：
  - `m_localMatrix` (Mat4) — 由本地属性计算
  - `m_worldMatrix` (Mat4) — 由 local × parent.world 计算
  - `m_dirty` 标志 — 任一本地属性变化时设为 `true`，请求世界矩阵时若 dirty 则重新计算
- Getter/Setter：
  - `GetPosition()` / `SetPosition(Vec3)` — 本地位置的便捷访问
  - `GetRotation()` / `SetRotation(Quat)` — 本地旋转
  - `GetScale()` / `SetScale(Vec3)` — 本地缩放
  - `GetLocalMatrix()` — 返回本地矩阵（dirty 时重算）
  - `GetWorldMatrix()` — 返回世界矩阵（dirty 时向上递归重算）
- 便捷方向：
  - `GetForward()` — 返回世界空间前方向量
  - `GetRight()` — 返回世界空间右方向量
  - `GetUp()` — 返回世界空间上方向量
- 父子联动：
  - 父级 Transform 变化时，所有子级 dirty 标记传播（设置 `m_dirty = true`）
  - 不立即重算矩阵——延迟到 `GetWorldMatrix()` 被调用时才重算

**设计约束：**

- Transform 不直接持有父子指针——父子关系由 GameObject 维护，Transform 通过 `gameObject->GetParent()->GetTransform()` 访问父级
- Phase 2 只实现本地属性 + 矩阵缓存 + dirty 标记 + 世界矩阵计算。`OnSerialize()` 读写 position/rotation/scale，`OnInspector()` 预留空实现
- 矩阵计算使用 GLM 的 `glm::translate` / `glm::rotate` / `glm::scale`

---

#### 3.4.3 GameObject — 游戏对象

**文件：** `Engine/Core/GameObject/GameObject.h` + `GameObject.cpp`

**设计意图：**

GameObject 是场景的基本单元，持有 Transform 和一组 Component。通过 `SetParent()` 构建层级树。

**接口要点：**

- 构造：`GameObject(const std::string& name)` — 自动创建并附加一个 Transform
- 属性：
  - `m_name` (string) — 对象名称
  - `m_transform` (Transform*) — 必有组件，构造时创建
  - `m_components` (vector<Component*>) — 除 Transform 外的所有组件
  - `m_parent` (GameObject*) — 父对象
  - `m_children` (vector<GameObject*>) — 子对象列表
  - `m_active` (bool) — 是否激活，默认 `true`
  - `m_inScene` (bool) — 是否已加入场景（控制 OnStart 调用时机）
- 核心 API：
  - `GetName()` / `SetName()`
  - `GetTransform()` — 返回 Transform 指针
  - `GetParent()` / `GetChildren()`
  - `IsActive()` / `SetActive(bool)` — 设置 active 时递归调用组件的 OnEnable/OnDisable
  - `AddComponent<T>(Args...)` — 模板函数，创建组件并设置 `gameObject` 指针，若 `m_inScene` 则立即调用 `OnStart()`，返回组件指针
  - `GetComponent<T>()` — 模板函数，遍历 `m_components` 用 `dynamic_cast` 查找第一个匹配类型的组件
  - `GetComponents<T>()` — 返回所有匹配类型的组件
  - `RemoveComponent<T>()` — 找到组件后调用 `OnDestroy()` 并 `delete`
  - `SetParent(GameObject*)` — 从旧父级移除，加入新父级，自动维护 `m_children`
  - `Destroy()` — 递归销毁所有子对象，对所有组件调用 `OnDestroy()` 并 `delete`，从父级移除
- 序列化：
  - `Serialize(JsonArchive& ar)` — 序列化名称、active、Transform、所有 Component
  - `Deserialize(JsonArchive& ar)` — 反序列化还原 GameObject（需要类型注册机制创建正确的 Component 子类）

**Component 类型创建问题：**

反序列化时需要根据类型名字符串创建对应的 Component 子类实例。Phase 2 采用最简单的方案：

- 维护一个 `std::unordered_map<std::string, std::function<Component*()>>` 类型工厂映射
- 提供 `RegisterComponentType<T>(name)` 模板函数，注册 `T` 的默认构造
- `Deserialize` 时查表创建实例，然后调用 `OnSerialize(ar)` 填充属性
- 引擎内置类型（Transform 等）在引擎初始化时自动注册

这个工厂不是 ClassDB——它只做"从字符串到构造函数"的映射，不涉及 PropertyInfo / Variant / 属性遍历。

**设计约束：**

- `AddComponent` 不检查重复类型——同一个 GameObject 可以挂多个同类型 Component（如多个 LuaScript）
- `GetComponent<T>()` 用 `dynamic_cast`——需要 RTTI。引擎不禁用 RTTI（简单优先，不追求极致性能）
- GameObject 的内存管理目前由用户/Scene 管理（`new` / `delete`），不使用智能指针——Phase 2 没有 Scene，Sandbox 中手动管理
- `Destroy()` 不立即删除自身——调用者负责 `delete`。Destroy 负责清理内部状态（子对象、组件）

---

### 3.5 Engine.h 修改

**文件：** `Engine/Core/Engine.h` [修改]

Phase 2 对 Engine 类的修改：

- 新增成员：
  - `LinearAllocator m_frameAllocator` — 帧分配器实例（固定大小，如 1MB）
  - `EventBus m_eventBus` — 事件总线实例
- `Run()` 循环修改：
  - 每帧开始调用 `m_frameAllocator.Reset()`
  - （可选）每帧遍历所有活跃 GameObject 调用 `OnUpdate(dt)`
- 新增访问器：
  - `GetFrameAllocator()` — 返回帧分配器引用
  - `GetEventBus()` — 返回事件总线引用
- 新增静态或全局访问：
  - `Engine::GetInstance()` — 返回当前 Engine 实例指针（供 Component 等需要访问引擎子系统的代码使用）
- 构造函数中增加 `FUN_INFO("FunEngine v0.1.0 -- Phase 2")` 版本标记

**关于 GameObject 的更新循环：**

Phase 2 暂不引入 Scene 层。Engine 直接持有一个 `std::vector<GameObject*>` 作为根级对象列表。`Run()` 中每帧遍历所有对象，递归调用 `OnUpdate(dt)`。这是临时方案，Phase 4 场景系统时会被 `Scene` 替代。

---

## 4. 实现顺序

按依赖关系自底向上实现：

| 步骤 | 模块 | 依赖 | 说明 |
|------|------|------|------|
| 1 | LinearAllocator | 无 | 最底层，无外部依赖 |
| 2 | JsonArchive | nlohmann_json | 依赖 JSON 库 |
| 3 | Event / EventBus | 无 | 依赖 STL + std::any |
| 4 | Component.h | 无 | 纯接口，依赖 JsonArchive 声明 |
| 5 | Transform | Component, MathTypes | 第一个具体 Component |
| 6 | GameObject | Component, Transform, JsonArchive | 核心容器 |
| 7 | Engine.h 修改 | 以上所有 | 集成到引擎主循环 |
| 8 | Sandbox 测试 | 以上所有 | 验证功能 |

---

## 5. Sandbox 验证

Phase 2 的 Sandbox `main.cpp` 需要验证所有核心层功能。验证内容：

### 5.1 LinearAllocator 验证

- 创建 LinearAllocator，分配若干对象，验证偏移正确
- Reset 后再次分配，验证从零开始
- 分配超过容量时返回 nullptr

### 5.2 JsonArchive 验证

- 写模式：创建一个包含基本类型 + Vec3 + 嵌套对象的 JsonArchive，调用 `ToString()` 输出 JSON
- 读模式：用上一步输出的 JSON 字符串构造 JsonArchive，逐属性读回并验证值一致

### 5.3 EventBus 验证

- 订阅一个 `"TestEvent"` 事件
- 发布该事件，验证订阅者收到正确的数据
- 取消订阅后再次发布，验证不再收到

### 5.4 GameObject / Component 验证

- 创建 `new GameObject("Test")`，验证自动附带 Transform
- `AddComponent` 添加自定义测试组件，`GetComponent` 取回验证
- 设置父子关系，验证 `GetParent()` / `GetChildren()` 正确
- 修改子级 Transform 位置，验证 `GetWorldMatrix()` 受父级影响
- 序列化 GameObject 树到 JSON，再反序列化还原，验证属性一致
- 调用 `Destroy()`，验证组件 `OnDestroy()` 被调用

### 5.5 自定义测试 Component

Sandbox 中定义一个简单的测试组件用于验证：

- `TestComponent`：继承 Component，包含 `float speed` 和 `int health` 两个属性
- 实现 `GetTypeName()` 返回 `"TestComponent"`
- 实现 `OnSerialize(JsonArchive& ar)` 读写 `speed` 和 `health`
- 实现 `OnUpdate(float dt)` 打印日志（验证生命周期调用）
- `OnInspector()` 和 `OnBindLua()` 留空

---

## 6. 构建与运行

```bash
# 首次：安装依赖（Phase 2 新增 nlohmann_json）
xmake f -m debug

# 构建
xmake build Sandbox

# 运行
xmake run Sandbox
```

**预期结果：**

- 窗口仍然显示 Phase 1 的三角形（渲染部分不变）
- 控制台输出 Phase 2 的各项验证日志：
  - LinearAllocator 分配/重置测试通过
  - JsonArchive 序列化/反序列化测试通过
  - EventBus 发布/订阅测试通过
  - GameObject 创建/组件添加/父子层级/Transform 世界矩阵测试通过
  - GameObject 序列化到 JSON 再还原测试通过
- 所有测试通过后输出 `"Phase 2 -- All core tests passed"`

---

## 7. 验收标准

| 检查项 | 标准 |
|--------|------|
| LinearAllocator | 能分配、Reset、分配超限返回 nullptr |
| JsonArchive 写 | 基本类型 + Vec3 + 嵌套对象能序列化为合法 JSON 字符串 |
| JsonArchive 读 | 从 JSON 字符串反序列化后所有属性值与原始一致 |
| EventBus | Subscribe 后能收到 Emit 的事件数据；Unsubscribe 后不再收到 |
| GameObject 创建 | `new GameObject("Test")` 后自动附带 Transform |
| Component 添加/获取 | `AddComponent<T>()` 返回非空，`GetComponent<T>()` 能取回 |
| Component 生命周期 | `OnStart()` 和 `OnUpdate(dt)` 在正确时机被调用 |
| 父子层级 | `SetParent()` 后 `GetParent()` / `GetChildren()` 正确 |
| Transform 级联 | 子对象 `GetWorldMatrix()` 受父级 Transform 影响 |
| 序列化往返 | GameObject 树序列化 → JSON → 反序列化后属性值完全一致 |
| Engine 集成 | `Engine::GetInstance()` 可用；`GetFrameAllocator()` / `GetEventBus()` 可用 |
| 构建通过 | `xmake build Sandbox` 无错误无警告 |
| 无 ClassDB/Variant | 代码中不出现 PropertyInfo / Variant / ClassDB 等概念 |
| 文件数 | 新增文件不超过 10 个（.h + .cpp） |

---

## 8. 编码约定补充

**运行时输出必须 ASCII 兼容：** 所有通过 `FUN_INFO` / `FUN_ERROR` / 窗口标题等输出的字符串，只允许使用 ASCII 字符（0x20-0x7E）。禁止使用 em dash (`—`)、中文引号、全角符号等非 ASCII 字符。

原因：Windows 控制台默认使用 CP936 (GBK) 编码，UTF-8 多字节字符（如 em dash `E2 80 94`）会被错误解码为乱码（如 `鈥?`）。使用 ASCII 兼容的替代写法：

| 禁止 | 替代 |
|------|------|
| `—` (em dash) | `--` |
| `–` (en dash) | `-` |
| `"` / `"` | `"` |
| `'` / `'` | `'` |
| `×` | `x` |

---

## 9. Phase 2 不做什么

这些是 DEVELOPMENT.md 提到的，但 Phase 2 **不做**：

- ❌ 资源管理（Resource / ResourceHandle / ResourceManager）
- ❌ 模型 / 纹理 / 材质加载
- ❌ 渲染系统（MeshRenderer / Camera / Light）
- ❌ 物理系统（RigidBody / Collider / Jolt）
- ❌ 动画系统（Animator / ozz）
- ❌ 输入系统（InputSystem / InputAction）
- ❌ 音频系统（AudioSource / SDL_mixer）
- ❌ Lua 脚本系统（LuaScript / sol2）
- ❌ 场景管理（Scene / SceneManager / Prefab）
- ❌ ImGui 编辑器
- ❌ 多线程 / 异步加载
- ❌ 单元测试框架

Phase 2 只建立核心层的数据结构和基础设施。后续 Phase 在此基础上叠加功能——每个 Component 子类（MeshRenderer, RigidBody 等）在对应的 Phase 中实现。
