////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_LOD_PLANET_HPP
#define CSP_LOD_BODIES_LOD_PLANET_HPP

#include "../../../src/cs-graphics/Shadows.hpp"
#include "../../../src/cs-scene/CelestialBody.hpp"

#include "PlanetShader.hpp"
#include "TileSource.hpp"
#include "TileTextureArray.hpp"
#include "VistaPlanet.hpp"
#include <VistaKernel/GraphicsManager/VistaOpenGLDraw.h>

#include <memory>

namespace cs::scene {
class CelestialAnchorNode;
}

namespace cs::core {
class GraphicsEngine;
class GuiManager;
} // namespace cs::core

namespace csp::lodbodies {

/// An LodBody renders a planet from databases of hierarchical tiles. The tile data consists of
/// two components. Image data which determines the texture of the tiles and elevation data
/// (Digital Elevation Model or DEM) which determines the height map of each tile.
///
/// Each planet can make use of multiple data sources for image and elevation data. The user can
/// choose at runtime which data source should be used.
// DocTODO There probably are a thousand more things to explain.
class LodBody : public cs::scene::CelestialBody, public IVistaOpenGLDraw {
 public:
  /// The currently selected data source for elevation data.
  cs::utils::Property<std::string> pActiveTileSourceDEM;

  /// The currently selected data source for image data.
  cs::utils::Property<std::string> pActiveTileSourceIMG;

  LodBody(std::shared_ptr<cs::core::Settings> const&   settings,
      std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
      std::shared_ptr<cs::core::SolarSystem>           solarSystem,
      std::shared_ptr<Plugin::Properties> const&       pProperties,
      std::shared_ptr<cs::core::GuiManager> const& pGuiManager, std::string const& sCenterName,
      std::string const& sFrameName, std::shared_ptr<GLResources> const& glResources,
      std::vector<std::shared_ptr<TileSource>> const& dems,
      std::vector<std::shared_ptr<TileSource>> const& imgs, double tStartExistence,
      double tEndExistence);

  LodBody(LodBody const& other) = delete;
  LodBody(LodBody&& other)      = delete;

  LodBody& operator=(LodBody const& other) = delete;
  LodBody& operator=(LodBody&& other) = delete;

  ~LodBody() override;

  PlanetShader const& getShader() const;

  void setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun);

  /// A list of all data sources for elevation data.
  std::vector<std::shared_ptr<TileSource>> const& getDEMtileSources() const;

  /// A list of all data sources for image data.
  std::vector<std::shared_ptr<TileSource>> const& getIMGtileSources() const;

  bool getIntersection(
      glm::dvec3 const& rayPos, glm::dvec3 const& rayDir, glm::dvec3& pos) const override;
  double     getHeight(glm::dvec2 lngLat) const override;
  glm::dvec3 getRadii() const override;

  void update(double tTime, cs::scene::CelestialObserver const& oObs) override;

  bool Do() override;
  bool GetBoundingBox(VistaBoundingBox& bb) override;

 private:
  std::shared_ptr<cs::core::Settings>               mSettings;
  std::shared_ptr<cs::core::GraphicsEngine>         mGraphicsEngine;
  std::shared_ptr<cs::core::SolarSystem>            mSolarSystem;
  std::shared_ptr<Plugin::Properties>               mProperties;
  std::shared_ptr<const cs::scene::CelestialObject> mSun;
  std::shared_ptr<cs::core::GuiManager>             mGuiManager;

  std::vector<std::shared_ptr<TileSource>> mDEMtileSources;
  std::vector<std::shared_ptr<TileSource>> mIMGtileSources;

  VistaPlanet  mPlanet;
  PlanetShader mShader;
  glm::dvec3   mRadii;
  int          mHeightScaleConnection = -1;
};

} // namespace csp::lodbodies

#endif // CSP_LOD_BODIES_LOD_PLANET_HPP
