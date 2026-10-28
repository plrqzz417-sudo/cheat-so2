// ─────────────────────────────────────────────────────────────────────────────
//  TRIGGERBOT — адаптирован под normal5 API (player::get_bones вместо bone_position)
// ─────────────────────────────────────────────────────────────────────────────

#include "triggerbot.hpp"
#include "../game/game.hpp"
#include "../game/player.hpp"
#include "../game/math.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include "imgui.h"
#include <cmath>

namespace {
    static constexpr uint64_t kOffPmLocalPlayer = 0x70;
    static constexpr uint64_t kOffWeaponRoot    = 0x88;
    static constexpr uint64_t kOffActiveWeapon  = 0xA0;
    static constexpr uint64_t kOffFireButton    = 0x148;
    static constexpr uint64_t kOffPlayerTeam    = 0x79;

    static constexpr float kShotDuration   = 0.08f;
    static constexpr float kMinRadiusPx    = 12.f;
    static constexpr float kMaxRadiusPx    = 80.f;
    static constexpr float kRefDistM       = 100.f;

    static inline bool valid_addr(uint64_t a) {
        return (a >= 0x10000ULL && a <= 0x7FFFFFFFFFFFULL);
    }

    static uint64_t get_weapon_ctrl(uint64_t lp) {
        if (!valid_addr(lp)) return 0;
        uint64_t wrc = rpm<uint64_t>(lp + oxorany(kOffWeaponRoot));
        if (!valid_addr(wrc)) return 0;
        uint64_t wc  = rpm<uint64_t>(wrc + oxorany(kOffActiveWeapon));
        return valid_addr(wc) ? wc : 0;
    }

    static float timer    = 0.f;
    static float shot_dur = 0.f;
    static bool  pulsing  = false;

    // Проверяем кости через get_bones (normal5 API)
    static bool try_bones(uint64_t player, const matrix& vm, const ImVec2& center, float radius_px) {
        player::bones_t bones;
        if (!player::get_bones(player, bones)) return false;

        // Голова и торс (spine2)
        Vector3 targets[2] = { bones.head, bones.spine2 };
        for (int i = 0; i < 2; i++) {
            ImVec2 sc;
            if (!world_to_screen(targets[i], vm, sc)) continue;
            float dx = sc.x - center.x;
            float dy = sc.y - center.y;
            if ((dx * dx + dy * dy) < (radius_px * radius_px))
                return true;
        }
        return false;
    }
}

void triggerbot::run(uint64_t lp) {
    if (!valid_addr(lp)) {
        pulsing = false; timer = 0.f; shot_dur = 0.f;
        return;
    }

    if (!cfg::aim::triggerbot) {
        if (pulsing || timer > 0.f) {
            uint64_t wc = get_weapon_ctrl(lp);
            if (valid_addr(wc)) wpm<uint8_t>(wc + oxorany(kOffFireButton), 2);
            pulsing = false; timer = 0.f; shot_dur = 0.f;
        }
        return;
    }

    uint64_t wc = get_weapon_ctrl(lp);
    if (!valid_addr(wc)) {
        pulsing = false; timer = 0.f; shot_dur = 0.f;
        return;
    }

    uint64_t player_manager = get_player_manager();
    if (!valid_addr(player_manager)) return;

    int     LocalTeam  = rpm<uint8_t>(lp + oxorany(kOffPlayerTeam));
    Vector3 local_pos  = player::position(lp);
    matrix  vm         = player::view_matrix(lp);

    uint64_t player_list = rpm<uint64_t>(player_manager + oxorany(0x28));
    if (!valid_addr(player_list)) return;
    int count = rpm<int>(player_list + oxorany(0x20));
    if (count <= 0 || count > 128) return;
    uint64_t list_buffer = rpm<uint64_t>(player_list + oxorany(0x18));
    if (!valid_addr(list_buffer)) return;

    const ImVec2 center(g_sw * 0.5f, g_sh * 0.5f);
    bool can_trigger = false;

    for (int i = 0; i < count && !can_trigger; i++) {
        uint64_t player = rpm<uint64_t>(list_buffer + oxorany(0x30) + oxorany(0x18) * i);
        if (!valid_addr(player) || player == lp) continue;
        if (rpm<uint8_t>(player + oxorany(kOffPlayerTeam)) == (uint8_t)LocalTeam) continue;
        if (player::health(player) <= 0) continue;

        // Visible-only check через is_visible (normal5 API)
        if (cfg::aim::trigger_visible_only && !player::is_visible(player)) continue;

        Vector3 proot = player::position(player);
        float dx = proot.x - local_pos.x;
        float dy = proot.y - local_pos.y;
        float dz = proot.z - local_pos.z;
        float dist_m = sqrtf(dx*dx + dy*dy + dz*dz);
        float ref    = dist_m < 1.f ? 1.f : dist_m;

        float radius_px = cfg::aim::trigger_range * (kRefDistM / ref);
        if (radius_px < kMinRadiusPx) radius_px = kMinRadiusPx;
        if (radius_px > kMaxRadiusPx) radius_px = kMaxRadiusPx;

        if (try_bones(player, vm, center, radius_px))
            can_trigger = true;
    }

    float dt = ImGui::GetIO().DeltaTime;
    if (dt <= 0.f) dt = 0.016f;

    if (can_trigger) {
        if (!pulsing) {
            timer += dt;
            if (timer >= cfg::aim::trigger_delay) {
                wpm<uint8_t>(wc + oxorany(kOffFireButton), 3);
                pulsing  = true;
                shot_dur = 0.f;
            }
        } else {
            shot_dur += dt;
            if (shot_dur >= kShotDuration) {
                wpm<uint8_t>(wc + oxorany(kOffFireButton), 2);
                pulsing = false;
                timer   = 0.f;
            }
        }
    } else {
        if (pulsing || timer > 0.f) {
            wpm<uint8_t>(wc + oxorany(kOffFireButton), 2);
            pulsing = false;
            timer   = 0.f;
        }
    }
}
