//go:build cgo

package main

/*
#cgo CFLAGS: -I${SRCDIR} -I${SRCDIR}/../../_tmp/bzip2
#define BZ_NO_STDIO 1
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "../../_tmp/bzip2/bzlib.c"
#include "../../_tmp/bzip2/decompress.c"
#include "../../_tmp/bzip2/huffman.c"
#include "../../_tmp/bzip2/crctable.c"
#include "../../_tmp/bzip2/randtable.c"
#include "../../_tmp/bzip2/compress.c"
#include "../../_tmp/bzip2/blocksort.c"

static _Thread_local int bz_guard_active = 0;
static _Thread_local jmp_buf bz_guard_env;

void bz_internal_error(int errcode) {
	if (bz_guard_active) {
		longjmp(bz_guard_env, errcode ? errcode : 1);
	}
	abort();
}

int bz2_decompress_guard(bz_stream* strm) {
	bz_guard_active = 1;
	int j = setjmp(bz_guard_env);
	if (j != 0) {
		bz_guard_active = 0;
		return -10000 - j;
	}
	int ret = BZ2_bzDecompress(strm);
	bz_guard_active = 0;
	return ret;
}
*/
import "C"

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"unsafe"
)

func libbzip2Available() bool {
	return true
}

type cgoBzip2Reader struct {
	s      *C.bz_stream
	in     []byte
	inPos  int
	inBuf  unsafe.Pointer
	inCap  int
	outBuf unsafe.Pointer
	outCap int
	closed bool
	eof    bool
}

func newCgoBzip2Reader(in []byte) (*cgoBzip2Reader, error) {
	r := &cgoBzip2Reader{in: in}
	r.s = (*C.bz_stream)(C.calloc(1, C.size_t(C.sizeof_bz_stream)))
	if r.s == nil {
		return nil, errors.New("calloc bz_stream failed")
	}
	r.s.bzalloc = nil
	r.s.bzfree = nil
	r.s.opaque = nil
	r.inCap = 128 * 1024
	r.outCap = 128 * 1024
	r.inBuf = C.malloc(C.size_t(r.inCap))
	r.outBuf = C.malloc(C.size_t(r.outCap))
	if r.inBuf == nil || r.outBuf == nil {
		_ = r.Close()
		return nil, errors.New("malloc bzip2 buffers failed")
	}
	r.s.next_in = (*C.char)(r.inBuf)
	r.s.avail_in = 0
	ret := C.BZ2_bzDecompressInit(r.s, 0, 0)
	if ret != C.BZ_OK {
		_ = r.Close()
		return nil, fmt.Errorf("BZ2_bzDecompressInit failed: %d", int(ret))
	}
	return r, nil
}

func (r *cgoBzip2Reader) Close() error {
	if r.closed {
		return nil
	}
	r.closed = true
	if r.s != nil {
		_ = C.BZ2_bzDecompressEnd(r.s)
	}
	if r.inBuf != nil {
		C.free(r.inBuf)
		r.inBuf = nil
	}
	if r.outBuf != nil {
		C.free(r.outBuf)
		r.outBuf = nil
	}
	if r.s != nil {
		C.free(unsafe.Pointer(r.s))
		r.s = nil
	}
	return nil
}

func (r *cgoBzip2Reader) Read(p []byte) (int, error) {
	if r.eof {
		return 0, io.EOF
	}
	if len(p) == 0 {
		return 0, nil
	}
	if r.s == nil || r.inBuf == nil || r.outBuf == nil {
		return 0, errors.New("bzip2 reader closed")
	}

	for {
		// libbzip2 keeps next_in inside this fixed C buffer. Refill only after
		// consuming it: appending on every small Read accumulated the entire
		// compressed stream and repeatedly memmoved its unread tail.
		if r.s.avail_in == 0 && r.inPos < len(r.in) {
			n := min(r.inCap, len(r.in)-r.inPos)
			C.memcpy(r.inBuf, unsafe.Pointer(&r.in[r.inPos]), C.size_t(n))
			r.inPos += n
			r.s.next_in = (*C.char)(r.inBuf)
			r.s.avail_in = C.uint(n)
		}

		r.s.next_out = (*C.char)(r.outBuf)
		r.s.avail_out = C.uint(min(len(p), r.outCap))

		availOut0 := int(r.s.avail_out)
		ret := C.bz2_decompress_guard(r.s)
		if ret <= -10000 {
			_ = r.Close()
			return 0, fmt.Errorf("BZ2 internal error: %d", int(-10000-ret))
		}

		produced := availOut0 - int(r.s.avail_out)
		remain2 := int(r.s.avail_in)
		if remain2 < 0 {
			remain2 = 0
		}
		if ret != C.BZ_OK && ret != C.BZ_STREAM_END {
			_ = r.Close()
			return 0, fmt.Errorf("BZ2_bzDecompress failed: %d", int(ret))
		}

		if produced > 0 {
			C.memcpy(unsafe.Pointer(&p[0]), r.outBuf, C.size_t(produced))
			if ret == C.BZ_STREAM_END {
				r.eof = true
				_ = r.Close()
			}
			return produced, nil
		}

		switch ret {
		case C.BZ_OK:
			if r.inPos >= len(r.in) && remain2 == 0 {
				return 0, io.ErrUnexpectedEOF
			}
		case C.BZ_STREAM_END:
			r.eof = true
			_ = r.Close()
			return 0, io.EOF
		default:
			_ = r.Close()
			return 0, fmt.Errorf("BZ2_bzDecompress failed: %d", int(ret))
		}
	}
}

