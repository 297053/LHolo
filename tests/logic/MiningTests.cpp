// LHolo - Mining-assist state machine logic tests.
// Pure C++ (no game symbols): covers default-off, independent toggles, tool
// scoring/ties, and slot-restore conditions.

#include "mining/MiningSession.h"

#include <array>
#include <cstdio>

namespace {

int gChecks = 0;
int gFailures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++gChecks;                                                           \
        if (!(cond)) {                                                       \
            ++gFailures;                                                     \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,     \
                #cond);                                                      \
        }                                                                    \
    } while (false)

} // namespace

int runMiningSessionTests() {
    using namespace lholo::mining::detail;
    auto& session = MiningSession::getInstance();

    // Start clean: the singleton may carry state from earlier tests in the same
    // process, and persisted toggles must survive a session reset.
    (void)session.resetSession();
    session.setNoBreakDelay(false);
    session.setAutoTool(false);

    // 1. Both assists default off and are independent.
    CHECK(!session.noBreakDelay());
    CHECK(!session.autoTool());

    session.setNoBreakDelay(true);
    CHECK(session.noBreakDelay());
    CHECK(!session.autoTool());

    session.setAutoTool(true);
    CHECK(session.noBreakDelay());
    CHECK(session.autoTool());

    session.setNoBreakDelay(false);
    CHECK(!session.noBreakDelay());
    CHECK(session.autoTool());

    session.setAutoTool(false);
    CHECK(!session.noBreakDelay());
    CHECK(!session.autoTool());

    // A session reset must not wipe the persisted toggles.
    session.setAutoTool(true);
    session.setNoBreakDelay(true);
    (void)session.resetSession();
    CHECK(session.autoTool());
    CHECK(session.noBreakDelay());
    session.setAutoTool(false);
    session.setNoBreakDelay(false);

    // 2. Tool scoring: pick the strictly-fastest hotbar slot, keep the current
    // selection on ties, clamp out-of-range current slots.
    std::array<float, kHotbarSlots> equal{};
    equal.fill(1.0f);
    CHECK(pickBestToolSlot(equal, 0) == -1);
    CHECK(pickBestToolSlot(equal, 8) == -1);
    CHECK(pickBestToolSlot(equal, -3) == -1);
    CHECK(pickBestToolSlot(equal, 42) == -1);

    std::array<float, kHotbarSlots> speeds{};
    speeds.fill(1.0f);
    speeds[3] = 5.0f;
    CHECK(pickBestToolSlot(speeds, 0) == 3);   // a strictly faster slot exists
    CHECK(pickBestToolSlot(speeds, 3) == -1);  // current is already optimal
    CHECK(pickBestToolSlot(speeds, 99) == 3);  // currentSlot clamped to 8 (speed 1)
    CHECK(pickBestToolSlot(speeds, -5) == 3);  // currentSlot clamped to 0 (speed 1)

    speeds[5] = 5.0f;                          // tie for best at slots 3 and 5
    CHECK(pickBestToolSlot(speeds, 3) == -1);  // tie keeps the current tool
    CHECK(pickBestToolSlot(speeds, 5) == -1);  // tie keeps the current tool
    CHECK(pickBestToolSlot(speeds, 0) == 3);   // first strictly-best wins the tie

    // 3. Auto-Tool switch/restore lifecycle.
    session.setAutoTool(true);

    // First switch of a session captures the original slot and returns the tool.
    auto switchTo = session.onBreakStart(1, 4);
    CHECK(switchTo.has_value() && *switchTo == 4);

    // Stop restores the original slot and clears the session.
    auto restore = session.onBreakStop();
    CHECK(restore.has_value() && *restore == 1);
    CHECK(!session.onBreakStop().has_value()); // a second stop restores nothing

    // No switch needed → nothing to restore, even after a stop.
    CHECK(!session.onBreakStart(2, -1).has_value());
    CHECK(!session.onBreakStop().has_value());

    // Continuous mining across targets keeps the original slot captured so the
    // final stop returns it, not an intermediate tool slot.
    CHECK(session.onBreakStart(1, 4).value() == 4);
    CHECK(session.onBreakStart(4, 7).value() == 7);
    CHECK(session.onBreakStart(7, -1) == std::nullopt); // current is optimal
    restore = session.onBreakStop();
    CHECK(restore.has_value() && *restore == 1);

    // Resetting mid-mining returns the captured slot and disarms restore.
    CHECK(session.onBreakStart(0, 2).value() == 2);
    restore = session.resetSession();
    CHECK(restore.has_value() && *restore == 0);
    CHECK(!session.onBreakStop().has_value());
    CHECK(!session.resetSession().has_value());

    session.setAutoTool(false);

    std::printf("MiningSession tests: %d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
