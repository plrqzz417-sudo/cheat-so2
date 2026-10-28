// ─────────────────────────────────────────────────────────────────────────────
//  ARMS — управление позицией viewmodel (рук с оружием) у локального игрока.
//  Порт из NexuraWare: lp → AimAnimController(0xA0) → viewmodelOffset(0xE8) Vector3.
//  Значения прибавляются к текущей позиции каждый тик.
// ─────────────────────────────────────────────────────────────────────────────

#include "arms.hpp"
#include "../game/game.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include "../other/vector3.h"

namespace {
    static constexpr uint64_t kOffPlayerManagerStaticLegacy = 132435632ULL;
    static constexpr uint64_t kOffAimAnimCtrl = 0xA0;
    static constexpr uint64_t kOffViewmodelOffset = 0xE8;

    static inline bool likely_ptr(uint64_t p) {
        return p > 0x10000ull && p < 0x0000FFFFFFFFFFFFull;
    }

    static uint64_t get_local_player() {
        uint64_t pm = get_player_manager();
        if (!likely_ptr(pm)) pm = get_static<uint64_t>(oxorany(kOffPlayerManagerStaticLegacy));
        if (!likely_ptr(pm)) return 0;
        return rpm<uint64_t>(pm + oxorany(0x70));
    }
}

void arms::run() {
    if (!cfg::arms::enabled) return;

    uint64_t lp = get_local_player();
    if (!likely_ptr(lp)) return;

    uint64_t aac = rpm<uint64_t>(lp + oxorany(kOffAimAnimCtrl));
    if (!likely_ptr(aac)) return;

    Vector3 pos = rpm<Vector3>(aac + oxorany(kOffViewmodelOffset));
    pos.x += cfg::arms::pos_x;
    pos.y += cfg::arms::pos_y;
    pos.z += cfg::arms::pos_z;
    wpm<Vector3>(aac + oxorany(kOffViewmodelOffset), pos);
}
