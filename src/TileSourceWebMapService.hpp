////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_TILESOURCEWMS_HPP
#define CSP_LOD_BODIES_TILESOURCEWMS_HPP

#include "ThreadPool.hpp"
#include "Tile.hpp"
#include "TileSource.hpp"

#include <cstdio>
#include <string>

namespace csp::lodbodies {

/// The data of the tiles is fetched via a web map service.
class TileSourceWebMapService : public TileSource {
 public:
  explicit TileSourceWebMapService(
      std::string const& xmlConfigFile, std::string const& cacheDirectory);
  virtual ~TileSourceWebMapService() {
  }

  virtual void init() {
  }

  virtual void fini() {
  }

  virtual TileNode* loadTile(int level, glm::int64 patchIdx);

  virtual void loadTileAsync(int level, glm::int64 patchIdx, OnLoadCallback cb);
  virtual int  getPendingRequests();

  virtual TileDataType getDataType() const {
    return mFormat;
  }

  int getMaxLevel() const {
    return mMaxLevel;
  }

  /// These can be used to pre-populate the local cache, returns true if the tile is on the diagonal
  /// of base patch 4 (the one which is cut in two halves).
  bool        getXY(int level, glm::int64 patchIdx, int& x, int& y);
  std::string loadData(int level, int x, int y);

 private:
  static std::mutex mTileSystemMutex;

  ThreadPool  mThreadPool;
  std::string mConfig;

  std::string  mUrl;
  std::string  mCache = "cache/img";
  std::string  mLayers;
  std::string  mStyles;
  TileDataType mFormat   = TileDataType::eU8Vec3;
  int          mMaxLevel = 10;
};
} // namespace csp::lodbodies

#endif // CSP_LOD_BODIES_TILESOURCEWMS_HPP
