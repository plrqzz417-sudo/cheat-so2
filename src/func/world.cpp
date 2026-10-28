// ─────────────────────────────────────────────────────────────────────────────
//  WORLD FEATURES — порт из intmain12_rework под экстернал NexuraWare 
//
//  Перенесены ТОЛЬКО функции, работающие через чтение/запись памяти
//  (process_vm_readv/writev) — они гарантированно работают из внешнего процесса:
//
//    1. FOV changer   — порт f_fov.hpp
//       Chain: PlayerManager(+0x70) → LocalPlayer → MainCamera(+0xE8)
//              → CameraSettings(+0x28) → fieldOfView(+0x38)
//
//    2. Big Head      — порт f_bighead.hpp
//       biped_map → head transform → TransformEntry.scale (@ +0x20 в 48-байтной
//       записи). Коллайдеры головы следуют за parent Transform scale.
//
//    3. Potato GFX    — порт f_gfxpotato.hpp (texture-часть)
//       Скан rw- регионов /proc/<pid>/maps: dword 0x40000042 -> 0x40C00000.
//       Оригиналы бэкапятся и восстанавливаются при выключении.
//
//  НЕ перенесены (требуют внутренних il2cpp-вызовов, из экстернала недоступны):
//    Sky Color (Camera.set_clearFlags/set_backgroundColor),
//    Fog (RenderSettings.set_fog*), World Color (Material.set_shader/set_color).
// ─────────────────────────────────────────────────────────────────────────────

#include "world.hpp"
#include "../game/game.hpp"
#include "../game/player.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"

#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <thread>
#include <atomic>

namespace world {

// ── запись в память целевого процесса ────────────────────────────────────────
static bool wpm_raw(uint64_t addr, const void* buf, size_t len) {
    if (proc::pid <= 0 || !addr) return false;
    struct iovec lv, rv;
    lv.iov_base = const_cast<void*>(buf);
    lv.iov_len  = len;
    rv.iov_base = reinterpret_cast<void*>(addr);
    rv.iov_len  = len;
    return process_vm_writev(proc::pid, &lv, 1, &rv, 1, 0) == (ssize_t)len;
}

template<typename T>
static bool wpm(uint64_t addr, const T& v) { return wpm_raw(addr, &v, sizeof(T)); }

// ═════════════════════════════════════════════════════════════════════════════
//  1. FOV CHANGER (порт f_fov.hpp)
// ═════════════════════════════════════════════════════════════════════════════
namespace fov {

#define FOV_OFF_MAIN_CAMERA   0xE8
#define FOV_OFF_CAM_SETTINGS  0x28
#define FOV_OFF_CAM_FOV       0x38

static bool  orig_saved = false;
static float orig_fov   = 60.f;

static void restore(uint64_t lp) {
    uint64_t cam = rpm<uint64_t>(lp + FOV_OFF_MAIN_CAMERA);
    if (!cam) { orig_saved = false; return; }
    uint64_t cs = rpm<uint64_t>(cam + FOV_OFF_CAM_SETTINGS);
    if (!cs)  { orig_saved = false; return; }
    wpm<float>(cs + FOV_OFF_CAM_FOV, orig_fov);
    orig_saved = false;
}

static void tick(uint64_t lp) {
    if (!cfg::world::fov) {
        if (orig_saved) restore(lp);
        return;
    }

    uint64_t cam = rpm<uint64_t>(lp + FOV_OFF_MAIN_CAMERA);
    if (!cam) return;
    uint64_t cs = rpm<uint64_t>(cam + FOV_OFF_CAM_SETTINGS);
    if (!cs) return;

    float cur = rpm<float>(cs + FOV_OFF_CAM_FOV);
    if (cur < 1.f || cur > 170.f) return; // sanity check

    if (!orig_saved) {
        orig_fov   = cur;
        orig_saved = true;
    }

    float target = cfg::world::fov_value;
    if (target < 30.f)  target = 30.f;
    if (target > 120.f) target = 120.f;

    if (fabsf(cur - target) > 0.1f)
        wpm<float>(cs + FOV_OFF_CAM_FOV, target);
}

static void reset() { orig_saved = false; }

} // namespace fov

// ═════════════════════════════════════════════════════════════════════════════
//  2. BIG HEAD (порт f_bighead.hpp)
//  TransformEntry { Vector4 position; Vector4 rotation; Vector4 scale; } = 48b
//  scale @ entry + 0x20. Свою голову и трупы не трогаем.
// ═════════════════════════════════════════════════════════════════════════════
namespace bighead {

static bool applied = false;

static void patch(uint64_t p, float mul) {
    if (!p) return;

    uint64_t map = player::biped_map(p);
    if (!player::likely_ptr(map)) return;

    // head transform: те же фоллбэки, что и в bone_position(mode 1)
    uint64_t head_tr = rpm<uint64_t>(map + oxorany(0x20));
    if (!player::likely_ptr(head_tr))
        head_tr = rpm<uint64_t>(map + oxorany(0x28));
    if (!player::likely_ptr(head_tr)) return;

    uint64_t n = rpm<uint64_t>(head_tr + oxorany(0x10));   // NativeTransform
    if (!player::likely_ptr(n)) return;
    uint64_t td = rpm<uint64_t>(n + oxorany(0x28));        // TransformData
    if (!player::likely_ptr(td)) return;
    int32_t idx = rpm<int32_t>(n + oxorany(0x30));         // index
    if (idx < 0 || idx > 100000) return;

    uint64_t arr = rpm<uint64_t>(td + oxorany(0x18));      // TransformEntry[]
    if (!player::likely_ptr(arr)) return;

    uint64_t sa = arr + (uint64_t)idx * 48ull + 0x20ull;   // &entry.scale
    float sc[4] = { mul, mul, mul, 1.f };
    wpm_raw(sa, sc, sizeof(sc));
}

static void tick(uint64_t player_manager, uint64_t local_player) {
    if (!cfg::world::bighead && !applied) return;

    float mul = cfg::world::bighead_scale;
    if (mul < 1.5f) mul = 1.5f;
    if (mul > 8.f)  mul = 8.f;
    if (!cfg::world::bighead) mul = 1.f; // выключили — вернуть нормальный scale

    uint8_t local_team = rpm<uint8_t>(local_player + oxorany(0x79));

    uint64_t player_list = rpm<uint64_t>(player_manager + oxorany(0x28));
    if (!player_list) return;
    int count = rpm<int>(player_list + oxorany(0x20));
    if (count <= 0 || count > 64) return;
    uint64_t list_buf = rpm<uint64_t>(player_list + oxorany(0x18));
    if (!list_buf) return;

    for (int i = 0; i < count; i++) {
        uint64_t p = rpm<uint64_t>(list_buf + oxorany(0x30) + oxorany(0x18) * i);
        if (!p || p == local_player) continue;
        if (rpm<uint8_t>(p + oxorany(0x79)) == local_team) continue;
        if (player::health(p) <= 0) continue;
        patch(p, mul);
    }

    applied = cfg::world::bighead;
}

static void reset() { applied = false; }

} // namespace bighead

// ═════════════════════════════════════════════════════════════════════════════
//  3. POTATO GFX (порт f_gfxpotato.hpp, texture-часть)
//  Ищем dword 0x40000042 (1073741890) во всех rw- регионах цели и патчим
//  на 0x40C00000 (1086324736 = float 6.0). Скан идёт в отдельном потоке.
// ═════════════════════════════════════════════════════════════════════════════
namespace potato {

static const uint32_t TEX_SEARCH = 1073741890u; // 0x40000042
static const uint32_t TEX_PATCH  = 1086324736u; // 0x40C00000

struct AddrVal32 { uint64_t addr; uint32_t orig; };

static std::vector<AddrVal32> bk;
static std::atomic<bool> busy{ false };
static bool applied = false;
static bool last_opt = false;

static bool read_chunk(uint64_t addr, void* buf, size_t len) {
    if (proc::pid <= 0) return false;
    struct iovec lv, rv;
    lv.iov_base = buf;
    lv.iov_len  = len;
    rv.iov_base = reinterpret_cast<void*>(addr);
    rv.iov_len  = len;
    return process_vm_readv(proc::pid, &lv, 1, &rv, 1, 0) == (ssize_t)len;
}

struct MemRegion { uint64_t start, end; };

static void get_rw_regions(std::vector<MemRegion>& out) {
    out.clear();
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc::pid);
    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        uint64_t st = 0, en = 0;
        char perm[8] = {0};
        if (sscanf(line, "%lx-%lx %7s", &st, &en, perm) != 3) continue;
        if (perm[0] == 'r' && perm[1] == 'w' && st && en > st && (en - st) >= 0x1000)
            out.push_back({ st, en });
    }
    fclose(f);
}

