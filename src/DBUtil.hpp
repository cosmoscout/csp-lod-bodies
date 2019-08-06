////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_PLANET_DBUTIL_HPP
#define CSP_LOD_PLANET_DBUTIL_HPP

#include "TileId.hpp"

#include <cstdio>
#include <unordered_map>

namespace csp::lodplanets {

/// Helper functions for reading MarsVis DB files (see MarsTileSourceBase for a description of the
/// format).
namespace DBUtil {
/// std::unordered_map<TileId, glm::int64> stores mapping from TileId/ to indices used in MarsVis DB
/// file format (see MarsTileSourceBase for a description of the format). The index can be converted
/// into a file offset by multiplying the index with the on disk size of a tile and adding the size
/// of the file header, SizeFileHeader. DocTODO where does this comment belong to?

/// Size in bytes of the header of a MarsVis DB file.
extern glm::int64 const SizeFileHeader;

/// Size in bytes of the header portion of a tile's entry in the MarsVis DB file format.
extern glm::int64 const SizeTileHeader;

/// Reads header of MarsVis DB file and stores the indices of the root nodes in indexMap.
void readFileHeader(FILE* file, std::unordered_map<TileId, glm::int64>& indexMap);

/// Reads the header of the entry for node tileId from the MarsVis DB file and stores the indices of
/// the child nodes in indexMap. Returns true if all children have valid indices (i.e. they exist),
/// false otherwise.
bool readTileHeader(FILE* file, glm::int64 sizeTile, TileId const& tileId,
    std::unordered_map<TileId, glm::int64>& indexMap);

/// Returns the index of tile tileId from indexMap or -1 if it can not be determined.
glm::int64 lookupIndex(
    TileId const& tileId, std::unordered_map<TileId, glm::int64> const& indexMap);

} // namespace DBUtil
} // namespace csp::lodplanets

#endif // CSP_LOD_PLANET_DBUTIL_HPP
