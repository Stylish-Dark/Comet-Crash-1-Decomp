import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "root_state.hpp"
DOC = ROOT / "docs" / "decomp" / "root-state-map.md"
TOOL = ROOT / "tools" / "decomp_root_state_refs.py"


class RootStateRecoveryTests(unittest.TestCase):
    def test_proven_semantic_offsets_are_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "player_object_table = 0x214920",
            "arena_model_table = 0x2D2DC0",
            "current_level_id = 0x2D451C",
            "provisional_transition_level_id = 0x2D4520",
            "player_count = 0x2D4538",
            "game_mode = 0x2D4560",
            "level_map_state = 0x2D6438",
            "kLegacyArenaModelStride = 0x90",
            "kLegacyArenaModelSlotCount = 40",
            "kLegacyPlayerSlotCount = 4",
        ):
            self.assertIn(token, text)

    def test_player_count_and_registry_are_recovered(self):
        text = HEADER.read_text(encoding="utf-8")
        doc = DOC.read_text(encoding="utf-8")
        self.assertIn("player_count = 0x2D4538", text)
        self.assertIn("player_object_table = 0x214920", text)
        for anchor in ("0x000CA288", "0x000CE52C", "0x000DB9F8", "0x000DC358", "0x000DC738"):
            self.assertIn(anchor, doc)

    def test_game_mode_mapping_is_recovered(self):
        text = HEADER.read_text(encoding="utf-8")
        doc = DOC.read_text(encoding="utf-8")
        for token in (
            "Campaign = 0",
            "Training = 1",
            "Battle = 2",
            "Unknown3 = 3",
        ):
            self.assertIn(token, text)
        for anchor in ("0x000D0A60", "0x000E03D8", "0x000E1408", "0x000E1B94"):
            self.assertIn(anchor, doc)

    def test_renderer_handle_regions_are_explicit(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in ("0x2D445C", "0x2D4488", "0x2D448C", "0x2D44B8"):
            self.assertIn(token, text)

    def test_dense_unknown_fields_are_recorded_without_fake_names(self):
        text = DOC.read_text(encoding="utf-8")
        for token in (
            "`+0x2D4538`",
            "`+0x2D4560`",
            "`+0x2D4580`",
            "`+0x2D4584`",
            "`+0x2D4594`",
            "`+0x2D459C`",
        ):
            self.assertIn(token, text)
        self.assertIn("deliberately remain unnamed", text)

    def test_access_miner_is_part_of_the_recovery_contract(self):
        text = TOOL.read_text(encoding="utf-8")
        self.assertIn("recover_opd_functions", text)
        self.assertIn("ROOT_HI16 = 0x2D", text)
        self.assertIn("scan_elf", text)


if __name__ == "__main__":
    unittest.main()
