package main

import (
	"archive/zip"
	"io"
	"strings"
)

// Weight synthesis by output size, not archive entry count: one large DLL
// should advance the progress bar while it is being patched.
type deltaProgress struct {
	weights                        map[string]float64
	total, done, current, fraction float64
	report                         func(float64)
}

func newDeltaProgress(files []*zip.File, report func(float64)) *deltaProgress {
	p := &deltaProgress{weights: make(map[string]float64), report: report}
	shasums := make(map[string]bool)
	for _, f := range files {
		if strings.HasPrefix(f.Name, "lib/net45/") && strings.HasSuffix(strings.ToLower(f.Name), ".shasum") {
			shasums[strings.ToLower(strings.TrimSuffix(f.Name, ".shasum"))] = true
		}
	}
	for _, f := range files {
		if !strings.HasPrefix(f.Name, "lib/net45/") || f.FileInfo().IsDir() {
			continue
		}
		lower := strings.ToLower(f.Name)
		if strings.HasSuffix(lower, ".diff") || strings.HasSuffix(lower, ".bsdiff") {
			continue
		}
		// The checksum entry represents the output file it validates. Count
		// that output once; otherwise changed DLLs are weighted twice.
		if !strings.HasSuffix(lower, ".shasum") && shasums[strings.ToLower(f.Name+".shasum")] {
			continue
		}
		weight := float64(f.UncompressedSize64)
		if strings.HasSuffix(lower, ".shasum") {
			if _, size, err := readShasumFile(f); err == nil && size > 0 {
				weight = float64(size)
			}
		}
		p.weights[f.Name] = max(1, weight)
		p.total += p.weights[f.Name]
	}
	return p
}

func (p *deltaProgress) start(name string) {
	p.done += p.current
	p.current = p.weights[name]
	p.fraction = 0
	p.update(0)
}

func (p *deltaProgress) update(fraction float64) {
	// Retrying another patch format or a full file must not move backwards.
	p.fraction = max(p.fraction, min(1, max(0, fraction)))
	if p.report != nil && p.total > 0 {
		// Leave the final 1% of this package for obsolete-file cleanup.
		p.report(0.99 * min(1, (p.done+p.current*p.fraction)/p.total))
	}
}

type progressReader struct {
	r           io.Reader
	done, total int64
	report      func(float64)
}

func (r *progressReader) Read(buf []byte) (int, error) {
	if isCanceled() {
		return 0, errCanceled
	}
	n, err := r.r.Read(buf)
	r.done += int64(n)
	if r.report != nil && r.total > 0 {
		r.report(min(1, float64(r.done)/float64(r.total)))
	}
	return n, err
}
