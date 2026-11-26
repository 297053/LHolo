// NoDelayMine - client-only "no break delay" mod for Minecraft Bedrock Windows
// Clears GameMode's inter-block destroy cooldown; break progress is untouched.

#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/level/BlockPos.h"

#include <chrono>

namespace nodelymine {

namespace {

void clearBreakDelay(GameMode& gm) {
    // mNoDestroyUntil at steady-clock epoch makes "now < mNoDestroyUntil" false,
    // so the next block can start immediately. mDestroyProgress is untouched.
    gm.mNoDestroyUntil = std::chrono::steady_clock::time_point{};
}

LL_TYPE_INSTANCE_HOOK(
    NoDelayStartDestroyHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$startDestroyBlock,
    bool,
    ::BlockPos const& pos,
    uchar             face,
    bool&             hasDestroyedBlock
) {
    clearBreakDelay(*this);
    return origin(pos, face, hasDestroyedBlock);
}

LL_TYPE_INSTANCE_HOOK(
    NoDelayContinueDestroyHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$continueDestroyBlock,
    bool,
    ::BlockPos const& pos,
    uchar             face,
    ::Vec3 const&     playerPos,
    bool&             hasDestroyedBlock
) {
    clearBreakDelay(*this);
    return origin(pos, face, playerPos, hasDestroyedBlock);
}

bool gStartHooked{};
bool gContinueHooked{};

} // namespace

class NoDelayMine {
public:
    static NoDelayMine& getInstance() {
        static NoDelayMine instance;
        return instance;
    }

    NoDelayMine(NoDelayMine const&)            = delete;
    NoDelayMine(NoDelayMine&&)                 = delete;
    NoDelayMine& operator=(NoDelayMine const&) = delete;
    NoDelayMine& operator=(NoDelayMine&&)      = delete;

    bool load() { return true; }

    bool enable() {
        gStartHooked = NoDelayStartDestroyHook::hook() == 0;
        if (!gStartHooked) {
            getSelf().getLogger().error("Failed to hook GameMode::startDestroyBlock");
            return false;
        }
        gContinueHooked = NoDelayContinueDestroyHook::hook() == 0;
        if (!gContinueHooked) {
            getSelf().getLogger().warn("Failed to hook GameMode::continueDestroyBlock; delay may remain on some ticks");
        }
        getSelf().getLogger().info("No break delay enabled");
        return true;
    }

    bool disable() {
        if (gContinueHooked) {
            NoDelayContinueDestroyHook::unhook();
            gContinueHooked = false;
        }
        if (gStartHooked) {
            NoDelayStartDestroyHook::unhook();
            gStartHooked = false;
        }
        return true;
    }

    bool unload() { return disable(); }

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

private:
    NoDelayMine() : mSelf(*ll::mod::NativeMod::current()) {}

    ll::mod::NativeMod& mSelf;
};

} // namespace nodelymine

LL_REGISTER_MOD(nodelymine::NoDelayMine, nodelymine::NoDelayMine::getInstance());
