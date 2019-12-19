////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "LodBody.hpp"

#include "../../../src/cs-core/GraphicsEngine.hpp"
#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-gui/GuiItem.hpp"
#include "../../../src/cs-utils/FrameTimings.hpp"

#include "utils.hpp"

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

LodBody::LodBody(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
    std::shared_ptr<Plugin::Properties> const&                    pProperties,
    std::shared_ptr<cs::core::GuiManager> const& pGuiManager, std::string const& sCenterName,
    std::string const& sFrameName, std::shared_ptr<GLResources> const& glResources,
    std::vector<std::shared_ptr<TileSource>> const& dems,
    std::vector<std::shared_ptr<TileSource>> const& imgs, double tStartExistence,
    double tEndExistence)
    : cs::scene::CelestialBody(sCenterName, sFrameName, tStartExistence, tEndExistence)
    , mGraphicsEngine(graphicsEngine)
    , mProperties(pProperties)
    , mGuiManager(pGuiManager)
    , mPlanet(glResources)
    , mShader(graphicsEngine, pProperties, pGuiManager)
    , mRadii(cs::core::SolarSystem::getRadii(sCenterName))
    , mDEMtileSources(dems)
    , mIMGtileSources(imgs) {

  pVisible.onChange().connect([this](bool val) {
    if (val)
      mGraphicsEngine->registerCaster(&mPlanet);
    else
      mGraphicsEngine->unregisterCaster(&mPlanet);
  });

  pActiveTileSourceDEM = dems.front()->getName();
  pActiveTileSourceIMG = imgs.front()->getName();

  mPlanet.setTerrainShader(&mShader);
  mPlanet.setLODFactor(mProperties->mLODFactor.get());

  // per-planet settings -----------------------------------------------------
  mPlanet.setEquatorialRadius(mRadii[0]);
  mPlanet.setPolarRadius(mRadii[0]);
  pVisibleRadius = mRadii[0];

  pActiveTileSourceDEM.onChange().connect([this](std::string const& val) {
    for (auto const& s : mDEMtileSources) {
      if (s->getName() == val) {
        mPlanet.setDEMSource(s.get());
        mGuiManager->getGui()->callJavascript(
            "CosmoScout.call", "sidebar", "setElevationDataCopyright", s->getCopyright());
        break;
      }
    }
  });

  pActiveTileSourceIMG.onChange().connect([this](std::string const& val) {
    if (val == "None") {
      mShader.pEnableTexture = false;
      mPlanet.setIMGSource(nullptr);
      mGuiManager->getGui()->callJavascript("CosmoScout.call", "sidebar", "setMapDataCopyright", "");
    } else {
      for (auto const& s : mIMGtileSources) {
        if (s->getName() == val) {
          mShader.pEnableTexture = true;
          mShader.pTextureIsRGB  = (s->getDataType() == TileDataType::eU8Vec3);
          mPlanet.setIMGSource(s.get());
          mGuiManager->getGui()->callJavascript("CosmoScout.call", "sidebar", "setMapDataCopyright", s->getCopyright());
          break;
        }
      }
    }
  });

  // scene-wide settings -----------------------------------------------------
  mGraphicsEngine->pHeightScale.onChange().connect(
      [this](float val) { mPlanet.setHeightScale(val); });

  mProperties->mLODFactor.onChange().connect([this](float val) { mPlanet.setLODFactor(val); });

  mProperties->mEnableWireframe.onChange().connect(
      [this](bool val) { mPlanet.getTileRenderer().setWireframe(val); });

  mProperties->mEnableTilesFreeze.onChange().connect([this](bool val) {
    mPlanet.getLODVisitor().setUpdateLOD(!val);
    mPlanet.getLODVisitor().setUpdateCulling(!val);
  });

  pActiveTileSourceDEM.touch();
  pActiveTileSourceIMG.touch();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LodBody::~LodBody() {
  mGraphicsEngine->unregisterCaster(&mPlanet);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

PlanetShader const& LodBody::getShader() const {
  return mShader;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void LodBody::setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun) {
  mSun = sun;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool LodBody::getIntersection(
    glm::dvec3 const& rayPos, glm::dvec3 const& rayDir, glm::dvec3& pos) const {
  return utils::intersectPlanet(&mPlanet, rayPos, rayDir, pos);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double LodBody::getHeight(glm::dvec2 lngLat) const {
  return utils::getHeight(&mPlanet, HeightSamplePrecision::eActual, lngLat);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::dvec3 LodBody::getRadii() const {
  return mRadii;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::shared_ptr<TileSource>> const& LodBody::getDEMtileSources() const {
  return mDEMtileSources;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::shared_ptr<TileSource>> const& LodBody::getIMGtileSources() const {
  return mIMGtileSources;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void LodBody::update(double tTime, cs::scene::CelestialObserver const& oObs) {
  cs::scene::CelestialObject::update(tTime, oObs);

  if (getIsInExistence() && pVisible.get()) {
    mPlanet.setWorldTransform(getWorldTransform());

    if (mSun) {
      auto sunDir = glm::normalize(
          glm::inverse(matWorldTransform) * (mSun->getWorldPosition() - getWorldPosition()));
      mShader.setSunDirection(VistaVector3D(sunDir[0], sunDir[1], sunDir[2]));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool LodBody::Do() {
  if (getIsInExistence() && pVisible.get()) {
    cs::utils::FrameTimings::ScopedTimer timer("LoD-Body " + getCenterName());
    mPlanet.Do();
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool LodBody::GetBoundingBox(VistaBoundingBox& bb) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
