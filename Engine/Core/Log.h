#pragma once
#include <spdlog/spdlog.h>

namespace fun {

class Log {
public:
    static void Init() {
        spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::debug);
    }

    static const auto& Get() { return *spdlog::default_logger_raw(); }
};

// 便捷宏
#define FUN_TRACE(...) spdlog::trace(__VA_ARGS__)
#define FUN_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define FUN_INFO(...)  spdlog::info(__VA_ARGS__)
#define FUN_WARN(...)  spdlog::warn(__VA_ARGS__)
#define FUN_ERROR(...) spdlog::error(__VA_ARGS__)

#define FUN_ASSERT(cond, msg)                                   \
    do {                                                        \
        if (!(cond)) {                                          \
            FUN_ERROR("ASSERT: {}", msg);                       \
            __debugbreak();                                     \
        }                                                       \
    } while (0)

} // namespace fun
