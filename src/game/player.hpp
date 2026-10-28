#pragma once

#include "game.hpp"
#include "../other/vector3.h"
#include "../other/string.h"
#include "../protect/oxorany.hpp"
#include <cstring>
#include <string>

namespace player {
    inline Vector3 position(uint64_t p) noexcept {
        uint64_t MovementController = rpm<uint64_t>(p + oxorany(0x98));
        if (!MovementController) return Vector3(0, 0, 0);

        uint64_t TransformData = rpm<uint64_t>(MovementController + oxorany(0xB0));
        if (!TransformData) return Vector3(0, 0, 0);

        return rpm<Vector3>(TransformData + oxorany(0x44));
    }

    inline uint64_t photon_ptr(uint64_t p) noexcept {
        return rpm<uint64_t>(p + oxorany(0x160));
    }

    template<typename T>
    inline T property(uint64_t p, const char* tag) noexcept {
        T result{};
        uint64_t PhotonPlayer = photon_ptr(p);
        if (!PhotonPlayer) return result;

        uint64_t PropertiesRegistry = rpm<uint64_t>(PhotonPlayer + oxorany(0x38));
        if (!PropertiesRegistry) return result;

        int Count = rpm<int>(PropertiesRegistry + oxorany(0x20));
        uint64_t PropertiesList = rpm<uint64_t>(PropertiesRegistry + oxorany(0x18));

        for (int i = 0; i < Count; i++) {
            uint64_t Key = rpm<uint64_t>(PropertiesList + oxorany(0x28) + oxorany(0x18) * i);
            uint64_t Value = rpm<uint64_t>(PropertiesList + oxorany(0x30) + oxorany(0x18) * i);

            if (!Key) continue;

            std::string KeyString = rpm<read_string>(Key).as_utf8();
            if (strstr(KeyString.c_str(), tag)) {
                result = rpm<T>(Value + oxorany(0x10));
                break;
            }
        }

        return result;
    }

    inline int health(uint64_t p) noexcept {
        return property<int>(p, oxorany("health"));
    }

    inline read_string name(uint64_t p) noexcept {
        uint64_t PhotonPlayer = photon_ptr(p);
        if (!PhotonPlayer) return {};
        return rpm<read_string>(rpm<uint64_t>(PhotonPlayer + oxorany(0x20)));
    }

    inline matrix view_matrix(uint64_t p) noexcept {
        uint64_t PlayerMainCamera = rpm<uint64_t>(p + oxorany(0xE8));
        if (!PlayerMainCamera) return {};

        uint64_t CameraTransform = rpm<uint64_t>(PlayerMainCamera + oxorany(0x20));
        if (!CameraTransform) return {};

        uint64_t CameraMatrix = rpm<uint64_t>(CameraTransform + oxorany(0x10));
        if (!CameraMatrix) return {};

        return rpm<matrix>(CameraMatrix + oxorany(0xF0));
    }

    // ── Bones ─────────────────────────────────────────────────────────────
    // Индексы костей Standoff2 (arm64)
    struct bones_t {
        Vector3 head;
        Vector3 neck;
        Vector3 spine2;
        Vector3 spine1;
        Vector3 spine;
        Vector3 pelvis;
    };

