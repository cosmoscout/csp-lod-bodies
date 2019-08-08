////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TileSourceDB.hpp"
#include "DBUtil.hpp"
#include "TileNode.hpp"

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void createDummyData(Tile<T>* tile) {
  const float   maxLevel = 10;
  TileId const& tileId   = tile->getTileId();

  T* t = tile->data().data();
  for (int y = 0; y < Tile<T>::SizeY; ++y) {
    for (int x = 0; x < Tile<T>::SizeX; ++x) {
      if (x == 0 || (x + 1) == Tile<T>::SizeX || y == 0 || (y + 1) == Tile<T>::SizeY)
        t[y * Tile<T>::SizeX + x] = T(255.0);
      else
        t[y * Tile<T>::SizeX + x] = T(255.0 * tileId.level() / maxLevel);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Bilinear interpolation of values in @a src sampled at @a sx, @c sx+1,
// x @a sy, @c sy+1 and weighted with @a wx, @a wy.
template <typename T>
T interpolate(T const* src, int sx, int sy, float wx, float wy) {
  float s00 = src[sy * 255 + sx];
  float s10 = src[sy * 255 + sx + 1];
  float s01 = src[(sy + 1) * 255 + sx];
  float s11 = src[(sy + 1) * 255 + sx + 1];

  s00 = std::isnan(s00) ? 0.f : s00;
  s10 = std::isnan(s10) ? 0.f : s10;
  s01 = std::isnan(s01) ? 0.f : s01;
  s11 = std::isnan(s11) ? 0.f : s11;

  float v0 = (1.f - wy) * s00 + wy * s01;
  float v1 = (1.f - wy) * s10 + wy * s11;
  return (1.f - wx) * v0 + wx * v1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Resamples data from @a data into an tile @a tile and takes
// into account that the MarsVis DB stores 255^2 tiles
// while here 257^2 tiles are used.
template <typename DataT, typename TileT>
void resampleData(std::array<DataT, 255 * 255> const& data, Tile<TileT>* tile, unsigned channel,
    unsigned numChannels) {
  DataT const* src = data.data();
  DataT*       dst = (DataT*)tile->data().data();

  for (int y = 0; y < TileBase::SizeY; ++y) {
    // sampleY:  sample position in diskData
    // weightY:  interpolation weight for (sampleYi + 1)
    // sampleYi: integer sample position, interpolated samples are
    //           taken from sampleYi and (sampleYi + 1)
    float const sampleY  = (y / 256.f) * 254.f;
    float       weightY  = sampleY - std::floor(sampleY);
    int         sampleYi = static_cast<int>(std::floor(sampleY));
    // adjust weight and sample location at border
    weightY  = (sampleYi == 254) ? 1.f - weightY : weightY;
    sampleYi = (sampleYi == 254) ? sampleYi - 1 : sampleYi;

    for (int x = 0; x < TileBase::SizeX; ++x) {
      float const sampleX  = (x / 256.f) * 254.f;
      float       weightX  = sampleX - std::floor(sampleX);
      int         sampleXi = static_cast<int>(std::floor(sampleX));
      // adjust weight and sample location at border
      weightX  = (sampleXi == 254) ? 1.f - weightX : weightX;
      sampleXi = (sampleXi == 254) ? sampleXi - 1 : sampleXi;

      DataT v(interpolate(src, sampleXi, sampleYi, weightX, weightY));

      dst[(y * TileBase::SizeX + x) * numChannels + channel] = v;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////

TileSourceDB::TileSourceDB(TileDataType eDataType)
    : m_eDataType(eDataType) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileSourceDB::TileSourceDB(TileDataType eDataType, std::string filename)
    : m_eDataType(eDataType) {
  Channel oChannel;
  oChannel.filename_ = filename;
  oChannel.file_     = std::fopen(filename.c_str(), "rb");

  if (!oChannel.file_)
    throw std::runtime_error("Failed to open file " + filename);

  DBUtil::readFileHeader(oChannel.file_, oChannel.indexMap_);
  mecChannels.push_back(oChannel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileSourceDB::TileSourceDB(TileDataType eDataType, std::array<std::string, 3> const& filenames)
    : m_eDataType(eDataType) {
  for (auto const& filename : filenames) {
    Channel oChannel;
    oChannel.filename_ = filename;
    oChannel.file_     = std::fopen(filename.c_str(), "rb");

    if (!oChannel.file_)
      throw std::runtime_error("Failed to open file " + filename);

    DBUtil::readFileHeader(oChannel.file_, oChannel.indexMap_);
    mecChannels.push_back(oChannel);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileDataType TileSourceDB::getDataType() const {
  return m_eDataType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileNode* TileSourceDB::loadTile(int level, glm::int64 patchIdx) {
  bool childrenAvailable = true;
  auto n                 = new TileNode();
  switch (m_eDataType) {
  case TileDataType::eFloat32: {
    auto tile = new Tile<float>(level, patchIdx);
    n->setTile(tile);
    childrenAvailable = readTileDEM(level, patchIdx, n);
    break;
  }
  case TileDataType::eUInt8: {
    auto tile = new Tile<glm::uint8>(level, patchIdx);
    n->setTile(tile);
    childrenAvailable = readTileR(level, patchIdx, n);
    break;
  }
  case TileDataType::eU8Vec3: {
    auto tile = new Tile<glm::u8vec3>(level, patchIdx);
    n->setTile(tile);
    childrenAvailable = readTileRGB(level, patchIdx, n);
    break;
  }
  }

  if (childrenAvailable) {
    // all children exist -> there is at least one additional level
    n->setChildMaxLevel(level + 1);
  } else {
    // not all children exist -> this is the max level
    n->setChildMaxLevel(level);
  }

  return n;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::loadTileAsync(int level, glm::int64 patchIdx, OnLoadCallback cb) {
  // put request into queue and notify iothread_
  {
    std::unique_lock<std::mutex> rl(requestsMtx_);
    requests_.push_back(TileRequest(TileId(level, patchIdx), cb));
  }
  requestsNotify_.notify_all();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int TileSourceDB::getPendingRequests() {
  std::unique_lock<std::mutex> rl(requestsMtx_);
  return requests_.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::init() {
  if (!ioThread_.joinable()) {
    startThread();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::fini() {
  if (ioThread_.joinable()) {
    stopThread();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool TileSourceDB::readTileR(int level, glm::int64 patchIdx, TileNode* node) {
  glm::int64 const  SizeTileData = 255 * 255 * sizeof(glm::uint8);
  glm::int64 const  SizeTile     = DBUtil::SizeTileHeader + SizeTileData + 7;
  TileId            tileId(level, patchIdx);
  Tile<glm::uint8>* tile              = static_cast<Tile<glm::uint8>*>(node->getTile());
  bool              childrenAvailable = true;

  if (mecChannels.empty()) {
    // create dummy data
    createDummyData(tile);
    childrenAvailable = level < 10;
  } else {
    childrenAvailable &=
        DBUtil::readTileHeader(mecChannels[0].file_, SizeTile, tileId, mecChannels[0].indexMap_);

    std::array<glm::uint8, 255 * 255> dataR;
    std::fread(dataR.data(), sizeof(glm::uint8), dataR.size(), mecChannels[0].file_);

    resampleData(dataR, tile, 0, 1);
  }

  return childrenAvailable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool TileSourceDB::readTileDEM(int level, glm::int64 patchIdx, TileNode* node) {
  glm::int64 const SizeTileData = 255 * 255 * sizeof(float);
  glm::int64 const SizeTile     = DBUtil::SizeTileHeader + SizeTileData + 4;
  TileId           tileId(level, patchIdx);
  Tile<float>*     tile              = static_cast<Tile<float>*>(node->getTile());
  bool             childrenAvailable = true;

  if (mecChannels.empty()) {
    // create dummy data
    createDummyData(tile);
    childrenAvailable = level < 10;
  } else {
    childrenAvailable &=
        DBUtil::readTileHeader(mecChannels[0].file_, SizeTile, tileId, mecChannels[0].indexMap_);

    std::array<float, 255 * 255> data;
    std::fread(data.data(), sizeof(float), data.size(), mecChannels[0].file_);

    resampleData(data, tile, 0, 1);
  }

  // Creating a MinMaxPyramid alongside the sampling beginning with a resolution of 128x128
  // The MinMaxPyramid is later needed to deduce height information from this
  // coarser level DEM tile to deeper level IMG tiles
  auto minMaxPyramid = new MinMaxPyramid(tile);
  tile->setMinMaxPyramid(minMaxPyramid);

  return childrenAvailable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool TileSourceDB::readTileRGB(int level, glm::int64 patchIdx, TileNode* node) {
  glm::int64 const SizeTileData = 255 * 255 * sizeof(glm::uint8);
  glm::int64 const SizeTile     = DBUtil::SizeTileHeader + SizeTileData + 7;

  TileId             tileId(level, patchIdx);
  Tile<glm::u8vec3>* tile              = static_cast<Tile<glm::u8vec3>*>(node->getTile());
  bool               childrenAvailable = true;

  if (mecChannels.empty()) {
    // create dummy data
    createDummyData(tile);
    childrenAvailable = level < 10;
  } else {
    for (int i : {0, 1, 2}) {
      // update index maps with mappings for children
      childrenAvailable &=
          DBUtil::readTileHeader(mecChannels[i].file_, SizeTile, tileId, mecChannels[i].indexMap_);

      // read tile data into temporary storage ...
      std::array<glm::uint8, 255 * 255> data;
      std::fread(data.data(), sizeof(glm::uint8), data.size(), mecChannels[i].file_);

      // ... and resample to 257^2 tile size
      resampleData(data, tile, i, 3);
    }
  }

  return childrenAvailable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::startThread() {
  ioThreadExit_ = false;
  std::thread ioThread(std::bind(&TileSourceDB::ioThreadFunc, this));
  ioThread_.swap(ioThread);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::stopThread() {
  {
    std::unique_lock<std::mutex> rl(requestsMtx_);
    ioThreadExit_ = true;
  }

  requestsNotify_.notify_all();
  ioThread_.join();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileSourceDB::ioThreadFunc() {
  while (!ioThreadExit_) {
    TileRequest req;
    {
      // fetch next TileRequest from queue (requests_) or block on the
      // condition variable (requestsNotify_) until a new request is
      // available
      std::unique_lock<std::mutex> rl(requestsMtx_);
      while (!ioThreadExit_ && requests_.empty()) {
        requestsNotify_.wait(rl);
      }

      if (ioThreadExit_)
        break;

      req = requests_.front();
      requests_.pop_front();
    }

    // load tile and invoke callback
    TileNode* node = loadTile(req.tileId_.level(), req.tileId_.patchIdx());
    assert(req.tileId_.level() == node->getLevel());
    assert(req.tileId_.patchIdx() == node->getPatchIdx());

    req.cb_(this, node->getLevel(), node->getPatchIdx(), node);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileSourceDB::TileRequest::TileRequest()
    : tileId_()
    , cb_() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/* explicit */
TileSourceDB::TileRequest::TileRequest(TileId const& tileId, OnLoadCallback cb)
    : tileId_(tileId)
    , cb_(cb) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
