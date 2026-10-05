import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "decomp" / "include" / "comet" / "settings.hpp"
SOURCE = ROOT / "decomp" / "src" / "settings.cpp"
DOC = ROOT / "docs" / "decomp" / "settings-format.md"


class SettingsRecoveryTests(unittest.TestCase):
    def test_exact_blob_shape_is_pinned(self):
        text = HEADER.read_text(encoding="utf-8")
        for token in (
            "kSettingsBlobSize = 3912",
            "kSettingsFormatVersion = 8",
            "kSettingsBankCount = 3",
            "kSettingsLaneCount = 3",
            "kSettingsEntryCount = 100",
        ):
            self.assertIn(token, text)

    def test_exact_layout_formulas_are_preserved(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("12 + bank * 1200 + lane * 400 + index * 4", text)
        self.assertIn("3612 + bank * 100 + index", text)

    def test_native_codec_is_endian_explicit(self):
        text = SOURCE.read_text(encoding="utf-8")
        self.assertIn("read_be_u32", text)
        self.assertIn("write_be_u32", text)
        self.assertIn("bytes[0] != kSettingsFormatVersion", text)

    def test_exact_file_and_savedata_evidence_is_documented(self):
        text = DOC.read_text(encoding="utf-8")
        for token in (
            "%s/settings.dat",
            '"wb"',
            '"rb"',
            "sdu_auto_load",
            "sdu_auto_save",
            "NPEB00142-AUTO",
            "0x000D5C64",
            "0x000D6078",
            "0x001392FC",
            "0x0013953C",
        ):
            self.assertIn(token, text)

    def test_shipped_default_validation_is_metadata_only(self):
        text = DOC.read_text(encoding="utf-8")
        self.assertIn("08 01 3C 5A 00 00 00 00 01 01 00 00", text)
        self.assertIn("No proprietary settings bytes are committed", text)


if __name__ == "__main__":
    unittest.main()
