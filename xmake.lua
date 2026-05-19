set_project("FunEngine")
set_version("0.1.0")
set_languages("c++20")
set_encodings("utf-8")
set_runtimes("MT")


add_rules("mode.debug", "mode.release")

-- Phase 2 新增 nlohmann_json; Phase 7 新增 gtest
add_requires("libsdl3", "bgfx", "spdlog", "glm", "nlohmann_json", "gtest")


-- ── 引擎库 ────────────────────────────────────
target("FunEngine")
set_kind("static")
add_packages("libsdl3", "bgfx", "spdlog", "glm", "nlohmann_json", { public = true })
add_files("Engine/Core/**.cpp")
add_files("Engine/Platform/**.cpp")
add_includedirs("Engine", { public = true })
add_includedirs("Assets/Shaders")

-- ── 单元测试 ──────────────────────────────────
target("UnitTests")
set_kind("binary")
add_files("Tests/*.cpp")
add_deps("FunEngine")
add_packages("gtest")

-- ── 沙盒 ──────────────────────────────────────
target("Sandbox")
set_kind("binary")
add_files("Samples/Sandbox/main.cpp")
add_deps("FunEngine")
