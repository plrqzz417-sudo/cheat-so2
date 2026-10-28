#include "ambient_light.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "../ui/cfg.hpp"
#include <cmath>

/*
 * Ambient Light — самопис overlay через ImGui BackgroundDrawList.
 * Рисует цветной полупрозрачный rect поверх всего экрана,
 * с пульсацией (sine-wave) по alpha — даёт эффект ambient glow.
 * Не требует оффсетов, работает без рута.
 *
 * Параметры (в cfg::ambient):
 *   enabled   — вкл/выкл
 *   color     — ImVec4 RGBA цвет подсветки
 *   intensity — 0.0 .. 1.0  базовая непрозрачность
 *   pulse     — bool  пульсировать или статично
 *   pulse_spd — скорость пульсации (0.5 .. 5.0)
 */

extern float g_sw;   // screen width  — объявлены в draw.h / main
extern float g_sh;   // screen height

namespace ambient_light {

    void run() {
        if (!cfg::ambient::enabled) return;

        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        if (!bg) return;

        float base_alpha = cfg::ambient::intensity;

        float final_alpha = base_alpha;

        if (cfg::ambient::pulse) {
            // ImGui::GetTime() — double, cast safe для NDK r17 clang6
            float t = static_cast<float>(ImGui::GetTime());
            float spd = cfg::ambient::pulse_spd;
            // sine 0..1 -> mapped to 0.4..1.0 диапазон чтобы не гасло в ноль
            float wave = (sinf(t * spd) + 1.f) * 0.5f;
            float lo = 0.4f;
            float hi = 1.0f;
            float mod = lo + wave * (hi - lo);
            final_alpha = base_alpha * mod;
        }

        // Клампим: не даём превысить 0.85 чтобы экран не перекрывался полностью
        if (final_alpha > 0.85f) final_alpha = 0.85f;
        if (final_alpha < 0.f)   final_alpha = 0.f;

        const ImVec4& c = cfg::ambient::color;
        ImU32 col = IM_COL32(
            static_cast<int>(c.x * 255.f),
            static_cast<int>(c.y * 255.f),
            static_cast<int>(c.z * 255.f),
            static_cast<int>(final_alpha * 255.f)
        );

        bg->AddRectFilled(ImVec2(0.f, 0.f), ImVec2(g_sw, g_sh), col);
    }

} // namespace ambient_light
