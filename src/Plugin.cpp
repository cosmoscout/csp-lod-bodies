////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "LodBody.hpp"
#include "TileSourceWebMapService.hpp"
#include "logger.hpp"

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
  delete pluginBase; // NOLINT(cppcoreguidelines-owning-memory)
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, TileDataType& o) {
  auto s = j.get<std::string>();
  if (s == "Float32") {
    o = TileDataType::eFloat32;
  } else if (s == "UInt8") {
    o = TileDataType::eUInt8;
  } else if (s == "U8Vec3") {
    o = TileDataType::eU8Vec3;
  } else {
    throw std::runtime_error(
        "Failed to parse TileDataType! Only 'Float32', 'UInt8' or 'U8Vec3' are allowed.");
  }
}

void to_json(nlohmann::json& j, TileDataType o) {
  switch (o) {
  case TileDataType::eFloat32:
    j = "Float32";
    break;
  case TileDataType::eUInt8:
    j = "UInt8";
    break;
  case TileDataType::eU8Vec3:
    j = "U8Vec3";
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Plugin::Settings::Dataset& o) {
  cs::core::Settings::deserialize(j, "format", o.mFormat);
  cs::core::Settings::deserialize(j, "name", o.mName);
  cs::core::Settings::deserialize(j, "copyright", o.mCopyright);
  cs::core::Settings::deserialize(j, "layers", o.mLayers);
  cs::core::Settings::deserialize(j, "maxLevel", o.mMaxLevel);
  cs::core::Settings::deserialize(j, "url", o.mURL);
}

void to_json(nlohmann::json& j, Plugin::Settings::Dataset const& o) {
  cs::core::Settings::serialize(j, "format", o.mFormat);
  cs::core::Settings::serialize(j, "name", o.mName);
  cs::core::Settings::serialize(j, "copyright", o.mCopyright);
  cs::core::Settings::serialize(j, "layers", o.mLayers);
  cs::core::Settings::serialize(j, "maxLevel", o.mMaxLevel);
  cs::core::Settings::serialize(j, "url", o.mURL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Plugin::Settings::Body& o) {
  cs::core::Settings::deserialize(j, "demDatasets", o.mDemDatasets);
  cs::core::Settings::deserialize(j, "imgDatasets", o.mImgDatasets);
}

void to_json(nlohmann::json& j, Plugin::Settings::Body const& o) {
  cs::core::Settings::serialize(j, "demDatasets", o.mDemDatasets);
  cs::core::Settings::serialize(j, "imgDatasets", o.mImgDatasets);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Plugin::Settings& o) {
  cs::core::Settings::deserialize(j, "terrainProjectionType", o.mTerrainProjectionType);
  cs::core::Settings::deserialize(j, "lodFactor", o.mLODFactor);
  cs::core::Settings::deserialize(j, "autoLod", o.mAutoLOD);
  cs::core::Settings::deserialize(j, "textureGamma", o.mTextureGamma);
  cs::core::Settings::deserialize(j, "enableHeightlines", o.mEnableHeightlines);
  cs::core::Settings::deserialize(j, "enableLatLongGrid", o.mEnableLatLongGrid);
  cs::core::Settings::deserialize(j, "enableLatLongGridLabels", o.mEnableLatLongGridLabels);
  cs::core::Settings::deserialize(j, "colorMappingType", o.mColorMappingType);
  cs::core::Settings::deserialize(j, "terrainColorMap", o.mTerrainColorMap);
  cs::core::Settings::deserialize(j, "enableColorMixing", o.mEnableColorMixing);
  cs::core::Settings::deserialize(j, "heightMax", o.mHeightMax);
  cs::core::Settings::deserialize(j, "heightMin", o.mHeightMin);
  cs::core::Settings::deserialize(j, "slopeMax", o.mSlopeMax);
  cs::core::Settings::deserialize(j, "slopeMin", o.mSlopeMin);
  cs::core::Settings::deserialize(j, "enableWireframe", o.mEnableWireframe);
  cs::core::Settings::deserialize(j, "enableTilesDebug", o.mEnableTilesDebug);
  cs::core::Settings::deserialize(j, "enableTilesFreeze", o.mEnableTilesFreeze);
  cs::core::Settings::deserialize(j, "maxGPUTilesColor", o.mMaxGPUTilesColor);
  cs::core::Settings::deserialize(j, "maxGPUTilesGray", o.mMaxGPUTilesGray);
  cs::core::Settings::deserialize(j, "maxGPUTilesDEM", o.mMaxGPUTilesDEM);
  cs::core::Settings::deserialize(j, "mapCache", o.mMapCache);
  cs::core::Settings::deserialize(j, "bodies", o.mBodies);
}

void to_json(nlohmann::json& j, Plugin::Settings const& o) {
  cs::core::Settings::serialize(j, "terrainProjectionType", o.mTerrainProjectionType);
  cs::core::Settings::serialize(j, "lodFactor", o.mLODFactor);
  cs::core::Settings::serialize(j, "autoLod", o.mAutoLOD);
  cs::core::Settings::serialize(j, "textureGamma", o.mTextureGamma);
  cs::core::Settings::serialize(j, "enableHeightlines", o.mEnableHeightlines);
  cs::core::Settings::serialize(j, "enableLatLongGrid", o.mEnableLatLongGrid);
  cs::core::Settings::serialize(j, "enableLatLongGridLabels", o.mEnableLatLongGridLabels);
  cs::core::Settings::serialize(j, "colorMappingType", o.mColorMappingType);
  cs::core::Settings::serialize(j, "terrainColorMap", o.mTerrainColorMap);
  cs::core::Settings::serialize(j, "enableColorMixing", o.mEnableColorMixing);
  cs::core::Settings::serialize(j, "heightMax", o.mHeightMax);
  cs::core::Settings::serialize(j, "heightMin", o.mHeightMin);
  cs::core::Settings::serialize(j, "slopeMax", o.mSlopeMax);
  cs::core::Settings::serialize(j, "slopeMin", o.mSlopeMin);
  cs::core::Settings::serialize(j, "enableWireframe", o.mEnableWireframe);
  cs::core::Settings::serialize(j, "enableTilesDebug", o.mEnableTilesDebug);
  cs::core::Settings::serialize(j, "enableTilesFreeze", o.mEnableTilesFreeze);
  cs::core::Settings::serialize(j, "maxGPUTilesColor", o.mMaxGPUTilesColor);
  cs::core::Settings::serialize(j, "maxGPUTilesGray", o.mMaxGPUTilesGray);
  cs::core::Settings::serialize(j, "maxGPUTilesDEM", o.mMaxGPUTilesDEM);
  cs::core::Settings::serialize(j, "mapCache", o.mMapCache);
  cs::core::Settings::serialize(j, "bodies", o.mBodies);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {

  logger().info("Loading plugin...");

  mOnLoadConnection = mAllSettings->onLoad().connect([this]() { onLoad(); });

  mOnSaveConnection = mAllSettings->onSave().connect(
      [this]() { mAllSettings->mPlugins["csp-lod-bodies"] = *mPluginSettings; });

  mGuiManager->addPluginTabToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_body_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_body_settings.html");
  mGuiManager->addScriptToGuiFromJS("../share/resources/gui/js/csp-lod-bodies.js");

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableTilesFreeze",
      "If set to true, the level of detail and the frustum culling of the planet's tiles will not "
      "be updated anymore.",
      std::function([this](bool enable) { mPluginSettings->mEnableTilesFreeze = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableTilesDebug",
      "Enables or disables debug coloring of the planet's tiles.",
      std::function([this](bool enable) { mPluginSettings->mEnableTilesDebug = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableWireframe",
      "Enables or disables wireframe rendering of the planet.",
      std::function([this](bool enable) { mPluginSettings->mEnableWireframe = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableHeightlines",
      "Enables or disables rendering of iso-altitude lines.",
      std::function([this](bool enable) { mPluginSettings->mEnableHeightlines = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableLatLongGrid",
      "Enables or disables rendering of a latidude-longitude-grid.",
      std::function([this](bool enable) {
        mPluginSettings->mEnableLatLongGrid       = enable;
        mPluginSettings->mEnableLatLongGridLabels = enable;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableLatLongGridLabels",
      "If the latitude-longitude-grid is enabled, this function can be used to enable or disable "
      "rendering of grid labels.",
      std::function([this](bool enable) { mPluginSettings->mEnableLatLongGridLabels = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableColorMixing",
      "When enabled, the values of the colormap will be multiplied with the image channel.",
      std::function([this](bool enable) { mPluginSettings->mEnableColorMixing = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainLod",
      "Specifies the amount of detail of the planet's surface. Should be in the range 1-100.",
      std::function([this](double value) {
        if (!mPluginSettings->mAutoLOD.get()) {
          mPluginSettings->mLODFactor = static_cast<float>(value);
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setEnableAutoTerrainLod",
      "If set to true, the level-of-detail will be chosen automatically based on the current "
      "rendering performance.",
      std::function([this](bool enable) { mPluginSettings->mAutoLOD = enable; }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTextureGamma",
      "A multiplier for the brightness of the image channel.", std::function([this](double value) {
        mPluginSettings->mTextureGamma = static_cast<float>(value);
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setHeightRange",
      "Sets one end of the height range for the color mapping. The first parameter is the actual "
      "value, the second specifies which end to set: Zero for the lower end; One for the upper "
      "end.",
      std::function([this](double val, double handle) {
        if (handle == 0.0) {
          mPluginSettings->mHeightMin = static_cast<float>(val * 1000);
        } else {
          mPluginSettings->mHeightMax = static_cast<float>(val * 1000);
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSlopeRange",
      "Sets one end of the slope range for the color mapping. The first parameter is the actual "
      "value, the second specifies which end to set: Zero for the lower end; One for the upper "
      "end.",
      std::function([this](double val, double handle) {
        if (handle == 0.0) {
          mPluginSettings->mSlopeMin = static_cast<float>(cs::utils::convert::toRadians(val));
        } else {
          mPluginSettings->mSlopeMax = static_cast<float>(cs::utils::convert::toRadians(val));
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode0",
      "Call this to deselect any surface coloring.", std::function([this] {
        mPluginSettings->mColorMappingType = Settings::ColorMappingType::eNone;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode1",
      "Call this to enable height based surface coloring.", std::function([this] {
        mPluginSettings->mColorMappingType = Settings::ColorMappingType::eHeight;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setSurfaceColoringMode2",
      "Call this to enable slope based surface coloring.", std::function([this] {
        mPluginSettings->mColorMappingType = Settings::ColorMappingType::eSlope;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode0",
      "Call this to use a GPU-based HEALPix projection for the planet's surface.",
      std::function([this] {
        mPluginSettings->mTerrainProjectionType = Settings::TerrainProjectionType::eHEALPix;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode1",
      "Call this to use a CPU-based HEALPix projection and a linear interpolation on the GPU-side "
      "for the planet's surface.",
      std::function([this] {
        mPluginSettings->mTerrainProjectionType = Settings::TerrainProjectionType::eLinear;
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTerrainProjectionMode2",
      "Call this to choose a projection for the planet's surface based on the observer's distance.",
      std::function([this] {
        mPluginSettings->mTerrainProjectionType = Settings::TerrainProjectionType::eHybrid;
      }));

  mGLResources = std::make_shared<csp::lodbodies::GLResources>(mPluginSettings->mMaxGPUTilesDEM,
      mPluginSettings->mMaxGPUTilesGray, mPluginSettings->mMaxGPUTilesColor);

  for (auto const& bodySettings : mPluginSettings->mBodies) {
    auto anchor = mAllSettings->mAnchors.find(bodySettings.first);

    if (anchor == mAllSettings->mAnchors.end()) {
      throw std::runtime_error(
          "There is no Anchor \"" + bodySettings.first + "\" defined in the settings.");
    }

    auto [tStartExistence, tEndExistence] = anchor->second.getExistence();

    std::vector<std::shared_ptr<TileSource>> DEMs;
    std::vector<std::shared_ptr<TileSource>> IMGs;
    for (auto const& dataset : bodySettings.second.mDemDatasets) {
      auto dem = std::make_shared<TileSourceWebMapService>();
      dem->setCacheDirectory(mPluginSettings->mMapCache);
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
      img->setCacheDirectory(mPluginSettings->mMapCache);
      img->setMaxLevel(dataset.mMaxLevel);
      img->setLayers(dataset.mLayers);
      img->setUrl(dataset.mURL);
      img->setDataType(dataset.mFormat);
      img->setName(dataset.mName);
      img->setCopyright(dataset.mCopyright);
      IMGs.push_back(img);
    }

    auto body = std::make_shared<LodBody>(mAllSettings, mGraphicsEngine, mSolarSystem,
        mPluginSettings, mGuiManager, anchor->second.mCenter, anchor->second.mFrame, mGLResources,
        DEMs, IMGs, tStartExistence, tEndExistence);

    mSolarSystem->registerBody(body);

    body->setSun(mSolarSystem->getSun());
    auto* parent = mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get());
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
          for (auto const& source : body->getIMGtileSources()) {
            if (source->getName() == name) {
              mGuiManager->getGui()->callJavascript(
                  "CosmoScout.lodBodies.setMapDataCopyright", source->getCopyright());
            }
          }
        }
      }));

  mGuiManager->getGui()->registerCallback("lodBodies.setTilesDem",
      "Set the current planet's elevation channel to the TileSource with the given name.",
      std::function([this](std::string&& name) {
        auto body = std::dynamic_pointer_cast<LodBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          body->pActiveTileSourceDEM = name;
          for (auto const& source : body->getDEMtileSources()) {
            if (source->getName() == name) {
              mGuiManager->getGui()->callJavascript(
                  "CosmoScout.lodBodies.setElevationDataCopyright", source->getCopyright());
            }
          }
        }
      }));

  mNonAutoLod = mPluginSettings->mLODFactor.get();

  mPluginSettings->mAutoLOD.connect([this](bool enabled) {
    if (enabled) {
      mNonAutoLod = mPluginSettings->mLODFactor.get();
    } else {
      mPluginSettings->mLODFactor = mNonAutoLod;
      mGuiManager->getGui()->callJavascript(
          "CosmoScout.gui.setSliderValue", "lodBodies.setTerrainLod", mNonAutoLod);
    }
  });

  mPluginSettings->mLODFactor.connect([this](float value) {
    if (mPluginSettings->mAutoLOD()) {
      mGuiManager->getGui()->callJavascript(
          "CosmoScout.gui.setSliderValue", "lodBodies.setTerrainLod", value);
    }
  });

  logger().info("Loading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  logger().info("Unloading plugin...");

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

  logger().info("Unloading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::update() {
  if (mPluginSettings->mAutoLOD.get()) {

    double minLODFactor = 15.0;
    double maxLODFactor = 50.0;
    double minTime      = 13.5;
    double maxTime      = 14.5;

    if (mFrameTimings->pFrameTime.get() > maxTime) {
      mPluginSettings->mLODFactor = static_cast<float>(std::max(
          minLODFactor, mPluginSettings->mLODFactor.get() -
                            std::min(1.0, 0.1 * (mFrameTimings->pFrameTime.get() - maxTime))));
    } else if (mFrameTimings->pFrameTime.get() < minTime) {
      mPluginSettings->mLODFactor = static_cast<float>(std::min(
          maxLODFactor, mPluginSettings->mLODFactor.get() +
                            std::min(1.0, 0.02 * (minTime - mFrameTimings->pFrameTime.get()))));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
