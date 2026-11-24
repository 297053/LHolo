// LHolo - Client-side projection renderer for Minecraft Bedrock Windows
// Copyright (C) 2026  MarmieQi
//
// Mining assists: No Break Delay (cancel the inter-block destroy cooldown while
// keeping normal break progress) and Auto Tool (auto-select the fastest hotbar
// tool for the block being mined, restoring the previous slot afterwards).
//
// Both apply to manual mining and to LHolo range destruction, because they hook
// the shared GameMode break flow. The toggles are independent and default off.

#pragma once

namespace lholo::mining {

// No Break Delay: clears the destroy cooldown gate before each break starts, so
// the next block can begin immediately. Break progress is unchanged.
void setNoBreakDelay(bool enabled);
bool isNoBreakDelay();

// Auto Tool: when a block starts breaking, switch to the hotbar slot whose item
// is fastest on that block; restore the previously selected slot when mining
// stops or the session resets.
void setAutoTool(bool enabled);
bool isAutoTool();

// Clears live mining state (captured tool slot / target) for a new world. Safe
// to call when no player is available.
void resetWorldSession();

// Clears live mining state on dimension change.
void resetDimensionSession();

bool installHook();
void uninstallHook();

} // namespace lholo::mining
