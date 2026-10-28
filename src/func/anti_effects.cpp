// ─────────────────────────────────────────────────────────────────────────────
//  ANTI EFFECTS — портировано из Scarecrow (FlashOn / SmokeOn / MolotovOn)
//  В SDK neverlose2 scan-engine отсутствует, поэтому работаем через статику
//  il2cpp (GrenadeEffectManager). Патч идемпотентен, можно звать каждый тик.
// ─────────────────────────────────────────────────────────────────────────────

#include "anti_effects.hpp"
#include "../game/game.hpp"
#include "../other/memory.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <cstdint>

namespace {
    // GrenadeEffectManager — класс il2cpp, найден через il2cpp-dump (Standoff2 arm64).
    // Оффсеты зависят от версии игры, помечены как «TODO: верифицировать».
    static constexpr uint64_t kGrenadeEffectMgrStatic = 0x0928A320ULL;
    static constexpr uint64_t kGEMStaticFields       = 0x60ULL;
    static constexpr uint64_t kGEMFlashDuration      = 0x28;
    static constexpr uint64_t kGEMSmokeDensity       = 0x34;
    static constexpr uint64_t kGEMMolotovRadius      = 0x40;

    static inline bool likely_ptr(uint64_t p) {
        return p > 0x10000ull && p < 0x0000FFFFFFFFFFFFull;
    }

    static bool read_ptr(uint64_t addr, uint64_t& out) {
        out = 0;
        if (!likely_ptr(addr)) return false;
        if (!mem_read(addr, &out, sizeof(out))) return false;
        return likely_ptr(out);
    }
}

void anti_effects::run() {
    if (!cfg::anti_flash::enabled && !cfg::anti_smoke::enabled && !cfg::anti_molotov::enabled)
        return;

    if (!likely_ptr(proc::lib)) return;

    uint64_t klass_ptr = 0;
    if (!read_ptr(proc::lib + oxorany(kGrenadeEffectMgrStatic), klass_ptr)) return;
    uint64_t static_fields = 0;
    if (!read_ptr(klass_ptr + oxorany(kGEMStaticFields), static_fields)) return;

    if (cfg::anti_flash::enabled)
        wpm<float>(static_fields + oxorany(kGEMFlashDuration), 0.0f);

    if (cfg::anti_smoke::enabled)
        wpm<float>(static_fields + oxorany(kGEMSmokeDensity), 0.0f);

    if (cfg::anti_molotov::enabled)
        wpm<float>(static_fields + oxorany(kGEMMolotovRadius), 0.0f);
}
