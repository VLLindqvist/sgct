/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_PFM__H__
#define __SGCT__CORRECTION_PFM__H__

#include <sgct/correction/buffer.h>
#include <glm/fwd.hpp>
#include <string>


namespace sgct::correction {

Buffer generatePerEyeMeshFromPFMImage(const std::string& path, const glm::ivec2& pos,
    const glm::ivec2& size);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_PFM__H__
