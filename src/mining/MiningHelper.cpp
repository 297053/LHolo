// LHolo - Client-side projection renderer for Minecraft Bedrock Windows
// Copyright (C) 2026  MarmieQi

#include "mining/MiningHelper.h"

#include "mining/MiningSession.h"
#include "plugin/LHolo.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/Bedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"

#include <array>
#include <chrono>

namespace lholo::mining {
namespace {

auto& session() {
    return detail::MiningSession::getInstance();
}

auto& logger() {
    return LHolo::getInstance().getSelf().getLogger();
}

Player* localPlayer() {
    auto client = ll::service::getClientInstance();
    auto* lp = client ? client->getLocalPlayer() : nullptr;
    return static_cast<Player*>(lp);
}

// Only ever touch the local player's GameMode. On a client build the local
// player is the only one that breaks blocks, but the guard keeps the hooks from
// interfering with any other GameMode instance and matches PlaceHelper's
// local-player contract.
bool isLocalGameMode(GameMode& gm) {
    auto* lp = localPlayer();
    return lp != nullptr && &gm.mPlayer == lp;
}

// Setting mNoDestroyUntil to the steady-clock epoch makes Bedrock's
// "now < mNoDestroyUntil" cooldown gate always false, so the next block can
// start immediately. Break progress (mDestroyProgress) is left untouched.
void clearBreakDelay(GameMode& gm) {
    if (!session().noBreakDelay()) return;
    gm.mNoDestroyUntil = std::chrono::steady_clock::time_point{};
}

void applyAutoToolStart(GameMode& gm, BlockPos const& pos) {
    if (!session().autoTool() || !isLocalGameMode(gm)) return;
    Player& player = gm.mPlayer;
    Block const& block = player.getDimensionBlockSource().getBlock(pos);

    std::array<float, detail::kHotbarSlots> speeds{};
    auto& inventory = player.getInventory();
    for (int slot = 0; slot < detail::kHotbarSlots; ++slot) {
        speeds[slot] = inventory.getItem(slot).getDestroySpeed(block);
    }
    int const current = player.getSelectedItemSlot();
    int const best    = detail::pickBestToolSlot(speeds, current);
    if (auto switchTo = session().onBreakStart(current, best)) {
        player.setSelectedSlot(*switchTo);
    }
}

// Restores the slot captured by Auto Tool (if any) and clears live session
// state. Safe when no local player exists: state is still dropped so it cannot
// leak across a world/dimension teardown.
void restoreCapturedSlot() {
    auto restore = session().resetSession();
    if (!restore) return;
    if (auto* player = localPlayer()) {
        player->setSelectedSlot(*restore);
    }
}

struct MiningHookStatus {
    bool start{};
    bool cont{};
    bool stop{};
};
MiningHookStatus gHookStatus;

LL_TYPE_INSTANCE_HOOK(
    GameModeStartDestroyHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$startDestroyBlock,
    bool,
    ::BlockPos const& pos,
    uchar             face,
    bool&             hasDestroyedBlock
) {
    clearBreakDelay(*this);
    applyAutoToolStart(*this, pos);
    return origin(pos, face, hasDestroyedBlock);
}

LL_TYPE_INSTANCE_HOOK(
    GameModeContinueDestroyHook,
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

LL_TYPE_INSTANCE_HOOK(
    GameModeStopDestroyHook,
    ll::memory::HookPriority::Normal,
    GameMode,
    &GameMode::$stopDestroyBlock,
    void,
    ::BlockPos const& pos
) {
    if (session().autoTool() && isLocalGameMode(*this)) {
        if (auto restore = session().onBreakStop()) {
            this->mPlayer.setSelectedSlot(*restore);
        }
    }
    origin(pos);
}

} // namespace

void setNoBreakDelay(bool enabled) {
    session().setNoBreakDelay(enabled);
    logger().info("No-break-delay {}", enabled ? "enabled" : "disabled");
}

bool isNoBreakDelay() {
    return session().noBreakDelay();
}

void setAutoTool(bool enabled) {
    session().setAutoTool(enabled);
    if (!enabled) {
        // Turning Auto Tool off mid-mining must immediately return the hotbar
        // to whatever the player was holding before we switched it.
        restoreCapturedSlot();
    }
    logger().info("Auto-tool {}", enabled ? "enabled" : "disabled");
}

bool isAutoTool() {
    return session().autoTool();
}

void resetWorldSession() {
    restoreCapturedSlot();
}

void resetDimensionSession() {
    restoreCapturedSlot();
}

bool installHook() {
    gHookStatus.start = GameModeStartDestroyHook::hook() == 0;
    if (!gHookStatus.start) {
        logger().error("Failed to install mining start-destroy hook");
        return false;
    }
    gHookStatus.cont = GameModeContinueDestroyHook::hook() == 0;
    if (!gHookStatus.cont) {
        logger().warn("Failed to install mining continue-destroy hook; no-break-delay may not cover all ticks");
    }
    gHookStatus.stop = GameModeStopDestroyHook::hook() == 0;
    if (!gHookStatus.stop) {
        logger().warn("Failed to install mining stop-destroy hook; auto-tool may not restore the hotbar");
    }
    return true;
}

void uninstallHook() {
    if (gHookStatus.stop) GameModeStopDestroyHook::unhook();
    if (gHookStatus.cont) GameModeContinueDestroyHook::unhook();
    if (gHookStatus.start) GameModeStartDestroyHook::unhook();
    gHookStatus = {};
}

} // namespace lholo::mining
