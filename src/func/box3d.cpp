// ─────────────────────────────────────────────────────────────────────────────
//  BOX3D — проецирование 8 углов AABB через world_to_screen и рисование
//  рёбер. Координаты локальные относительно точки опоры игрока.
// ─────────────────────────────────────────────────────────────────────────────

#include "box3d.hpp"
#include "../game/game.hpp"
#include "../game/math.hpp"
#include "../ui/cfg.hpp"
#include "../ui/theme/theme.hpp"
#include "../protect/oxorany.hpp"
#include "../other/vector3.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace {
    // Габариты стандартного игрока Standoff 2 (юниты).
    static constexpr float kHalfWidth  = 0.40f;
    static constexpr float kHalfDepth  = 0.40f;
    static constexpr float kFootOffset = 0.00f;   // base_y — ступни
    static constexpr float kHeight     = 1.80f;

    struct Edge { int a, b; };

    static const Edge kEdges[12] = {
        {0,1}, {1,2}, {2,3}, {3,0},   // low ring (CCW)
        {4,5}, {5,6}, {6,7}, {7,4},   // top ring
        {0,4}, {1,5}, {2,6}, {3,7}    // verticals
    };
}

void box3d::draw(ImDrawList* dl, const matrix& vm, float base_x, float base_y, float base_z) {
    if (!cfg::esp::box3d) return;

    // 8 углов: (X ± hw,  Y0..Y0+h,  Z ± hd)
    Vector3 corners[8];
    corners[0] = Vector3(base_x - kHalfWidth, base_y + kFootOffset,    base_z - kHalfDepth);
    corners[1] = Vector3(base_x + kHalfWidth, base_y + kFootOffset,    base_z - kHalfDepth);
    corners[2] = Vector3(base_x + kHalfWidth, base_y + kFootOffset,    base_z + kHalfDepth);
    corners[3] = Vector3(base_x - kHalfWidth, base_y + kFootOffset,    base_z + kHalfDepth);
    corners[4] = Vector3(base_x - kHalfWidth, base_y + kFootOffset + kHeight, base_z - kHalfDepth);
    corners[5] = Vector3(base_x + kHalfWidth, base_y + kFootOffset + kHeight, base_z - kHalfDepth);
    corners[6] = Vector3(base_x + kHalfWidth, base_y + kFootOffset + kHeight, base_z + kHalfDepth);
    corners[7] = Vector3(base_x - kHalfWidth, base_y + kFootOffset + kHeight, base_z + kHalfDepth);

    ImVec2 sc[8];
    int visible = 0;
    int last_visible = -1;
    for (int i = 0; i < 8; i++) {
        ImVec2 p(-10000.f, -10000.f);
        if (world_to_screen(corners[i], vm, p)) {
            sc[i] = p;
            visible++;
            last_visible = i;
        } else {
            sc[i] = ImVec2(-10000.f, -10000.f);
        }
    }

    // Если все углы за камерой — пропускаем
    if (visible < 2) return;

    ImU32 col = IM_COL32(
        (int)(cfg::esp::box3d_col.x * 255),
        (int)(cfg::esp::box3d_col.y * 255),
        (int)(cfg::esp::box3d_col.z * 255),
        (int)(cfg::esp::box3d_col.w * 255));

    ImU32 shadow = IM_COL32(0, 0, 0, (int)(cfg::esp::box3d_col.w * 140));

    float thick = std::clamp(cfg::esp::box3d_thickness, 0.5f, 4.f);

    auto on_screen = [&](ImVec2 p) {
        return p.x > -5000.f && p.x < g_sw + 5000.f &&
               p.y > -5000.f && p.y < g_sh + 5000.f;
    };

    for (int e = 0; e < 12; e++) {
        const ImVec2& a = sc[kEdges[e].a];
        const ImVec2& b = sc[kEdges[e].b];
        if (!on_screen(a) && !on_screen(b)) continue;

        // Чёрная «тень» под основной линией
        dl->AddLine(ImVec2(a.x + 2.f, a.y + 2.f), ImVec2(b.x + 2.f, b.y + 2.f), shadow, thick);
        // Основная линия
        dl->AddLine(a, b, col, thick);
    }

    (void)last_visible;
}
