/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2021                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__READCONFIG__H__
#define __SGCT__READCONFIG__H__

#include <sgct/config.h>
#include <string>

namespace sgct {

[[nodiscard]] sgct::config::Cluster readConfig(const std::string& filename);

} // namespace sgct

#endif // __SGCT__READCONFIG__H__
