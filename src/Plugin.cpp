////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "LodBody.hpp"
#include "TileSourceWebMapService.hpp"

#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-utils/convert.hpp"
#include "../../../src/cs-utils/logger.hpp"

#include <VistaKernel/GraphicsManager/VistaGroupNode.h>
#include <VistaKernel/GraphicsManager/VistaOpenGLNode.h>
#include <VistaKernel/GraphicsManager/VistaSceneGraph.h>
#include <VistaKernelOpenSGExt/VistaOpenSGMaterialTools.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN cs::core::PluginBase* create() {
  return new csp::lodbodies::Plugin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN void destroy(cs::core::PluginBase* pluginBase) {
  delete pluginBase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, TileDataType& o) {
  if (j.get<std::string>() == "Float32") {
    o = TileDataType::eFloat32;
    return;
  }

  if (j.get<std::string>() == "UInt8") {
    o = TileDataType::eUInt8;
    return;
  }

  if (j.get<std::string>() == "U8Vec3") {
    o = TileDataType::eU8Vec3;
    return;
  }

  throw std::runtime_error("Invalid data set format \"" + j.get<std::string>() + "\" in config!");
}

void from_json(const nlohmann::json& j, Plugin::Settings::Dataset& o) {
  o.mFormat    = cs::core::parseProperty<TileDataType>("format", j);
  o.mName      = cs::core::parseProperty<std::string>("name", j);
  o.mCopyright = cs::core::parseProperty<std::string>("copyright", j);
  o.mLayers    = cs::core::parseProperty<std::string>("layers", j);
  o.mMaxLevel  = cs::core::parseProperty<uint32_t>("maxLevel", j);
  o.mURL       = cs::core::parseProperty<std::string>("url", j);
}

void from_json(const nlohmann::json& j, Plugin::Settings::Body& o) {
  o.mDemDatasets = cs::core::parseVector<Plugin::Settings::Dataset>("demDatasets", j);
  o.mImgDatasets = cs::core::parseVector<Plugin::Settings::Dataset>("imgDatasets", j);
}

