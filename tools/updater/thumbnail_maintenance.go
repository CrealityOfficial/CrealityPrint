package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"unsafe"
)

// Runs before the normal updater UI and version checks. NSIS invokes this while
// updater.exe still exists, and handles 3010 as a reboot-required success.
func runThumbnailMaintenance(action, root, installDir string) int {
	if root == "" || (action != "install" && action != "uninstall") {
		return 1
	}
	var err error
	reboot := false
	if action == "install" {
		if installDir == "" {
			return 1
		}
		var dll string
		dll, err = stageThumbnailHelpers(root, installDir, "")
		if err == nil {
			err = registerThumbnailProvider(dll, true)
		}
	} else {
		err = unregisterThumbnailProvider(root)
		if err == nil {
			reboot, err = cleanThumbnailHelpers(root, scheduleThumbnailDeletion)
		}
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, "thumbnail maintenance:", err)
		return 1
	}
	if reboot {
		return 3010
	}
	return 0
}

func thumbnailAssociationKeys(uninstall bool) []string {
	var keys []string
	seen := make(map[string]bool)
	for _, ext := range []string{".stl", ".3mf", ".gcode"} {
		progID, _ := thumbnailRegistryString(ext)
		fileTypes := []string{ext}
		if progID != "" {
			fileTypes = []string{progID}
		}
		if uninstall {
			fileTypes = []string{ext, strings.TrimPrefix(ext, ".") + "_cxsw", progID}
		}
		for _, fileType := range fileTypes {
			if fileType != "" && !seen[strings.ToLower(fileType)] {
				seen[strings.ToLower(fileType)] = true
				keys = append(keys, fileType+`\shellex\`+thumbnailShellExtKey)
			}
		}
	}
	return keys
}

func thumbnailRegistryString(key string) (string, error) {
	subKey, err := syscall.UTF16PtrFromString(key)
	if err != nil {
		return "", err
	}
	var size uint32
	const rrfString = 2
	ret, _, _ := procRegGetValueW.Call(hkeyClassesRoot, uintptr(unsafe.Pointer(subKey)), 0, rrfString, 0, 0, uintptr(unsafe.Pointer(&size)))
	if ret == 2 || ret == 3 {
		return "", nil
	}
	if ret != 0 {
		return "", syscall.Errno(ret)
	}
	buffer := make([]uint16, size/2+1)
	ret, _, _ = procRegGetValueW.Call(hkeyClassesRoot, uintptr(unsafe.Pointer(subKey)), 0, rrfString, 0, uintptr(unsafe.Pointer(&buffer[0])), uintptr(unsafe.Pointer(&size)))
	if ret != 0 {
		return "", syscall.Errno(ret)
	}
	return syscall.UTF16ToString(buffer), nil
}

func unregisterThumbnailProvider(root string) error {
	clsid := `CLSID\` + thumbnailProviderCLSID
	dll, err := thumbnailRegistryString(clsid + `\InprocServer32`)
	if err != nil {
		return err
	}
	// Another installation may now own this CLSID. Never unregister it.
	if !managedThumbnailDLL(root, dll) {
		return nil
	}
	deleteValue := modAdvapi32.NewProc("RegDeleteKeyValueW")
	for _, key := range thumbnailAssociationKeys(true) {
		value, err := thumbnailRegistryString(key)
		if err != nil {
			return err
		}
		if !strings.EqualFold(value, thumbnailProviderCLSID) {
			continue
		}
		p := syscall.StringToUTF16Ptr(key)
		ret, _, _ := deleteValue.Call(hkeyClassesRoot, uintptr(unsafe.Pointer(p)), 0)
		if ret != 0 && ret != 2 && ret != 3 {
			return syscall.Errno(ret)
		}
	}
	p := syscall.StringToUTF16Ptr(clsid)
	ret, _, _ := modAdvapi32.NewProc("RegDeleteTreeW").Call(hkeyClassesRoot, uintptr(unsafe.Pointer(p)))
	if ret != 0 && ret != 2 && ret != 3 {
		return syscall.Errno(ret)
	}
	procSHChangeNotify.Call(uintptr(0x08000000), uintptr(0x2000), 0, 0)
	return nil
}

func managedThumbnailDLL(root, dll string) bool {
	if !filepath.IsAbs(dll) || !strings.EqualFold(filepath.Base(dll), thumbnailDLLName) {
		return false
	}
	rel, err := filepath.Rel(root, filepath.Dir(dll))
	return err == nil && (rel == "." || (filepath.Base(rel) == rel && validThumbnailDirectory(rel)))
}

func validThumbnailDirectory(name string) bool {
	// MkdirTemp uses a decimal suffix after the SHA-256 pair fingerprint.
	if len(name) < 73 || !strings.HasPrefix(name, "update-") || name[71] != '-' {
		return false
	}
	for _, c := range name[7:71] {
		if !(c >= '0' && c <= '9' || c >= 'a' && c <= 'f') {
			return false
		}
	}
	for _, c := range name[72:] {
		if c < '0' || c > '9' {
			return false
		}
	}
	return true
}

func scheduleThumbnailDeletion(path string) error {
	p, err := syscall.UTF16PtrFromString(path)
	if err != nil {
		return err
	}
	const moveFileDelayUntilReboot = 4
	ret, _, callErr := modKernel32.NewProc("MoveFileExW").Call(uintptr(unsafe.Pointer(p)), 0, moveFileDelayUntilReboot)
	if ret == 0 {
		return callErr
	}
	return nil
}

func cleanThumbnailHelpers(root string, schedule func(string) error) (bool, error) {
	info, err := os.Lstat(root)
	if os.IsNotExist(err) {
		return false, nil
	}
	if err != nil {
		return false, err
	}
	if !info.IsDir() || info.Mode()&os.ModeSymlink != 0 {
		return false, fmt.Errorf("thumbnail directory is not a regular directory: %s", root)
	}
	entries, err := os.ReadDir(root)
	if err != nil {
		return false, err
	}
	dirs := []string{root}
	for _, entry := range entries {
		if entry.IsDir() && entry.Type()&os.ModeSymlink == 0 && validThumbnailDirectory(entry.Name()) {
			dirs = append(dirs, filepath.Join(root, entry.Name()))
		}
	}
	reboot := false
	var failures []error
	remove := func(path string) {
		err := os.Remove(path)
		if err == nil || os.IsNotExist(err) {
			return
		}
		if !errors.Is(err, syscall.Errno(32)) && !errors.Is(err, syscall.Errno(33)) && !errors.Is(err, syscall.Errno(5)) {
			failures = append(failures, err)
			return
		}
		if err := schedule(path); err != nil {
			failures = append(failures, err)
		} else {
			reboot = true
		}
	}
	for _, dir := range dirs {
		pending := false
		for _, name := range []string{thumbnailDLLName, thumbnailEXEName} {
			path := filepath.Join(dir, name)
			info, err := os.Lstat(path)
			if err == nil && info.Mode().IsRegular() {
				// A reinstall before reboot must not reuse scheduled-for-deletion files.
				if err := os.Remove(path); err != nil && !os.IsNotExist(err) {
					if err := os.WriteFile(filepath.Join(dir, thumbnailDeletePending), []byte("pending reboot\n"), 0644); err != nil {
						failures = append(failures, err)
						continue
					}
					pending = true
					remove(path)
				}
			} else if err != nil && !os.IsNotExist(err) {
				failures = append(failures, err)
			}
		}
		marker := filepath.Join(dir, thumbnailDeletePending)
		if pending {
			if err := schedule(marker); err != nil {
				failures = append(failures, err)
			} else {
				reboot = true
			}
		} else {
			_ = os.Remove(marker)
		}
		if dir != root {
			remaining, err := os.ReadDir(dir)
			if err != nil {
				failures = append(failures, err)
				continue
			}
			if len(remaining) == 0 {
				remove(dir)
			} else {
				onlyHelpers := true
				for _, file := range remaining {
					onlyHelpers = onlyHelpers && file.Type().IsRegular() && (file.Name() == thumbnailDLLName || file.Name() == thumbnailEXEName || file.Name() == thumbnailDeletePending)
				}
				if onlyHelpers {
					if err := schedule(dir); err != nil {
						failures = append(failures, err)
					} else {
						reboot = true
					}
				}
			}
		}
	}
	// Leave backups and unrelated files alone; no recursive deletion.
	_ = os.Remove(root)
	return reboot, errors.Join(failures...)
}
