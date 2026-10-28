// ─────────────────────────────────────────────────────────────────────────────
//  PROPS HACKS — порт из NexuraWare (props.cpp) под SDK neverlose2.
//  Через player::photon_ptr → PropertiesRegistry пишем значения photon-свойств
//  (score, kills, deaths, money, ping, mvp, team, uid, clan_tag, avatar, medal).
// ─────────────────────────────────────────────────────────────────────────────

#include "props.hpp"
#include "../game/player.hpp"
#include "../game/game.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <stdint.h>
#include <cstring>
#include <string>

namespace {
    static inline bool valid_addr(uint64_t a) {
        return (a >= 0x10000ull && a <= 0x7FFFFFFFFFFFull);
    }

    static constexpr uint64_t kOffPhotonPropsReg = 0x38;
    static constexpr uint64_t kOffPropsCount     = 0x20;
    static constexpr uint64_t kOffPropsList      = 0x18;
    static constexpr uint64_t kOffPropsKeyBase   = 0x28;
    static constexpr uint64_t kOffPropsValBase   = 0x30;
    static constexpr uint64_t kOffPropsValueData = 0x10;
    static constexpr uint64_t kOffListStride     = 0x18;

    static uint64_t get_prop_value_ptr(uint64_t player_ptr, const char* tag) {
        uint64_t photon_player = player::photon_ptr(player_ptr);
        if (!valid_addr(photon_player)) return 0;

        uint64_t props_reg = rpm<uint64_t>(photon_player + oxorany(kOffPhotonPropsReg));
        if (!valid_addr(props_reg)) return 0;

        int count = rpm<int>(props_reg + oxorany(kOffPropsCount));
        if (count <= 0 || count > 128) return 0;

        uint64_t props_list = rpm<uint64_t>(props_reg + oxorany(kOffPropsList));
        if (!valid_addr(props_list)) return 0;

        for (int i = 0; i < count; ++i) {
            uint64_t key = rpm<uint64_t>(props_list + oxorany(kOffPropsKeyBase) +
                                         oxorany(kOffListStride) * i);
            uint64_t val = rpm<uint64_t>(props_list + oxorany(kOffPropsValBase) +
                                         oxorany(kOffListStride) * i);
            if (!valid_addr(key) || !valid_addr(val)) continue;

            std::string key_string = rpm<read_string>(key).as_utf8();
            if (key_string.empty()) continue;

            if (std::strstr(key_string.c_str(), tag)) {
                return val + oxorany(kOffPropsValueData);
            }
        }
        return 0;
    }

    template<typename T>
    static bool set_prop(uint64_t player_ptr, const char* tag, const T& value) {
        uint64_t value_ptr = get_prop_value_ptr(player_ptr, tag);
        if (!valid_addr(value_ptr)) return false;
        return wpm<T>(value_ptr, value);
    }

    static uint64_t get_player_at(uint64_t pm, int i) {
        if (!valid_addr(pm)) return 0;
        uint64_t player_list = rpm<uint64_t>(pm + oxorany(0x28));
        if (!valid_addr(player_list)) return 0;
        int count = rpm<int>(player_list + oxorany(0x20));
        if (i < 0 || i >= count || count > 64) return 0;
        uint64_t list_buffer = rpm<uint64_t>(player_list + oxorany(0x18));
        if (!valid_addr(list_buffer)) return 0;
        return rpm<uint64_t>(list_buffer + oxorany(0x30) + oxorany(0x18) * i);
    }

    static int get_player_count(uint64_t pm) {
        if (!valid_addr(pm)) return 0;
        uint64_t player_list = rpm<uint64_t>(pm + oxorany(0x28));
        if (!valid_addr(player_list)) return 0;
        int count = rpm<int>(player_list + oxorany(0x20));
        return (count > 0 && count <= 64) ? count : 0;
    }

    template<typename T>
    static void set_all_players(uint64_t pm, const char* tag, const T& value) {
        int count = get_player_count(pm);
        for (int i = 0; i < count; ++i) {
            uint64_t p = get_player_at(pm, i);
            if (!valid_addr(p)) continue;
            set_prop<T>(p, tag, value);
        }
    }
}

void props_ns::tick(uint64_t pm, uint64_t lp) {
    if (!valid_addr(lp)) return;

    static int s_tick = 0;
    if (++s_tick < 5) return;
    s_tick = 0;

    if (cfg::props::set_score)       set_prop<int>(lp, oxorany("score"), cfg::props::score_val);
    if (cfg::props::set_score_all)   set_all_players<int>(pm, oxorany("score"), cfg::props::score_all_val);
    if (cfg::props::set_death)       set_prop<int>(lp, oxorany("death"), cfg::props::death_val);
    if (cfg::props::set_death_all)   set_all_players<int>(pm, oxorany("death"), cfg::props::death_all_val);
    if (cfg::props::set_kills)       set_prop<int>(lp, oxorany("kills"), cfg::props::kills_val);
    if (cfg::props::set_kills_all)   set_all_players<int>(pm, oxorany("kills"), cfg::props::kills_all_val);
    if (cfg::props::set_assists)     set_prop<int>(lp, oxorany("assists"), cfg::props::assists_val);
    if (cfg::props::set_money_all)   set_all_players<int>(pm, oxorany("money"), cfg::props::money_all_val);
    if (cfg::props::set_ping)        set_prop<int>(lp, oxorany("ping"), cfg::props::ping_val);
    if (cfg::props::set_ping_all)    set_all_players<int>(pm, oxorany("ping"), cfg::props::ping_all_val);
    if (cfg::props::set_mvp)         set_prop<int>(lp, oxorany("mvp"), 1);

    if (cfg::props::kick_players) {
        int count = get_player_count(pm);
        for (int i = 0; i < count; ++i) {
            uint64_t p = get_player_at(pm, i);
            if (!valid_addr(p) || p == lp) continue;
            set_prop<int>(p, oxorany("team"), 0);
            set_prop<int>(p, oxorany("match_team"), 0);
        }
    }

    if (cfg::props::hide_id)         set_prop<int>(lp, oxorany("uid"), 0);
    if (cfg::props::hide_id_all)     set_all_players<int>(pm, oxorany("uid"), 0);
    if (cfg::props::hide_clan)       set_prop<int>(lp, oxorany("clan_tag"), 0);
    if (cfg::props::hide_clan_all)   set_all_players<int>(pm, oxorany("clan_tag"), 0);
    if (cfg::props::fake_avatar)     set_prop<int>(lp, oxorany("avatar"), 0);
    if (cfg::props::fake_avatar_all) set_all_players<int>(pm, oxorany("avatar"), 0);
    if (cfg::props::fake_medal)      set_prop<int>(lp, oxorany("medal"), 0);
    if (cfg::props::fake_medal_all)  set_all_players<int>(pm, oxorany("medal"), 0);
}
