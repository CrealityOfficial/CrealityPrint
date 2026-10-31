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
        args = [str(EXE), str(self.target), str(self.resources), "7.3.0"]
        if self.target.name.endswith(" Alpha"):
            args.append("alpha")
        p = subprocess.run(args,
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
        self.assertEqual((self.target / "Creality.conf").read_bytes(), (source / "Creality.conf").read_bytes())
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
        self.run_tool(False)
        self.assertEqual((self.target / "Creality.conf").read_bytes(), (self.root / "7.0/Creality.conf").read_bytes())
        self.assertEqual(hashes(self.root / "7.0/user"), hashes(self.target / "user"))
        manifest.write_bytes(saved); self.run_tool()
        self.assertEqual(self.report()["valid_presets"], 1)
    def test_fallback_preserves_root_files_and_retries(self):
        source = self.source()
        put(source / "deviceInfo.json", {"devices": ["device-a"]})
        put(source / "user_info.json", {"account": "account-a"})
        before = hashes(source)
        resource = self.resources / "profiles/Creality.json"
        saved = resource.read_bytes(); resource.unlink()
        self.run_tool(False)
        report = json.loads((self.target / "migration/user-fallback.json").read_text(encoding="utf-8"))
        for name in ("Creality.conf", "deviceInfo.json", "user_info.json"):
            self.assertEqual((self.target / name).read_bytes(), (source / name).read_bytes())
            self.assertIn(name, report["root_files"])
        self.assertFalse((self.target / "migration/manifest.json").exists())
        self.assertEqual(hashes(source), before)
        resource.write_bytes(saved); self.run_tool()
        self.assertEqual(self.report()["valid_presets"], 1)
    def test_fallback_root_files_without_user_or_valid_json(self):
        source = self.root / "7.0"
        source.mkdir()
        for name in ("Creality.conf", "deviceInfo.json", "user_info.json"):
            (source / name).write_bytes(b"broken json - preserve exact bytes")
        before = hashes(source)
        self.run_tool(False)
        for name in before:
            self.assertEqual((self.target / name).read_bytes(), (source / name).read_bytes())
        self.assertEqual(hashes(source), before)
    def test_fallback_root_files_never_overwrite_existing_files(self):
        source = self.source()
        for name in ("Creality.conf", "deviceInfo.json", "user_info.json"):
            put(source / name, {"source": name})
            put(self.target / name, {"target": name})
        before = {name: (self.target / name).read_bytes()
                  for name in ("Creality.conf", "deviceInfo.json", "user_info.json")}
        self.run_tool(False)
        for name, data in before.items():
            self.assertEqual((self.target / name).read_bytes(), data)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_fallback_root_copy_error_continues_other_files_and_user(self):
        source = self.source()
        put(source / "deviceInfo.json", {"devices": [1]})
        put(source / "user_info.json", {"account": "a"})
        (self.target / "Creality.conf").mkdir(parents=True)
        self.run_tool(False)
        for name in ("deviceInfo.json", "user_info.json"):
            self.assertEqual((self.target / name).read_bytes(), (source / name).read_bytes())
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
        report = json.loads((self.target / "migration/user-fallback.json").read_text(encoding="utf-8"))
        self.assertEqual([item["file"] for item in report["errors"]], ["Creality.conf"])
    def test_changed_fallback_root_file_is_not_reimported(self):
        source = self.source()
        put(source / "deviceInfo.json", {"devices": [1]})
        resource = self.resources / "profiles/Creality.json"
        saved = resource.read_bytes(); resource.unlink()
        self.run_tool(False)
        put(self.target / "deviceInfo.json", {"devices": [2]})
        before = hashes(self.target)
        resource.write_bytes(saved); self.run_tool()
        self.assertEqual(hashes(self.target), before)
        self.assertFalse((self.target / "migration/manifest.json").exists())
    def test_unreadable_user_structure_does_not_block_root_files(self):
        source = self.root / "7.0"
        source.mkdir()
        (source / "Creality.conf").write_bytes(b"broken")
        put(source / "deviceInfo.json", {"devices": [1]})
        put(source / "user_info.json", {"account": "a"})
        (source / "user").write_bytes(b"not a directory")
        self.run_tool(False)
        for name in ("Creality.conf", "deviceInfo.json", "user_info.json"):
            self.assertEqual((self.target / name).read_bytes(), (source / name).read_bytes())
        report = json.loads((self.target / "migration/user-fallback.json").read_text(encoding="utf-8"))
        self.assertTrue(report["errors"])
    def test_missing_config_preserves_complete_user_directory(self):
        source = self.source()
        (source / "Creality.conf").unlink()
        put(source / "user/second-account/process/nested/Custom.json", {"name": "Custom"})
        put(source / "user/default/sync_data/queue.json", {"pending": [1]})
        (source / "user/default/machine/cache.cereal").write_bytes(b"legacy-cache")
        (source / "user/empty-account/process").mkdir(parents=True)
        before = hashes(source)
        self.run_tool(False)
        self.assertEqual(hashes(source), before)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
        self.assertTrue((self.target / "user/empty-account/process").is_dir())
        self.assertTrue((self.target / "migration/preserve-user-presets").is_file())
        self.assertFalse((self.target / "migration/manifest.json").exists())
        self.run_tool(False)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_six_missing_config_fallback(self):
        source = self.source("6.0", "01.09.03.50")
        (source / "Creality.conf").unlink()
        self.run_tool(False)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
        self.assertFalse((self.root / "7.0").exists())
    def test_resource_failure_preserves_selected_source(self):
        seven = self.source()
        (seven / "Creality.conf").write_bytes(b"broken")
        six = self.source("6.0", "01.09.03.50", "Six")
        (self.resources / "profiles/Creality.json").unlink()
        before = hashes(six)
        self.run_tool(False)
        self.assertEqual(hashes(six), before)
        self.assertEqual(hashes(six / "user"), hashes(self.target / "user"))
    def test_fallback_does_not_overwrite_existing_user_files(self):
        source = self.source()
        (source / "Creality.conf").unlink()
        existing = self.target / "user/default/machine/User printer.json"
        put(existing, {"name": "User modification"})
        saved = existing.read_bytes()
        self.run_tool(False)
        self.assertEqual(existing.read_bytes(), saved)
        self.assertTrue(existing.with_suffix(".info").exists())
        put(source / "Creality.conf", {"app": {"version": "7.2.2"}})
        self.run_tool(False)
        self.assertEqual(existing.read_bytes(), saved)
    def test_changed_fallback_user_files_are_not_reimported(self):
        self.source()
        manifest = self.resources / "profiles/Creality.json"
        saved = manifest.read_bytes(); manifest.unlink()
        self.run_tool(False)
        user = self.target / "user/default/machine/User printer.json"
        put(user, {"name": "Edited after fallback"})
        before = hashes(self.target)
        manifest.write_bytes(saved)
        self.run_tool()  # Existing valid config wins once fallback user files change.
        self.assertEqual(hashes(self.target), before)
    def test_failure_before_source_selection_still_preserves_user(self):
        source = self.source()
        (self.root / ".migration-7.3").write_bytes(b"blocked diagnostics path")
        self.run_tool(False)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_fallback_copy_error_does_not_block_remaining_files(self):
        source = self.source()
        (source / "Creality.conf").unlink()
        collision = self.target / "user/default/machine/User printer.info"
        collision.mkdir(parents=True)
        put(source / "user/second-account/process/Other.json", {"name": "Other"})
        p = self.run_tool(False)
        self.assertIn("user fallback failed", p.stderr)
        other = Path("user/second-account/process/Other.json")
        self.assertEqual((source / other).read_bytes(), (self.target / other).read_bytes())
        report = json.loads((self.target / "migration/user-fallback.json").read_text(encoding="utf-8"))
        self.assertEqual(len(report["errors"]), 1)
        self.assertEqual(Path(report["errors"][0]["file"]).as_posix(), "default/machine/User printer.info")
    def test_alpha_fallback_uses_same_channel_only(self):
        self.source(name="Release preset")
        source = self.source("7.0 Alpha", name="Alpha preset")
        (source / "Creality.conf").unlink()
        self.target = self.root / "7.3 Alpha"
        self.run_tool(False)
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
        self.assertFalse((self.target / "user/default/machine/Release preset.json").exists())
    def test_broken_target_config_still_preserves_user(self):
        source = self.source()
        self.target.mkdir()
        config = self.target / "Creality.conf"
        config.write_bytes(b"broken target config")
        self.run_tool(False)
        self.assertEqual(config.read_bytes(), b"broken target config")
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    def test_broken_target_manifest_still_preserves_user(self):
        source = self.source()
        manifest = self.target / "migration/manifest.json"
        manifest.parent.mkdir(parents=True)
        manifest.write_bytes(b"broken manifest")
        self.run_tool(False)
        self.assertEqual(manifest.read_bytes(), b"broken manifest")
        self.assertEqual(hashes(source / "user"), hashes(self.target / "user"))
    @unittest.skipUnless(os.name == "nt", "Windows directory sharing behavior")
    def test_commit_retries_temporary_directory_lock(self):
        import ctypes
        import threading
        import time
        self.source()
        self.target.mkdir()
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32,
                                        ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
        kernel32.CreateFileW.restype = ctypes.c_void_p
        kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
        # Permit normal reading/writing, but temporarily deny directory renaming.
        handle = kernel32.CreateFileW(str(self.target), 0, 3, None, 3, 0x02000000, None)
        self.assertNotEqual(handle, ctypes.c_void_p(-1).value, ctypes.get_last_error())
        release = threading.Timer(0.75, lambda: kernel32.CloseHandle(handle))
        release.start()
        started = time.monotonic()
        try:
            self.run_tool()
        finally:
            release.join()
        self.assertGreaterEqual(time.monotonic() - started, 0.65)
        self.assertTrue((self.target / "migration/manifest.json").exists())
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
