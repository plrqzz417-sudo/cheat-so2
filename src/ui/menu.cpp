#include "menu.hpp"
#include "cfg.hpp"
#include "imgui.h"
#include "Android_draw/draw.h"
#include "../func/mapcollider.hpp"
#include <cstdio>

extern float g_sw, g_sh;

namespace ui::menu {
    static const char* box_types[]    = { "Full", "Corner", "3D Box" };
    static const char* silent_bones[] = { "Head", "Neck", "Spine" };
    static ImVec2 window_size(640, 560);

    void render() {
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(
            (g_sw - window_size.x) * 0.5f,
            (g_sh - window_size.y) * 0.5f
        ), ImGuiCond_Once);

        if (!ImGui::Begin("NexuraWare | 0.39.2", nullptr, ImGuiWindowFlags_None)) {
            ImGui::End();
            return;
        }

        window_size = ImGui::GetWindowSize();
        if (ImGui::IsWindowCollapsed()) { ImGui::End(); return; }

        if (ImGui::BeginTabBar("MainTabs")) {

            // ─── VISUALS ─────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Visuals")) {

                ImGui::Text("ESP");
                ImGui::Separator();

                ImGui::Checkbox("Box", &cfg::esp::box);
                if (cfg::esp::box) {
                    ImGui::Combo("Box type", &cfg::esp::box_type, box_types, IM_ARRAYSIZE(box_types));
                    cfg::esp::box3d = (cfg::esp::box_type == 2);
                    if (cfg::esp::box_type != 2) {
                        ImGui::SliderFloat("Box rounding", &cfg::esp::box_rounding, 0.f, 10.f, "%.0f");
                        ImGui::ColorEdit4("Box color",    (float*)&cfg::esp::box_col);
                        ImGui::Checkbox("Box RGB",        &cfg::esp::box_rgb);
                    }
                }
                ImGui::Spacing();

                ImGui::Checkbox("3D Box", &cfg::esp::box3d);
                if (cfg::esp::box3d) {
                    ImGui::ColorEdit4("3D Box color",     (float*)&cfg::esp::box3d_col);
                    ImGui::Checkbox("3D Box RGB",         &cfg::esp::box3d_rgb);
                    ImGui::SliderFloat("3D Box thickness",&cfg::esp::box3d_thickness, 0.5f, 4.f, "%.1f");
                }
                ImGui::Spacing();

                ImGui::Checkbox("Health bar", &cfg::esp::health);
                if (cfg::esp::health) {
                    ImGui::ColorEdit4("Health color", (float*)&cfg::esp::health_col);
                    ImGui::Checkbox("Health RGB",     &cfg::esp::health_rgb);
                }
                ImGui::Spacing();

                ImGui::Checkbox("Line", &cfg::esp::line);
                if (cfg::esp::line) {
                    ImGui::ColorEdit4("Line color", (float*)&cfg::esp::line_col);
                    ImGui::Checkbox("Line RGB",     &cfg::esp::line_rgb);
                }
                ImGui::Spacing();
                ImGui::Separator();

                ImGui::Text("Chams");
                ImGui::Separator();
                ImGui::Checkbox("Material Chams", &cfg::chams::enabled);
                if (cfg::chams::enabled)
                    ImGui::Checkbox("Team check (skip allies)", &cfg::chams::team_check);
                ImGui::Spacing();
                ImGui::Separator();

                ImGui::Text("Map Collider");
                ImGui::Separator();
                bool prev_col = cfg::world::collider_enabled;
                ImGui::Checkbox("Map Collider", &cfg::world::collider_enabled);
                if (cfg::world::collider_enabled && !prev_col)
                    collider_esp::load_map(cfg::world::collider_map_id);
                if (cfg::world::collider_enabled) {
                    ImGui::SliderFloat("Collider alpha",   &cfg::world::collider_alpha,    0.f, 1.f,    "%.2f");
                    ImGui::SliderFloat("Max distance (m)", &cfg::world::collider_max_dist, 10.f, 200.f, "%.0f");
                    ImGui::ColorEdit4("Collider color",    (float*)&cfg::world::collider_color);
                    ImGui::Separator();
                    ImGui::TextDisabled("Select map:");
                    const int count = map_service::g_collider_map_count;
                    int cur = cfg::world::collider_map_id;
                    if (cur < 0 || cur >= count) cur = 0;
                    if (ImGui::Combo("##map_pick", &cur, map_service::g_collider_map_display_names, count))
                        collider_esp::load_map(cur);
                    if (!map_service::g_loaded)
                        ImGui::TextColored(ImVec4(1.f,0.4f,0.4f,1.f), "No GLB loaded. Put files in /sdcard/NexuraWare/maps/");
                    else
                        ImGui::TextColored(ImVec4(0.4f,1.f,0.4f,1.f), "GLB loaded OK.");
                }
                ImGui::Spacing();
                ImGui::Separator();

                // ── Ambient Light ─────────────────────────────────────────
                ImGui::Text("Ambient Light");
                ImGui::Separator();
                ImGui::Checkbox("Enabled##ambient",       &cfg::ambient::enabled);
                if (cfg::ambient::enabled) {
                    ImGui::ColorEdit4("Color##ambient",   (float*)&cfg::ambient::color);
                    ImGui::SliderFloat("Intensity",       &cfg::ambient::intensity, 0.f, 0.85f, "%.2f");
                    ImGui::Checkbox("Pulse",              &cfg::ambient::pulse);
                    if (cfg::ambient::pulse)
                        ImGui::SliderFloat("Pulse speed", &cfg::ambient::pulse_spd, 0.5f, 5.f, "%.1f");
                }

                ImGui::EndTabItem();
            }

