package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"testing"
	"unsafe"
)

// Redirect HKCR only inside this test process; never alter live COM registration.
func isolateThumbnailRegistry(t *testing.T) {
	t.Helper()
	path := syscall.StringToUTF16Ptr(fmt.Sprintf(`Software\CrealityThumbnailTest-%d-%s`, os.Getpid(), t.Name()))
	var key uintptr
	const currentUser = ^uintptr(0x7ffffffe)
	ret, _, _ := procRegCreateKeyExW.Call(currentUser, uintptr(unsafe.Pointer(path)), 0, 0, 0, 0xF003F, 0, uintptr(unsafe.Pointer(&key)), 0)
	if ret != 0 {
		t.Fatal("create isolated registry:", syscall.Errno(ret))
	}
	override := modAdvapi32.NewProc("RegOverridePredefKey")
	ret, _, _ = override.Call(hkeyClassesRoot, key)
	if ret != 0 {
		procRegCloseKey.Call(key)
		modAdvapi32.NewProc("RegDeleteTreeW").Call(currentUser, uintptr(unsafe.Pointer(path)))
		t.Fatal("override HKCR:", syscall.Errno(ret))
	}
	t.Cleanup(func() {
		override.Call(hkeyClassesRoot, 0)
		procRegCloseKey.Call(key)
		modAdvapi32.NewProc("RegDeleteTreeW").Call(currentUser, uintptr(unsafe.Pointer(path)))
	})
}

