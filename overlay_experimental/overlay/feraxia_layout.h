#pragma once
#include <algorithm>
#include <cmath>
namespace feraxia {
struct Layout { float x, y, width, height, rail_width; bool wide; };
// Font-relative breakpoints keep navigation readable when users scale text.
// Tiny viewports remain scrollable; never request a negative ImGui child size.
inline Layout layout(float width, float height, float font) {
    width = std::isfinite(width) ? (std::max)(1.f, width) : 1.f;
    height = std::isfinite(height) ? (std::max)(1.f, height) : 1.f;
    font = std::isfinite(font) ? (std::max)(1.f, font) : 16.f;
    const float margin = (std::min)(font * 2.f, (std::min)(width, height) * .04f);
    const float w = (std::min)(width - margin * 2.f, font * 58.f);
    const float h = (std::min)(height - margin * 2.f, font * 38.f);
    const bool wide = w >= font * 40.f;
    return {(width - w) * .5f, (height - h) * .5f, w, h,
        wide ? (std::min)(font * 13.f, w * .28f) : 0.f, wide};
}
}
