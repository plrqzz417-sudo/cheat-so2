#include "aim.hpp"
#include "../game/game.hpp"
#include "../game/math.hpp"
#include "../game/player.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <string>

namespace {
    static constexpr float    kPi               = 3.14159265f;
    static constexpr float    kVerticalFovDeg   = 70.f;
    static constexpr uint64_t kOffPlayerManagerLocalPlayer = 0x70;
    static constexpr uint64_t kOffPlayerAimController      = 0x80;
    static constexpr uint64_t kOffAimControllerAimingData  = 0x90;
    static constexpr uint64_t kOffAimingDataPitch          = 0x18;
    static constexpr uint64_t kOffAimingDataYaw            = 0x1C;
    static constexpr uint64_t kOffAimingDataPitchYawVec3   = 0x18;
    static constexpr uint64_t kOffAimingDataSecondVec3     = 0x24;

    static float fov_radius_px(float fov_deg, float screen_h) {
        if (screen_h <= 0.f) return 0.f;
        float clamped_fov   = std::clamp(fov_deg, 1.f, 179.f);
        float half_vertical = (kVerticalFovDeg * 0.5f) * (kPi / 180.f);
        float half_oim      = (clamped_fov     * 0.5f) * (kPi / 180.f);
        float tan_vertical  = std::tan(half_vertical);
        if (std::fabs(tan_vertical) < 0.00001f) return 0.f;
        return ((screen_h * 0.5f) / tan_vertical) * std::tan(half_oim);
    }

    // ── Целевая точка через get_bones ──────────────────────────────────────
    // cfg::aim::target: 0=голова, 1=шея, 2=грудь (spine2)
    static Vector3 target_point(uint64_t player_ptr, const Vector3& fallback_pos) {
        player::bones_t bones;
        if (!player::get_bones(player_ptr, bones)) {
            // кости не читаются — смещение по Y из позиции ног
            switch (cfg::aim::target) {
                case 0:  return Vector3(fallback_pos.x, fallback_pos.y + 1.67f, fallback_pos.z);
                case 1:  return Vector3(fallback_pos.x, fallback_pos.y + 1.55f, fallback_pos.z);
                default: return Vector3(fallback_pos.x, fallback_pos.y + 1.35f, fallback_pos.z);
            }
        }
        switch (cfg::aim::target) {
            case 0:  return bones.head;
            case 1:  return bones.neck;
            case 2:  return bones.spine2;
            default: return bones.head;
        }
    }

    static float normalize_yaw(float yaw) {
        while (yaw >  180.f) yaw -= 360.f;
        while (yaw < -180.f) yaw += 360.f;
        return yaw;
    }

    // ── Кэш visible_check: результат на 4 тика ────────────────────────────
    struct VisCache {
        uint64_t player_ptr = 0;
        bool     result     = true;
        uint8_t  ttl        = 0;
    };
    static VisCache s_vis_cache[64];
    static int      s_vis_count = 0;

    static bool find_int_property(uint64_t player_ptr, const char* tag, int& out_value) {
        uint64_t photon_player = player::photon_ptr(player_ptr);
        if (!photon_player) return false;

        uint64_t properties_registry = rpm<uint64_t>(photon_player + oxorany(0x38));
        if (!properties_registry) return false;

        int count = rpm<int>(properties_registry + oxorany(0x20));
        if (count <= 0 || count > 128) return false;

        uint64_t properties_list = rpm<uint64_t>(properties_registry + oxorany(0x18));
        if (!properties_list) return false;

        for (int i = 0; i < count; i++) {
            uint64_t key   = rpm<uint64_t>(properties_list + oxorany(0x28) + oxorany(0x18) * i);
            uint64_t value = rpm<uint64_t>(properties_list + oxorany(0x30) + oxorany(0x18) * i);
            if (!key || !value) continue;
            std::string key_string = rpm<read_string>(key).as_utf8();
            if (key_string.empty()) continue;
            if (std::strstr(key_string.c_str(), tag)) {
                out_value = rpm<int>(value + oxorany(0x10));
                return true;
            }
        }
        return false;
    }