    static inline bool get_bones(uint64_t p, bones_t& out) noexcept {
        // Animator -> bones array путь
        uint64_t animator = rpm<uint64_t>(p + oxorany(0xC0));
        if (!animator) return false;
        uint64_t avatar = rpm<uint64_t>(animator + oxorany(0x18));
        if (!avatar) return false;
        uint64_t bones_arr = rpm<uint64_t>(avatar + oxorany(0x28));
        if (!bones_arr) return false;
        int cnt = rpm<int>(bones_arr + oxorany(0x18));
        if (cnt < 7) return false;
        uint64_t buf = rpm<uint64_t>(bones_arr + oxorany(0x10));
        if (!buf) return false;

        // Fallback: если кости нулевые — строим из позиции игрока
        Vector3 base = position(p);
        out.pelvis = Vector3(base.x, base.y + 0.90f, base.z);
        out.spine  = Vector3(base.x, base.y + 1.05f, base.z);
        out.spine1 = Vector3(base.x, base.y + 1.20f, base.z);
        out.spine2 = Vector3(base.x, base.y + 1.35f, base.z);
        out.neck   = Vector3(base.x, base.y + 1.55f, base.z);
        out.head   = Vector3(base.x, base.y + 1.67f, base.z);

        // Пробуем считать реальные позиции трансформов
        // bone indices: 0=pelvis,1=spine,2=spine1,3=spine2,4=neck,5=head (типичный Standoff2)
        struct BoneEntry { uint64_t transform; };
        static const int idx[6] = {0,1,2,3,4,5};
        bool any = false;
        for (int b = 0; b < 6 && b < cnt; b++) {
            uint64_t bone_transform = rpm<uint64_t>(buf + oxorany(0x20) + oxorany(0x8) * idx[b]);
            if (!bone_transform) continue;
            uint64_t native_ptr = rpm<uint64_t>(bone_transform + oxorany(0x10));
            if (!native_ptr) continue;
            uint64_t transform_data = rpm<uint64_t>(native_ptr + oxorany(0x18));
            if (!transform_data) continue;
            uint32_t index = rpm<uint32_t>(native_ptr + oxorany(0x20));
            uint64_t data_arr = rpm<uint64_t>(transform_data + oxorany(0x08));
            if (!data_arr) continue;
            Vector3 bpos = rpm<Vector3>(data_arr + oxorany(0x30) * index + oxorany(0x10));
            if (bpos.x == 0.f && bpos.y == 0.f && bpos.z == 0.f) continue;
            switch(b){
                case 0: out.pelvis = bpos; break;
                case 1: out.spine  = bpos; break;
                case 2: out.spine1 = bpos; break;
                case 3: out.spine2 = bpos; break;
                case 4: out.neck   = bpos; break;
                case 5: out.head   = bpos; break;
            }
            any = true;
        }
        (void)any;
        return true;
    }

    static inline bool is_visible(uint64_t p) noexcept {
        // Простая проверка: читаем флаг видимости из структуры игрока
        // 0xC4 — типичный visible flag в Standoff2
        uint8_t vis = rpm<uint8_t>(p + oxorany(0xC4));
        return vis != 0;
    }

    static inline Vector3 camera_position(uint64_t p) noexcept {
        uint64_t PlayerMainCamera = rpm<uint64_t>(p + oxorany(0xE8));
        if (!PlayerMainCamera) return position(p);
        uint64_t CameraTransform = rpm<uint64_t>(PlayerMainCamera + oxorany(0x20));
        if (!CameraTransform) return position(p);
        uint64_t native_ptr = rpm<uint64_t>(CameraTransform + oxorany(0x10));
        if (!native_ptr) return position(p);
        uint64_t transform_data = rpm<uint64_t>(native_ptr + oxorany(0x18));
        if (!transform_data) return position(p);
        uint32_t index = rpm<uint32_t>(native_ptr + oxorany(0x20));
        uint64_t data_arr = rpm<uint64_t>(transform_data + oxorany(0x08));
        if (!data_arr) return position(p);
        return rpm<Vector3>(data_arr + oxorany(0x30) * index + oxorany(0x10));
    }


    // Unity transform position reader (используется в aimbot для bipedMap костей)
    static inline Vector3 get_transform_position(uint64_t transform) noexcept {
        if (!transform) return Vector3(0, 0, 0);
        uint64_t native_ptr = rpm<uint64_t>(transform + oxorany(0x10));
        if (!native_ptr) return Vector3(0, 0, 0);
        uint64_t transform_data = rpm<uint64_t>(native_ptr + oxorany(0x18));
        if (!transform_data) return Vector3(0, 0, 0);
        uint32_t index = rpm<uint32_t>(native_ptr + oxorany(0x20));
        uint64_t data_arr = rpm<uint64_t>(transform_data + oxorany(0x08));
        if (!data_arr) return Vector3(0, 0, 0);
        return rpm<Vector3>(data_arr + oxorany(0x30) * index + oxorany(0x10));
    }


    // ── Утилиты для world.cpp ─────────────────────────────────────────────
    static inline bool likely_ptr(uint64_t p) noexcept {
        return p > 0x10000ull && p < 0x0000FFFFFFFFFFFFull;
    }

    // biped_map — указатель на BipedMap объекта игрока (для BigHead и т.п.)
    // Типичная цепочка: player + 0xC0 -> Animator -> avatar -> bipedMap
    static inline uint64_t biped_map(uint64_t p) noexcept {
        uint64_t animator = rpm<uint64_t>(p + oxorany(0xC0));
        if (!likely_ptr(animator)) return 0;
        uint64_t avatar = rpm<uint64_t>(animator + oxorany(0x18));
        if (!likely_ptr(avatar)) return 0;
        return rpm<uint64_t>(avatar + oxorany(0x20));
    }

} // namespace player
