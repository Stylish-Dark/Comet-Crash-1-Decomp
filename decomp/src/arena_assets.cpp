#include "comet/arena_assets.hpp"

#include <array>

namespace comet::decomp {
namespace {

constexpr std::array<ArenaModelAssetSpec, 37> kArenaModels{{
    // path, root offset, slot, options, param1, param2
    {"models/care/playerShip.obj",                         0x2D2DC0,  0, 0x003, 1.30f,  0.85f},
    {"models/care/playerShipDonut.obj",                    0x2D2E50,  1, 0x003, 1.30f,  0.00f},
    {"models/care/resourceGeode/resourceGeode.obj",        0x2D4200, 36, 0x245, 3.3333001f, 1.00f},
    {"models/sun.obj",                                     0x2D43B0, 39, 0x000, 1.00f,  0.00f},
    {"models/care/stunner02.obj",                          0x2D3120,  6, 0x022, 1.20f,  0.50f},
    {"models/care/converter02.obj",                        0x2D31B0,  7, 0x022, 1.20f,  0.75f},
    {"models/care/agents/agentAdv_Medic.obj",              0x2D3240,  8, 0x063, 1.20f,  1.00f},
    {"models/care/agents/agentAdv_Carrier.obj",            0x2D32D0,  9, 0x063, 1.00f,  1.00f},
    {"models/care/agents/agentAdv_Laser.obj",              0x2D3360, 10, 0x063, 1.00f,  0.80f},
    {"models/care/scout02.obj",                            0x2D2EE0,  2, 0x063, 1.12f,  0.88f},
    {"models/care/fighter02.obj",                          0x2D2F70,  3, 0x063, 1.20f,  0.92f},
    {"models/care/agents/flyer.obj",                       0x2D3000,  4, 0x043, 1.30f,  0.85f},
    {"models/care/agents/beetle.obj",                      0x2D3090,  5, 0x063, 1.20f,  0.45f},

    {"models/care/platforms/platBasic.obj",                 0x2D3D80, 28, 0x002, 1.00f, -0.90f},
    {"models/care/platforms/platBasic.obj",                 0x2D3E10, 29, 0x002, 1.00f, -0.90f},
    {"models/care/platforms/platBasic.obj",                 0x2D3EA0, 30, 0x002, 1.00f, -0.90f},
    {"models/care/platforms/platBasic.obj",                 0x2D3F30, 31, 0x002, 1.00f, -0.90f},
    {"models/care/platforms/platAdv.obj",                   0x2D3FC0, 32, 0x002, 1.00f, -0.89f},

    {"models/care/weapons/structTurretStand.obj",           0x2D33F0, 11, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structTurretBarrelClasp.obj",     0x2D3480, 12, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structTurretBarrel.obj",          0x2D3510, 13, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structLaserStand.obj",            0x2D35A0, 14, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structLaserBarrel.obj",           0x2D3630, 15, 0x043, 1.00f,  0.00f},
    {"models/structTesla.obj",                              0x2D36C0, 16, 0x052, 1.00f,  0.00f},
    {"models/care/weapons/structMissileStand.obj",          0x2D37E0, 18, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structMissilePivot.obj",          0x2D3870, 19, 0x043, 1.00f,  0.00f},
    {"models/care/weapons/structMissileBarrels.obj",        0x2D3900, 20, 0x043, 1.00f,  0.00f},
    {"models/care/structures/structBeaconLamp.obj",         0x2D3750, 17, 0x043, 1.00f,  0.00f},
    {"models/structBaseLight.obj",                          0x2D3990, 21, 0x010, 1.00f,  0.00f},
    {"models/structBaseShell.obj",                          0x2D3A20, 22, 0x010, 1.00f,  0.00f},
    {"models/care/weapons/structBasePlat.obj",              0x2D3AB0, 23, 0x043, 1.00f, -1.00f},
    {"models/care/structures/structBarracksBasic.obj",      0x2D3B40, 24, 0x043, 1.00f,  0.00f},
    {"models/care/structures/structBarracksAdv.obj",        0x2D3BD0, 25, 0x043, 1.00f,  0.00f},
    {"models/mine.obj",                                    0x2D3C60, 26, 0x010, 1.00f,  1.30f},
    {"models/bullet.obj",                                  0x2D3CF0, 27, 0x010, 1.00f,  0.00f},
    {"models/playerSquare2.obj",                           0x2D4050, 33, 0x010, 1.00f,  0.00f},
    {"models/gateway.obj",                                 0x2D4170, 35, 0x010, 1.00f,  0.00f},
}};

constexpr bool provenance_offsets_match_slots() {
    for (const auto& asset : kArenaModels) {
        if (asset.original_root_offset !=
            kArenaModelTableBaseOffset +
                asset.original_slot_index * kArenaModelObjectStride) {
            return false;
        }
    }
    return true;
}

static_assert(provenance_offsets_match_slots());

}  // namespace

std::span<const ArenaModelAssetSpec> arena_model_asset_manifest() {
    return kArenaModels;
}

}  // namespace comet::decomp
