// ─────────────────────────────────────────────────────────────────────────────
//  INVISIBLE — Weapon Invisible (WeaponryController slot trick)
//  Порт invisible.cpp под normal7 SDK (rpm/wpm + oxorany).
//  Записывает слот 10 в WeaponryController, что делает оружие невидимым
//  для других игроков на стороне Photon-синхронизации.
// ─────────────────────────────────────────────────────────────────────────────

#include "invisible.hpp"
#include "../game/game.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"

namespace {
    static constexpr uint64_t kOffLocalPlayer        = 0x70;
    static constexpr uint64_t kOffWeaponryController = 0x88;  // OFF_PLAYER_WEAPON_ROOT
    static constexpr uint64_t kOffWeaponryState      = 0x88;  // slot byte внутри WRC

    static inline bool likely_ptr(uint64_t p) {
        return p > 0x10000ull && p < 0x0000FFFFFFFFFFFFull;
    }
}

namespace invisible_hack {

void run() {
    if (!cfg::misc::invisible) return;

    uint64_t pm = get_player_manager();
    if (!likely_ptr(pm)) return;

    uint64_t lp = rpm<uint64_t>(pm + oxorany(kOffLocalPlayer));
    if (!likely_ptr(lp)) return;

    uint64_t weaponry = rpm<uint64_t>(lp + oxorany(kOffWeaponryController));
    if (!likely_ptr(weaponry)) return;

    uint8_t slot = rpm<uint8_t>(weaponry + oxorany(kOffWeaponryState));
    if (slot > 20) return;

    wpm<uint8_t>(weaponry + oxorany(kOffWeaponryState), 10);
}

} // namespace invisible_hack
