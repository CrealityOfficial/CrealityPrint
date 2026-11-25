package main

import (
	"os"
	"path/filepath"
	"sync"
	"syscall"
	"testing"
)

func writeThumbnailPair(t *testing.T, dir, version string) {
	t.Helper()
	if err := os.MkdirAll(dir, 0755); err != nil {
		t.Fatal(err)
	}
	for _, name := range []string{thumbnailDLLName, thumbnailEXEName} {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(version+name), 0644); err != nil {
			t.Fatal(err)
		}
	}
}

func checkThumbnailPair(t *testing.T, dir, version string) {
	t.Helper()
	for _, name := range []string{thumbnailDLLName, thumbnailEXEName} {
		data, err := os.ReadFile(filepath.Join(dir, name))
		if err != nil || string(data) != version+name {
			t.Fatalf("%s: got %q, error %v; want version %s", name, data, err, version)
		}
	}
}

func lockThumbnailFile(t *testing.T, path string) func() {
	t.Helper()
	p, err := syscall.UTF16PtrFromString(path)
	if err != nil {
		t.Fatal(err)
	}
	h, err := syscall.CreateFile(p, syscall.GENERIC_READ, 0, nil, syscall.OPEN_EXISTING, syscall.FILE_ATTRIBUTE_NORMAL, 0)
	if err != nil {
		t.Fatal(err)
	}
	var once sync.Once
	unlock := func() { once.Do(func() { syscall.CloseHandle(h) }) }
	t.Cleanup(unlock)
	return unlock
}

func TestThumbnailUpdateWithExistingLockedHelpers(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, active, "old-")
	writeThumbnailPair(t, install, "new-")
	lockThumbnailFile(t, filepath.Join(active, thumbnailDLLName))
	lockThumbnailFile(t, filepath.Join(active, thumbnailEXEName))
	dll, err := stageThumbnailHelpers(active, install, "")
	if err != nil {
		t.Fatal(err)
	}
	if filepath.Dir(dll) == active {
		t.Fatal("updated helpers must not overwrite active files")
	}
	checkThumbnailPair(t, filepath.Dir(dll), "new-")
	// A later update must replace even a previously staged, locked version.
	lockThumbnailFile(t, dll)
	writeThumbnailPair(t, install, "next-")
	nextDLL, err := stageThumbnailHelpers(active, install, "")
	if err != nil || nextDLL == dll {
		t.Fatalf("next update: %s, %v", nextDLL, err)
	}
	checkThumbnailPair(t, filepath.Dir(nextDLL), "next-")
}

func TestThumbnailBackupRequiresCompletePair(t *testing.T) {
	root := t.TempDir()
	active, install, backup := filepath.Join(root, "active"), filepath.Join(root, "install"), filepath.Join(root, "backup")
	writeThumbnailPair(t, backup, "backup-")
	dll, err := stageThumbnailHelpers(active, install, backup)
	if err != nil {
		t.Fatal(err)
	}
	checkThumbnailPair(t, filepath.Dir(dll), "backup-")
	writeThumbnailPair(t, install, "new-")
	if err := os.Remove(filepath.Join(install, thumbnailEXEName)); err != nil {
		t.Fatal(err)
	}
	if path, err := stageThumbnailHelpers(active, install, backup); err == nil || path != "" {
		t.Fatalf("partial package must not mix with backup: %s, %v", path, err)
	}
	checkThumbnailPair(t, filepath.Dir(dll), "backup-")
}

func TestThumbnailUnreadableSourcePreservesOldPair(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, active, "old-")
	writeThumbnailPair(t, install, "new-")
	// An unreadable source must leave the current helpers and registration usable.
	lockThumbnailFile(t, filepath.Join(install, thumbnailEXEName))
	if dll, err := stageThumbnailHelpers(active, install, ""); err == nil || dll != "" {
		t.Fatalf("copy failure returned %s, %v", dll, err)
	}
	checkThumbnailPair(t, active, "old-")
	entries, err := os.ReadDir(active)
	if err != nil || len(entries) != 2 {
		t.Fatalf("partial staging directory remains: %v, %v", entries, err)
	}
}

func TestThumbnailUnchangedPairIsReused(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, active, "same-")
	writeThumbnailPair(t, install, "same-")
	dll, err := stageThumbnailHelpers(active, install, "")
	if err != nil || dll != filepath.Join(active, thumbnailDLLName) {
		t.Fatalf("identical existing pair was not reused: %s, %v", dll, err)
	}
	writeThumbnailPair(t, install, "new-")
	dll, err = stageThumbnailHelpers(active, install, "")
	if err != nil {
		t.Fatal(err)
	}
	reused, err := stageThumbnailHelpers(active, install, "")
	if err != nil || reused != dll {
		t.Fatalf("identical staged pair was not reused: %s, %v", reused, err)
	}
}

func TestThumbnailEitherFileChangeOrCorruptionRequiresUpdate(t *testing.T) {
	for _, name := range []string{thumbnailDLLName, thumbnailEXEName} {
		t.Run(name, func(t *testing.T) {
			root := t.TempDir()
			active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
			writeThumbnailPair(t, install, "original-")
			first, err := stageThumbnailHelpers(active, install, "")
			if err != nil {
				t.Fatal(err)
			}
			if err := os.WriteFile(filepath.Join(install, name), []byte("changed"), 0644); err != nil {
				t.Fatal(err)
			}
			second, err := stageThumbnailHelpers(active, install, "")
			if err != nil || second == first {
				t.Fatalf("changed %s was skipped: %s, %v", name, second, err)
			}
			if err := os.WriteFile(filepath.Join(filepath.Dir(second), name), []byte("corrupt"), 0644); err != nil {
				t.Fatal(err)
			}
			repaired, err := stageThumbnailHelpers(active, install, "")
			if err != nil || repaired == second {
				t.Fatalf("corrupted deployment was reused: %s, %v", repaired, err)
			}
			data, err := os.ReadFile(filepath.Join(filepath.Dir(repaired), name))
			if err != nil || string(data) != "changed" {
				t.Fatalf("repair failed: %q, %v", data, err)
			}
		})
	}
}

func TestThumbnailRejectsEmptySource(t *testing.T) {
	root := t.TempDir()
	active, install := filepath.Join(root, "active"), filepath.Join(root, "install")
	writeThumbnailPair(t, active, "old-")
	writeThumbnailPair(t, install, "new-")
	if err := os.WriteFile(filepath.Join(install, thumbnailDLLName), nil, 0644); err != nil {
		t.Fatal(err)
	}
	if _, err := stageThumbnailHelpers(active, install, ""); err == nil {
		t.Fatal("accepted empty DLL")
	}
	checkThumbnailPair(t, active, "old-")
}
