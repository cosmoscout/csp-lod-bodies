////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_LOD_BODIES_RENDERDATAIMG_HPP
#define CSP_LOD_BODIES_RENDERDATAIMG_HPP

#include "RenderData.hpp"

namespace csp::lodbodies {

/// Render data for image data.
class RenderDataImg : public RenderData {
 public:
  // static RenderDataImg* create(TileNode* node = NULL);

  explicit RenderDataImg(TileNode* node = NULL);
  virtual ~RenderDataImg();
};

} // namespace csp::lodbodies

#endif // CSP_LOD_BODIES_RENDERDATAIMG_HPP
