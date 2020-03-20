////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PlanetShader.hpp"

#include "../../../src/cs-core/GraphicsEngine.hpp"
#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-gui/GuiItem.hpp"
#include "../../../src/cs-utils/convert.hpp"
#include "../../../src/cs-utils/filesystem.hpp"
#include "../../../src/cs-utils/utils.hpp"

#include <VistaOGLExt/VistaGLSLShader.h>
#include <VistaOGLExt/VistaOGLUtils.h>
#include <VistaOGLExt/VistaShaderRegistry.h>

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
GLenum const TEXUNITNAMEFONT = GL_TEXTURE10;
GLenum const TEXUNITNAMELUT  = GL_TEXTURE11;
GLint const  TEXUNITFONT     = 10;
GLint const  TEXUNITLUT      = 11;
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<std::string, cs::graphics::ColorMap> PlanetShader::mColorMaps;

////////////////////////////////////////////////////////////////////////////////////////////////////

PlanetShader::PlanetShader(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
    std::shared_ptr<Plugin::Properties> const&                              pProperties,
    std::shared_ptr<cs::core::GuiManager> const&                            pGuiManager)
    : csp::lodbodies::TerrainShader()
    , mGraphicsEngine(graphicsEngine)
    , mGuiManager(pGuiManager)
    , mProperties(pProperties)
    , mFontTexture(VistaOGLUtils::LoadTextureFromTga("../share/resources/textures/font.tga")) {
  // clang-format off
    pTextureIsRGB.connect(
        [this](bool) { mShaderDirty = true; });
    pEnableTexture.connect(
        [this](bool) { mShaderDirty = true; });

    mProperties->mEnableHeightlines.connect(
        [this](bool) { mShaderDirty = true; });
    mProperties->mColorMappingType.connect(
        [this](Plugin::Properties::ColorMappingType) { mShaderDirty = true; });
    mProperties->mTerrainProjectionType.connect(
        [this](Plugin::Properties::TerrainProjectionType) { mShaderDirty = true; });
    mEnableLightingConnection = mGraphicsEngine->pEnableLighting.connect(
        [this](bool) { mShaderDirty = true; });
    mEnableShadowsDebugConnection = mGraphicsEngine->pEnableShadowsDebug.connect(
        [this](bool) { mShaderDirty = true; });
    mEnableShadowsConnection = mGraphicsEngine->pEnableShadows.connect(
        [this](bool) { mShaderDirty = true; });
    mEnableHDRConnection = mGraphicsEngine->pEnableHDR.connect(
        [this](bool) { mShaderDirty = true; });
    mLightingQualityConnection = mGraphicsEngine->pLightingQuality.connect(
        [this](int) { mShaderDirty = true; });
    mProperties->mEnableTilesDebug.connect(
        [this](bool) { mShaderDirty = true; });
    mProperties->mEnableLatLongGridLabels.connect(
        [this](bool) { mShaderDirty = true; });
    mProperties->mEnableLatLongGrid.connect(
        [this](bool) { mShaderDirty = true; });
    mProperties->mEnableColorMixing.connect(
        [this](bool) { mShaderDirty = true; });
  // clang-format on

  // TODO: color map mangement could be done in a separate class
  if (mColorMaps.empty()) {
    auto files(cs::utils::filesystem::listFiles("../share/resources/colormaps"));

    bool first = true;
    for (auto file : files) {
      std::string  name(file);
      const size_t lastSlashIdx = name.find_last_of("\\/");
      if (std::string::npos != lastSlashIdx) {
        name.erase(0, lastSlashIdx + 1);
      }

      mColorMaps.insert(std::make_pair(name, cs::graphics::ColorMap(file)));
      pGuiManager->getGui()->callJavascript(
          "CosmoScout.gui.addDropdownValue", "lodBodies.setColormap", name, name, first);
      if (first) {
        first                         = false;
        mProperties->mTerrainColorMap = name;
      }
    }

    pGuiManager->getGui()->registerCallback("lodBodies.setColormap",
        "Make the planet shader use the colormap with the given name.",
        std::function([this](std::string&& name) { mProperties->mTerrainColorMap = name; }));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

PlanetShader::~PlanetShader() {
  delete mFontTexture;
  mGraphicsEngine->pEnableLighting.disconnect(mEnableLightingConnection);
  mGraphicsEngine->pEnableShadowsDebug.disconnect(mEnableShadowsDebugConnection);
  mGraphicsEngine->pEnableShadows.disconnect(mEnableShadowsConnection);
  mGraphicsEngine->pLightingQuality.disconnect(mLightingQualityConnection);
  mGraphicsEngine->pEnableHDR.disconnect(mEnableHDRConnection);

  mGuiManager->getGui()->unregisterCallback("lodBodies.setColormap");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void PlanetShader::setSun(glm::vec3 const& direction, float illuminance) {
  mSunDirection   = direction;
  mSunIlluminance = illuminance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void PlanetShader::compile() {
  VistaShaderRegistry& reg = VistaShaderRegistry::GetInstance();
  mVertexSource            = reg.RetrieveShader("Planet.vert");
  mFragmentSource          = reg.RetrieveShader("Planet.frag");

  cs::utils::replaceString(
      mFragmentSource, "$TEXTURE_IS_RGB", cs::utils::toString(pTextureIsRGB.get()));
  cs::utils::replaceString(mFragmentSource, "$SHOW_HEIGHT_LINES",
      cs::utils::toString(mProperties->mEnableHeightlines.get()));
  cs::utils::replaceString(
      mFragmentSource, "$SHOW_TEXTURE", cs::utils::toString(pEnableTexture.get()));
  cs::utils::replaceString(mFragmentSource, "$COLOR_MAPPING_TYPE",
      cs::utils::toString(static_cast<int>(mProperties->mColorMappingType.get())));
  cs::utils::replaceString(mFragmentSource, "$ENABLE_LIGHTING",
      cs::utils::toString(mGraphicsEngine->pEnableLighting.get()));
  cs::utils::replaceString(
      mFragmentSource, "$ENABLE_HDR", cs::utils::toString(mGraphicsEngine->pEnableHDR.get()));
  cs::utils::replaceString(mFragmentSource, "$ENABLE_SHADOWS_DEBUG",
      cs::utils::toString(mGraphicsEngine->pEnableShadowsDebug.get()));
  cs::utils::replaceString(mFragmentSource, "$ENABLE_SHADOWS",
      cs::utils::toString(mGraphicsEngine->pEnableShadows.get()));
  cs::utils::replaceString(mFragmentSource, "$LIGHTING_QUALITY",
      cs::utils::toString(mGraphicsEngine->pLightingQuality.get()));
  cs::utils::replaceString(mFragmentSource, "$SHOW_TILE_BORDER",
      cs::utils::toString(mProperties->mEnableTilesDebug.get()));
  cs::utils::replaceString(mFragmentSource, "$SHOW_LAT_LONG_LABELS",
      cs::utils::toString(mProperties->mEnableLatLongGridLabels.get()));
  cs::utils::replaceString(mFragmentSource, "$SHOW_LAT_LONG",
      cs::utils::toString(mProperties->mEnableLatLongGrid.get()));
  cs::utils::replaceString(
      mFragmentSource, "$MIX_COLORS", cs::utils::toString(mProperties->mEnableColorMixing.get()));

  cs::utils::replaceString(mVertexSource, "$LIGHTING_QUALITY",
      cs::utils::toString(mGraphicsEngine->pLightingQuality.get()));
  cs::utils::replaceString(mVertexSource, "$TERRAIN_PROJECTION_TYPE",
      cs::utils::toString(static_cast<int>(mProperties->mTerrainProjectionType.get())));

  TerrainShader::compile();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void PlanetShader::bind() {
  TerrainShader::bind();

  GLint loc = -1;
  loc       = mShader->GetUniformLocation("heightTex");
  mShader->SetUniform(loc, TEXUNITLUT);

  loc = mShader->GetUniformLocation("fontTex");
  mShader->SetUniform(loc, TEXUNITFONT);

  loc = mShader->GetUniformLocation("heightMin");
  mShader->SetUniform(loc, mProperties->mHeightMin.get());

  loc = mShader->GetUniformLocation("heightMax");
  mShader->SetUniform(loc, mProperties->mHeightMax.get());

  loc = mShader->GetUniformLocation("slopeMin");
  mShader->SetUniform(loc, mProperties->mSlopeMin.get());

  loc = mShader->GetUniformLocation("slopeMax");
  mShader->SetUniform(loc, mProperties->mSlopeMax.get());

  loc = mShader->GetUniformLocation("ambientBrightness");
  mShader->SetUniform(loc, mGraphicsEngine->pAmbientBrightness.get());

  loc = mShader->GetUniformLocation("texGamma");
  mShader->SetUniform(loc, mProperties->mTextureGamma.get());

  loc = mShader->GetUniformLocation("uSunDirIlluminance");
  mShader->SetUniform(loc, mSunDirection.x, mSunDirection.y, mSunDirection.z, mSunIlluminance);

  loc = mShader->GetUniformLocation("farClip");
  mShader->SetUniform(loc, cs::utils::getCurrentFarClipDistance());

  mFontTexture->Bind(TEXUNITNAMEFONT);

  auto it(mColorMaps.find(mProperties->mTerrainColorMap.get()));
  if (it != mColorMaps.end()) {
    it->second.bind(TEXUNITNAMELUT);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void PlanetShader::release() {
  auto it(mColorMaps.find(mProperties->mTerrainColorMap.get()));
  if (it != mColorMaps.end()) {
    it->second.unbind(TEXUNITNAMELUT);
  }

  mFontTexture->Unbind(TEXUNITNAMEFONT);

  TerrainShader::release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
