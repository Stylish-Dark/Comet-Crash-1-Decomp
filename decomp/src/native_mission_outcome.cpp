#include "comet/native_mission_outcome.hpp"
namespace comet::decomp {
NativeMissionOutcome evaluate_native_mission(const ArenaConstructionSimulation& sim,
                                            std::size_t perspective_owner) {
    const auto& players = sim.accounts();
    if (perspective_owner >= players.size() || players[perspective_owner].route.inactive)
        return NativeMissionOutcome::Unconfigured;
    const auto team = players[perspective_owner].route.team;
    bool friendly = false, enemy = false;
    bool friendly_alive = false, enemy_alive = false;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (players[i].route.inactive) continue;
        const auto counter = sim.base_counter(i);
        if (!counter) return NativeMissionOutcome::Unconfigured;
        if (players[i].route.team == team) {
            friendly = true;
            friendly_alive |= *counter > 0;
        } else {
            enemy = true;
            enemy_alive |= *counter > 0;
        }
    }
    // Do not infer victory from a partially configured team/base table.
    if (!friendly || !enemy) return NativeMissionOutcome::Unconfigured;
    if (!friendly_alive && !enemy_alive) return NativeMissionOutcome::Draw;
    if (!friendly_alive) return NativeMissionOutcome::Defeat;
    if (!enemy_alive) return NativeMissionOutcome::Victory;
    return NativeMissionOutcome::InProgress;
}
}