            // ─── AIMBOT ──────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Aimbot")) {

                ImGui::Text("Aimbot");
                ImGui::Separator();
                ImGui::Checkbox("Enabled##aim",         &cfg::aim::enabled);
                ImGui::SliderFloat("FOV (deg)",          &cfg::aim::fov,    1.f, 179.f, "%.0f");
                ImGui::SliderFloat("Smooth##aim",        &cfg::aim::smooth, 0.f, 1.f,   "%.3f");
                ImGui::Combo("Bone##aim",               &cfg::aim::target, silent_bones, 3);
                ImGui::Checkbox("Visible check##aim",   &cfg::aim::visible_check);
                ImGui::SliderFloat("Max dist (m)",       &cfg::aim::max_distance, 10.f, 500.f, "%.0f");
                ImGui::Separator();
                ImGui::Text("FOV Circle");
                ImGui::Checkbox("Show FOV",             &cfg::aim::fov_display);
                ImGui::ColorEdit4("FOV color",          (float*)&cfg::aim::fov_color);
                ImGui::Checkbox("RGB FOV",              &cfg::aim::fov_rgb);
                if (cfg::aim::fov_rgb)
                    ImGui::SliderFloat("RGB speed",     &cfg::aim::fov_rgb_speed, 0.5f, 5.f, "%.1f");
                ImGui::SliderFloat("FOV glow",          &cfg::aim::fov_glow, 0.f, 10.f, "%.0f");
                ImGui::Checkbox("Lock line",            &cfg::aim::lock_line);
                ImGui::Checkbox("Lock dot",             &cfg::aim::lock_dot);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Triggerbot");
                ImGui::Separator();
                ImGui::Checkbox("Enabled##trigger",     &cfg::aim::triggerbot);
                ImGui::SliderFloat("Delay (s)",          &cfg::aim::trigger_delay, 0.f, 0.5f, "%.2f");
                ImGui::SliderFloat("Range (px)",         &cfg::aim::trigger_range, 5.f, 60.f, "%.0f");
                ImGui::Checkbox("Visible only##trigger",&cfg::aim::trigger_visible_only);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Silent Aim");
                ImGui::Separator();
                ImGui::Checkbox("Enabled##silent",      &cfg::aim::silent::enabled);
                ImGui::SliderFloat("FOV (deg)##silent", &cfg::aim::silent::fov, 1.f, 90.f, "%.0f");
                ImGui::Checkbox("Show FOV##silent",     &cfg::aim::silent::fov_display);
                ImGui::Combo("Bone##silent",            &cfg::aim::silent::target_bone, silent_bones, IM_ARRAYSIZE(silent_bones));
                ImGui::Checkbox("Visible check##silent",&cfg::aim::silent::visible_check);
                ImGui::SliderFloat("Recoil mult",       &cfg::aim::silent::recoil_mult, 0.05f, 3.f, "%.2f");

