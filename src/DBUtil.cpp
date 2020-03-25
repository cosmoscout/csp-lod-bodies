////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "DBUtil.hpp"

#include "HEALPix.hpp"

namespace csp::lodbodies::DBUtil {

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::int64 index2offset(glm::int64 idx, glm::int64 sizeTile) {
  return SizeFileHeader + idx * sizeTile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

glm::uint64 const InvalidIndex   = std::numeric_limits<glm::uint64>::max();
glm::int64 const  SizeFileHeader = 12 * sizeof(glm::uint64);
glm::int64 const  SizeTileHeader = 4 * sizeof(glm::uint64);

////////////////////////////////////////////////////////////////////////////////////////////////////

void readFileHeader(FILE* file, std::unordered_map<TileId, glm::int64>& indexMap) {
#if defined(_WIN32)
  _fseeki64(file, 0, SEEK_SET);
#else
  fseeko(file, 0, SEEK_SET);
#endif

  std::array<glm::uint64, 12> rootIndices;
  std::fread(rootIndices.data(), sizeof(glm::uint64), rootIndices.size(), file);

  for (std::size_t i = 0; i < rootIndices.size(); ++i) {
    indexMap.insert(std::make_pair(TileId(0, i), rootIndices[i]));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool readTileHeader(FILE* file, glm::int64 sizeTile, TileId const& tileId,
    std::unordered_map<TileId, glm::int64>& indexMap) {
  // base patches are shifted by 180 degree in the mars databases
  // therefore we have to shift the base patches by two
  glm::i64vec3 bxy = HEALPix::getBaseXY(tileId);
  glm::int64   mappedBasePatch;
  if (bxy.x == 0)
    mappedBasePatch = 2;
  else if (bxy.x == 1)
    mappedBasePatch = 3;
  else if (bxy.x == 2)
    mappedBasePatch = 0;
  else if (bxy.x == 3)
    mappedBasePatch = 1;
  else if (bxy.x == 4)
    mappedBasePatch = 6;
  else if (bxy.x == 5)
    mappedBasePatch = 7;
  else if (bxy.x == 6)
    mappedBasePatch = 4;
  else if (bxy.x == 7)
    mappedBasePatch = 5;
  else if (bxy.x == 8)
    mappedBasePatch = 10;
  else if (bxy.x == 9)
    mappedBasePatch = 11;
  else if (bxy.x == 10)
    mappedBasePatch = 8;
  else
    mappedBasePatch = 9;
  bxy.x = mappedBasePatch;
  TileId mappedTileId(tileId.level(), HEALPix::getLevel(tileId.level()).getPatchIdx(bxy));

  bool       result = true;
  glm::int64 index  = lookupIndex(mappedTileId, indexMap);

  assert(index >= 0);
  glm::int64 offset = index2offset(index, sizeTile);

#if defined(_WIN32)
  _fseeki64(file, offset, SEEK_SET);
#else
  fseeko(file, offset, SEEK_SET);
#endif

  std::array<glm::uint64, 4> childIndices;
  std::fread(childIndices.data(), sizeof(glm::uint64), childIndices.size(), file);

  for (std::size_t i = 0; i < childIndices.size(); ++i) {
    TileId childId = HEALPix::getChildTileId(mappedTileId, static_cast<int>(i));

    if (childIndices[i] != InvalidIndex) {
      indexMap.insert(std::unordered_map<TileId, glm::int64>::value_type(childId, childIndices[i]));
    } else {
      result = false;
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::int64 lookupIndex(
    TileId const& tileId, std::unordered_map<TileId, glm::int64> const& indexMap) {
  glm::int64 result = -1;
  auto       imIt   = indexMap.find(tileId);

  if (imIt != indexMap.end())
    result = imIt->second;

  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies::DBUtil