    static bool pass_visible_check(uint64_t player_ptr) {
        if (!cfg::aim::visible_check) return true;

        // Проверяем кэш
        for (int i = 0; i < s_vis_count; i++) {
            if (s_vis_cache[i].player_ptr == player_ptr && s_vis_cache[i].ttl > 0) {
                s_vis_cache[i].ttl--;
                return s_vis_cache[i].result;
            }
        }

        // Строковый поиск по photon-свойствам
        int  value  = 0;
        bool result = true;
        if (find_int_property(player_ptr, "visible",    value) ||
            find_int_property(player_ptr, "Visible",    value) ||
            find_int_property(player_ptr, "isVisible",  value) ||
            find_int_property(player_ptr, "is_visible", value)) {
            result = (value != 0);
        } else if (find_int_property(player_ptr, "occluded",   value) ||
                   find_int_property(player_ptr, "Occluded",   value) ||
                   find_int_property(player_ptr, "isOccluded", value)) {
            result = (value == 0);
        }

        if (s_vis_count < 64)
            s_vis_cache[s_vis_count++] = { player_ptr, result, 4 };
        return result;
    }
}

void aim::run() {
    if (!cfg::aim::enabled) return;
    float smooth_setting = std::clamp(cfg::aim::smooth, 0.f, 1.f);
    bool hard_lock_mode  = smooth_setting <= 0.0001f;

    uint64_t PlayerManager = get_player_manager();
    if (!PlayerManager) return;

    uint64_t LocalPlayer = rpm<uint64_t>(PlayerManager + oxorany(kOffPlayerManagerLocalPlayer));
    if (!LocalPlayer) return;

    matrix  ViewMatrix    = player::view_matrix(LocalPlayer);
    Vector3 LocalPosition = player::position(LocalPlayer);
    int     LocalTeam     = rpm<uint8_t>(LocalPlayer + oxorany(0x79));

    uint64_t PlayerList = rpm<uint64_t>(PlayerManager + oxorany(0x28));
    if (!PlayerList) return;

    int PlayerCount = rpm<int>(PlayerList + oxorany(0x20));
    if (PlayerCount <= 0 || PlayerCount > 64) return;

    uint64_t ListBuffer = rpm<uint64_t>(PlayerList + oxorany(0x18));
    if (!ListBuffer) return;

    const ImVec2 center(g_sw * 0.5f, g_sh * 0.5f);
    const float  radius    = fov_radius_px(cfg::aim::fov, g_sh);
    float        best_dist = radius + 1.f;
    ImVec2       best_screen(0.f, 0.f);
    bool         found = false;

    // Сброс устаревших записей кэша видимости
    {
        int w = 0;
        for (int i = 0; i < s_vis_count; i++)
            if (s_vis_cache[i].ttl > 0) s_vis_cache[w++] = s_vis_cache[i];
        s_vis_count = w;
    }

    for (int i = 0; i < PlayerCount; i++) {
        uint64_t Player = rpm<uint64_t>(ListBuffer + oxorany(0x30) + oxorany(0x18) * i);
        if (!Player || Player == LocalPlayer) continue;

        uint8_t PlayerTeam = rpm<uint8_t>(Player + oxorany(0x79));
        if (PlayerTeam == static_cast<uint8_t>(LocalTeam)) continue;

        int Health = player::health(Player);
        if (Health <= 0) continue;

        Vector3 Position = player::position(Player);
        if (Position.x == 0.f && Position.y == 0.f && Position.z == 0.f) continue;

        float Distance = calculate_distance(Position, LocalPosition);
        if (Distance > cfg::aim::max_distance) continue;
        if (!pass_visible_check(Player)) continue;

        // ── ФИКС: целимся в кость (голова/шея/грудь) через get_bones ──────
        Vector3 TargetPoint = target_point(Player, Position);
        ImVec2  ScreenPos;
        if (!world_to_screen(TargetPoint, ViewMatrix, ScreenPos)) continue;

        float dx = ScreenPos.x - center.x;
        float dy = ScreenPos.y - center.y;
        float dist_to_center = std::sqrt(dx * dx + dy * dy);

        if (dist_to_center <= radius && dist_to_center < best_dist) {
            best_dist   = dist_to_center;
            best_screen = ScreenPos;
            found       = true;
        }
    }

    if (!found) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    if (cfg::aim::lock_line) {
        dl->AddLine(center, best_screen, IM_COL32(0, 0, 0, 180), 2.f);
        dl->AddLine(center, best_screen, IM_COL32(255, 255, 255, 220), 1.f);
    }
    if (cfg::aim::lock_dot) {
        dl->AddCircleFilled(best_screen, 3.5f, IM_COL32(255, 255, 255, 230), 10);
        dl->AddCircle(best_screen, 5.5f, IM_COL32(0, 0, 0, 170), 12, 1.f);
    }

    uint64_t aim_controller = rpm<uint64_t>(LocalPlayer + oxorany(kOffPlayerAimController));
    if (!aim_controller) return;
    uint64_t aiming_data = rpm<uint64_t>(aim_controller + oxorany(kOffAimControllerAimingData));
    if (!aiming_data) return;

    float dx_screen = best_screen.x - center.x;
    float dy_screen = best_screen.y - center.y;

    float half_v_fov_rad = (kVerticalFovDeg * 0.5f) * (kPi / 180.f);
    float tan_half_v     = std::tan(half_v_fov_rad);
    if (tan_half_v < 0.00001f) return;
    float focal_px = (g_sh * 0.5f) / tan_half_v;
    if (focal_px < 1.f) return;

    const float rad2deg    = 57.2957795f;
    float pitch_delta_deg  = std::atan(dy_screen / focal_px) * rad2deg;
    float yaw_delta_deg    = std::atan(dx_screen / focal_px) * rad2deg;

    float cur_pitch = rpm<float>(aiming_data + oxorany(kOffAimingDataPitch));
    float cur_yaw   = rpm<float>(aiming_data + oxorany(kOffAimingDataYaw));

    float step      = hard_lock_mode ? 1.f : std::clamp(smooth_setting, 0.0001f, 1.f);
    float out_pitch = std::clamp(cur_pitch + pitch_delta_deg * step, -89.f, 89.f);
    float out_yaw   = normalize_yaw(cur_yaw + yaw_delta_deg * step);

    float vec[3] = { out_pitch, out_yaw, 0.f };
    wpm<float>(aiming_data + oxorany(kOffAimingDataPitch), out_pitch);
    wpm<float>(aiming_data + oxorany(kOffAimingDataYaw),   out_yaw);
    mem_write(aiming_data + oxorany(kOffAimingDataPitchYawVec3), vec, sizeof(vec));
    mem_write(aiming_data + oxorany(kOffAimingDataSecondVec3),   vec, sizeof(vec));
}

void aim::draw_fov(ImDrawList* draw, float screen_w, float screen_h) {
    if (!draw || !cfg::aim::enabled || !cfg::aim::fov_display || screen_w <= 0.f || screen_h <= 0.f) return;
    float cx     = screen_w * 0.5f;
    float cy     = screen_h * 0.5f;
    float radius = fov_radius_px(cfg::aim::fov, screen_h);
    draw->AddCircle(ImVec2(cx, cy), radius, IM_COL32(0, 0, 0, 130), 32, 2.f);
    draw->AddCircle(ImVec2(cx, cy), radius, ImGui::ColorConvertFloat4ToU32(cfg::aim::fov_col), 32, 1.f);
}
