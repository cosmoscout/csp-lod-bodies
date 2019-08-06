////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_PLANET_TERRAINSHADER_HPP
#define CSP_LOD_PLANET_TERRAINSHADER_HPP

#include <memory>
#include <string>

class VistaGLSLShader;

namespace csp::lodplanets {

/// The base class for the PlanetShader. It builds the shader from various sources and links it.
class TerrainShader {
 public:
  TerrainShader() {
  }
  TerrainShader(std::string const& vertexSource, std::string const& fragmentSource);
  virtual ~TerrainShader();

  virtual void bind();
  virtual void release();

  friend class TileRenderer;

 protected:
  virtual void compile();

  bool             mShaderDirty = true;
  std::string      mVertexSource;
  std::string      mFragmentSource;
  VistaGLSLShader* mShader = nullptr;
};

} // namespace csp::lodplanets

#endif // CSP_LOD_PLANET_TERRAINSHADER_HPP
