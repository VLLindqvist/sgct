/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__FREETYPE__H__
#define __SGCT__FREETYPE__H__

#include <glm/fwd.hpp>
#include <string>

namespace sgct { class Window; }

namespace sgct::text {

class Font;

enum class TextAlignMode { TopLeft, TopCenter, TopRight};

void print(sgct::Window& window, Font& font, TextAlignMode mode, float x, float y, const char* format, ...);
void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const char* format, ...);

// with color
void print(Window& window, Font& font, TextAlignMode mode, float x, float y, const glm::vec4& color,
    const char* format, ...);
void print(Window& window, Font& font, TextAlignMode mode, float x, float y, const glm::vec4& color,
    const glm::vec4& strokeColor, const char* format, ...);
void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
    const char* format, ...);
void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
    const glm::vec4& strokeColor, const char* format, ...);

} // namespace sgct::text

#endif // __SGCT__FREETYPE__H__