                ImGui::EndTabItem();
            }

            // ─── WEAPON ──────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Weapon")) {

                ImGui::Text("Rage");
                ImGui::Separator();

                ImGui::Checkbox("Wallshot",            &cfg::wallshot::enabled);
                if (cfg::wallshot::enabled)
                    ImGui::SliderInt("Penetration",    &cfg::wallshot::value, 100, 2000);

                ImGui::Checkbox("No Recoil",           &cfg::norecoil::enabled);
                if (cfg::norecoil::enabled)
                    ImGui::SliderFloat("Recoil mult",  &cfg::norecoil::multiplier, 0.f, 0.5f, "%.2f");

                ImGui::Checkbox("Inf Ammo",            &cfg::inf_ammo::enabled);
                if (cfg::inf_ammo::enabled)
                    ImGui::SliderInt("Ammo value",     &cfg::inf_ammo::value, 100, 9999);

                ImGui::Checkbox("Inf Shop",            &cfg::inf_shop::enabled);
                if (cfg::inf_shop::enabled)
                    ImGui::SliderInt("Shop value",     &cfg::inf_shop::value, 100, 9999);

                ImGui::Checkbox("Fast Fire Rate",      &cfg::fire_rate::enabled);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Knife / Sigma");
                ImGui::Separator();

                ImGui::Checkbox("Fast Knife",          &cfg::fustknife::enabled);

                ImGui::Checkbox("Sigma (OHK)",         &cfg::sigma::enabled);
                if (cfg::sigma::enabled)
                    ImGui::SliderInt("Damage",         &cfg::sigma::damage, 1, 1000);

                ImGui::EndTabItem();
            }

            // ─── MOVEMENT ────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Movement")) {

                ImGui::Text("Movement Hacks");
                ImGui::Separator();

                ImGui::Checkbox("Air Jump",            &cfg::movement::air_jump::enabled);

                ImGui::Checkbox("High Jump",           &cfg::movement::high_jump::enabled);
                if (cfg::movement::high_jump::enabled)
                    ImGui::SliderFloat("Jump Height",  &cfg::movement::high_jump::height, 1.f, 50.f, "%.0f");

                ImGui::Checkbox("Bunny Hop",           &cfg::movement::bunny_hop::enabled);
                ImGui::Checkbox("Strafe",              &cfg::movement::strafe::enabled);
                ImGui::Checkbox("Air Strafe",          &cfg::movement::air_strafe::enabled);
                ImGui::Checkbox("Fast Walk",           &cfg::movement::fast_walk::enabled);
                ImGui::Checkbox("Crouch Speed",        &cfg::movement::crouch_speed::enabled);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Teleport");
                ImGui::Separator();
                ImGui::Checkbox("Teleport Enabled",    &cfg::movement::teleport::enabled);
                if (cfg::movement::teleport::enabled) {
                    ImGui::SameLine();
                    ImGui::Checkbox("Active##tp",      &cfg::movement::teleport::active);
                }

                ImGui::EndTabItem();
            }

            // ─── WORLD ───────────────────────────────────────────────────
            if (ImGui::BeginTabItem("World")) {

                ImGui::Text("FOV Changer");
                ImGui::Separator();
                ImGui::Checkbox("FOV Override",        &cfg::world::fov);
                if (cfg::world::fov)
                    ImGui::SliderFloat("FOV value",    &cfg::world::fov_value, 30.f, 120.f, "%.0f");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Big Head");
                ImGui::Separator();
                ImGui::Checkbox("Big Head",            &cfg::world::bighead);
                if (cfg::world::bighead)
                    ImGui::SliderFloat("Head scale",   &cfg::world::bighead_scale, 1.5f, 8.f, "%.1f");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Graphics");
                ImGui::Separator();
                ImGui::Checkbox("Potato GFX",          &cfg::world::potato);
                ImGui::TextDisabled("(scan patches texture quality, runs in background)");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Anti Effects");
                ImGui::Separator();
                ImGui::Checkbox("Anti Flash",          &cfg::anti_flash::enabled);
                ImGui::Checkbox("Anti Smoke",          &cfg::anti_smoke::enabled);
                ImGui::Checkbox("Anti Molotov",        &cfg::anti_molotov::enabled);

                ImGui::EndTabItem();
            }

            // ─── MISC ────────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Misc")) {

                ImGui::Text("Camera");
                ImGui::Separator();
                ImGui::Checkbox("Camera Unlock (360 pitch)", &cfg::misc::camera_unlock);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Rage");
                ImGui::Separator();
                ImGui::Checkbox("Invisible (weapon trick)", &cfg::misc::invisible);
                ImGui::TextDisabled("(makes your weapon invisible to others via slot trick)");

                ImGui::Spacing();
                ImGui::Checkbox("Auto Win",                 &cfg::misc::autowin);
                ImGui::TextDisabled("(zeros enemy HP + round timers; host = syncs to all)");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Arms (Viewmodel)");
                ImGui::Separator();
                ImGui::Checkbox("Arms Offset##arms",    &cfg::arms::enabled);
                if (cfg::arms::enabled) {
                    ImGui::SliderFloat("X##arms",       &cfg::arms::pos_x, -0.5f, 0.5f, "%.3f");
                    ImGui::SliderFloat("Y##arms",       &cfg::arms::pos_y, -0.5f, 0.5f, "%.3f");
                    ImGui::SliderFloat("Z##arms",       &cfg::arms::pos_z, -0.5f, 0.5f, "%.3f");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Props (Photon)");
                ImGui::Separator();

                ImGui::Checkbox("Set Score",            &cfg::props::set_score);
                if (cfg::props::set_score)
                    ImGui::SliderInt("Score value",     &cfg::props::score_val, 0, 999);

                ImGui::Checkbox("Set Kills",            &cfg::props::set_kills);
                if (cfg::props::set_kills)
                    ImGui::SliderInt("Kills value",     &cfg::props::kills_val, 0, 999);

                ImGui::Checkbox("Set Deaths",           &cfg::props::set_death);
                if (cfg::props::set_death)
                    ImGui::SliderInt("Deaths value",    &cfg::props::death_val, 0, 999);

                ImGui::Checkbox("Set Ping (self)",      &cfg::props::set_ping);
                if (cfg::props::set_ping)
                    ImGui::SliderInt("Ping value",      &cfg::props::ping_val, 1, 999);

                ImGui::Checkbox("Set Ping (all)",       &cfg::props::set_ping_all);
                if (cfg::props::set_ping_all)
                    ImGui::SliderInt("Ping all value",  &cfg::props::ping_all_val, 1, 999);

                ImGui::Checkbox("MVP (self)",           &cfg::props::set_mvp);
                ImGui::Checkbox("Hide ID (self)",       &cfg::props::hide_id);
                ImGui::Checkbox("Hide Clan (self)",     &cfg::props::hide_clan);
                ImGui::Checkbox("Kick Players",         &cfg::props::kick_players);

                ImGui::EndTabItem();
            }

            // ─── SETTINGS ────────────────────────────────────────────────
            if (ImGui::BeginTabItem("Settings")) {
                char buf[64];
                snprintf(buf, sizeof(buf), "Screen: %.0f x %.0f", g_sw, g_sh);
                ImGui::Text("%s", buf);
                ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
                ImVec2 sz = ImGui::GetWindowSize();
                ImGui::Text("Window: %.0f x %.0f", sz.x, sz.y);
                if (ImGui::Button("Reset Size")) {
                    window_size = ImVec2(640, 560);
                    ImGui::SetWindowSize(window_size);
                }
                ImGui::Separator();
                if (ImGui::Button("Exit")) exit(0);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
