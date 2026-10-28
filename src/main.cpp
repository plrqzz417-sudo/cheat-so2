#include "Android_draw/draw.h"
#include "ui/theme/theme.hpp"
#include "ui/menu.hpp"
#include "other/memory.hpp"
#include "game/game.hpp"
#include "game/offsets.hpp"
#include "game/player.hpp"
#include "func/visuals.hpp"
#include "func/aim.hpp"
#include "func/triggerbot.hpp"
#include "func/silent.hpp"
#include "func/chams.hpp"
#include "func/mapcollider.hpp"
#include "func/maps_embedded.h"
#include "protect/oxorany.hpp"
#include "ui/cfg.hpp"
#include "imgui.h"

// ── Новые функции ─────────────────────────────────────────────────────────
#include "func/weapon.hpp"
#include "func/movement.hpp"
#include "func/world.hpp"
#include "func/ambient_light.hpp"
#include "func/anti_effects.hpp"
#include "func/arms.hpp"
#include "func/camera_unlock.hpp"
#include "func/props.hpp"
#include "func/invisible.hpp"
#include "func/autowin.hpp"

#include <cstdio>
#include <thread>
#include <chrono>

static void print_status(const char* status) {
    printf(oxorany("\033[2J\033[H\033[1;38;2;162;144;225m[@nnrpg]\033[0m \033[1;37m%s\033[0m\n"), status);
}

static void launch_standoff() {
    system(oxorany("am start -n com.standoff/com.standoff.MainActivity"));
}

int main() {
    screen_config();

    int max_size = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    int min_size = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);

    native_window_screen_x = max_size;
    native_window_screen_y = max_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) return -1;

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    embedded_maps::ExtractAll(oxorany("/sdcard/NexuraWare/maps/"));

    print_status(oxorany("Game detect \u2705"));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print_status(oxorany("start cheat...."));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    launch_standoff();

    game::init();

    static bool prev         = false;
    static bool game_started = false;

    while (true) {
        drawBegin();

        bool run = game::valid();

#if defined(__x86_64__)
        bool is_landscape = (displayInfo.orientation == 0 || displayInfo.orientation == 2);
#else
        bool is_landscape = (displayInfo.orientation == 1 || displayInfo.orientation == 3);
#endif

        if (run && !prev) {
            if (!game_started) {
                print_status(oxorany("Game detect \u2705"));
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                print_status(oxorany("start cheat...."));
                game_started = true;
            } else {
                print_status(oxorany("Game detect \u2705"));
            }
            prev = true;
        } else if (!run && prev) {
            print_status(oxorany("game closed"));
            prev         = false;
            game_started = false;
        }

        if (is_landscape) {
            ui::menu::render();

            // ── Ambient Light (overlay, не требует игры) ──────────────────
            ambient_light::run();

            if (run && proc::lib != 0) {
                game::check_lib(get_player_manager());

                uint64_t pm = get_player_manager();
                uint64_t lp = pm ? rpm<uint64_t>(pm + oxorany(0x70ULL)) : 0;

                // ── ESP ───────────────────────────────────────────────────
                visuals::draw();

                // ── Chams ─────────────────────────────────────────────────
                chams::handle();

                // ── Map Collider ───────────────────────────────────────────
                if (cfg::world::collider_enabled && pm && lp) {
                    matrix  vm  = player::view_matrix(lp);
                    Vector3 cam = player::camera_position(lp);
                    ImDrawList* dl = ImGui::GetBackgroundDrawList();
                    collider_esp::draw(dl, vm, cam);
                }

                // ── Aimbot ────────────────────────────────────────────────
                aim::run();
                aim::draw_fov(ImGui::GetBackgroundDrawList(), g_sw, g_sh);

                // ── Triggerbot ────────────────────────────────────────────
                if (lp) triggerbot::run(lp);

                // ── Silent Aim ────────────────────────────────────────────
                silent_aim::run();
                silent_aim::draw_fov(ImGui::GetBackgroundDrawList(), g_sw, g_sh);

                // ── Weapon hacks ──────────────────────────────────────────
                weapon::run_all();

                // ── Movement hacks ────────────────────────────────────────
                movement_cache_refresh();
                air_jump::run();
                high_jump::run();
                bunny_hop::run();
                strafe::run();
                air_strafe::run();
                fast_walk::run();
                crouch_speed::run();
                // teleport: нужен weaponry ptr
                if (lp) {
                    uint64_t weaponry = rpm<uint64_t>(lp + oxorany(0x88ULL));
                    teleport_hack::run(weaponry);
                }

                // ── World features (FOV / BigHead / Potato) ───────────────
                world::run();

                // ── Anti Effects ──────────────────────────────────────────
                anti_effects::run();

                // ── Camera Unlock ─────────────────────────────────────────
                camera_unlock::run();

                // ── Arms (Viewmodel offset) ───────────────────────────────
                arms::run();

                // ── Props (Photon, каждые 5 тиков внутри) ────────────────
                if (pm && lp) props_ns::tick(pm, lp);

                // ── Invisible (weapon slot trick) ─────────────────────────
                invisible_hack::run();

                // ── Auto Win ──────────────────────────────────────────────
                autowin::run();
            }
        }

        drawEnd();
        usleep(4000);
    }

    shutdown();
    return 0;
}
