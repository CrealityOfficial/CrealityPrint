"""Integration tests against the production C++ engine and native preset parser.
Usage: python tests/data_migration/test_migration.py PATH_TO_DATA_DIRECTORY_MIGRATE
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve()
os.environ["PATH"] = str(EXE.parent.parent) + os.pathsep + os.environ["PATH"]

def put(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")

def hashes(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob("*") if p.is_file()}

class Migration(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="data-migration-")
        self.root = Path(self.tmp.name)
        self.target = self.root / "7.3"
        self.resources = self.root / "resources"
        put(self.resources / "printers" / "printer.json", {})
        put(self.resources / "profiles" / "Creality.json", {
            "machine_list": [{"name": "Printer", "sub_path": "machine/Printer.json"}],
            "filament_list": [], "process_list": []})
        put(self.resources / "profiles" / "Creality" / "machine" / "Printer.json",
            {"name": "Printer", "type": "machine", "inherits": ""})
    def tearDown(self):
        self.tmp.cleanup()
    def source(self, directory="7.0", version="7.2.2.5483", name="User printer"):
        root = self.root / directory
        put(root / "Creality.conf", {"app": {"version": version}, "models": []})
        put(root / "user/default/machine" / (name + ".json"),
            {"name": name, "inherits": "Printer", "from": "User", "version": "1.0.0", "printable_height": "250"})
        (root / "user/default/machine" / (name + ".info")).write_bytes(b"setting_id = 123\nsync_info = update\n")
        return root
    def run_tool(self, ok=True):
        p = subprocess.run([str(EXE), str(self.target), str(self.resources), "7.3.0"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        self.assertEqual(p.returncode == 0, ok, p.stdout[-2000:] + p.stderr[-2000:])
        return p
    def report(self):
        return json.loads((self.target / "migration/report.json").read_text(encoding="utf-8"))
    def test_722_preserves_bytes_and_sync_metadata(self):
        source = self.source(name="用户预设")
        before = hashes(source)
        self.run_tool()
        self.assertEqual(hashes(source), before)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
        self.assertEqual(self.report()["source_application_version"], "7.2.2.5483")
        self.assertEqual(self.report()["valid_presets"], 1)
        self.assertFalse((self.target / "migration/cloud-sync-hold.json").exists())
    def test_60_direct(self):
        self.source("6.0", "6.3.0.1000")
        self.run_tool()
        self.assertFalse((self.root / "7.0").exists())
        self.assertEqual(self.report()["source_application_version"], "6.3.0.1000")
    def test_73_release_written_into_70_directory(self):
        source = self.source(version="7.3.0.6069")
        before = hashes(source)
        self.run_tool()
        self.assertEqual(self.report()["source_application_version"], "7.3.0.6069")
        self.assertEqual(self.report()["valid_presets"], 1)
        self.assertEqual(self.report()["skipped_sources"], [])
        self.assertEqual(hashes(source), before)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_60_internal_version_number(self):
        source = self.source("6.0", "01.09.03.50")
        before = hashes(source)
        self.run_tool()
        self.assertEqual(self.report()["source_application_version"], "01.09.03.50")
        self.assertEqual(self.report()["valid_presets"], 1)
        self.assertEqual(hashes(source), before)
    def test_application_version_does_not_select_data_epoch(self):
        self.source("6.0", "01.09.03.50", "Six")
        self.source(version="7.4.0.1000", name="Seven")
        self.run_tool()
        self.assertTrue((self.target / "user/default/machine/Seven.json").exists())
        self.assertFalse((self.target / "user/default/machine/Six.json").exists())
    def test_invalid_sources_do_not_commit_empty_target_and_can_retry(self):
        source = self.source()
        saved = (source / "Creality.conf").read_bytes()
        (source / "Creality.conf").write_bytes(b"broken-secret-do-not-log")
        six = self.source("6.0", "01.09.03.50")
        put(six / "Creality.conf", {"app": []})
        self.run_tool(False)
        self.assertFalse((self.target / "migration/manifest.json").exists())
        self.assertFalse((self.target / "Creality.conf").exists())
        diagnostics = self.root / ".migration-7.3/report.json"
        report = json.loads(diagnostics.read_text(encoding="utf-8"))
        self.assertEqual(report["state"], "SOURCE_UNAVAILABLE")
        self.assertEqual(len(report["skipped_sources"]), 2)
        self.assertEqual(report["skipped_sources"][0]["attempts"][0]["reason"], "Invalid configuration JSON")
        self.assertEqual(report["skipped_sources"][0]["attempts"][1]["reason"], "Configuration file is missing")
        self.assertIn("app object", report["skipped_sources"][1]["attempts"][0]["reason"])
        self.assertNotIn("broken-secret", diagnostics.read_text(encoding="utf-8"))
        (source / "Creality.conf").write_bytes(saved)
        self.run_tool()
        self.assertEqual(self.report()["valid_presets"], 1)
    def test_existing_target_without_manifest_is_not_overwritten(self):
        self.source(version="7.3.0.6069")
        put(self.target / "Creality.conf", {"app": {"version": "7.3.0.5000"}})
        put(self.target / "user/default/machine/New.json", {"name": "New"})
        before = hashes(self.target)
        self.run_tool()
        self.assertEqual(hashes(self.target), before)
    def test_priority_and_no_merge(self):
        self.source("6.0", "6.3.0", "Six")
        self.source(name="Seven")
        self.run_tool()
        self.assertTrue((self.target / "user/default/machine/Seven.json").exists())
        self.assertFalse((self.target / "user/default/machine/Six.json").exists())
    def test_corrupt_core_falls_back(self):
        seven = self.source()
        (seven / "Creality.conf").write_bytes(b"broken")
        self.source("6.0", "6.0.0")
        self.run_tool()
        self.assertEqual(self.report()["source_application_version"], "6.0.0")
    def test_backup_recovery_does_not_modify_source(self):
        source = self.source()
        (source / "Creality.conf.bak").write_bytes((source / "Creality.conf").read_bytes())
        (source / "Creality.conf").write_bytes(b"broken")
        before = hashes(source)
        self.run_tool()
        self.assertEqual(hashes(source), before)
        self.assertEqual((self.target / "Creality.conf").read_bytes(), (source / "Creality.conf.bak").read_bytes())
    def test_broken_preset_isolated_without_rewriting(self):
        source = self.source()
        file = source / "user/default/machine/User printer.json"
        payload = json.loads(file.read_text(encoding="utf-8")); payload["inherits"] = "Missing parent"; put(file, payload)
        self.run_tool()
        isolated = self.target / "migration/quarantine/user/default/machine/User printer.json"
        self.assertEqual(file.read_bytes(), isolated.read_bytes())
        self.assertEqual(self.report()["quarantined_presets"], 1)
        self.assertTrue(isolated.with_suffix(".info").exists())
    def test_bad_parameter_isolated(self):
        source = self.source()
        file = source / "user/default/machine/User printer.json"
        payload = json.loads(file.read_text(encoding="utf-8")); payload["printable_height"] = "invalid"; put(file, payload)
        self.run_tool()
        self.assertEqual(self.report()["quarantined_presets"], 1)
    def test_nested_user_root(self):
        source = self.source()
        put(source / "user/default/filament/base/Custom.json", {"name": "Custom", "inherits": "", "from": "User", "version": "1.0.0", "filament_type": ["PLA"]})
        self.run_tool()
        self.assertEqual(self.report()["valid_presets"], 2)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_idempotent_and_existing_target_protected(self):
        self.source(); self.run_tool()
        # First restart acknowledges the completed directory rename.
        self.run_tool(); before = hashes(self.target)
        self.source(name="Late preset")
        self.run_tool(); self.assertEqual(before, hashes(self.target))
    def test_runtime_only_target(self):
        self.source(); (self.target / "log").mkdir(parents=True)
        (self.target / "log/startup.log").write_text("test")
        self.run_tool(); self.assertEqual(self.report()["valid_presets"], 1)
    def test_empty_runtime_backup_removed_and_lock_retained(self):
        self.source(); self.target.mkdir()
        self.run_tool()
        self.assertEqual(list(self.root.glob(".7.3.runtime-backup-*")), [])
        self.assertTrue((self.root / ".migration-7.3.lock").is_file())
    def test_nonempty_runtime_backup_retained(self):
        self.source(); (self.target / "log").mkdir(parents=True)
        (self.target / "log/startup.log").write_bytes(b"keep")
        self.run_tool(); self.run_tool()
        backups = list(self.root.glob(".7.3.runtime-backup-*"))
        self.assertEqual(len(backups), 1)
        self.assertEqual((backups[0] / "log/startup.log").read_bytes(), b"keep")
        self.assertTrue((self.root / ".migration-7.3.lock").is_file())
    def test_completed_and_recovered_migration_cleanup(self):
        self.source(); self.run_tool()
        path = self.target / "migration/manifest.json"
        manifest = json.loads(path.read_text(encoding="utf-8"))
        backup = self.root / (".7.3.runtime-backup-" + manifest["transaction"])
        unrelated = self.root / ".7.3.runtime-backup-999"
        unrelated.mkdir()
        for state in ["COMMITTED", "READY_TO_COMMIT"]:
            with self.subTest(state=state):
                backup.mkdir(); manifest["state"] = state; put(path, manifest)
                self.run_tool()
                self.assertFalse(backup.exists())
                self.assertTrue(unrelated.is_dir())
                self.assertTrue((self.root / ".migration-7.3.lock").is_file())
                self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["state"], "COMMITTED")
    def test_invalid_transaction_does_not_cleanup(self):
        self.source(); self.run_tool()
        path = self.target / "migration/manifest.json"
        manifest = json.loads(path.read_text(encoding="utf-8"))
        backup = self.root / (".7.3.runtime-backup-" + manifest["transaction"])
        backup.mkdir(); manifest["transaction"] = "../outside"; put(path, manifest)
        self.run_tool()
        self.assertTrue(backup.is_dir())
        self.assertTrue((self.root / ".migration-7.3.lock").is_file())
    def test_unknown_target_not_overwritten(self):
        self.source(); self.target.mkdir(); (self.target / "keep.txt").write_text("keep")
        self.run_tool(False); self.assertEqual((self.target / "keep.txt").read_text(encoding="utf-8"), "keep")
    def test_missing_resources_can_retry(self):
        self.source()
        manifest = self.resources / "profiles/Creality.json"
        saved = manifest.read_bytes(); manifest.unlink()
        self.run_tool(False); self.assertFalse((self.target / "Creality.conf").exists())
        manifest.write_bytes(saved); self.run_tool()
        self.assertEqual(self.report()["valid_presets"], 1)
    def test_fresh_initialization(self):
        self.run_tool(); self.assertEqual(self.report()["valid_presets"], 0)
        self.assertFalse((self.target / "migration/cloud-sync-hold.json").exists())
    def test_inheritance_cycle(self):
        source = self.source()
        for name, parent in [("A", "B"), ("B", "A")]:
            put(source / "user/default/machine" / (name + ".json"), {"name": name, "inherits": parent, "from": "User", "version": "1.0.0"})
        self.run_tool(); self.assertEqual(self.report()["quarantined_presets"], 2)

if __name__ == "__main__":
    unittest.main(verbosity=2)
