// 无挖掘延迟 + 倍数挖掘（仅客户端）/ No break delay + mining speed multiplier
// Target: Minecraft Bedrock Windows 26.60.1, LeviLamina native mod.
//
// 1) mNoDestroyUntil 清零：连续破坏方块时不再等待冷却。
// 2) 每 tick 重复调用原版 continueDestroyBlock speed 次，speed 取值 1~12。

#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/level/BlockPos.h"

#include <windows.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace speedmine {
namespace {

int  gSpeed   = 4;
bool gNoDelay = true;

std::filesystem::path moduleDir() {
    HMODULE handle = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&moduleDir),
            &handle
        )) {
        return {};
    }
    wchar_t buffer[MAX_PATH]{};
    if (!GetModuleFileNameW(handle, buffer, MAX_PATH)) return {};
    return std::filesystem::path(buffer).parent_path();
}

int parseInt(std::string const& json, char const* key, int fallback) {
    auto pos = json.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos) return fallback;
    auto colon = json.find(':', pos);
    if (colon == std::string::npos) return fallback;
    return std::atoi(json.c_str() + colon + 1);
}

bool parseBool(std::string const& json, char const* key, bool fallback) {
    auto pos = json.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos) return fallback;
    auto colon = json.find(':', pos);
    if (colon == std::string::npos) return fallback;
    auto value = json.find_first_not_of(" \t\r\n", colon + 1);
    if (value == std::string::npos) return fallback;
    return json.compare(value, 5, "false") != 0;
}

void loadConfig() {
    auto path = moduleDir() / "config.json";
    std::ifstream in(path, std::ios::binary);
    if (!in) return;
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    int speed = parseInt(json, "speed", gSpeed);
    if (speed < 1) speed = 1;
    if (speed > 12) speed = 12;
    gSpeed   = speed;
    gNoDelay = parseBool(json, "noBreakDelay", gNoDelay);
}

void clearDelay(GameMode& gm) {
    if (gNoDelay) gm.mNoDestroyUntil = std::chrono::steady_clock::time_point{};
}

LL_TYPE_INSTANCE_HOOK(
    SpeedMineStartHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$startDestroyBlock,
    bool,
    ::BlockPos const& pos,
    uchar             face,
    bool&             hasDestroyedBlock
) {
    clearDelay(*this);
    return origin(pos, face, hasDestroyedBlock);
}

LL_TYPE_INSTANCE_HOOK(
    SpeedMineContinueHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$continueDestroyBlock,
    bool,
    ::BlockPos const& pos,
    uchar             face,
    ::Vec3 const&     playerPos,
    bool&             hasDestroyedBlock
) {
    clearDelay(*this);
    bool result = origin(pos, face, playerPos, hasDestroyedBlock);
    for (int i = 1; i < gSpeed; ++i) {
        clearDelay(*this);
        result = origin(pos, face, playerPos, hasDestroyedBlock);
    }
    return result;
}

} // namespace

class SpeedMine {
public:
    static SpeedMine& getInstance() {
        static SpeedMine instance;
        return instance;
    }

    SpeedMine(SpeedMine const&)            = delete;
    SpeedMine(SpeedMine&&)                 = delete;
    SpeedMine& operator=(SpeedMine const&) = delete;
    SpeedMine& operator=(SpeedMine&&)      = delete;

    bool load() { return true; }

    bool enable() {
        loadConfig();
        if (SpeedMineStartHook::hook() != 0) {
            getSelf().getLogger().error("Failed to hook GameMode::startDestroyBlock");
            return false;
        }
        if (SpeedMineContinueHook::hook() != 0) {
            getSelf().getLogger().error("Failed to hook GameMode::continueDestroyBlock");
            return false;
        }
        getSelf().getLogger().info("无挖掘延迟+倍数挖掘 已启用：speed={} noBreakDelay={}", gSpeed, gNoDelay);
        return true;
    }

    bool disable() {
        SpeedMineContinueHook::unhook();
        SpeedMineStartHook::unhook();
        return true;
    }

    bool unload() { return true; }

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *ll::mod::NativeMod::current(); }

private:
    SpeedMine() = default;
};

} // namespace speedmine

LL_REGISTER_MOD(speedmine::SpeedMine, speedmine::SpeedMine::getInstance());
