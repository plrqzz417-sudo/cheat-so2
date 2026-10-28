#pragma once
#include "imgui.h"
#include <math.h>

namespace cfg {

// ── RGB helpers ────────────────────────────────────────────────────────────
namespace rgb {
    inline float speed = 1.2f;

    inline ImU32 color32(int alpha = 255, float offset = 0.f) {
        float t = (float)ImGui::GetTime() * speed + offset;
        return IM_COL32(
            (int)((sinf(t)          * 0.5f + 0.5f) * 255),
            (int)((sinf(t + 2.094f) * 0.5f + 0.5f) * 255),
            (int)((sinf(t + 4.189f) * 0.5f + 0.5f) * 255),
            alpha
        );
    }
}

// ── ESP ───────────────────────────────────────────────────────────────────
namespace esp {
    inline bool   box          = false;
    inline int    box_type     = 0;
    inline float  box_rounding = 0.f;
    inline ImVec4 box_col      = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline bool   box_rgb      = false;

    inline bool   box3d           = false;
    inline ImVec4 box3d_col       = ImVec4(1.f, 0.85f, 0.1f, 1.f);
    inline bool   box3d_rgb       = false;
    inline float  box3d_thickness = 1.5f;

    inline bool   health      = false;
    inline ImVec4 health_col  = ImVec4(0.1f, 1.f, 0.3f, 1.f);
    inline bool   health_rgb  = false;
    inline bool   health_text = false;

    inline bool   line     = false;
    inline ImVec4 line_col = ImVec4(1.f, 0.2f, 0.2f, 1.f);
    inline bool   line_rgb = false;

    inline bool   name         = false;
    inline bool   distance     = false;
    inline bool   box_fill     = false;
    inline float  fill_alpha   = 0.15f;
    inline ImVec4 name_col     = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline bool   name_rgb     = false;
    inline ImVec4 distance_col = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline bool   distance_rgb = false;
    inline bool   head_circle      = false;
    inline ImVec4 head_circle_col  = ImVec4(1.f, 0.f, 0.f, 1.f);
    inline bool   head_circle_rgb  = false;
    inline bool   skeleton           = false;
    inline ImVec4 skeleton_col       = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline bool   skeleton_rgb       = false;
    inline float  skeleton_thickness = 1.2f;
    inline ImVec4 joint_col          = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline float  joint_size         = 2.5f;
    inline bool   hitlogs         = false;
    inline ImVec4 hitlog_col      = ImVec4(1.f, 0.2f, 0.1f, 1.f);
    inline float  hitlog_bg_alpha = 0.25f;
    inline bool   footprints      = false;
    inline ImVec4 footprints_col  = ImVec4(1.f, 1.f, 1.f, 1.f);
    inline float  footprints_time = 3.0f;
}

namespace crosshair {
    inline bool   enabled = false;
    inline int    type    = 1;
    inline float  size    = 8.0f;
    inline float  thick   = 1.8f;
    inline float  gap     = 2.0f;
    inline bool   outline = true;
    inline bool   rgb     = false;
    inline ImVec4 color   = ImVec4(1.f, 1.f, 1.f, 1.f);
}

// ── Aimbot (legacy) ───────────────────────────────────────────────────────
namespace aimbot {
    inline bool  enabled = false;
    inline float fov     = 5.f;
    inline float smooth  = 3.f;
    inline int   bone    = 0;
}

// ── Aim ───────────────────────────────────────────────────────────────────
namespace aim {
    inline bool   enabled       = false;
    inline float  fov           = 70.f;
    inline float  fov_size      = 70.f;
    inline float  smooth        = 0.25f;
    inline int    bone          = 0;
    inline bool   visible_check = false;
    inline bool   show_fov      = true;
    inline bool   fov_display   = true;
    inline ImVec4 fov_col       = ImVec4(1.f, 1.f, 1.f, 0.5f);
    inline ImVec4 fov_color     = ImVec4(1.f, 1.f, 1.f, 0.5f);
    inline bool   fov_rgb       = false;
    inline float  fov_rgb_speed = 1.5f;
    inline float  fov_glow      = 0.f;
    inline bool   lock_line     = true;
    inline bool   lock_dot      = true;
    inline int    target        = 0;
    inline float  max_distance  = 250.f;

    inline bool  triggerbot           = false;
    inline float trigger_delay        = 0.05f;
    inline float trigger_range        = 30.f;
    inline bool  trigger_visible_only = true;