static void scan_and_patch() {
    std::vector<MemRegion> regions;
    get_rw_regions(regions);

    static uint8_t buf[65536];
    for (const auto& r : regions) {
        for (uint64_t a = r.start; a + 4 <= r.end; a += sizeof(buf)) {
            size_t len = (size_t)((r.end - a) < sizeof(buf) ? (r.end - a) : sizeof(buf));
            if (!read_chunk(a, buf, len)) continue;
            for (size_t o = 0; o + 4 <= len; o += 4) {
                uint32_t v;
                memcpy(&v, buf + o, 4);
                if (v == TEX_SEARCH) {
                    uint64_t addr = a + o;
                    if (wpm<uint32_t>(addr, TEX_PATCH))
                        bk.push_back({ addr, v });
                }
            }
        }
    }
}

static void restore_all() {
    for (const auto& e : bk)
        wpm<uint32_t>(e.addr, e.orig);
    bk.clear();
}

// вызывается из главного цикла; тяжёлая работа — в потоке
static void tick() {
    bool opt = cfg::world::potato;
    if (opt == last_opt) return;          // реагируем только на переключение
    if (busy.exchange(true)) { return; }  // уже идёт скан/восстановление

    bool enable = opt;
    std::thread([enable]() {
        if (enable) {
            bk.clear();
            scan_and_patch();
            applied = !bk.empty();
        } else {
            restore_all();
            applied = false;
        }
        last_opt = enable;
        busy = false;
    }).detach();
}

} // namespace potato

// ═════════════════════════════════════════════════════════════════════════════
void run() {
    uint64_t pm = get_player_manager();
    if (pm) {
        uint64_t lp = rpm<uint64_t>(pm + oxorany(0x70));
        if (lp) {
            fov::tick(lp);
            bighead::tick(pm, lp);
        } else {
            fov::reset();
            bighead::reset();
        }
    }
    potato::tick();
}

} // namespace world
