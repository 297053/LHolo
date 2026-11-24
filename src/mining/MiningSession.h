// LHolo - Client-side projection renderer for Minecraft Bedrock Windows
// Copyright (C) 2026  MarmieQi
//
// Mining-assist state machine. This header is a pure C++ leaf: it must not
// include any Minecraft, LeviLamina, or OreUI header, so its decisions can be
// unit-tested without loading the game. The hook layer in MiningHelper.cpp
// reads the live inventory, feeds events in here, and applies the returned
// slot decisions to the player.

#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>

namespace lholo::mining::detail {

// Hotbar slots scanned when looking for the fastest tool. Bedrock hotbar is
// always the first nine inventory slots.
inline constexpr int kHotbarSlots = 9;

// Returns the strictly-fastest hotbar slot for a block, or -1 when the
// currently selected slot is already optimal (or ties for best). Ties keep the
// current selection so holding a matching tool never churns the hotbar.
// `speeds` is indexed 0..kHotbarSlots-1; `currentSlot` is clamped to the
// hotbar range before comparison.
int pickBestToolSlot(std::array<float, kHotbarSlots> const& speeds, int currentSlot) noexcept;

// Persisted mining toggles plus the live Auto-Tool slot-restore session.
class MiningSession {
public:
    static MiningSession& getInstance();

    MiningSession(MiningSession const&)            = delete;
    MiningSession(MiningSession&&)                 = delete;
    MiningSession& operator=(MiningSession const&) = delete;
    MiningSession& operator=(MiningSession&&)      = delete;

    bool noBreakDelay() const noexcept;
    void setNoBreakDelay(bool enabled) noexcept;
    bool autoTool() const noexcept;
    void setAutoTool(bool enabled) noexcept;

    // A break target is starting. `bestSlot` is the strictly-better tool slot
    // (-1 when the current slot is already optimal). Returns the slot the player
    // must be switched to, or std::nullopt when no switch is needed. The first
    // switch in a session captures the original slot so it can be restored.
    std::optional<int> onBreakStart(int currentSlot, int bestSlot);

    // The break stream ended (player released or a target finished). Returns
    // the original slot to restore when Auto Tool switched it, otherwise
    // std::nullopt, and clears the active switch.
    std::optional<int> onBreakStop();

    // Drop all live session state (dimension/world teardown or Auto Tool turned
    // off). Returns the captured original slot to restore, if any.
    std::optional<int> resetSession();

private:
    MiningSession() = default;

    std::atomic_bool mNoBreakDelay{false};
    std::atomic_bool mAutoTool{false};

    // Guarded: GameMode callbacks are serialized on the client thread, but
    // resetSession can run from teardown, so the mutex keeps them coherent.
    std::mutex       mMutex;
    int              mOriginalSlot{-1};
    bool             mSwitched{false};
};

} // namespace lholo::mining::detail
