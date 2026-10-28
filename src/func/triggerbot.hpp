#pragma once
#include <stdint.h>

// Triggerbot — порт из NexuraWare (combat.cpp triggerbot_tick).
// Автострельба по врагу, попавшему в адаптивный px-радиус вокруг центра.
namespace triggerbot {
    void run(uint64_t local_player);
}
