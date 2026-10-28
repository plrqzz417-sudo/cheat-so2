#pragma once

// ============================================================
//  OFFSETS — Standoff2 arm64-v8a
//  Перенесено из anuswin. Меняй только этот файл.
// ============================================================

// --- PlayerManager static offset ---
#define OFF_PLAYER_MANAGER              180740496

// --- PlayerManager fields ---
#define OFF_PM_LOCAL_PLAYER             0x70
#define OFF_PM_PLAYER_LIST              0x28

// --- PlayerList fields ---
#define OFF_LIST_COUNT                  0x20
#define OFF_LIST_BUFFER                 0x18
#define OFF_LIST_ENTRY_BASE             0x30
#define OFF_LIST_ENTRY_STRIDE           0x18

// --- Player fields ---
#define OFF_PLAYER_TEAM                 0x79
#define OFF_PLAYER_MOVEMENT_CTRL        0x98
#define OFF_PLAYER_PHOTON_PTR           0x160
#define OFF_PLAYER_MAIN_CAMERA          0xE8
#define OFF_PLAYER_WEAPON_ROOT          0x88
#define OFF_PLAYER_AIM_ANIM_CTRL        0xA0
#define OFF_AAC_VIEWMODEL_OFFSET        0xE8
#define OFF_PLAYER_AIM_CONTROLLER       0x80
#define OFF_AIM_CONTROLLER_DATA         0x90

// --- Camera fields ---
#define OFF_CAM_SETTINGS                0x28
#define OFF_CAM_FOV                     0x38

// --- WeaponRootController ---
#define OFF_WRC_ACTIVE_WEAPON           0xA0

// --- WeaponController ---
#define OFF_WC_FIRE_DURATION            0x108
#define OFF_WC_WEAPON_PROPS             0xA8
#define OFF_WC_RECOIL_CTRL              0x160

// --- WeaponProperties ---
#define OFF_WP_WEAPON_ID                0x18

// --- MovementController sub-controllers ---
#define OFF_MC_TRAJECTORY               0xA8
#define OFF_MC_THRUST_DATA              0xB0

// --- TrajectoryPredictor ---
#define OFF_TP_JUMP_PARAMS              0x50
#define OFF_TP_CROUCH_PARAMS            0x48

// --- JumpParams ---
#define OFF_JP_JUMP_SPEED               0x10
#define OFF_JP_JUMP_SPEED2              0x60

// --- ThrustData ---
#define OFF_TD_THRUST_VEC               0x68

// --- Player height ---
#define PLAYER_HEIGHT                   1.67f

// --- Alternative PlayerManager offset (fallback) ---
#define OFF_PLAYER_MANAGER_ALT          132435632ULL

// --- Unity Transform (для antiaim) ---
#define OFF_TRANSFORM_NATIVE            0x10
#define OFF_NATIVE_TRANSFORM_DATA       0x18
#define OFF_NATIVE_TRANSFORM_INDEX      0x20
#define OFF_TRANSFORM_DATA_ARRAY        0x08

// --- Arms controller ---
#define OFF_PLAYER_ARMS_CONTROLLER      0xA0
#define OFF_ARMS_POSITION               0xE8

// ─── C++ namespace offsets (для aimbot) ──────────────────────────────────────
namespace offsets {
    namespace player {
        static constexpr uint64_t team           = OFF_PLAYER_TEAM;
        static constexpr uint64_t aim_controller = OFF_PLAYER_AIM_CONTROLLER;
    }
    namespace aim {
        static constexpr uint64_t aiming_data   = OFF_AIM_CONTROLLER_DATA;
    }
}