    namespace silent {
        inline bool  enabled       = false;
        inline float fov           = 15.f;
        inline bool  fov_display   = true;
        inline bool  visible_check = false;
        inline bool  smoke_check   = false;
        inline bool  jump_check    = false;
        inline int   target_bone   = 0;
        inline float recoil_mult   = 1.f;
    }
}

// ── Chams ─────────────────────────────────────────────────────────────────
namespace chams {
    inline bool enabled    = false;
    inline bool team_check = true;
}

// ── World ─────────────────────────────────────────────────────────────────
namespace world {
    inline bool   collider_enabled  = false;
    inline int    collider_map_id   = 0;
    inline ImVec4 collider_color    = ImVec4(1.f, 1.f, 0.f, 1.f);
    inline float  collider_alpha    = 0.6f;
    inline float  collider_max_dist = 80.f;

    // world.cpp features
    inline bool  fov         = false;
    inline float fov_value   = 70.f;
    inline bool  bighead     = false;
    inline float bighead_scale = 2.5f;
    inline bool  potato      = false;
}

// ── Weapon ────────────────────────────────────────────────────────────────
namespace wallshot {
    inline bool enabled       = false;
    inline int  value         = 999;
    inline int  restore_value = 1;
}

namespace norecoil {
    inline bool  enabled    = false;
    inline float multiplier = 0.0f;
}

namespace inf_ammo {
    inline bool enabled = false;
    inline int  value   = 9999;
}

namespace inf_shop {
    inline bool enabled = false;
    inline int  value   = 9999;
}

namespace fustknife {
    inline bool enabled = false;
}

namespace sigma {
    inline bool enabled = false;
    inline int  damage  = 999;
}

namespace fire_rate {
    inline bool enabled = false;
}

namespace weapon {
    inline bool fire_rate_enabled = true;
}

// ── Movement ──────────────────────────────────────────────────────────────
namespace movement {
    namespace air_jump {
        inline bool enabled = false;
    }
    namespace strafe {
        inline bool enabled = false;
    }
    namespace bunny_hop {
        inline bool enabled = false;
    }
    namespace crouch_speed {
        inline bool enabled = false;
    }
    namespace fast_walk {
        inline bool enabled = false;
    }
    namespace air_strafe {
        inline bool enabled = false;
    }
    namespace teleport {
        inline bool enabled = false;
        inline bool active  = false;
    }
    namespace high_jump {
        inline bool  enabled = false;
        inline float height  = 15.f;  // 1–50, как в lua
    }
}

// ── Misc ──────────────────────────────────────────────────────────────────
namespace misc {
    inline bool camera_unlock = false;
    inline bool invisible     = false;
    inline bool autowin       = false;
}

namespace arms {
    inline bool  enabled = false;
    inline float pos_x   = 0.f;
    inline float pos_y   = 0.f;
    inline float pos_z   = 0.f;
}

namespace ambient {
    inline bool   enabled   = false;
    inline ImVec4 color     = ImVec4(0.4f, 0.0f, 1.0f, 0.3f);
    inline float  intensity = 0.3f;
    inline bool   pulse     = false;
    inline float  pulse_spd = 1.5f;
}

namespace anti_flash {
    inline bool enabled = false;
}

namespace anti_smoke {
    inline bool enabled = false;
}

namespace anti_molotov {
    inline bool enabled = false;
}

// ── Props ─────────────────────────────────────────────────────────────────
namespace props {
    inline bool set_score       = false;   inline int  score_val      = 99;
    inline bool set_score_all   = false;   inline int  score_all_val  = 0;
    inline bool set_death       = false;   inline int  death_val      = 0;
    inline bool set_death_all   = false;   inline int  death_all_val  = 0;
    inline bool set_kills       = false;   inline int  kills_val      = 99;
    inline bool set_kills_all   = false;   inline int  kills_all_val  = 0;
    inline bool set_assists     = false;   inline int  assists_val    = 0;
    inline bool set_money_all   = false;   inline int  money_all_val  = 9999;
    inline bool set_ping        = false;   inline int  ping_val       = 1;
    inline bool set_ping_all    = false;   inline int  ping_all_val   = 999;
    inline bool set_mvp         = false;
    inline bool kick_players    = false;
    inline bool hide_id         = false;
    inline bool hide_id_all     = false;
    inline bool hide_clan       = false;
    inline bool hide_clan_all   = false;
    inline bool fake_avatar     = false;
    inline bool fake_avatar_all = false;
    inline bool fake_medal      = false;
    inline bool fake_medal_all  = false;
}

} // namespace cfg
