#pragma once

struct ImDrawList;

namespace aim {
    void run();
    void draw_fov(ImDrawList* draw, float screen_w, float screen_h);
}
