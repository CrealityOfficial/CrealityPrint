package main

import (
	"crypto/sha256"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
)

const (
	thumbnailDLLName       = "STLThumbWinShellExtension.dll"
	thumbnailEXEName       = "stl-thumb.exe"
	thumbnailDeletePending = ".thumbnail-delete-pending"
)

// Stage the DLL and its adjacent renderer together before changing registration.
// Explorer may have the current DLL mapped, so do not overwrite or remove the
// active files. A fresh directory lets subsequent COM loads use the new pair.
func stageThumbnailHelpers(thumbnailDir, installDir, backupDir string) (string, error) {
	names := []string{thumbnailDLLName, thumbnailEXEName}
	sourceDir := installDir
	missing := 0
	for _, name := range names {
		_, err := os.Stat(filepath.Join(sourceDir, name))
		if os.IsNotExist(err) {
			missing++
		} else if err != nil {
			return "", err
		}
	}
	// Older packages may omit the helpers. Restore a complete backup pair only
	// when both are absent; never mix files from different package versions.
	if missing == len(names) && backupDir != "" {
		sourceDir = backupDir
	}
	for _, name := range names {
		info, err := os.Stat(filepath.Join(sourceDir, name))
		if err != nil {
			return "", fmt.Errorf("thumbnail helper %s: %w", name, err)
		}
		if !info.Mode().IsRegular() || info.Size() == 0 {
			return "", fmt.Errorf("thumbnail helper %s is not a nonempty file", name)
		}
	}
	var hashes [2][sha256.Size]byte
	version := sha256.New()
	for i, name := range names {
		hash, err := thumbnailFileHash(filepath.Join(sourceDir, name))
		if err != nil {
			return "", err
		}
		hashes[i] = hash
		version.Write(hash[:])
	}
	// Compare both files: rendering fixes can change only the EXE while the DLL
	// and its version resource stay unchanged. Verify bytes instead of trusting
	// the directory name, so a damaged previous deployment is not reused.
	prefix := fmt.Sprintf("update-%x-", version.Sum(nil))
	candidates := []string{thumbnailDir}
	entries, _ := os.ReadDir(thumbnailDir)
	for _, entry := range entries {
		if entry.IsDir() && strings.HasPrefix(entry.Name(), prefix) {
			candidates = append(candidates, filepath.Join(thumbnailDir, entry.Name()))
		}
	}
	for _, dir := range candidates {
		if thumbnailPairMatches(dir, hashes) {
			return filepath.Join(dir, thumbnailDLLName), nil
		}
	}
	if err := os.MkdirAll(thumbnailDir, 0755); err != nil {
		return "", err
	}
	stagedDir, err := os.MkdirTemp(thumbnailDir, prefix)
	if err != nil {
		return "", err
	}
	complete := false
	defer func() {
		if !complete {
			for _, name := range names {
				_ = os.Remove(filepath.Join(stagedDir, name))
			}
			_ = os.Remove(stagedDir)
		}
	}()
	for _, name := range names {
		if err := copyFile(filepath.Join(sourceDir, name), filepath.Join(stagedDir, name)); err != nil {
			return "", fmt.Errorf("stage thumbnail helper %s: %w", name, err)
		}
	}
	if !thumbnailPairMatches(stagedDir, hashes) {
		return "", fmt.Errorf("staged thumbnail helpers do not match source hashes")
	}
	complete = true
	return filepath.Join(stagedDir, thumbnailDLLName), nil
}

func thumbnailFileHash(path string) ([sha256.Size]byte, error) {
	var hash [sha256.Size]byte
	f, err := os.Open(path)
	if err != nil {
		return hash, err
	}
	defer f.Close()
	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return hash, err
	}
	copy(hash[:], h.Sum(nil))
	return hash, nil
}

func thumbnailPairMatches(dir string, hashes [2][sha256.Size]byte) bool {
	if _, err := os.Stat(filepath.Join(dir, thumbnailDeletePending)); err == nil || !os.IsNotExist(err) {
		return false
	}
	for i, name := range []string{thumbnailDLLName, thumbnailEXEName} {
		hash, err := thumbnailFileHash(filepath.Join(dir, name))
		if err != nil || hash != hashes[i] {
			return false
		}
	}
	return true
}
