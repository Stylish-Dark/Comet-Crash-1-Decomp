#include "comet/native_session.hpp"
#include <utility>
#include "comet/native_construction_scene.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace comet::decomp {
namespace {
bool valid_dimensions(const NativeSessionConfig& config) {
    return config.display_width && config.display_height &&
           config.auxiliary_width && config.auxiliary_height;
}
}
NativeSessionResult initialize_native_session(
    const NativeSessionConfig& config, std::uint32_t level_id,
    NativeSession& session) {
    if (!valid_dimensions(config))
        return {NativeSessionError::InvalidDimensions, "zero render dimension"};
    LevelMapData new_level{};
    const auto path = level_map_path(config.data_root, level_id);
    if (!load_level_map_file(path, false, new_level))
        return {NativeSessionError::MissingOrInvalidMap,
                "could not parse " + path.string()};
    NativeSession next{};
    next.config = config;
    next.current_level_id = level_id;
    next.level = std::move(new_level);
    auto arena_config=config.arena;
    arena_config.level_id=level_id;
    const auto arena = initialize_arena_state(next.level, arena_config, next.arena);
    if (!arena) return {NativeSessionError::InvalidArenaData, arena.detail};
    next.targets = recover_arena_render_target_plan(
        config.display_width, config.display_height,
        config.auxiliary_width, config.auxiliary_height,
        config.renderer_mode);
    const auto assets = arena_model_asset_manifest();
    next.model_assets.assign(assets.begin(), assets.end());
    session = std::move(next);
    return {};
}
NativeSessionResult initialize_native_skirmish(
    const NativeSessionConfig& config,
    const ArenaConstructionSimulationConfig& construction,
    NativeSession& session) {
    if(!valid_dimensions(config))
        return {NativeSessionError::InvalidDimensions,"zero render dimension"};
    if(!construction.native_extent || construction.native_extent>24 ||
       construction.players.size()<2 || construction.players.size()>4 ||
       !construction.initial_structures.empty() || !construction.initial_players.empty() ||
       std::any_of(construction.initial_visibility.begin(),construction.initial_visibility.end(),[](auto v){return v!=0;}))
        return {NativeSessionError::InvalidArenaData,"unsupported native skirmish configuration"};
    NativeSession next;
    next.skirmish=true;next.config=config;
    next.arena.extent=construction.native_extent;
    next.level.arena_extent=construction.native_extent;
    next.arena.background_position={12,-.2f,12};
    next.arena.players.resize(construction.players.size());
    unsigned teams=0;
    for(std::size_t owner=0;owner<construction.players.size();++owner) {
        const auto &account=construction.players[owner];
        if(account.route.inactive)continue;
        const auto base=account.route.base;
        if(base.x>=next.arena.extent || base.z>=next.arena.extent || account.route.team>3 ||
           !construction.unit_base_available[owner] || !construction.base_counters[owner] ||
           *construction.base_counters[owner]<=0)
            return {NativeSessionError::InvalidArenaData,"active skirmish players need distinct bases and positive base counters"};
        auto &cell=next.arena.cells[base.z*24+base.x];
        if(cell.entity_index)
            return {NativeSessionError::InvalidArenaData,"overlapping skirmish bases"};
        teams|=1u<<account.route.team;
        next.arena.players[owner].start_grid=base;
        ArenaEntity entity;entity.type=29;entity.model_slot=*initial_model_slot_for_record(29);
        entity.owner=static_cast<unsigned>(owner);entity.grid=base;
        entity.position={base.x+.5f,0,base.z+.5f};
        cell.entity_index=next.arena.entities.size();
        // Same passable-but-occupied base encoding used by original map records.
        cell.visibility=static_cast<std::uint8_t>((owner+1)<<4);
        next.arena.entities.push_back(entity);
    }
    if(!teams || (teams&(teams-1))==0)
        return {NativeSessionError::InvalidArenaData,"skirmish needs two active opposing teams"};
    try {
        auto validated=std::make_unique<NativeConstructionScene>(next.arena,construction);
        next.arena=validated->arena();
    } catch(const std::invalid_argument& e) {
        return {NativeSessionError::InvalidArenaData,e.what()};
    }
    next.targets=recover_arena_render_target_plan(config.display_width,config.display_height,
        config.auxiliary_width,config.auxiliary_height,config.renderer_mode);
    const auto assets=arena_model_asset_manifest();
    next.model_assets.assign(assets.begin(),assets.end());
    session=std::move(next);
    return {};
}
NativeSessionResult change_native_level(
    std::uint32_t level_id, NativeSession& session) {
    if(session.skirmish)return {NativeSessionError::InvalidArenaData,"skirmish has no campaign level transition"};
    return initialize_native_session(session.config, level_id, session);
}
} // namespace comet::decomp