void from_json(const nlohmann::json& j, Plugin::Settings& o) {
  cs::core::parseSection("csp-lod-bodies", [&] {
    o.mMaxGPUTilesColor = cs::core::parseProperty<uint32_t>("maxGPUTilesColor", j);
    o.mMaxGPUTilesGray  = cs::core::parseProperty<uint32_t>("maxGPUTilesGray", j);
    o.mMaxGPUTilesDEM   = cs::core::parseProperty<uint32_t>("maxGPUTilesDEM", j);
    o.mMapCache         = cs::core::parseProperty<std::string>("mapCache", j);

    o.mBodies = cs::core::parseMap<std::string, Plugin::Settings::Body>("bodies", j);
  });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Plugin::Plugin()
    : mProperties(std::make_shared<Properties>()) {

  // Create default logger for this plugin.
  spdlog::set_default_logger(cs::utils::logger::createLogger("csp-lod-bodies"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {

  spdlog::info("Loading plugin...");

  mPluginSettings = mAllSettings->mPlugins.at("csp-lod-bodies");

  mGuiManager->addPluginTabToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_body_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_body_settings.html");
  mGuiManager->addScriptToGuiFromJS("../share/resources/gui/js/csp-lod-bodies.js");

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableTilesFreeze",
      "If set to true, the level of detail and the frustum culling of the planet's tiles will not "
      "be updated anymore.",
      std::function([this](bool enable) { mProperties->mEnableTilesFreeze = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableTilesDebug",
      "Enables or disables coloring of the planet's tiles.",
      std::function([this](bool enable) { mProperties->mEnableTilesDebug = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableWireframe",
      "Enables or disables wireframe rendering of the planet.",
      std::function([this](bool enable) { mProperties->mEnableWireframe = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableHeightlines",
      "Enables or disables rendering of iso-altitude lines.",
      std::function([this](bool enable) { mProperties->mEnableHeightlines = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableLatLongGrid",
      "Enables or disables rendering of a latidude-longitude-grid.",
      std::function([this](bool enable) {
        mProperties->mEnableLatLongGrid       = enable;
        mProperties->mEnableLatLongGridLabels = enable;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableLatLongGridLabels",
      "If the latitude-longitude-grid is enabled, this function can be used to enable or disable "
      "rendering of grid labels.",
      std::function([this](bool enable) { mProperties->mEnableLatLongGridLabels = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableColorMixing",
      "When enabled, the values of the colormap will be multiplied with the image channel.",
      std::function([this](bool enable) { mProperties->mEnableColorMixing = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainLod",
      "Specifies the amount of detail of the planet's surface. Should be in the range 1-100.",
      std::function([this](double value) {
        if (!mProperties->mAutoLOD.get()) {
          mProperties->mLODFactor = static_cast<float>(value);
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableAutoTerrainLod",
      "If set to true, the level-of-detail will be chosen automatically based on the current "
      "rendering performance.",
      std::function([this](bool enable) { mProperties->mAutoLOD = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTextureGamma",
      "A multiplier for the brightness of the image channel.", std::function([this](double value) {
        mProperties->mTextureGamma = static_cast<float>(value);
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setHeightRange",
      "Sets one end of the height range for the color mapping. The first parameter is the actual "
      "value, the second specifies which end to set: Zero for the lower end; One for the upper "
      "end.",
      std::function([this](double val, double handle) {
        if (handle == 0.0)
          mProperties->mHeightMin = static_cast<float>(val * 1000);
        else
          mProperties->mHeightMax = static_cast<float>(val * 1000);
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSlopeRange",
      "Sets one end of the slope range for the color mapping. The first parameter is the actual "
      "value, the second specifies which end to set: Zero for the lower end; One for the upper "
      "end.",
      std::function([this](double val, double handle) {
        if (handle == 0.0)
          mProperties->mSlopeMin = static_cast<float>(cs::utils::convert::toRadians(val));
        else
          mProperties->mSlopeMax = static_cast<float>(cs::utils::convert::toRadians(val));
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode0",
      "Call this to deselect any surface coloring.", std::function([this] {
        mProperties->mColorMappingType = Properties::ColorMappingType::eNone;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode1",
      "Call this to enable height based surface coloring.", std::function([this] {
        mProperties->mColorMappingType = Properties::ColorMappingType::eHeight;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode2",
      "Call this to enable slope based surface coloring.", std::function([this] {
        mProperties->mColorMappingType = Properties::ColorMappingType::eSlope;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode0",
      "Call this to use a GPU-based HEALPix projection for the planet's surface.",
      std::function([this] {
        mProperties->mTerrainProjectionType = Properties::TerrainProjectionType::eHEALPix;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode1",
      "Call this to use a CPU-based HEALPix projection and a linear interpolation on the GPU-side "
      "for the planet's surface.",
      std::function([this] {
        mProperties->mTerrainProjectionType = Properties::TerrainProjectionType::eLinear;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode2",
      "Call this to choose a projection for the planet's surface based on the observer's distance.",
      std::function([this] {
        mProperties->mTerrainProjectionType = Properties::TerrainProjectionType::eHybrid;
      }));

  mGLResources = std::make_shared<csp::lodbodies::GLResources>(mPluginSettings.mMaxGPUTilesDEM,
      mPluginSettings.mMaxGPUTilesGray, mPluginSettings.mMaxGPUTilesColor);

  for (auto const& bodySettings : mPluginSettings.mBodies) {
    auto anchor = mAllSettings->mAnchors.find(bodySettings.first);

    if (anchor == mAllSettings->mAnchors.end()) {
      throw std::runtime_error(
          "There is no Anchor \"" + bodySettings.first + "\" defined in the settings.");
    }

    auto   existence       = cs::core::getExistenceFromSettings(*anchor);
    double tStartExistence = existence.first;
    double tEndExistence   = existence.second;

    std::vector<std::shared_ptr<TileSource>> DEMs;
    std::vector<std::shared_ptr<TileSource>> IMGs;
    for (auto const& dataset : bodySettings.second.mDemDatasets) {
      auto dem = std::make_shared<TileSourceWebMapService>();
      dem->setCacheDirectory(mPluginSettings.mMapCache);
      dem->setMaxLevel(dataset.mMaxLevel);
      dem->setLayers(dataset.mLayers);
      dem->setUrl(dataset.mURL);
      dem->setDataType(dataset.mFormat);
      dem->setName(dataset.mName);
      dem->setCopyright(dataset.mCopyright);
      DEMs.push_back(dem);
    }

    for (auto const& dataset : bodySettings.second.mImgDatasets) {
      auto img = std::make_shared<TileSourceWebMapService>();
      img->setCacheDirectory(mPluginSettings.mMapCache);
      img->setMaxLevel(dataset.mMaxLevel);
      img->setLayers(dataset.mLayers);
      img->setUrl(dataset.mURL);
      img->setDataType(dataset.mFormat);
      img->setName(dataset.mName);
      img->setCopyright(dataset.mCopyright);
      IMGs.push_back(img);
    }

    auto body = std::make_shared<LodBody>(mGraphicsEngine, mSolarSystem, mProperties, mGuiManager,
        anchor->second.mCenter, anchor->second.mFrame, mGLResources, DEMs, IMGs, tStartExistence,
        tEndExistence);

    mSolarSystem->registerBody(body);

    body->setSun(mSolarSystem->getSun());
    auto parent = mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get());
    VistaOpenSGMaterialTools::SetSortKeyOnSubtree(
        parent, static_cast<int>(cs::utils::DrawOrder::ePlanets));

    mOpenGLNodes.emplace_back(parent);

    mInputManager->registerSelectable(body);
    mLodBodies.push_back(body);
  }

  mActiveBodyConnection = mSolarSystem->pActiveBody.connectAndTouch(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        auto lodBody = std::dynamic_pointer_cast<LodBody>(body);

        mGuiManager->getGui()->callJavascript(
            "CosmoScout.sidebar.setTabEnabled", "Body Settings", lodBody != nullptr);

        if (!lodBody) {
          return;
        }

        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.clearDropdown", "lodBodies.setTilesImg");
        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.clearDropdown", "lodBodies.setTilesDem");
        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.addDropdownValue", "lodBodies.setTilesImg", "None", "None", "false");
        for (auto const& source : lodBody->getIMGtileSources()) {
          bool active = source->getName() == lodBody->pActiveTileSourceIMG.get();
          mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue",
              "lodBodies.setTilesImg", source->getName(), source->getName(), active);
          if (active) {
            mGuiManager->getGui()->callJavascript(
                "CosmoScout.lodBodies.setMapDataCopyright", source->getCopyright());
          }
        }
        for (auto const& source : lodBody->getDEMtileSources()) {
          bool active = source->getName() == lodBody->pActiveTileSourceDEM.get();
          mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue",
              "lodBodies.setTilesDem", source->getName(), source->getName(), active);
          if (active) {
            mGuiManager->getGui()->callJavascript(
                "CosmoScout.lodBodies.setElevationDataCopyright", source->getCopyright());
          }
        }
      });

  mGuiManager->getGui()->registerCallback("lodBodies.setTilesImg",
      "Set the current planet's image channel to the TileSource with the given name.",
      std::function([this](std::string&& name) {
        auto body = std::dynamic_pointer_cast<LodBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          body->pActiveTileSourceIMG = name;
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTilesDem",
      "Set the current planet's elevation channel to the TileSource with the given name.",
      std::function([this](std::string&& name) {
        auto body = std::dynamic_pointer_cast<LodBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          body->pActiveTileSourceDEM = name;
        }
      }));

  mNonAutoLod = mProperties->mLODFactor.get();

  mProperties->mAutoLOD.connect([this](bool enabled) {
    if (enabled) {
      mNonAutoLod = mProperties->mLODFactor.get();
    } else {
      mProperties->mLODFactor = mNonAutoLod;
      mGuiManager->getGui()->callJavascript(
          "CosmoScout.gui.setSliderValue", "lodBodies.setTerrainLod", mNonAutoLod);
    }
  });

  mProperties->mLODFactor.connect([this](float value) {
    if (mProperties->mAutoLOD()) {
      mGuiManager->getGui()->callJavascript(
          "CosmoScout.gui.setSliderValue", "lodBodies.setTerrainLod", value);
    }
  });

  spdlog::info("Loading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  spdlog::info("Unloading plugin...");

  for (auto const& body : mLodBodies) {
    mInputManager->unregisterSelectable(body);
    mSolarSystem->unregisterBody(body);
  }

  for (auto const& node : mOpenGLNodes) {
    mSceneGraph->GetRoot()->DisconnectChild(node.get());
  }

  mSolarSystem->pActiveBody.disconnect(mActiveBodyConnection);

  mGuiManager->removePluginTab("Body Settings");
  mGuiManager->removeSettingsSection("Body Settings");

  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableTilesFreeze");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableTilesDebug");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableWireframe");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableHeightlines");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableLatLongGrid");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableLatLongGridLabels");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableColorMixing");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTerrainLod");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setEnableAutoTerrainLod");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTextureGamma");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setHeightRange");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setSlopeRange");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setSurfaceColoringMode0");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setSurfaceColoringMode1");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setSurfaceColoringMode2");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTerrainProjectionMode0");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTerrainProjectionMode1");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTerrainProjectionMode2");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTilesImg");
  mGuiManager->getGui()->unregisterCallback("lodBodies.setTilesDem");

  spdlog::info("Unloading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::update() {
  if (mProperties->mAutoLOD.get()) {

    double minLODFactor = 15.0;
    double maxLODFactor = 50.0;
    double minTime      = 13.5;
    double maxTime      = 14.5;

    if (mFrameTimings->pFrameTime.get() > maxTime) {
      mProperties->mLODFactor = static_cast<float>(std::max(
          minLODFactor, mProperties->mLODFactor.get() -
                            std::min(1.0, 0.1 * (mFrameTimings->pFrameTime.get() - maxTime))));
    } else if (mFrameTimings->pFrameTime.get() < minTime) {
      mProperties->mLODFactor = static_cast<float>(std::min(
          maxLODFactor, mProperties->mLODFactor.get() +
                            std::min(1.0, 0.02 * (minTime - mFrameTimings->pFrameTime.get()))));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
