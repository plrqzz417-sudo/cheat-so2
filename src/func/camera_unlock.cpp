// ─────────────────────────────────────────────────────────────────────────────
//  CAMERA UNLOCK — разблокировка вертикального угла камеры
//  Снимает ограничения pitch: записывает ±360° в поля clamp AimController.
//
//  Chain: PlayerManager(+0x70) → LocalPlayer(+0x80) → AimController
//         aimController + 0x64  → maxPitch  = +360.99f
//         aimController + 0x60  → minPitch  = -360.00f
// ─────────────────────────────────────────────────────────────────────────────

#include "camera_unlock.hpp"
#include "../game/game.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <stdint.h>

static constexpr uint64_t kOffLocalPlayer    = 0x70;
static constexpr uint64_t kOffAimController  = 0x80;
static constexpr uint64_t kOffMinPitch       = 0x60;
static constexpr uint64_t kOffMaxPitch       = 0x64;

static inline bool likely_ptr(uint64_t p) {
    return p > 0x10000ull && p < 0x0000FFFFFFFFFFFFull;
}

void camera_unlock::run() {
    if (!cfg::misc::camera_unlock) return;

    uint64_t pm = get_player_manager();
    if (!likely_ptr(pm)) return;

    uint64_t lp = rpm<uint64_t>(pm + oxorany(kOffLocalPlayer));
    if (!likely_ptr(lp)) return;

    uint64_t aimController = rpm<uint64_t>(lp + oxorany(kOffAimController));
    if (!likely_ptr(aimController)) return;

    wpm<float>(aimController + oxorany(kOffMaxPitch), 360.99f);
    wpm<float>(aimController + oxorany(kOffMinPitch), -360.00f);
}
