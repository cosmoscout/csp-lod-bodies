////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_PLUGIN_HPP
#define CSP_LOD_BODIES_PLUGIN_HPP

#include "../../../src/cs-core/PluginBase.hpp"
#include "../../../src/cs-utils/Property.hpp"

#include <glm/gtc/constants.hpp>
#include <vector>

namespace csp::lodbodies {

class GLResources;
class LodBody;

/// This plugin provides planets with level of detail data. It uses separate image and elevation
/// data from either files or web map services to display the information onto the surface.
/// Multiple sources can be given at startup and they can be cycled through at runtime via the GUI.
/// The configuration is  done via the applications config file. See README.md for details.
class Plugin : public cs::core::PluginBase {
 public:
  struct Properties {
    enum class ColorMappingType { eNone = 0, eHeight = 1, eSlope = 2 };
    enum class TerrainProjectionType { eHEALPix = 0, eLinear = 1, eHybrid = 2 };

    cs::utils::Property<TerrainProjectionType> mTerrainProjectionType =
        TerrainProjectionType::eHybrid;
    cs::utils::Property<float>            mLODFactor               = 15.f;
    cs::utils::Property<bool>             mAutoLOD                 = true;
    cs::utils::Property<float>            mTextureGamma            = 1.f;
    cs::utils::Property<bool>             mEnableHeightlines       = false;
    cs::utils::Property<bool>             mEnableLatLongGrid       = false;
    cs::utils::Property<bool>             mEnableLatLongGridLabels = false;
    cs::utils::Property<ColorMappingType> mColorMappingType        = ColorMappingType::eNone;
    cs::utils::Property<std::string>      mTerrainColorMap;
    cs::utils::Property<bool>             mEnableColorMixing = true;
    cs::utils::Property<float>            mHeightMax         = 12000.f;
    cs::utils::Property<float>            mHeightMin         = -8000.f;
    cs::utils::Property<float>            mSlopeMax          = 0.25f * glm::pi<float>();
    cs::utils::Property<float>            mSlopeMin          = 0.f;
    cs::utils::Property<bool>             mEnableWireframe   = false;
    cs::utils::Property<bool>             mEnableTilesDebug  = false;
    cs::utils::Property<bool>             mEnableTilesFreeze = false;
  };

  /// The startup settings of the plugin.
  struct Settings {

    /// A single data set containing either elevation or image data.
    struct Dataset {
      std::string mName;               ///< The name of the data set.
      std::string mCopyright;          ///< The copyright holder of the data set.
      bool        mIsWebMapService;    ///< If true the data can be fetched from a web
                                       ///< server, otherwise from the file system.
      std::vector<std::string> mFiles; ///< The files/WMS with the data.
    };

    /// The startup settings for a planet.
    struct Body {
      std::vector<Dataset> mDemDatasets; ///< The data sets containing elevation data.
      std::vector<Dataset> mImgDatasets; ///< The data sets containing image data.
    };

    uint32_t                    mMaxGPUTilesColor; ///< The maximum allowed colored tiles.
    uint32_t                    mMaxGPUTilesGray;  ///< The maximum allowed gray tiles.
    uint32_t                    mMaxGPUTilesDEM;   ///< The maximum allowed elevation tiles.
    std::string                 mMapCache;         ///< Path to the map cache folder.
    std::map<std::string, Body> mBodies;           ///< A list of planets with their anchor names.
  };

  Plugin();

  void init() override;
  void deInit() override;

  void update() override;

 private:
  Settings                              mPluginSettings;
  std::shared_ptr<GLResources>          mGLResources;
  std::vector<std::shared_ptr<LodBody>> mLodBodies;
  std::shared_ptr<Properties>           mProperties;
  float                                 mNonAutoLod;
};

} // namespace csp::lodbodies

#endif // CSP_LOD_BODIES_PLUGIN_HPP
