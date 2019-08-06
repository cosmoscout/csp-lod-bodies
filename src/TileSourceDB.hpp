////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_PLANET_TILESOURCEDB_HPP
#define CSP_LOD_PLANET_TILESOURCEDB_HPP

#include "Tile.hpp"
#include "TileSource.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace csp::lodplanets {

class TileNode;

/// Base class to load tiles from databases built for the MarsVis renderer.
/// The format for the MarsVis files is a header followed by a (large) number of per patch data.
///
/// @code
///     Header:
///     12 uint64       Each uint64 is the index (see below) of one of the 12 base patches.
///
///     Patch Data:
///     4 uint64        Each uint64 is the index (see below) of one of the 4 children of this patch,
///                     in the order: S, E, W, N
///
///     255^2 values    The data type of the values depends on the type of file being read (e.g. for
///                     dtm.db it is float, for red.db or bw.db it is uint8).
///
///     padding         Patch data is padded so its length is a multiple of 8.
/// @endcode
///
/// The patch indices used in the Header and Patch Data can be used to compute the offset from the
/// beginning of the file where the data for the patch can be located. The offset for `index` is
/// calculated as:
///
///     sizeof(Header) + sizeof(Patch Data) * index
class TileSourceDB : public TileSource {
 public:
  TileSourceDB(TileDataType);
  TileSourceDB(TileDataType, std::string filename);
  TileSourceDB(TileDataType, std::array<std::string, 3> const& filenames);

  virtual TileDataType getDataType() const;

  virtual TileNode* loadTile(int level, glm::int64 patchIdx);
  virtual void      loadTileAsync(int level, glm::int64 patchIdx, OnLoadCallback cb);
  virtual int       getPendingRequests();

  virtual void init();
  virtual void fini();

 protected:
  virtual bool readTileR(int level, glm::int64 patchIdx, TileNode* node);
  virtual bool readTileDEM(int level, glm::int64 patchIdx, TileNode* node);
  virtual bool readTileRGB(int level, glm::int64 patchIdx, TileNode* node);

  void startThread();
  void stopThread();

 private:
  struct Channel {
    std::string                            filename_;
    FILE*                                  file_;
    std::unordered_map<TileId, glm::int64> indexMap_;
  };

  struct TileRequest {
    TileRequest();
    explicit TileRequest(TileId const& tileId, OnLoadCallback cb);

    TileId         tileId_;
    OnLoadCallback cb_;
  };

  void ioThreadFunc();

  std::deque<TileRequest> requests_;
  std::mutex              requestsMtx_;
  std::condition_variable requestsNotify_;
  std::thread             ioThread_;
  bool                    ioThreadExit_;
  std::vector<Channel>    mecChannels;
  TileDataType            m_eDataType;
};
} // namespace csp::lodplanets

#endif // CSP_LOD_PLANET_TILESOURCEDB_HPP