func TestThumbnailNSISInstallReuseAndUninstall(t *testing.T) {
	isolateThumbnailRegistry(t)
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, install, "new-")
	for _, ext := range []string{".stl", ".3mf", ".gcode"} {
		if err := setRegistryString("HKCR\\"+ext, "", strings.TrimPrefix(ext, ".")+"_cxsw"); err != nil {
			t.Fatal(err)
		}
	}
	if code := runThumbnailMaintenance("install", active, install); code != 0 {
		t.Fatalf("install returned %d", code)
	}
	key := `CLSID\` + thumbnailProviderCLSID + `\InprocServer32`
	dll, err := thumbnailRegistryString(key)
	if err != nil || !managedThumbnailDLL(active, dll) {
		t.Fatalf("unexpected registered DLL %s: %v", dll, err)
	}
	checkThumbnailPair(t, filepath.Dir(dll), "new-")
	if code := runThumbnailMaintenance("install", active, install); code != 0 {
		t.Fatalf("repeat install returned %d", code)
	}
	if reused, _ := thumbnailRegistryString(key); reused != dll {
		t.Fatalf("unchanged install was not reused: %s", reused)
	}
	for _, progID := range []string{"stl_cxsw", "3mf_cxsw", "gcode_cxsw"} {
		value, err := thumbnailRegistryString(progID + `\shellex\` + thumbnailShellExtKey)
		if err != nil || value != thumbnailProviderCLSID {
			t.Fatalf("missing association for %s: %s, %v", progID, value, err)
		}
	}
	// Preserve a handler selected by another application after installation.
	otherKey := `HKCR\gcode_cxsw\shellex\` + thumbnailShellExtKey
	if err := setRegistryString(otherKey, "", "other-provider"); err != nil {
		t.Fatal(err)
	}
	if code := runThumbnailMaintenance("uninstall", active, ""); code != 0 {
		t.Fatalf("uninstall returned %d", code)
	}
	if value, _ := thumbnailRegistryString(key); value != "" {
		t.Fatalf("COM registration remains: %s", value)
	}
	if value, _ := thumbnailRegistryString(strings.TrimPrefix(otherKey, `HKCR\`)); value != "other-provider" {
		t.Fatalf("another handler was removed: %s", value)
	}
	if _, err := os.Stat(dll); !os.IsNotExist(err) {
		t.Fatalf("deployed DLL remains: %v", err)
	}
}

func TestThumbnailUninstallPreservesOtherInstallation(t *testing.T) {
	isolateThumbnailRegistry(t)
	root := t.TempDir()
	otherDLL := filepath.Join(root, "other", thumbnailDLLName)
	if err := registerThumbnailProvider(otherDLL, false); err != nil {
		t.Fatal(err)
	}
	if err := unregisterThumbnailProvider(filepath.Join(root, "ours")); err != nil {
		t.Fatal(err)
	}
	value, err := thumbnailRegistryString(`CLSID\` + thumbnailProviderCLSID + `\InprocServer32`)
	if err != nil || value != otherDLL {
		t.Fatalf("other installation changed: %s, %v", value, err)
	}
}

func TestThumbnailCleanupPreservesBackupsAndUnrelatedFiles(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, active, "old-")
	writeThumbnailPair(t, install, "new-")
	dll, err := stageThumbnailHelpers(active, install, "")
	if err != nil {
		t.Fatal(err)
	}
	backup := filepath.Join(active, "backups")
	unrelated := filepath.Join(active, "update-unrelated")
	writeThumbnailPair(t, backup, "backup-")
	writeThumbnailPair(t, unrelated, "unrelated-")
	if err := os.WriteFile(filepath.Join(filepath.Dir(dll), "notes.txt"), []byte("keep"), 0644); err != nil {
		t.Fatal(err)
	}
	reboot, err := cleanThumbnailHelpers(active, func(string) error { t.Fatal("unexpected reboot deletion"); return nil })
	if err != nil || reboot {
		t.Fatalf("cleanup: reboot=%v, %v", reboot, err)
	}
	checkThumbnailPair(t, backup, "backup-")
	checkThumbnailPair(t, unrelated, "unrelated-")
	if data, err := os.ReadFile(filepath.Join(filepath.Dir(dll), "notes.txt")); err != nil || string(data) != "keep" {
		t.Fatalf("unrelated file removed: %s, %v", data, err)
	}
}

func TestThumbnailLockedUninstallAndReinstallBeforeReboot(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, install, "new-")
	dll, err := stageThumbnailHelpers(active, install, "")
	if err != nil {
		t.Fatal(err)
	}
	unlock := lockThumbnailFile(t, dll)
	var scheduled []string
	reboot, err := cleanThumbnailHelpers(active, func(path string) error { scheduled = append(scheduled, path); return nil })
	if err != nil || !reboot {
		t.Fatalf("locked cleanup: reboot=%v, %v", reboot, err)
	}
	if len(scheduled) != 3 || scheduled[0] != dll || scheduled[2] != filepath.Dir(dll) {
		t.Fatalf("expected DLL, marker, directory scheduling: %v", scheduled)
	}
	unlock()
	// Simulate files remaining until reboot, with a readable but pending old pair.
	if err := os.WriteFile(filepath.Join(filepath.Dir(dll), thumbnailEXEName), []byte("new-"+thumbnailEXEName), 0644); err != nil {
		t.Fatal(err)
	}
	next, err := stageThumbnailHelpers(active, install, "")
	if err != nil || next == dll {
		t.Fatalf("reinstall reused files pending deletion: %s, %v", next, err)
	}
	checkThumbnailPair(t, filepath.Dir(next), "new-")
}

func TestThumbnailManagedPathBoundary(t *testing.T) {
	root := filepath.Join(t.TempDir(), "active")
	dir := "update-" + strings.Repeat("a", 64) + "-1234"
	for _, path := range []string{filepath.Join(root, thumbnailDLLName), filepath.Join(root, dir, thumbnailDLLName)} {
		if !managedThumbnailDLL(root, path) {
			t.Fatalf("valid path rejected: %s", path)
		}
	}
	for _, path := range []string{filepath.Join(root+"-other", thumbnailDLLName), filepath.Join(root, "backups", thumbnailDLLName), filepath.Join(root, dir, "nested", thumbnailDLLName)} {
		if managedThumbnailDLL(root, path) {
			t.Fatalf("unmanaged path accepted: %s", path)
		}
	}
}
