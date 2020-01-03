////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_PLANET_SHADER_HPP
#define CSP_LOD_BODIES_PLANET_SHADER_HPP

#include "../../../src/cs-graphics/ColorMap.hpp"
#include "../../../src/cs-utils/Property.hpp"

#include "Plugin.hpp"
#include "TerrainShader.hpp"

#include <VistaBase/VistaVector3D.h>
#include <vector>

class VistaTexture;

namespace cs::core {
class GuiManager;
} // namespace cs::core

namespace cs::graphics {
class GraphicsEngine;
} // namespace cs::graphics

namespace csp::lodbodies {

/// The shader for rendering a planet.
class PlanetShader : public TerrainShader {
 public:
  cs::utils::Property<bool> pTextureIsRGB  = true;
  cs::utils::Property<bool> pEnableTexture = true; ///< If false the image data will not be drawn.

  PlanetShader(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
      std::shared_ptr<Plugin::Properties> const&                pProperties,
      std::shared_ptr<cs::core::GuiManager> const&              pGuiManager);
  virtual ~PlanetShader();

  void setSunDirection(VistaVector3D const& sunDirection);

  virtual void bind() override;
  virtual void release() override;

 private:
  void compile() override;

  std::shared_ptr<cs::core::GraphicsEngine> mGraphicsEngine;
  std::shared_ptr<cs::core::GuiManager>     mGuiManager;
  std::shared_ptr<Plugin::Properties>       mProperties;
  bool                                      mColorscaleTextureDirty       = true;
  VistaVector3D                             mSunDirection                 = VistaVector3D(0, 1, 0);
  VistaTexture*                             mFontTexture                  = nullptr;
  unsigned                                  mLutTexID                     = 0;
  int                                       mEnableLightingConnection     = -1;
  int                                       mEnableShadowsDebugConnection = -1;
  int                                       mEnableShadowsConnection      = -1;
  int                                       mLightingQualityConnection    = -1;

  static std::map<std::string, cs::graphics::ColorMap> mColorMaps;
};

} // namespace csp::lodbodies

#endif // CS_CORE_PLANET_SHADER_HPP
