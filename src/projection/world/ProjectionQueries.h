// LHolo - Client-side projection renderer for Minecraft Bedrock Windows
// Copyright (C) 2026  MarmieQi
//
// Read-only projection cell queries used by assisted placement. Locking and
// world identity validation remain in the public Projection.cpp boundary.

#pragma once

#include "projection/ProjectionTypes.h"

#include <vector>

class BlockPos;
class Vec3;

namespace lholo::projection::detail {

struct ProjectionState;

ProjectionQuery queryProjectionCell(
    ProjectionState const& state,
    BlockPos const&        worldPosition
);

std::vector<RangeCandidate> queryMissingProjectionCells(
    ProjectionState const& state,
    Vec3 const&            center,
    float                  radius
);

// Cells the projection expects to hold a block while the world currently holds
// a different solid block (wrong type or wrong state). Range destruction uses
// these as its break targets.
std::vector<RangeCandidate> queryWrongProjectionCells(
    ProjectionState const& state,
    Vec3 const&            center,
    float                  radius
);

} // namespace lholo::projection::detail
