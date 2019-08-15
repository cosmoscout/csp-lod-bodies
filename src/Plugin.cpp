////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "LodBody.hpp"
#include "TileSourceDB.hpp"
#include "TileSourceWebMapService.hpp"

#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-gui/GuiItem.hpp"
#include "../../../src/cs-utils/convert.hpp"

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

void from_json(const nlohmann::json& j, Plugin::Settings::Dataset& o) {
  o.mName            = j.at("name").get<std::string>();
  o.mCopyright       = j.at("copyright").get<std::string>();
  o.mIsWebMapService = j.at("wms").get<bool>();
  o.mFiles           = j.at("files").get<std::vector<std::string>>();
}

void from_json(const nlohmann::json& j, Plugin::Settings::Body& o) {
  o.mDemDatasets = j.at("demDatasets").get<std::vector<Plugin::Settings::Dataset>>();
  o.mImgDatasets = j.at("imgDatasets").get<std::vector<Plugin::Settings::Dataset>>();
}

void from_json(const nlohmann::json& j, Plugin::Settings& o) {
  o.mMaxGPUTilesColor = j.at("maxGPUTilesColor").get<uint32_t>();
  o.mMaxGPUTilesGray  = j.at("maxGPUTilesGray").get<uint32_t>();
  o.mMaxGPUTilesDEM   = j.at("maxGPUTilesDEM").get<uint32_t>();
  o.mMapCache         = j.at("mapCache").get<std::string>();
  o.mBodies           = j.at("bodies").get<std::map<std::string, Plugin::Settings::Body>>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Plugin::Plugin()
    : mProperties(std::make_shared<Properties>()) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {
  std::cout << "Loading: CosmoScout VR Plugin Lod-Bodies" << std::endl;

  mPluginSettings = mAllSettings->mPlugins.at("csp-lod-bodies");

  mGuiManager->addPluginTabToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_bodies_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "Body Settings", "landscape", "../share/resources/gui/lod_body_settings.html");
  mGuiManager->addScriptToSideBarFromJS("../share/resources/gui/js/lod_body_settings.js");

  mGuiManager->getSideBar()->registerCallback<bool>("set_enable_tiles_freeze",
      ([this](bool enable) { mProperties->mEnableTilesFreeze = enable; }));

  mGuiManager->getSideBar()->registerCallback<bool>(
      "set_enable_tiles_debug", ([this](bool enable) { mProperties->mEnableTilesDebug = enable; }));

  mGuiManager->getSideBar()->registerCallback<bool>(
      "set_enable_wireframe", ([this](bool enable) { mProperties->mEnableWireframe = enable; }));

  mGuiManager->getSideBar()->registerCallback<bool>("set_enable_heightlines",
      ([this](bool enable) { mProperties->mEnableHeightlines = enable; }));

  mGuiManager->getSideBar()->registerCallback<bool>(
      "set_enable_lat_long_grid", ([this](bool enable) {
        mProperties->mEnableLatLongGrid       = enable;
        mProperties->mEnableLatLongGridLabels = enable;
      }));

  mGuiManager->getSideBar()->registerCallback<bool>("set_enable_lat_long_grid_labels",
      ([this](bool enable) { mProperties->mEnableLatLongGridLabels = enable; }));

  mGuiManager->getSideBar()->registerCallback<bool>("set_enable_color_mixing",
      ([this](bool enable) { mProperties->mEnableColorMixing = enable; }));

  mGuiManager->getSideBar()->registerCallback<double>("set_terrain_lod", ([this](double value) {
    if (!mProperties->mAutoLOD.get()) {
      mProperties->mLODFactor = value;
    }
  }));

  mGuiManager->getSideBar()->registerCallback<bool>(
      "set_enable_auto_terrain_lod", ([this](bool enable) { mProperties->mAutoLOD = enable; }));

  mGuiManager->getSideBar()->registerCallback<double>(
      "set_texture_gamma", ([this](double value) { mProperties->mTextureGamma = value; }));

  mGuiManager->getSideBar()->registerCallback<double, double>(
      "set_height_range", ([this](double val, double handle) {
        if (handle == 0.0)
          mProperties->mHeightMin = val * 1000;
        else
          mProperties->mHeightMax = val * 1000;
      }));

  mGuiManager->getSideBar()->registerCallback<double, double>(
      "set_slope_range", ([this](double val, double handle) {
        if (handle == 0.0)
          mProperties->mSlopeMin = cs::utils::convert::toRadians(val);
        else
          mProperties->mSlopeMax = cs::utils::convert::toRadians(val);
      }));

  mGuiManager->getSideBar()->registerCallback("set_surface_coloring_mode_0",
      ([this]() { mProperties->mColorMappingType = Properties::ColorMappingType::eNone; }));

  mGuiManager->getSideBar()->registerCallback("set_surface_coloring_mode_1",
      ([this]() { mProperties->mColorMappingType = Properties::ColorMappingType::eHeight; }));

  mGuiManager->getSideBar()->registerCallback("set_surface_coloring_mode_2",
      ([this]() { mProperties->mColorMappingType = Properties::ColorMappingType::eSlope; }));

  mGuiManager->getSideBar()->registerCallback("set_terrain_projection_mode_0", ([this]() {
    mProperties->mTerrainProjectionType = Properties::TerrainProjectionType::eHEALPix;
  }));

  mGuiManager->getSideBar()->registerCallback("set_terrain_projection_mode_1", ([this]() {
    mProperties->mTerrainProjectionType = Properties::TerrainProjectionType::eLinear;
  }));

  mGuiManager->getSideBar()->registerCallback("set_terrain_projection_mode_2", ([this]() {
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

    double tStartExistence = cs::utils::convert::toSpiceTime(anchor->second.mStartExistence);
    double tEndExistence   = cs::utils::convert::toSpiceTime(anchor->second.mEndExistence);

    std::vector<std::shared_ptr<TileSource>> DEMs;
    std::vector<std::shared_ptr<TileSource>> IMGs;
    for (auto const& dataset : bodySettings.second.mDemDatasets) {
      if (dataset.mIsWebMapService) {
        auto dem = std::make_shared<TileSourceWebMapService>(
            dataset.mFiles.front(), mPluginSettings.mMapCache);
        dem->setName(dataset.mName);
        dem->setCopyright(dataset.mCopyright);
        DEMs.push_back(dem);
      } else {
        auto dem = std::make_shared<TileSourceDB>(TileDataType::eFloat32, dataset.mFiles.front());
        dem->setName(dataset.mName);
        dem->setCopyright(dataset.mCopyright);
        DEMs.push_back(dem);
      }
    }

    for (auto const& dataset : bodySettings.second.mImgDatasets) {
      if (dataset.mIsWebMapService) {
        auto img = std::make_shared<TileSourceWebMapService>(
            dataset.mFiles.front(), mPluginSettings.mMapCache);
        img->setName(dataset.mName);
        img->setCopyright(dataset.mCopyright);
        IMGs.push_back(img);
      } else {
        if (dataset.mFiles.size() == 1) {
          auto img = std::make_shared<TileSourceDB>(TileDataType::eUInt8, dataset.mFiles.front());
          img->setName(dataset.mName);
          img->setCopyright(dataset.mCopyright);
          IMGs.push_back(img);
        } else {
          auto img = std::make_shared<TileSourceDB>(TileDataType::eU8Vec3,
              std::array<std::string, 3>{dataset.mFiles[0], dataset.mFiles[1], dataset.mFiles[2]});
          img->setName(dataset.mName);
          img->setCopyright(dataset.mCopyright);
          IMGs.push_back(img);
        }
      }
    }

    auto body = std::make_shared<LodBody>(mGraphicsEngine, mSolarSystem, mProperties, mGuiManager,
        anchor->second.mCenter, anchor->second.mFrame, mGLResources, DEMs, IMGs, tStartExistence,
        tEndExistence);

    mSolarSystem->registerBody(body);

    body->setSun(mSolarSystem->getSun());
    auto parent = mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get());
    VistaOpenSGMaterialTools::SetSortKeyOnSubtree(
        parent, static_cast<int>(cs::utils::DrawOrder::ePlanets));

    mInputManager->registerSelectable(body);
    mLodBodies.push_back(body);
  }

  mSolarSystem->pActiveBody.onChange().connect(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        auto lodBody = std::dynamic_pointer_cast<LodBody>(body);

        if (!lodBody) {
          return;
        }

        mGuiManager->getSideBar()->callJavascript("clear_container", "set_tiles_img");
        mGuiManager->getSideBar()->callJavascript("clear_container", "set_tiles_dem");
        mGuiManager->getSideBar()->callJavascript(
            "add_dropdown_value", "set_tiles_img", "None", "None", false);
        for (auto const& source : lodBody->getIMGtileSources()) {
          bool active = source->getName() == lodBody->pActiveTileSourceIMG.get();
          mGuiManager->getSideBar()->callJavascript(
              "add_dropdown_value", "set_tiles_img", source->getName(), source->getName(), active);
          if (active) {
            mGuiManager->getSideBar()->callJavascript(
                "set_map_data_copyright", source->getCopyright());
          }
        }
        for (auto const& source : lodBody->getDEMtileSources()) {
          bool active = source->getName() == lodBody->pActiveTileSourceDEM.get();
          mGuiManager->getSideBar()->callJavascript(
              "add_dropdown_value", "set_tiles_dem", source->getName(), source->getName(), active);
          if (active) {
            mGuiManager->getSideBar()->callJavascript(
                "set_elevation_data_copyright", source->getCopyright());
          }
        }
      });

  mGuiManager->getSideBar()->registerCallback<std::string>(
      "set_tiles_img", ([this](std::string const& name) {
        auto body = std::dynamic_pointer_cast<LodBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          body->pActiveTileSourceIMG = name;
        }
      }));

  mGuiManager->getSideBar()->registerCallback<std::string>(
      "set_tiles_dem", ([this](std::string const& name) {
        auto body = std::dynamic_pointer_cast<LodBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          body->pActiveTileSourceDEM = name;
        }
      }));

  mNonAutoLod = mProperties->mLODFactor.get();

  mProperties->mAutoLOD.onChange().connect([this](bool enabled) {
    if (enabled) {
      mNonAutoLod = mProperties->mLODFactor.get();
    } else {
      mProperties->mLODFactor = mNonAutoLod;
      mGuiManager->getSideBar()->callJavascript("set_slider_value", "set_terrain_lod", mNonAutoLod);
    }
  });

  mProperties->mLODFactor.onChange().connect([this](float value) {
    if (mProperties->mAutoLOD()) {
      mGuiManager->getSideBar()->callJavascript("set_slider_value", "set_terrain_lod", value);
    }
  });

  mSolarSystem->pActiveBody = mLodBodies[0];
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  for (auto const& body : mLodBodies) {
    mInputManager->unregisterSelectable(body);
  }

  mGuiManager->getSideBar()->unregisterCallback("set_enable_tiles_freeze");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_tiles_debug");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_wireframe");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_heightlines");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_lat_long_grid");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_lat_long_grid_labels");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_color_mixing");
  mGuiManager->getSideBar()->unregisterCallback("set_terrain_lod");
  mGuiManager->getSideBar()->unregisterCallback("set_enable_auto_terrain_lod");
  mGuiManager->getSideBar()->unregisterCallback("set_texture_gamma");
  mGuiManager->getSideBar()->unregisterCallback("set_height_range");
  mGuiManager->getSideBar()->unregisterCallback("set_slope_range");
  mGuiManager->getSideBar()->unregisterCallback("set_surface_coloring_mode_0");
  mGuiManager->getSideBar()->unregisterCallback("set_surface_coloring_mode_1");
  mGuiManager->getSideBar()->unregisterCallback("set_surface_coloring_mode_2");
  mGuiManager->getSideBar()->unregisterCallback("set_terrain_projection_mode_0");
  mGuiManager->getSideBar()->unregisterCallback("set_terrain_projection_mode_1");
  mGuiManager->getSideBar()->unregisterCallback("set_terrain_projection_mode_2");
  mGuiManager->getSideBar()->unregisterCallback("set_tiles_img");
  mGuiManager->getSideBar()->unregisterCallback("set_tiles_dem");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::update() {
  if (mProperties->mAutoLOD.get()) {

    double minLODFactor = 10.0;
    double maxLODFactor = 50.0;
    double minTime      = 13.5;
    double maxTime      = 14.5;

    if (mFrameTimings->pFrameTime.get() > maxTime) {
      mProperties->mLODFactor = std::max(
          minLODFactor, mProperties->mLODFactor.get() -
                            std::min(1.0, 0.1 * (mFrameTimings->pFrameTime.get() - maxTime)));
    } else if (mFrameTimings->pFrameTime.get() < minTime) {
      mProperties->mLODFactor = std::min(
          maxLODFactor, mProperties->mLODFactor.get() +
                            std::min(1.0, 0.02 * (minTime - mFrameTimings->pFrameTime.get())));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
