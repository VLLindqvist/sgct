/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2021                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_SKYSKAN__H__
#define __SGCT__CORRECTION_SKYSKAN__H__

#include <sgct/correction/buffer.h>
#include <string>

namespace sgct { class BaseViewport; }

namespace sgct::correction {

Buffer generateSkySkanMesh(const std::string& meshPath, BaseViewport& parent);

} // namespace sgct::correction

#endif // __SGCT__CORRECTION_SKYSKAN__H__
