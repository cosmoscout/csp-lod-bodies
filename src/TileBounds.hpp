////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_PLANET_TILEBOUNDS_HPP
#define CSP_LOD_PLANET_TILEBOUNDS_HPP

#include "BoundingBox.hpp"
#include "Tile.hpp"

namespace csp::lodplanets {

/// Returns the bounds of tile for a planet of the given equatorial radius radiusE, polar radius
/// radiusP, and heightScale.
///
/// Assumes that tile stores elevation data (i.e. a single scalar) and has valid min/max values set.
BoundingBox<double> calcTileBounds(
    TileBase const& tile, double radiusE = 1.f, double radiusP = 1.f, double heightScale = 1.f);

BoundingBox<double> calcTileBounds(double tmin, double tmax, int tileLevel, glm::int64 patchIdx,
    double radiusE = 1.f, double radiusP = 1.f, double heightScale = 1.f);
} // namespace csp::lodplanets

#endif // CSP_LOD_PLANET_TILEBOUNDS_HPP
