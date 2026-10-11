#include <cstdio>
#include <cmath>
#include "../../overlay_experimental/overlay/feraxia_layout.h"
int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    for (float width : {1.f, 240.f, 640.f, 1280.f, 1920.f, 3840.f}) {
        for (float height : {1.f, 180.f, 480.f, 720.f, 2160.f}) {
            for (float font : {12.f, 16.f, 24.f, 48.f}) {
                const auto l = feraxia::layout(width, height, font);
                check(l.x >= 0 && l.y >= 0, "shell origin inside viewport");
                check(l.width > 0 && l.height > 0, "positive shell size");
                check(l.x + l.width <= width + .01f && l.y + l.height <= height + .01f, "shell fits viewport at all scales");
                check(l.rail_width >= 0 && l.rail_width < l.width, "navigation leaves room for content");
            }
        }
    }
    check(!feraxia::layout(640, 480, 24).wide, "large font on small screen stacks navigation");
    check(feraxia::layout(1920, 1080, 16).wide, "desktop has side navigation");
    check(feraxia::layout(1920, 1080, 16).width < 1920, "desktop shell leaves game visible around it");
    check(feraxia::layout(1920, 1080, 16).rail_width >= 150, "desktop navigation has readable width");
    std::printf("%s: viewport/font layout regressions\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
