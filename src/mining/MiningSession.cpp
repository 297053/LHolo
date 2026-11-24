// LHolo - Client-side projection renderer for Minecraft Bedrock Windows
// Copyright (C) 2026  MarmieQi

#include "mining/MiningSession.h"

#include <algorithm>

namespace lholo::mining::detail {

int pickBestToolSlot(std::array<float, kHotbarSlots> const& speeds, int currentSlot) noexcept {
    int const clamped = std::clamp(currentSlot, 0, kHotbarSlots - 1);
    float const currentSpeed = speeds[clamped];
    int   bestSlot     = -1;
    float bestSpeed    = currentSpeed;
    for (int slot = 0; slot < kHotbarSlots; ++slot) {
        if (speeds[slot] > bestSpeed) {
            bestSpeed = speeds[slot];
            bestSlot = slot;
        }
    }
    return bestSlot;
}

MiningSession& MiningSession::getInstance() {
    static MiningSession instance;
    return instance;
}

bool MiningSession::noBreakDelay() const noexcept {
    return mNoBreakDelay.load(std::memory_order_relaxed);
}

void MiningSession::setNoBreakDelay(bool enabled) noexcept {
    mNoBreakDelay.store(enabled, std::memory_order_relaxed);
}

bool MiningSession::autoTool() const noexcept {
    return mAutoTool.load(std::memory_order_relaxed);
}

void MiningSession::setAutoTool(bool enabled) noexcept {
    mAutoTool.store(enabled, std::memory_order_relaxed);
}

std::optional<int> MiningSession::onBreakStart(int currentSlot, int bestSlot) {
    std::lock_guard lock(mMutex);
    if (bestSlot < 0 || bestSlot == currentSlot) {
        // Current selection is already optimal for this target; keep the
        // captured original slot so a later stop still restores it.
        return std::nullopt;
    }
    if (mOriginalSlot < 0) {
        // First switch of the session: remember what the player was holding.
        mOriginalSlot = currentSlot;
    }
    mSwitched = true;
    return bestSlot;
}

std::optional<int> MiningSession::onBreakStop() {
    std::lock_guard lock(mMutex);
    if (!mSwitched || mOriginalSlot < 0) {
        mOriginalSlot = -1;
        mSwitched = false;
        return std::nullopt;
    }
    int const restore = mOriginalSlot;
    mOriginalSlot = -1;
    mSwitched = false;
    return restore;
}

std::optional<int> MiningSession::resetSession() {
    std::lock_guard lock(mMutex);
    int const restore = mSwitched ? mOriginalSlot : -1;
    mOriginalSlot = -1;
    mSwitched = false;
    return restore >= 0 ? std::optional<int>{restore} : std::nullopt;
}

} // namespace lholo::mining::detail