func bspatchLibBzip2(oldData []byte, patchData []byte) (out []byte, err error) {
	return bspatchLibBzip2WithProgress(oldData, patchData, nil)
}

func bspatchLibBzip2WithProgress(oldData, patchData []byte, progress func(int64, int64)) (out []byte, err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("bspatch panic: %v", r)
		}
	}()
	if len(patchData) < 32 {
		return nil, errors.New("patch too short")
	}
	if string(patchData[:8]) != "BSDIFF40" {
		return nil, errors.New("invalid patch magic")
	}

	ctrlLen := bsdiffOfftIn(patchData[8:16])
	diffLen := bsdiffOfftIn(patchData[16:24])
	newSize := bsdiffOfftIn(patchData[24:32])
	if ctrlLen < 0 || diffLen < 0 || newSize < 0 {
		return nil, errors.New("invalid patch header")
	}
	total := int64(len(patchData))
	if ctrlLen > total-32 || diffLen > total-32-ctrlLen {
		return nil, errors.New("invalid patch lengths")
	}
	if newSize > int64(^uint(0)>>1) {
		return nil, errors.New("new file too large")
	}

	ctrlComp := patchData[32 : 32+ctrlLen]
	diffComp := patchData[32+ctrlLen : 32+ctrlLen+diffLen]
	extraComp := patchData[32+ctrlLen+diffLen:]

	ctrlR, err := newCgoBzip2Reader(ctrlComp)
	if err != nil {
		return nil, err
	}
	defer ctrlR.Close()
	diffR, err := newCgoBzip2Reader(diffComp)
	if err != nil {
		return nil, err
	}
	defer diffR.Close()
	extraR, err := newCgoBzip2Reader(extraComp)
	if err != nil {
		return nil, err
	}
	defer extraR.Close()
	// Control records and diff spans can be only a few bytes. Buffer all
	// streams to avoid a cgo transition for each tiny read.
	ctrl := bufio.NewReaderSize(ctrlR, 64*1024)
	diff := bufio.NewReaderSize(diffR, 64*1024)
	extra := bufio.NewReaderSize(extraR, 64*1024)

	newData := make([]byte, int(newSize))
	var oldPos int64
	var newPos int64

	ctrlBuf := make([]byte, 24)
	for newPos < newSize {
		if isCanceled() {
			return nil, errCanceled
		}
		if _, err := io.ReadFull(ctrl, ctrlBuf); err != nil {
			return nil, err
		}
		x := bsdiffOfftIn(ctrlBuf[0:8])
		y := bsdiffOfftIn(ctrlBuf[8:16])
		z := bsdiffOfftIn(ctrlBuf[16:24])
		if x < 0 || y < 0 {
			return nil, errors.New("invalid control data")
		}
		if x == 0 && y == 0 && z == 0 {
			return nil, errors.New("empty control record")
		}
		if x > newSize-newPos {
			return nil, errors.New("control overflows output")
		}

		if x > 0 {
			var iBase int64
			for iBase < x {
				if isCanceled() {
					return nil, errCanceled
				}
				chunk := min(x-iBase, 64*1024)
				buf := newData[newPos+iBase : newPos+iBase+chunk]
				if _, err := io.ReadFull(diff, buf); err != nil {
					return nil, err
				}
				for i := int64(0); i < chunk; i++ {
					op := oldPos + iBase + i
					if op >= 0 && op < int64(len(oldData)) {
						buf[i] += oldData[op]
					}
				}
				iBase += chunk
				if progress != nil {
					progress(newPos+iBase, newSize)
				}
			}
			newPos += x
			oldPos += x
		}

		if y > newSize-newPos {
			return nil, errors.New("control overflows output")
		}
		for y > 0 {
			if isCanceled() {
				return nil, errCanceled
			}
			chunk := min(y, 64*1024)
			if _, err := io.ReadFull(extra, newData[newPos:newPos+chunk]); err != nil {
				return nil, err
			}
			newPos += chunk
			y -= chunk
			if progress != nil {
				progress(newPos, newSize)
			}
		}
		oldPos += z
	}

	return newData, nil
}

func bsdiffOfftIn(buf []byte) int64 {
	if len(buf) < 8 {
		return 0
	}
	y := int64(buf[7] & 0x7F)
	y = y<<8 + int64(buf[6])
	y = y<<8 + int64(buf[5])
	y = y<<8 + int64(buf[4])
	y = y<<8 + int64(buf[3])
	y = y<<8 + int64(buf[2])
	y = y<<8 + int64(buf[1])
	y = y<<8 + int64(buf[0])
	if (buf[7] & 0x80) != 0 {
		y = -y
	}
	return y
}
