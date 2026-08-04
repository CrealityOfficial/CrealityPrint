package main

import (
	"archive/zip"
	"bufio"
	"bytes"
	"crypto/sha1"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
	"unsafe"

	"github.com/kr/binarydist"
)

type ReleaseEntry struct {
	FileName string
	Version  string
	IsFull   bool
}

type VersionInfo struct {
	Version string
	Full    *ReleaseEntry
	Delta   *ReleaseEntry
}

var (
	modKernel32 = syscall.NewLazyDLL("kernel32.dll")
	modUser32   = syscall.NewLazyDLL("user32.dll")
	modComctl32 = syscall.NewLazyDLL("comctl32.dll")
	modShell32  = syscall.NewLazyDLL("shell32.dll")
	modGdi32    = syscall.NewLazyDLL("gdi32.dll")
	modAdvapi32 = syscall.NewLazyDLL("advapi32.dll")
	modRstrtmgr = syscall.NewLazyDLL("rstrtmgr.dll")
	modMsDelta  = syscall.NewLazyDLL("msdelta.dll")

	procGetModuleHandleW           = modKernel32.NewProc("GetModuleHandleW")
	procGetCurrentProcessId        = modKernel32.NewProc("GetCurrentProcessId")
	procOpenProcess                = modKernel32.NewProc("OpenProcess")
	procTerminateProcess           = modKernel32.NewProc("TerminateProcess")
	procCloseHandle                = modKernel32.NewProc("CloseHandle")
	procWaitForSingleObject        = modKernel32.NewProc("WaitForSingleObject")
	procCreateToolhelp32Snapshot   = modKernel32.NewProc("CreateToolhelp32Snapshot")
	procProcess32FirstW            = modKernel32.NewProc("Process32FirstW")
	procProcess32NextW             = modKernel32.NewProc("Process32NextW")
	procQueryFullProcessImageNameW = modKernel32.NewProc("QueryFullProcessImageNameW")
	procMessageBoxW                = modUser32.NewProc("MessageBoxW")
	procRegisterClassExW           = modUser32.NewProc("RegisterClassExW")
	procCreateWindowExW            = modUser32.NewProc("CreateWindowExW")
	procDefWindowProcW             = modUser32.NewProc("DefWindowProcW")
	procDestroyWindow              = modUser32.NewProc("DestroyWindow")
	procShowWindow                 = modUser32.NewProc("ShowWindow")
	procUpdateWindow               = modUser32.NewProc("UpdateWindow")
	procGetMessageW                = modUser32.NewProc("GetMessageW")
	procTranslateMessage           = modUser32.NewProc("TranslateMessage")
	procDispatchMessageW           = modUser32.NewProc("DispatchMessageW")
	procPostQuitMessage            = modUser32.NewProc("PostQuitMessage")
	procPostMessageW               = modUser32.NewProc("PostMessageW")
	procSendMessageW               = modUser32.NewProc("SendMessageW")
	procSetWindowTextW             = modUser32.NewProc("SetWindowTextW")
	procGetWindowTextW             = modUser32.NewProc("GetWindowTextW")
	procGetClientRect              = modUser32.NewProc("GetClientRect")
	procMoveWindow                 = modUser32.NewProc("MoveWindow")
	procGetSystemMetrics           = modUser32.NewProc("GetSystemMetrics")
	procSetWindowLongPtrW          = modUser32.NewProc("SetWindowLongPtrW")
	procGetWindowLongPtrW          = modUser32.NewProc("GetWindowLongPtrW")
	procCallWindowProcW            = modUser32.NewProc("CallWindowProcW")
	procEnableWindow               = modUser32.NewProc("EnableWindow")
	procFillRect                   = modUser32.NewProc("FillRect")
	procFrameRect                  = modUser32.NewProc("FrameRect")
	procInvalidateRect             = modUser32.NewProc("InvalidateRect")
	procGetWindowRect              = modUser32.NewProc("GetWindowRect")
	procIsWindowEnabled            = modUser32.NewProc("IsWindowEnabled")
	procScreenToClient             = modUser32.NewProc("ScreenToClient")
	procEnumWindows                = modUser32.NewProc("EnumWindows")
	procGetWindowThreadProcessId   = modUser32.NewProc("GetWindowThreadProcessId")
	procBeginPaint                 = modUser32.NewProc("BeginPaint")
	procEndPaint                   = modUser32.NewProc("EndPaint")
	procTrackMouseEvent            = modUser32.NewProc("TrackMouseEvent")
	procSetCapture                 = modUser32.NewProc("SetCapture")
	procReleaseCapture             = modUser32.NewProc("ReleaseCapture")
	procDrawTextW                  = modUser32.NewProc("DrawTextW")
	procSetWindowRgn               = modUser32.NewProc("SetWindowRgn")
	procInitCommonControlsEx       = modComctl32.NewProc("InitCommonControlsEx")
	procShellExecuteW              = modShell32.NewProc("ShellExecuteW")
	procCreateSolidBrush           = modGdi32.NewProc("CreateSolidBrush")
	procCreatePen                  = modGdi32.NewProc("CreatePen")
	procSelectObject               = modGdi32.NewProc("SelectObject")
	procDeleteObject               = modGdi32.NewProc("DeleteObject")
	procRoundRect                  = modGdi32.NewProc("RoundRect")
	procRectangle                  = modGdi32.NewProc("Rectangle")
	procCreateRoundRectRgn         = modGdi32.NewProc("CreateRoundRectRgn")
	procSetTextColor               = modGdi32.NewProc("SetTextColor")
	procSetBkColor                 = modGdi32.NewProc("SetBkColor")
	procSetBkMode                  = modGdi32.NewProc("SetBkMode")
	procGetStockObject             = modGdi32.NewProc("GetStockObject")
	procMoveToEx                   = modGdi32.NewProc("MoveToEx")
	procLineTo                     = modGdi32.NewProc("LineTo")
	procGetParent                  = modUser32.NewProc("GetParent")
	procGetDlgCtrlID               = modUser32.NewProc("GetDlgCtrlID")
	procGetDpiForSystem            = modUser32.NewProc("GetDpiForSystem")
	procGetDpiForWindow            = modUser32.NewProc("GetDpiForWindow")

	procCreateFontIndirectW = modGdi32.NewProc("CreateFontIndirectW")

	procRegGetValueW = modAdvapi32.NewProc("RegGetValueW")

	procRmStartSession      = modRstrtmgr.NewProc("RmStartSession")
	procRmRegisterResources = modRstrtmgr.NewProc("RmRegisterResources")
	procRmGetList           = modRstrtmgr.NewProc("RmGetList")
	procRmEndSession        = modRstrtmgr.NewProc("RmEndSession")

	procApplyDeltaB = modMsDelta.NewProc("ApplyDeltaB")
	procDeltaFree   = modMsDelta.NewProc("DeltaFree")
)

const (
	mbOK              = 0x00000000
	mbIconInformation = 0x00000040
	mbIconError       = 0x00000010
	mbSystemModal     = 0x00001000

	wsOverlapped   = 0x00000000
	wsCaption      = 0x00C00000
	wsSysMenu      = 0x00080000
	wsMinimizeBox  = 0x00020000
	wsVisible      = 0x10000000
	wsChild        = 0x40000000
	wsTabStop      = 0x00010000
	wsDisabled     = 0x08000000
	wsClipsiblings = 0x04000000

	bsPushButton = 0x00000000
	bsFlat       = 0x00008000

	wmCreate         = 0x0001
	wmDestroy        = 0x0002
	wmClose          = 0x0010
	wmCommand        = 0x0111
	wmEraseBkgnd     = 0x0014
	wmCtlColorBtn    = 0x0135
	wmCtlColorStatic = 0x0138
	wmNCHitTest      = 0x0084
	wmPaint          = 0x000F
	wmMouseMove      = 0x0200
	wmMouseLeave     = 0x02A3
	wmLButtonDown    = 0x0201
	wmLButtonUp      = 0x0202
	wmNclButtonDown  = 0x00A1
	wmDpiChanged     = 0x02E0

	wmApp         = 0x8000
	wmAppProgress = wmApp + 1
	wmAppDone     = wmApp + 2

	swShow = 5

	smCxScreen = 0
	smCyScreen = 1

	ccsNoResize = 0x00000004

	pbmSetRange32  = 0x0406
	pbmSetPos      = 0x0402
	pbmSetBarColor = 0x0409
	pbmSetBkColor  = 0x2001

	gwlStyle    = ^uintptr(15)
	gwlpWndProc = ^uintptr(3)
	wmSetFont   = 0x0030

	idCancel  = 1001
	idClose   = 1002
	idManual  = 2001
	idConfirm = 2002

	iccProgressClass = 0x00000020

	bkTransparent  = 1
	bnClicked      = 0
	defaultGuiFont = 17
	htCaption      = 2

	installPadding     = 28
	installCloseWidth  = 36
	installTitlebarHit = 44

	tmeLeave = 0x00000002

	psSolid      = 0
	dtCenter     = 0x00000001
	dtVCenter    = 0x00000004
	dtSingleLine = 0x00000020
)

const (
	windowsErrSharingViolation syscall.Errno = 32
	windowsErrLockViolation    syscall.Errno = 33
)

const (
	th32csSnapprocess              = 0x00000002
	invalidHandleValue             = ^uintptr(0)
	processQueryLimitedInformation = 0x00001000
	processTerminate               = 0x00000001
	synchronize                    = 0x00100000
	waitObject0                    = 0x00000000
	waitTimeout                    = 0x00000102
	errorMoreData                  = 234
)

const (
	hkeyCurrentUser uintptr = 0x80000001

	rrfRtRegDword uint32 = 0x00000010
)

func messageBox(text, caption string, flags uint32) int {
	t, _ := syscall.UTF16PtrFromString(text)
	c, _ := syscall.UTF16PtrFromString(caption)
	ret, _, _ := procMessageBoxW.Call(
		0,
		uintptr(unsafe.Pointer(t)),
		uintptr(unsafe.Pointer(c)),
		uintptr(flags),
	)
	return int(ret)
}

func detectSystemLightTheme() bool {
	subKey := toUTF16Ptr(`Software\Microsoft\Windows\CurrentVersion\Themes\Personalize`)
	tryRead := func(name string) (uint32, bool) {
		var data uint32
		var dataSize uint32 = 4
		ret, _, _ := procRegGetValueW.Call(
			hkeyCurrentUser,
			uintptr(unsafe.Pointer(subKey)),
			uintptr(unsafe.Pointer(toUTF16Ptr(name))),
			uintptr(rrfRtRegDword),
			0,
			uintptr(unsafe.Pointer(&data)),
			uintptr(unsafe.Pointer(&dataSize)),
		)
		if ret != 0 {
			return 0, false
		}
		return data, true
	}

	if v, ok := tryRead("AppsUseLightTheme"); ok {
		return v != 0
	}
	if v, ok := tryRead("SystemUsesLightTheme"); ok {
		return v != 0
	}
	return false
}

func parseDarkColorModeFromConfigBytes(b []byte) (bool, bool) {
	if len(b) == 0 {
		return false, false
	}

	end := bytes.LastIndexByte(b, '}')
	if end < 0 {
		return false, false
	}
	payload := b[:end+1]

	var root map[string]any
	if err := json.Unmarshal(payload, &root); err != nil {
		return false, false
	}

	readBool := func(v any) (bool, bool) {
		switch t := v.(type) {
		case bool:
			return t, true
		case float64:
			return t != 0, true
		case string:
			s := strings.TrimSpace(strings.ToLower(t))
			if s == "1" || s == "true" {
				return true, true
			}
			if s == "0" || s == "false" {
				return false, true
			}
			return false, false
		default:
			return false, false
		}
	}

	if appV, ok := root["app"]; ok {
		if appM, ok := appV.(map[string]any); ok {
			if v, ok := appM["dark_color_mode"]; ok {
				return readBool(v)
			}
		}
	}
	if v, ok := root["dark_color_mode"]; ok {
		return readBool(v)
	}
	return false, false
}

func detectCPDarkModeFromConfig() (bool, bool) {
	appData := strings.TrimSpace(os.Getenv("APPDATA"))
	if appData == "" {
		return false, false
	}

	patterns := []string{
		filepath.Join(appData, "Creality", "CrealityPrint", "*", "CrealityPrint.conf"),
		filepath.Join(appData, "CrealityPrint", "*", "CrealityPrint.conf"),
		filepath.Join(appData, "Creality", "CrealityPrint", "CrealityPrint.conf"),
		filepath.Join(appData, "CrealityPrint", "CrealityPrint.conf"),
	}

	var bestPath string
	var bestMod time.Time
	for _, p := range patterns {
		matches, _ := filepath.Glob(p)
		for _, m := range matches {
			fi, err := os.Stat(m)
			if err != nil || fi.IsDir() {
				continue
			}
			if bestPath == "" || fi.ModTime().After(bestMod) {
				bestPath = m
				bestMod = fi.ModTime()
			}
		}
	}
	if bestPath == "" {
		return false, false
	}

	b, err := os.ReadFile(bestPath)
	if err != nil {
		return false, false
	}
	return parseDarkColorModeFromConfigBytes(b)
}

func buildTheme() uiTheme {
	switch gThemeMode.Load() {
	case 0:
		return uiTheme{
			isLight:          true,
			bg:               rgb(0xFF, 0xFF, 0xFF),
			text:             rgb(0x1B, 0x1F, 0x24),
			textDisabled:     rgb(0xA0, 0xA7, 0xB4),
			btnBorder:        rgb(0xC7, 0xCD, 0xD6),
			btnBorderHover:   rgb(0x9A, 0xA3, 0xB2),
			btnBorderPressed: rgb(0x15, 0xBF, 0x59),
			btnFill:          rgb(0xFF, 0xFF, 0xFF),
			btnFillHover:     rgb(0xF1, 0xF4, 0xF8),
			btnFillPressed:   rgb(0xE7, 0xEC, 0xF4),
			btnFillDisabled:  rgb(0xFF, 0xFF, 0xFF),
			progressTrack:    rgb(0xD2, 0xD7, 0xE3),
			progressFill:     rgb(0x15, 0xBF, 0x59),
		}
	case 1:
		return uiTheme{
			isLight:          false,
			bg:               rgb(0x2B, 0x36, 0x45),
			text:             rgb(0xE7, 0xEC, 0xF4),
			textDisabled:     rgb(0x8A, 0x95, 0xA6),
			btnBorder:        rgb(0x55, 0x5F, 0x6E),
			btnBorderHover:   rgb(0x7D, 0x87, 0x95),
			btnBorderPressed: rgb(0x15, 0xBF, 0x59),
			btnFill:          rgb(0x2B, 0x36, 0x45),
			btnFillHover:     rgb(0x35, 0x42, 0x55),
			btnFillPressed:   rgb(0x23, 0x2D, 0x3B),
			btnFillDisabled:  rgb(0x2B, 0x36, 0x45),
			progressTrack:    rgb(0x3A, 0x43, 0x50),
			progressFill:     rgb(0x15, 0xBF, 0x59),
		}
	}

	if isDark, ok := detectCPDarkModeFromConfig(); ok {
		if !isDark {
			return uiTheme{
				isLight:          true,
				bg:               rgb(0xFF, 0xFF, 0xFF),
				text:             rgb(0x1B, 0x1F, 0x24),
				textDisabled:     rgb(0xA0, 0xA7, 0xB4),
				btnBorder:        rgb(0xC7, 0xCD, 0xD6),
				btnBorderHover:   rgb(0x9A, 0xA3, 0xB2),
				btnBorderPressed: rgb(0x15, 0xBF, 0x59),
				btnFill:          rgb(0xFF, 0xFF, 0xFF),
				btnFillHover:     rgb(0xF1, 0xF4, 0xF8),
				btnFillPressed:   rgb(0xE7, 0xEC, 0xF4),
				btnFillDisabled:  rgb(0xFF, 0xFF, 0xFF),
				progressTrack:    rgb(0xD2, 0xD7, 0xE3),
				progressFill:     rgb(0x15, 0xBF, 0x59),
			}
		}
		return uiTheme{
			isLight:          false,
			bg:               rgb(0x2B, 0x36, 0x45),
			text:             rgb(0xE7, 0xEC, 0xF4),
			textDisabled:     rgb(0x8A, 0x95, 0xA6),
			btnBorder:        rgb(0x55, 0x5F, 0x6E),
			btnBorderHover:   rgb(0x7D, 0x87, 0x95),
			btnBorderPressed: rgb(0x15, 0xBF, 0x59),
			btnFill:          rgb(0x2B, 0x36, 0x45),
			btnFillHover:     rgb(0x35, 0x42, 0x55),
			btnFillPressed:   rgb(0x23, 0x2D, 0x3B),
			btnFillDisabled:  rgb(0x2B, 0x36, 0x45),
			progressTrack:    rgb(0x3A, 0x43, 0x50),
			progressFill:     rgb(0x15, 0xBF, 0x59),
		}
	}

	if detectSystemLightTheme() {
		return uiTheme{
			isLight:          true,
			bg:               rgb(0xFF, 0xFF, 0xFF),
			text:             rgb(0x1B, 0x1F, 0x24),
			textDisabled:     rgb(0xA0, 0xA7, 0xB4),
			btnBorder:        rgb(0xC7, 0xCD, 0xD6),
			btnBorderHover:   rgb(0x9A, 0xA3, 0xB2),
			btnBorderPressed: rgb(0x15, 0xBF, 0x59),
			btnFill:          rgb(0xFF, 0xFF, 0xFF),
			btnFillHover:     rgb(0xF1, 0xF4, 0xF8),
			btnFillPressed:   rgb(0xE7, 0xEC, 0xF4),
			btnFillDisabled:  rgb(0xFF, 0xFF, 0xFF),
			progressTrack:    rgb(0xD2, 0xD7, 0xE3),
			progressFill:     rgb(0x15, 0xBF, 0x59),
		}
	}

	return uiTheme{
		isLight:          false,
		bg:               rgb(0x2B, 0x36, 0x45),
		text:             rgb(0xE7, 0xEC, 0xF4),
		textDisabled:     rgb(0x8A, 0x95, 0xA6),
		btnBorder:        rgb(0x55, 0x5F, 0x6E),
		btnBorderHover:   rgb(0x7D, 0x87, 0x95),
		btnBorderPressed: rgb(0x15, 0xBF, 0x59),
		btnFill:          rgb(0x2B, 0x36, 0x45),
		btnFillHover:     rgb(0x35, 0x42, 0x55),
		btnFillPressed:   rgb(0x23, 0x2D, 0x3B),
		btnFillDisabled:  rgb(0x2B, 0x36, 0x45),
		progressTrack:    rgb(0x3A, 0x43, 0x50),
		progressFill:     rgb(0x15, 0xBF, 0x59),
	}
}

type wndClassExW struct {
	cbSize        uint32
	style         uint32
	lpfnWndProc   uintptr
	cbClsExtra    int32
	cbWndExtra    int32
	hInstance     uintptr
	hIcon         uintptr
	hCursor       uintptr
	hbrBackground uintptr
	lpszMenuName  *uint16
	lpszClassName *uint16
	hIconSm       uintptr
}

type msg struct {
	hwnd    uintptr
	message uint32
	wParam  uintptr
	lParam  uintptr
	time    uint32
	pt      struct {
		x int32
		y int32
	}
}

type rect struct {
	left   int32
	top    int32
	right  int32
	bottom int32
}

type point struct {
	x int32
	y int32
}

type paintStruct struct {
	hdc         uintptr
	fErase      int32
	rcPaint     rect
	fRestore    int32
	fIncUpdate  int32
	rgbReserved [32]byte
}

type trackMouseEvent struct {
	cbSize      uint32
	dwFlags     uint32
	hwndTrack   uintptr
	dwHoverTime uint32
}

type initCommonControlsEx struct {
	dwSize uint32
	dwICC  uint32
}

type logFontW struct {
	lfHeight         int32
	lfWidth          int32
	lfEscapement     int32
	lfOrientation    int32
	lfWeight         int32
	lfItalic         byte
	lfUnderline      byte
	lfStrikeOut      byte
	lfCharSet        byte
	lfOutPrecision   byte
	lfClipPrecision  byte
	lfQuality        byte
	lfPitchAndFamily byte
	lfFaceName       [32]uint16
}

type processEntry32W struct {
	dwSize              uint32
	cntUsage            uint32
	th32ProcessID       uint32
	th32DefaultHeapID   uintptr
	th32ModuleID        uint32
	cntThreads          uint32
	th32ParentProcessID uint32
	pcPriClassBase      int32
	dwFlags             uint32
	szExeFile           [260]uint16
}

type filetime struct {
	dwLowDateTime  uint32
	dwHighDateTime uint32
}

type rmUniqueProcess struct {
	dwProcessID      uint32
	processStartTime filetime
}

type rmProcessInfo struct {
	process             rmUniqueProcess
	strAppName          [256]uint16
	strServiceShortName [64]uint16
	applicationType     uint32
	appStatus           uint32
	tssessionId         uint32
	bRestartable        int32
}

type deltaInput struct {
	lpStart  uintptr
	uSize    uintptr
	editable int32
}

type deltaOutput struct {
	lpStart uintptr
	uSize   uintptr
}

type uiTheme struct {
	isLight          bool
	bg               uintptr
	text             uintptr
	textDisabled     uintptr
	btnBorder        uintptr
	btnBorderHover   uintptr
	btnBorderPressed uintptr
	btnFill          uintptr
	btnFillHover     uintptr
	btnFillPressed   uintptr
	btnFillDisabled  uintptr
	progressTrack    uintptr
	progressFill     uintptr
}

var (
	errCanceled                = errors.New("canceled")
	errDeltaNoop               = errors.New("delta produced no patched files")
	errDeltaCriticalNotPatched = errors.New("delta did not patch critical file")
	gCancelRequested           atomic.Bool
	gProgressPercent           atomic.Int32

	gLanguage string

	gInstallHwnd         uintptr
	gInstallTitle        uintptr
	gInstallStatus       uintptr
	gInstallCancel       uintptr
	gInstallClose        uintptr
	gInstallProgressRect rect

	gIncompleteHwnd    uintptr
	gIncompleteManual  uintptr
	gIncompleteConfirm uintptr
	gIncompleteText    uintptr

	gProgressText      atomic.Value // string
	gManualURL         atomic.Value // string
	gIncompleteMessage atomic.Value // string

	gInstallBgBrush uintptr
	gInstallFont    uintptr
	gInstallDpi     int32 = 96
	gTheme          uiTheme
	gThemeMode      atomic.Int32

	gCancelBtnOldWndProc uintptr
	gCancelBtnHover      bool
	gCancelBtnDown       bool
	gCancelBtnTracking   bool

	gCloseBtnOldWndProc uintptr
	gCloseBtnHover      bool
	gCloseBtnDown       bool
	gCloseBtnTracking   bool

	gBtnMu     sync.Mutex
	gBtnStates = map[uintptr]*btnState{}

	gEnumPidMu  sync.Mutex
	gEnumPidSet map[uint32]struct{}
)

type cancelReader struct{ r io.Reader }

func (cr cancelReader) Read(p []byte) (int, error) {
	if isCanceled() {
		return 0, errCanceled
	}
	return cr.r.Read(p)
}

func toUTF16Ptr(s string) *uint16 {
	p, _ := syscall.UTF16PtrFromString(s)
	return p
}

func hInstance() uintptr {
	h, _, _ := procGetModuleHandleW.Call(0)
	return h
}

func rgb(r, g, b byte) uintptr {
	return uintptr(uint32(r) | (uint32(g) << 8) | (uint32(b) << 16))
}

func initCommonControls() {
	var icc initCommonControlsEx
	icc.dwSize = uint32(unsafe.Sizeof(icc))
	icc.dwICC = iccProgressClass
	procInitCommonControlsEx.Call(uintptr(unsafe.Pointer(&icc)))
}

// scaleDPI scales a logical pixel value (defined at 96 DPI baseline) to physical pixels.
func scaleDPI(value, dpi int32) int32 {
	if dpi <= 0 {
		return value
	}
	return value * dpi / 96
}

// getWindowDpi returns the DPI for the given window.
// Pass 0 (or an invalid hwnd) to query the system (primary monitor) DPI.
func getWindowDpi(hwnd uintptr) int32 {
	var dpi uintptr
	if hwnd != 0 {
		dpi, _, _ = procGetDpiForWindow.Call(hwnd)
	}
	if dpi == 0 {
		dpi, _, _ = procGetDpiForSystem.Call()
	}
	if dpi == 0 {
		return 96
	}
	return int32(dpi)
}

// createDpiFont creates a Segoe UI font scaled to the given DPI.
func createDpiFont(dpi int32) uintptr {
	if dpi <= 0 {
		dpi = 96
	}
	lf := logFontW{}
	lf.lfHeight = -scaleDPI(13, dpi) // ~9.75pt — matches Windows default UI font at 96 DPI
	lf.lfWeight = 400
	lf.lfCharSet = 1   // DEFAULT_CHARSET
	lf.lfQuality = 5   // CLEARTYPE_QUALITY
	faceName := syscall.StringToUTF16("Segoe UI")
	n := len(faceName)
	if n > 32 {
		n = 32
	}
	copy(lf.lfFaceName[:], faceName[:n])
	h, _, _ := procCreateFontIndirectW.Call(uintptr(unsafe.Pointer(&lf)))
	if h != 0 {
		return h
	}
	// Fallback: stock default GUI font
	h, _, _ = procGetStockObject.Call(defaultGuiFont)
	return h
}

// layoutInstallWindow positions all child controls of the install-progress window
// using DPI-scaled dimensions. Safe to call during WM_CREATE and WM_DPICHANGED.
func layoutInstallWindow(hwnd uintptr, dpi int32) {
	var rc rect
	procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
	width := rc.right - rc.left
	height := rc.bottom - rc.top

	cornerRadius := scaleDPI(10, dpi)
	rgn, _, _ := procCreateRoundRectRgn.Call(0, 0, uintptr(width), uintptr(height), uintptr(cornerRadius), uintptr(cornerRadius))
	if rgn != 0 {
		procSetWindowRgn.Call(hwnd, rgn, 1)
		procDeleteObject.Call(rgn)
	}

	padding := scaleDPI(28, dpi)
	titleY := scaleDPI(14, dpi)
	titleH := scaleDPI(20, dpi)
	statusY := scaleDPI(78, dpi)
	statusH := scaleDPI(22, dpi)
	barY := statusY + statusH + scaleDPI(18, dpi)
	barH := scaleDPI(16, dpi)
	btnY := barY + barH + scaleDPI(40, dpi)
	btnW := scaleDPI(140, dpi)
	btnH := scaleDPI(36, dpi)
	closeW := scaleDPI(installCloseWidth, dpi)
	closeH := scaleDPI(28, dpi)
	rightPad := scaleDPI(10, dpi)
	titlebarClosePad := scaleDPI(4, dpi)

	if gInstallTitle != 0 {
		procMoveWindow.Call(gInstallTitle, uintptr(padding), uintptr(titleY),
			uintptr(width-2*padding-closeW), uintptr(titleH), 1)
		setControlFont(gInstallTitle)
	}
	if gInstallClose != 0 {
		procMoveWindow.Call(gInstallClose, uintptr(width-rightPad-closeW), uintptr(titleY-titlebarClosePad),
			uintptr(closeW), uintptr(closeH), 1)
		setControlFont(gInstallClose)
	}
	if gInstallStatus != 0 {
		procMoveWindow.Call(gInstallStatus, uintptr(padding), uintptr(statusY),
			uintptr(width-2*padding), uintptr(statusH), 1)
		setControlFont(gInstallStatus)
	}
	if gInstallCancel != 0 {
		btnX := (width - btnW) / 2
		procMoveWindow.Call(gInstallCancel, uintptr(btnX), uintptr(btnY),
			uintptr(btnW), uintptr(btnH), 1)
		setControlFont(gInstallCancel)
	}

	gInstallProgressRect = rect{
		left:   padding,
		top:    barY,
		right:  width - padding,
		bottom: barY + barH,
	}
}

// layoutIncompleteWindow positions all child controls of the incomplete-update window
// using DPI-scaled dimensions. Safe to call during WM_CREATE and WM_DPICHANGED.
func layoutIncompleteWindow(hwnd uintptr, dpi int32) {
	var rc rect
	procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
	width := rc.right - rc.left
	height := rc.bottom - rc.top

	cornerRadius := scaleDPI(10, dpi)
	rgn, _, _ := procCreateRoundRectRgn.Call(0, 0, uintptr(width), uintptr(height), uintptr(cornerRadius), uintptr(cornerRadius))
	if rgn != 0 {
		procSetWindowRgn.Call(hwnd, rgn, 1)
		procDeleteObject.Call(rgn)
	}

	padding := scaleDPI(28, dpi)
	titleY := scaleDPI(14, dpi)
	titleH := scaleDPI(20, dpi)
	closeW := scaleDPI(installCloseWidth, dpi)
	closeH := scaleDPI(28, dpi)
	rightPad := scaleDPI(10, dpi)
	textY := scaleDPI(78, dpi)
	textH := scaleDPI(56, dpi)
	btnY := scaleDPI(168, dpi)
	btnW := scaleDPI(140, dpi)
	btnH := scaleDPI(36, dpi)
	gap := scaleDPI(16, dpi)

	if gInstallTitle != 0 {
		procMoveWindow.Call(gInstallTitle, uintptr(padding), uintptr(titleY),
			uintptr(width-2*padding-closeW), uintptr(titleH), 1)
		setControlFont(gInstallTitle)
	}
	if gInstallClose != 0 {
		procMoveWindow.Call(gInstallClose, uintptr(width-rightPad-closeW), uintptr(scaleDPI(10, dpi)),
			uintptr(closeW), uintptr(closeH), 1)
		setControlFont(gInstallClose)
	}
	if gIncompleteText != 0 {
		procMoveWindow.Call(gIncompleteText, uintptr(padding), uintptr(textY),
			uintptr(width-2*padding), uintptr(textH), 1)
		setControlFont(gIncompleteText)
	}
	totalBtnW := btnW*2 + gap
	btnX := (width - totalBtnW) / 2
	if gIncompleteManual != 0 {
		procMoveWindow.Call(gIncompleteManual, uintptr(btnX), uintptr(btnY),
			uintptr(btnW), uintptr(btnH), 1)
		setControlFont(gIncompleteManual)
	}
	if gIncompleteConfirm != 0 {
		procMoveWindow.Call(gIncompleteConfirm, uintptr(btnX+btnW+gap), uintptr(btnY),
			uintptr(btnW), uintptr(btnH), 1)
		setControlFont(gIncompleteConfirm)
	}
}

func ensureInstallUIResources() {
	if gInstallBgBrush == 0 {
		gTheme = buildTheme()
		h, _, _ := procCreateSolidBrush.Call(gTheme.bg)
		gInstallBgBrush = h
	}
	if gInstallFont == 0 {
		dpi := gInstallDpi
		if dpi <= 0 {
			dpi = 96
		}
		gInstallFont = createDpiFont(dpi)
	}
}

func postInstallProgress(percent int, statusText string) {
	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	gProgressPercent.Store(int32(percent))
	gProgressText.Store(statusText)
	if gInstallHwnd != 0 {
		procPostMessageW.Call(gInstallHwnd, wmAppProgress, 0, 0)
	}
}

func requestInstallClose() {
	if gInstallHwnd != 0 {
		procPostMessageW.Call(gInstallHwnd, wmAppDone, 0, 0)
	}
}

func openURL(url string) {
	if strings.TrimSpace(url) == "" {
		return
	}
	verb := toUTF16Ptr("open")
	u := toUTF16Ptr(url)
	procShellExecuteW.Call(0, uintptr(unsafe.Pointer(verb)), uintptr(unsafe.Pointer(u)), 0, 0, swShow)
}

func setControlFont(hwnd uintptr) {
	if hwnd == 0 || gInstallFont == 0 {
		return
	}
	procSendMessageW.Call(hwnd, wmSetFont, gInstallFont, 1)
}

func invalidateWindow(hwnd uintptr) {
	if hwnd == 0 {
		return
	}
	procInvalidateRect.Call(hwnd, 0, 1)
	procUpdateWindow.Call(hwnd)
}

func normalizePath(p string) string {
	s := strings.TrimSpace(p)
	if s == "" {
		return ""
	}
	s = filepath.Clean(s)
	return strings.ToLower(s)
}

func isPathInDir(p, dir string) bool {
	pn := normalizePath(p)
	dn := normalizePath(dir)
	if pn == "" || dn == "" {
		return false
	}
	if pn == dn {
		return true
	}
	if strings.HasPrefix(pn, dn+string(os.PathSeparator)) {
		return true
	}
	return false
}

func openProcess(desiredAccess uint32, pid uint32) uintptr {
	h, _, _ := procOpenProcess.Call(uintptr(desiredAccess), 0, uintptr(pid))
	return h
}

func closeHandle(h uintptr) {
	if h != 0 && h != invalidHandleValue {
		procCloseHandle.Call(h)
	}
}

func queryProcessImagePath(hProc uintptr) (string, bool) {
	if hProc == 0 {
		return "", false
	}
	buf := make([]uint16, 32768)
	size := uint32(len(buf))
	r, _, _ := procQueryFullProcessImageNameW.Call(hProc, 0, uintptr(unsafe.Pointer(&buf[0])), uintptr(unsafe.Pointer(&size)))
	if r == 0 || size == 0 {
		return "", false
	}
	return syscall.UTF16ToString(buf[:size]), true
}

func pidsUsingInstallDir(installDir string) []uint32 {
	snap, _, _ := procCreateToolhelp32Snapshot.Call(th32csSnapprocess, 0)
	if snap == invalidHandleValue || snap == 0 {
		return nil
	}
	defer closeHandle(snap)

	selfPID := uint32(0)
	if r, _, _ := procGetCurrentProcessId.Call(); r != 0 {
		selfPID = uint32(r)
	}

	var pe processEntry32W
	pe.dwSize = uint32(unsafe.Sizeof(pe))
	r, _, _ := procProcess32FirstW.Call(snap, uintptr(unsafe.Pointer(&pe)))
	var out []uint32
	for r != 0 {
		pid := pe.th32ProcessID
		if pid != 0 && pid != selfPID {
			hProc := openProcess(processQueryLimitedInformation|synchronize, pid)
			if hProc != 0 {
				if p, ok := queryProcessImagePath(hProc); ok && isPathInDir(p, installDir) {
					out = append(out, pid)
				}
				closeHandle(hProc)
			}
		}
		r, _, _ = procProcess32NextW.Call(snap, uintptr(unsafe.Pointer(&pe)))
	}
	return out
}

func topLevelRestartManagerPaths(installDir string) []string {
	entries, err := os.ReadDir(installDir)
	if err != nil {
		return nil
	}
	var out []string
	for _, e := range entries {
		if e.IsDir() {
			continue
		}
		name := strings.ToLower(e.Name())
		if !strings.HasSuffix(name, ".exe") && !strings.HasSuffix(name, ".dll") {
			continue
		}
		out = append(out, filepath.Join(installDir, e.Name()))
		if len(out) >= 128 {
			break
		}
	}
	return out
}

func pidsLockingPathsViaRestartManager(paths []string) []uint32 {
	if len(paths) == 0 {
		return nil
	}
	var session uint32
	key := make([]uint16, 33)
	r, _, _ := procRmStartSession.Call(uintptr(unsafe.Pointer(&session)), 0, uintptr(unsafe.Pointer(&key[0])))
	if r != 0 || session == 0 {
		return nil
	}
	defer procRmEndSession.Call(uintptr(session))

	ptrs := make([]*uint16, 0, len(paths))
	for _, p := range paths {
		if strings.TrimSpace(p) == "" {
			continue
		}
		pp, err := syscall.UTF16PtrFromString(p)
		if err != nil {
			continue
		}
		ptrs = append(ptrs, pp)
	}
	if len(ptrs) == 0 {
		return nil
	}

	r, _, _ = procRmRegisterResources.Call(
		uintptr(session),
		uintptr(uint32(len(ptrs))),
		uintptr(unsafe.Pointer(&ptrs[0])),
		0,
		0,
		0,
		0,
	)
	if r != 0 {
		return nil
	}

	var needed uint32
	var count uint32
	var reasons uint32
	r, _, _ = procRmGetList.Call(
		uintptr(session),
		uintptr(unsafe.Pointer(&needed)),
		uintptr(unsafe.Pointer(&count)),
		0,
		uintptr(unsafe.Pointer(&reasons)),
	)
	if r != errorMoreData || needed == 0 {
		return nil
	}

	infos := make([]rmProcessInfo, needed)
	count = needed
	r, _, _ = procRmGetList.Call(
		uintptr(session),
		uintptr(unsafe.Pointer(&needed)),
		uintptr(unsafe.Pointer(&count)),
		uintptr(unsafe.Pointer(&infos[0])),
		uintptr(unsafe.Pointer(&reasons)),
	)
	if r != 0 {
		return nil
	}
	out := make([]uint32, 0, count)
	for i := uint32(0); i < count; i++ {
		pid := infos[i].process.dwProcessID
		if pid != 0 {
			out = append(out, pid)
		}
	}
	return out
}

func pidsLockingInstallDir(installDir string) []uint32 {
	paths := topLevelRestartManagerPaths(installDir)
	return pidsLockingPathsViaRestartManager(paths)
}

func pidImagePath(pid uint32) string {
	h := openProcess(processQueryLimitedInformation, pid)
	if h == 0 {
		return ""
	}
	defer closeHandle(h)
	if p, ok := queryProcessImagePath(h); ok {
		return p
	}
	return ""
}

func postCloseToWindows(pids []uint32) {
	set := make(map[uint32]struct{}, len(pids))
	for _, pid := range pids {
		set[pid] = struct{}{}
	}
	gEnumPidMu.Lock()
	gEnumPidSet = set
	gEnumPidMu.Unlock()

	cb := syscall.NewCallback(func(hwnd uintptr, lParam uintptr) uintptr {
		var pid uint32
		procGetWindowThreadProcessId.Call(hwnd, uintptr(unsafe.Pointer(&pid)))
		gEnumPidMu.Lock()
		_, ok := gEnumPidSet[pid]
		gEnumPidMu.Unlock()
		if ok {
			procPostMessageW.Call(hwnd, wmClose, 0, 0)
		}
		return 1
	})
	procEnumWindows.Call(cb, 0)

	gEnumPidMu.Lock()
	gEnumPidSet = nil
	gEnumPidMu.Unlock()
}

func waitForExit(pids []uint32, timeout time.Duration) []uint32 {
	deadline := time.Now().Add(timeout)
	alive := make([]uint32, 0, len(pids))
	for {
		alive = alive[:0]
		for _, pid := range pids {
			h := openProcess(synchronize, pid)
			if h == 0 {
				continue
			}
			r, _, _ := procWaitForSingleObject.Call(h, 0)
			closeHandle(h)
			if r == waitTimeout {
				alive = append(alive, pid)
			}
		}
		if len(alive) == 0 || time.Now().After(deadline) {
			break
		}
		time.Sleep(200 * time.Millisecond)
	}
	return alive
}

func terminatePids(pids []uint32) {
	for _, pid := range pids {
		h := openProcess(processTerminate|synchronize|processQueryLimitedInformation, pid)
		if h == 0 {
			continue
		}
		procTerminateProcess.Call(h, 1)
		procWaitForSingleObject.Call(h, 1500)
		closeHandle(h)
	}
}

func releaseInstallDirLocks(log func(args ...interface{}), installDir string) {
	pidSet := map[uint32]struct{}{}
	for _, pid := range pidsUsingInstallDir(installDir) {
		pidSet[pid] = struct{}{}
	}
	for _, pid := range pidsLockingInstallDir(installDir) {
		pidSet[pid] = struct{}{}
	}
	if len(pidSet) == 0 {
		return
	}

	pids := make([]uint32, 0, len(pidSet))
	for pid := range pidSet {
		pids = append(pids, pid)
	}

	inDir := map[uint32]bool{}
	log("install dir is in use, candidate processes:")
	for _, pid := range pids {
		path := pidImagePath(pid)
		in := isPathInDir(path, installDir)
		inDir[pid] = in
		log("  pid:", pid, "path:", path, "inDir:", in)
	}

	postCloseToWindows(pids)
	alive := waitForExit(pids, 8*time.Second)
	if len(alive) == 0 {
		return
	}

	var kill []uint32
	for _, pid := range alive {
		if inDir[pid] {
			kill = append(kill, pid)
		}
	}
	if len(kill) > 0 {
		log("processes still running in install dir, terminating:", kill)
		terminatePids(kill)
	}
	_ = waitForExit(alive, 8*time.Second)
}

type btnState struct {
	oldWndProc uintptr
	hover      bool
	down       bool
	tracking   bool
	isClose    bool
}

func getBtnState(hwnd uintptr) *btnState {
	gBtnMu.Lock()
	defer gBtnMu.Unlock()
	return gBtnStates[hwnd]
}

func setBtnState(hwnd uintptr, s *btnState) {
	gBtnMu.Lock()
	gBtnStates[hwnd] = s
	gBtnMu.Unlock()
}

func deleteBtnState(hwnd uintptr) {
	gBtnMu.Lock()
	delete(gBtnStates, hwnd)
	gBtnMu.Unlock()
}

func drawButton(hwnd uintptr, hdc uintptr, st *btnState) {
	if gInstallBgBrush != 0 {
		var rcPaint rect
		procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rcPaint)))
		procFillRect.Call(hdc, uintptr(unsafe.Pointer(&rcPaint)), gInstallBgBrush)
	}

	var rc rect
	procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))

	enabled := true
	if r, _, _ := procIsWindowEnabled.Call(hwnd); r == 0 {
		enabled = false
	}

	borderColor := gTheme.btnBorder
	textColor := gTheme.text
	fillColor := gTheme.btnFill

	if st.isClose {
		// 关闭按钮：永远无边框，背景色与窗口一致
		fillColor = gTheme.bg
		borderColor = fillColor // 边框与背景一致，无边框效果
		if !enabled {
			textColor = gTheme.textDisabled
		} else if st.down {
			// 按下时背景稍微变化
			if gTheme.isLight {
				fillColor = rgb(0xE7, 0xEC, 0xF4)
			} else {
				fillColor = rgb(0x35, 0x42, 0x55)
			}
			borderColor = fillColor // 保持边框与背景一致
		} else if st.hover {
			// 悬停时背景稍微变化
			if gTheme.isLight {
				fillColor = rgb(0xF1, 0xF4, 0xF8)
			} else {
				fillColor = rgb(0x3A, 0x45, 0x54)
			}
			borderColor = fillColor // 保持边框与背景一致
		}
	} else {
		if !enabled {
			borderColor = gTheme.btnBorder
			textColor = gTheme.textDisabled
			fillColor = gTheme.btnFillDisabled
		} else if st.down {
			borderColor = gTheme.btnBorderPressed
			fillColor = gTheme.btnFillPressed
		} else if st.hover {
			borderColor = gTheme.btnBorderHover
		}
	}

	brush, _, _ := procCreateSolidBrush.Call(fillColor)
	pen, _, _ := procCreatePen.Call(psSolid, 1, borderColor)
	oldBrush, _, _ := procSelectObject.Call(hdc, brush)
	oldPen, _, _ := procSelectObject.Call(hdc, pen)

	if st.isClose {
		// 关闭按钟不需要圆角，直接绘制矩形
		procRectangle.Call(hdc, uintptr(rc.left), uintptr(rc.top), uintptr(rc.right), uintptr(rc.bottom))
	} else {
		// 其他按钟使用 DPI-缩放圆角
		radius := int32(scaleDPI(10, gInstallDpi))
		procRoundRect.Call(hdc, uintptr(rc.left), uintptr(rc.top), uintptr(rc.right), uintptr(rc.bottom), uintptr(radius), uintptr(radius))
	}

	procSelectObject.Call(hdc, oldPen)
	procSelectObject.Call(hdc, oldBrush)
	procDeleteObject.Call(pen)
	procDeleteObject.Call(brush)

	procSetBkMode.Call(hdc, bkTransparent)
	procSetTextColor.Call(hdc, textColor)

	var textPtr *uint16
	if st.isClose {
		// 关闭按钟：使用 GDI 绘制精确的 X 符号（尺寸随 DPI 缩放）
		centerX := (rc.left + rc.right) / 2
		centerY := (rc.top + rc.bottom) / 2
		halfSize := scaleDPI(5, gInstallDpi)
		penWidth := scaleDPI(2, gInstallDpi)
		if penWidth < 1 {
			penWidth = 1
		}
	
		closePen, _, _ := procCreatePen.Call(psSolid, uintptr(penWidth), textColor)
		oldClosePen, _, _ := procSelectObject.Call(hdc, closePen)
	
		// 绘制第一条线：左上到右下
		procMoveToEx.Call(hdc, uintptr(centerX-halfSize), uintptr(centerY-halfSize), 0)
		procLineTo.Call(hdc, uintptr(centerX+halfSize), uintptr(centerY+halfSize))
	
		// 绘制第二条线：右上到左下
		procMoveToEx.Call(hdc, uintptr(centerX+halfSize), uintptr(centerY-halfSize), 0)
		procLineTo.Call(hdc, uintptr(centerX-halfSize), uintptr(centerY+halfSize))
	
		procSelectObject.Call(hdc, oldClosePen)
		procDeleteObject.Call(closePen)
	} else {
		buf := make([]uint16, 256)
		n, _, _ := procGetWindowTextW.Call(hwnd, uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
		if n == 0 {
			textPtr = toUTF16Ptr("")
		} else {
			textPtr = &buf[0]
		}
		// Select the DPI-scaled font so DrawTextW renders at the correct size.
		var oldFont uintptr
		if gInstallFont != 0 {
			oldFont, _, _ = procSelectObject.Call(hdc, gInstallFont)
		}
		drawRc := rc
		procDrawTextW.Call(hdc, uintptr(unsafe.Pointer(textPtr)), ^uintptr(0), uintptr(unsafe.Pointer(&drawRc)), dtCenter|dtVCenter|dtSingleLine)
		if oldFont != 0 {
			procSelectObject.Call(hdc, oldFont)
		}
	}
}

func themedBtnWndProc(hwnd uintptr, msgID uint32, wParam, lParam uintptr) uintptr {
	st := getBtnState(hwnd)
	if st == nil {
		ret, _, _ := procDefWindowProcW.Call(hwnd, uintptr(msgID), wParam, lParam)
		return ret
	}

	switch msgID {
	case wmMouseMove:
		if !st.hover {
			st.hover = true
			invalidateWindow(hwnd)
		}
		if !st.tracking {
			tme := trackMouseEvent{
				cbSize:    uint32(unsafe.Sizeof(trackMouseEvent{})),
				dwFlags:   tmeLeave,
				hwndTrack: hwnd,
			}
			procTrackMouseEvent.Call(uintptr(unsafe.Pointer(&tme)))
			st.tracking = true
		}

	case wmMouseLeave:
		st.hover = false
		st.down = false
		st.tracking = false
		invalidateWindow(hwnd)
		return 0

	case wmLButtonDown:
		st.down = true
		invalidateWindow(hwnd)
		return 0 // 阻止默认绘制

	case wmLButtonUp:
		if st.down {
			st.down = false
			invalidateWindow(hwnd)
			// 发送 BN_CLICKED 通知给父窗口。必须使用按钮自身的控件 ID，
			// 之前这里写死了 idClose，导致所有子类化按钮（手动下载、确认、关闭）
			// 点击后都触发父窗口的 idClose 分支，直接关窗退出。
			ctrlID, _, _ := procGetDlgCtrlID.Call(hwnd)
			parentHwnd, _, _ := procGetParent.Call(hwnd)
			wParam := uintptr(bnClicked)<<16 | (ctrlID & 0xFFFF)
			procPostMessageW.Call(parentHwnd, wmCommand, wParam, hwnd)
		}
		return 0 // 阻止默认绘制

	case wmPaint:
		var ps paintStruct
		hdc, _, _ := procBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		drawButton(hwnd, hdc, st)
		procEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0

	case wmDestroy:
		deleteBtnState(hwnd)
	}

	ret, _, _ := procCallWindowProcW.Call(st.oldWndProc, hwnd, uintptr(msgID), wParam, lParam)
	return ret
}

func subclassThemedButton(hwnd uintptr, isClose bool) {
	if hwnd == 0 {
		return
	}
	old, _, _ := procGetWindowLongPtrW.Call(hwnd, gwlpWndProc)
	st := &btnState{oldWndProc: old, isClose: isClose}
	setBtnState(hwnd, st)
	procSetWindowLongPtrW.Call(hwnd, gwlpWndProc, syscall.NewCallback(themedBtnWndProc))
}

func cancelBtnWndProc(hwnd uintptr, msgID uint32, wParam, lParam uintptr) uintptr {
	switch msgID {
	case wmMouseMove:
		if !gCancelBtnHover {
			gCancelBtnHover = true
			invalidateWindow(hwnd)
		}
		if !gCancelBtnTracking {
			tme := trackMouseEvent{
				cbSize:    uint32(unsafe.Sizeof(trackMouseEvent{})),
				dwFlags:   tmeLeave,
				hwndTrack: hwnd,
			}
			procTrackMouseEvent.Call(uintptr(unsafe.Pointer(&tme)))
			gCancelBtnTracking = true
		}

	case wmMouseLeave:
		gCancelBtnHover = false
		gCancelBtnDown = false
		gCancelBtnTracking = false
		invalidateWindow(hwnd)
		return 0

	case wmLButtonDown:
		gCancelBtnDown = true
		invalidateWindow(hwnd)

	case wmLButtonUp:
		if gCancelBtnDown {
			gCancelBtnDown = false
			invalidateWindow(hwnd)
		}

	case wmPaint:
		var ps paintStruct
		hdc, _, _ := procBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))

		if gInstallBgBrush != 0 {
			procFillRect.Call(hdc, uintptr(unsafe.Pointer(&ps.rcPaint)), gInstallBgBrush)
		}

		var rc rect
		procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))

		enabled := true
		if r, _, _ := procIsWindowEnabled.Call(hwnd); r == 0 {
			enabled = false
		}

		borderColor := gTheme.btnBorder
		textColor := gTheme.text
		fillColor := gTheme.btnFill

		if !enabled {
			borderColor = gTheme.btnBorder
			textColor = gTheme.textDisabled
			fillColor = gTheme.btnFillDisabled
		} else if gCancelBtnDown {
			borderColor = gTheme.btnBorderPressed
			fillColor = gTheme.btnFillPressed
		} else if gCancelBtnHover {
			borderColor = gTheme.btnBorderHover
		}

		brush, _, _ := procCreateSolidBrush.Call(fillColor)
		pen, _, _ := procCreatePen.Call(psSolid, 1, borderColor)
		oldBrush, _, _ := procSelectObject.Call(hdc, brush)
		oldPen, _, _ := procSelectObject.Call(hdc, pen)

		radius := int32(scaleDPI(10, gInstallDpi)) // DPI-缩放圆角
		procRoundRect.Call(hdc, uintptr(rc.left), uintptr(rc.top), uintptr(rc.right), uintptr(rc.bottom), uintptr(radius), uintptr(radius))
		
		procSelectObject.Call(hdc, oldPen)
		procSelectObject.Call(hdc, oldBrush)
		procDeleteObject.Call(pen)
		procDeleteObject.Call(brush)
		
		procSetBkMode.Call(hdc, bkTransparent)
		procSetTextColor.Call(hdc, textColor)
		
		text := toUTF16Ptr(getText("cancel"))
		drawRc := rc
		// Select the DPI-scaled font so DrawTextW renders at the correct size.
		var oldFont uintptr
		if gInstallFont != 0 {
			oldFont, _, _ = procSelectObject.Call(hdc, gInstallFont)
		}
		procDrawTextW.Call(hdc, uintptr(unsafe.Pointer(text)), ^uintptr(0), uintptr(unsafe.Pointer(&drawRc)), dtCenter|dtVCenter|dtSingleLine)
		if oldFont != 0 {
			procSelectObject.Call(hdc, oldFont)
		}

		procEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0
	}

	ret, _, _ := procCallWindowProcW.Call(gCancelBtnOldWndProc, hwnd, uintptr(msgID), wParam, lParam)
	return ret
}

func subclassCancelButton(hwnd uintptr) {
	if hwnd == 0 {
		return
	}
	old, _, _ := procGetWindowLongPtrW.Call(hwnd, gwlpWndProc)
	gCancelBtnOldWndProc = old
	procSetWindowLongPtrW.Call(hwnd, gwlpWndProc, syscall.NewCallback(cancelBtnWndProc))
}

func closeBtnWndProc(hwnd uintptr, msgID uint32, wParam, lParam uintptr) uintptr {
	switch msgID {
	case wmMouseMove:
		if !gCloseBtnHover {
			gCloseBtnHover = true
			invalidateWindow(hwnd)
		}
		if !gCloseBtnTracking {
			tme := trackMouseEvent{
				cbSize:    uint32(unsafe.Sizeof(trackMouseEvent{})),
				dwFlags:   tmeLeave,
				hwndTrack: hwnd,
			}
			procTrackMouseEvent.Call(uintptr(unsafe.Pointer(&tme)))
			gCloseBtnTracking = true
		}

	case wmMouseLeave:
		gCloseBtnHover = false
		gCloseBtnDown = false
		gCloseBtnTracking = false
		invalidateWindow(hwnd)
		return 0

	case wmLButtonDown:
		gCloseBtnDown = true
		invalidateWindow(hwnd)
		return 0 // 阻止默认绘制

	case wmLButtonUp:
		if gCloseBtnDown {
			gCloseBtnDown = false
			invalidateWindow(hwnd)
			// 发送 BN_CLICKED 通知给父窗口
			parentHwnd, _, _ := procGetParent.Call(hwnd)
			wParam := uintptr(bnClicked)<<16 | uintptr(idClose)
			procPostMessageW.Call(parentHwnd, wmCommand, wParam, hwnd)
		}
		return 0 // 阻止默认绘制

	case wmPaint:
		var ps paintStruct
		hdc, _, _ := procBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		if gInstallBgBrush != 0 {
			procFillRect.Call(hdc, uintptr(unsafe.Pointer(&ps.rcPaint)), gInstallBgBrush)
		}

		var rc rect
		procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))

		enabled := true
		if r, _, _ := procIsWindowEnabled.Call(hwnd); r == 0 {
			enabled = false
		}

		borderColor := gTheme.bg // 关闭按钮不需要边框
		textColor := gTheme.text
		fillColor := gTheme.bg // 关闭按钮背景色与弹窗背景一致

		if !enabled {
			textColor = gTheme.textDisabled
		} else if gCloseBtnDown {
			fillColor = gTheme.btnFillPressed
		} else if gCloseBtnHover {
			fillColor = gTheme.btnFillHover
		}

		brush, _, _ := procCreateSolidBrush.Call(fillColor)
		pen, _, _ := procCreatePen.Call(psSolid, 1, borderColor)
		oldBrush, _, _ := procSelectObject.Call(hdc, brush)
		oldPen, _, _ := procSelectObject.Call(hdc, pen)

		// 关闭按钮不需要圆角，直接绘制矩形
		procRectangle.Call(hdc, uintptr(rc.left), uintptr(rc.top), uintptr(rc.right), uintptr(rc.bottom))

		procSelectObject.Call(hdc, oldPen)
		procSelectObject.Call(hdc, oldBrush)
		procDeleteObject.Call(pen)
		procDeleteObject.Call(brush)

		procSetBkMode.Call(hdc, bkTransparent)
		procSetTextColor.Call(hdc, textColor)

		text := toUTF16Ptr("×")
		drawRc := rc
		// Select the DPI-scaled font so × renders at the correct size.
		var oldFont uintptr
		if gInstallFont != 0 {
			oldFont, _, _ = procSelectObject.Call(hdc, gInstallFont)
		}
		procDrawTextW.Call(hdc, uintptr(unsafe.Pointer(text)), ^uintptr(0), uintptr(unsafe.Pointer(&drawRc)), dtCenter|dtVCenter|dtSingleLine)
		if oldFont != 0 {
			procSelectObject.Call(hdc, oldFont)
		}

		procEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0
	}

	ret, _, _ := procCallWindowProcW.Call(gCloseBtnOldWndProc, hwnd, uintptr(msgID), wParam, lParam)
	return ret
}

func subclassCloseButton(hwnd uintptr) {
	if hwnd == 0 {
		return
	}
	old, _, _ := procGetWindowLongPtrW.Call(hwnd, gwlpWndProc)
	gCloseBtnOldWndProc = old
	procSetWindowLongPtrW.Call(hwnd, gwlpWndProc, syscall.NewCallback(closeBtnWndProc))
}

func launchCrealityPrint(log func(args ...interface{}), installDir string) {
	exePath := filepath.Join(installDir, "CrealityPrint.exe")
	if _, err := os.Stat(exePath); err != nil {
		log("skip launch, CrealityPrint.exe not found:", err)
		return
	}
	verb := toUTF16Ptr("open")
	file := toUTF16Ptr(exePath)
	dir := toUTF16Ptr(installDir)
	ret, _, err := procShellExecuteW.Call(
		0,
		uintptr(unsafe.Pointer(verb)),
		uintptr(unsafe.Pointer(file)),
		0,
		uintptr(unsafe.Pointer(dir)),
		swShow,
	)
	if ret <= 32 {
		log("launch CrealityPrint.exe failed:", ret, err)
		return
	}
	log("launched CrealityPrint.exe:", exePath)
}

func installWndProc(hwnd uintptr, msgID uint32, wParam, lParam uintptr) uintptr {
	switch msgID {
	case wmCreate:
		gInstallHwnd = hwnd
		dpi := getWindowDpi(hwnd)
		gInstallDpi = dpi
		ensureInstallUIResources()

		if style, _, _ := procGetWindowLongPtrW.Call(hwnd, uintptr(gwlStyle)); style != 0 {
			newStyle := style &^ (wsCaption | wsSysMenu | wsMinimizeBox)
			procSetWindowLongPtrW.Call(hwnd, uintptr(gwlStyle), newStyle)
		}

		// Create child controls with placeholder size; layoutInstallWindow will position them.
		gInstallTitle, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("STATIC"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("installing_update")))),
			wsChild|wsVisible,
			0, 0, 1, 1,
			hwnd,
			0,
			hInstance(),
			0,
		)
		setControlFont(gInstallTitle)

		gInstallClose, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("BUTTON"))),
			uintptr(unsafe.Pointer(toUTF16Ptr("\u00d7"))),
			wsChild|wsVisible|wsTabStop|bsPushButton|bsFlat,
			0, 0, 1, 1,
			hwnd,
			uintptr(idClose),
			hInstance(),
			0,
		)
		setControlFont(gInstallClose)
		subclassThemedButton(gInstallClose, true)

		gInstallStatus, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("STATIC"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("install_progress", 0)))),
			wsChild|wsVisible,
			0, 0, 1, 1,
			hwnd,
			0,
			hInstance(),
			0,
		)
		setControlFont(gInstallStatus)

		gInstallCancel, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("BUTTON"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("cancel")))),
			wsChild|wsVisible|wsTabStop|bsPushButton|bsFlat,
			0, 0, 1, 1,
			hwnd,
			uintptr(idCancel),
			hInstance(),
			0,
		)
		setControlFont(gInstallCancel)
		subclassCancelButton(gInstallCancel)

		layoutInstallWindow(hwnd, dpi)
		return 0

	case wmCommand:
		switch uint32(wParam & 0xFFFF) {
		case idCancel, idClose:
			gCancelRequested.Store(true)
			if gInstallCancel != 0 {
				procEnableWindow.Call(gInstallCancel, 0)
			}
			gProgressText.Store(getText("canceling"))
			if gInstallStatus != 0 {
				procSetWindowTextW.Call(gInstallStatus, uintptr(unsafe.Pointer(toUTF16Ptr(getText("canceling_status")))))
			}
			procDestroyWindow.Call(hwnd)
			return 0
		}

	case wmEraseBkgnd:
		if gInstallBgBrush != 0 {
			var rc rect
			procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
			procFillRect.Call(wParam, uintptr(unsafe.Pointer(&rc)), gInstallBgBrush)
			return 1
		}
		return 0

	case wmCtlColorStatic:
		if gInstallBgBrush != 0 {
			procSetBkMode.Call(wParam, bkTransparent)
			procSetTextColor.Call(wParam, gTheme.text)
			return gInstallBgBrush
		}
		return 0

	case wmCtlColorBtn:
		if gInstallBgBrush != 0 {
			procSetBkMode.Call(wParam, bkTransparent)
			procSetTextColor.Call(wParam, gTheme.text)
			procSetBkColor.Call(wParam, gTheme.bg)
			return gInstallBgBrush
		}
		return 0

	case wmLButtonDown:
		x := int32(int16(lParam & 0xFFFF))
		y := int32(int16((lParam >> 16) & 0xFFFF))
		if y >= 0 && y <= scaleDPI(installTitlebarHit, gInstallDpi) {
			var rc rect
			procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
			rightPad := scaleDPI(installPadding, gInstallDpi)
			closeW := scaleDPI(installCloseWidth, gInstallDpi)
			if x < rc.right-rightPad-closeW || x > rc.right-rightPad {
				procReleaseCapture.Call()
				procSendMessageW.Call(hwnd, wmNclButtonDown, htCaption, 0)
				return 0
			}
		}

	case wmClose:
		gCancelRequested.Store(true)
		if gInstallCancel != 0 {
			procEnableWindow.Call(gInstallCancel, 0)
		}
		gProgressText.Store(getText("canceling"))
		if gInstallStatus != 0 {
			procSetWindowTextW.Call(gInstallStatus, uintptr(unsafe.Pointer(toUTF16Ptr(getText("canceling_status")))))
		}
		procDestroyWindow.Call(hwnd)
		return 0

	case wmAppProgress:
		if gInstallStatus != 0 {
			if v := gProgressText.Load(); v != nil {
				if s, ok := v.(string); ok {
					procSetWindowTextW.Call(gInstallStatus, uintptr(unsafe.Pointer(toUTF16Ptr(s))))
				}
			}
		}
		procInvalidateRect.Call(hwnd, uintptr(unsafe.Pointer(&gInstallProgressRect)), 0)
		procUpdateWindow.Call(hwnd)
		return 0

	case wmDpiChanged:
		newDpi := int32(wParam >> 16)
		if newDpi <= 0 {
			newDpi = int32(wParam & 0xFFFF)
		}
		if newDpi <= 0 {
			newDpi = 96
		}
		gInstallDpi = newDpi
		suggestedRect := (*rect)(unsafe.Pointer(lParam))
		procMoveWindow.Call(hwnd,
			uintptr(suggestedRect.left), uintptr(suggestedRect.top),
			uintptr(suggestedRect.right-suggestedRect.left),
			uintptr(suggestedRect.bottom-suggestedRect.top), 1)
		oldFont := gInstallFont
		gInstallFont = createDpiFont(newDpi)
		layoutInstallWindow(hwnd, newDpi)
		if oldFont != 0 {
			procDeleteObject.Call(oldFont)
		}
		procInvalidateRect.Call(hwnd, 0, 1)
		return 0

	case wmAppDone:
		procDestroyWindow.Call(hwnd)
		return 0

	case wmPaint:
		var ps paintStruct
		hdc, _, _ := procBeginPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		if gInstallBgBrush != 0 {
			procFillRect.Call(hdc, uintptr(unsafe.Pointer(&ps.rcPaint)), gInstallBgBrush)
		}

		bar := gInstallProgressRect
		if bar.right > bar.left && bar.bottom > bar.top {
			pct := int32(gProgressPercent.Load())
			if pct < 0 {
				pct = 0
			}
			if pct > 100 {
				pct = 100
			}

			barW := bar.right - bar.left
			fillW := int32(int64(barW) * int64(pct) / 100)
			radius := bar.bottom - bar.top
			if radius < 0 {
				radius = 0
			}

			trackColor := gTheme.progressTrack
			fillColor := gTheme.progressFill

			trackBrush, _, _ := procCreateSolidBrush.Call(trackColor)
			trackPen, _, _ := procCreatePen.Call(psSolid, 1, trackColor)
			oldBrush, _, _ := procSelectObject.Call(hdc, trackBrush)
			oldPen, _, _ := procSelectObject.Call(hdc, trackPen)
			procRoundRect.Call(hdc, uintptr(bar.left), uintptr(bar.top), uintptr(bar.right), uintptr(bar.bottom), uintptr(radius), uintptr(radius))
			procSelectObject.Call(hdc, oldPen)
			procSelectObject.Call(hdc, oldBrush)
			procDeleteObject.Call(trackPen)
			procDeleteObject.Call(trackBrush)

			if fillW > 0 {
				fill := bar
				fill.right = fill.left + fillW
				ellipse := radius
				if fillW < ellipse {
					ellipse = fillW
				}
				fillBrush, _, _ := procCreateSolidBrush.Call(fillColor)
				fillPen, _, _ := procCreatePen.Call(psSolid, 1, fillColor)
				oldBrush2, _, _ := procSelectObject.Call(hdc, fillBrush)
				oldPen2, _, _ := procSelectObject.Call(hdc, fillPen)
				procRoundRect.Call(hdc, uintptr(fill.left), uintptr(fill.top), uintptr(fill.right), uintptr(fill.bottom), uintptr(ellipse), uintptr(ellipse))
				procSelectObject.Call(hdc, oldPen2)
				procSelectObject.Call(hdc, oldBrush2)
				procDeleteObject.Call(fillPen)
				procDeleteObject.Call(fillBrush)
			}
		}

		procEndPaint.Call(hwnd, uintptr(unsafe.Pointer(&ps)))
		return 0

	case wmDestroy:
		gInstallHwnd = 0
		gInstallTitle = 0
		procPostQuitMessage.Call(0)
		return 0
	}

	ret, _, _ := procDefWindowProcW.Call(hwnd, uintptr(msgID), wParam, lParam)
	return ret
}

func incompleteWndProc(hwnd uintptr, msgID uint32, wParam, lParam uintptr) uintptr {
	switch msgID {
	case wmCreate:
		gIncompleteHwnd = hwnd
		dpi := getWindowDpi(hwnd)
		gInstallDpi = dpi
		ensureInstallUIResources()

		if style, _, _ := procGetWindowLongPtrW.Call(hwnd, uintptr(gwlStyle)); style != 0 {
			newStyle := style &^ (wsCaption | wsSysMenu | wsMinimizeBox)
			procSetWindowLongPtrW.Call(hwnd, uintptr(gwlStyle), newStyle)
		}

		// Create child controls with placeholder size; layoutIncompleteWindow will position them.
		gInstallTitle, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("STATIC"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("update_incomplete")))),
			wsChild|wsVisible,
			0, 0, 1, 1,
			hwnd,
			0,
			hInstance(),
			0,
		)
		setControlFont(gInstallTitle)

		gInstallClose, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("BUTTON"))),
			uintptr(unsafe.Pointer(toUTF16Ptr("\u00d7"))),
			wsChild|wsVisible|wsTabStop|bsPushButton|bsFlat,
			0, 0, 1, 1,
			hwnd,
			uintptr(idClose),
			hInstance(),
			0,
		)
		setControlFont(gInstallClose)
		subclassThemedButton(gInstallClose, true)

		text := "更新过程中出现异常。程序已恢复到原有稳定版本，可继续正常使用。"
		if v := gIncompleteMessage.Load(); v != nil {
			if s, ok := v.(string); ok && strings.TrimSpace(s) != "" {
				text = s
			}
		}
		gIncompleteText, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("STATIC"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(text))),
			wsChild|wsVisible,
			0, 0, 1, 1,
			hwnd,
			0,
			hInstance(),
			0,
		)
		setControlFont(gIncompleteText)

		gIncompleteManual, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("BUTTON"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("manual_download")))),
			wsChild|wsVisible|wsTabStop|bsPushButton|bsFlat,
			0, 0, 1, 1,
			hwnd,
			uintptr(idManual),
			hInstance(),
			0,
		)
		setControlFont(gIncompleteManual)
		subclassThemedButton(gIncompleteManual, false)

		gIncompleteConfirm, _, _ = procCreateWindowExW.Call(
			0,
			uintptr(unsafe.Pointer(toUTF16Ptr("BUTTON"))),
			uintptr(unsafe.Pointer(toUTF16Ptr(getText("confirm")))),
			wsChild|wsVisible|wsTabStop|bsPushButton|bsFlat,
			0, 0, 1, 1,
			hwnd,
			uintptr(idConfirm),
			hInstance(),
			0,
		)
		setControlFont(gIncompleteConfirm)
		subclassThemedButton(gIncompleteConfirm, false)

		layoutIncompleteWindow(hwnd, dpi)
		return 0

	case wmDpiChanged:
		newDpi := int32(wParam >> 16)
		if newDpi <= 0 {
			newDpi = int32(wParam & 0xFFFF)
		}
		if newDpi <= 0 {
			newDpi = 96
		}
		gInstallDpi = newDpi
		suggestedRect := (*rect)(unsafe.Pointer(lParam))
		procMoveWindow.Call(hwnd,
			uintptr(suggestedRect.left), uintptr(suggestedRect.top),
			uintptr(suggestedRect.right-suggestedRect.left),
			uintptr(suggestedRect.bottom-suggestedRect.top), 1)
		oldFont := gInstallFont
		gInstallFont = createDpiFont(newDpi)
		layoutIncompleteWindow(hwnd, newDpi)
		if oldFont != 0 {
			procDeleteObject.Call(oldFont)
		}
		procInvalidateRect.Call(hwnd, 0, 1)
		return 0

	case wmLButtonDown:
		x := int32(int16(lParam & 0xFFFF))
		y := int32(int16((lParam >> 16) & 0xFFFF))
		if y >= 0 && y <= scaleDPI(installTitlebarHit, gInstallDpi) {
			var rc rect
			procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
			rightPad := scaleDPI(installPadding, gInstallDpi)
			closeW := scaleDPI(installCloseWidth, gInstallDpi)
			if x < rc.right-rightPad-closeW || x > rc.right-rightPad {
				procReleaseCapture.Call()
				procSendMessageW.Call(hwnd, wmNclButtonDown, htCaption, 0)
				return 0
			}
		}

	case wmEraseBkgnd:
		ensureInstallUIResources()
		if gInstallBgBrush != 0 {
			var rc rect
			procGetClientRect.Call(hwnd, uintptr(unsafe.Pointer(&rc)))
			procFillRect.Call(wParam, uintptr(unsafe.Pointer(&rc)), gInstallBgBrush)
			return 1
		}
		return 0

	case wmCtlColorStatic:
		ensureInstallUIResources()
		if gInstallBgBrush != 0 {
			procSetBkMode.Call(wParam, bkTransparent)
			procSetTextColor.Call(wParam, gTheme.text)
			return gInstallBgBrush
		}

	case wmCtlColorBtn:
		ensureInstallUIResources()
		if gInstallBgBrush != 0 {
			procSetBkMode.Call(wParam, bkTransparent)
			procSetTextColor.Call(wParam, gTheme.text)
			return gInstallBgBrush
		}

	case wmCommand:
		switch uint32(wParam & 0xFFFF) {
		case idManual:
			if v := gManualURL.Load(); v != nil {
				if u, ok := v.(string); ok {
					openURL(u)
				}
			}
			// After opening the manual download page in the browser, close the updater
			// window so the process exits.
			procDestroyWindow.Call(hwnd)
			return 0
		case idConfirm:
			procDestroyWindow.Call(hwnd)
			return 0
		case idClose:
			procDestroyWindow.Call(hwnd)
			return 0
		}

	case wmClose:
		procDestroyWindow.Call(hwnd)
		return 0

	case wmDestroy:
		gIncompleteHwnd = 0
		procPostQuitMessage.Call(0)
		return 0
	}

	ret, _, _ := procDefWindowProcW.Call(hwnd, uintptr(msgID), wParam, lParam)
	return ret
}

func runWindowLoop() {
	var m msg
	for {
		r, _, _ := procGetMessageW.Call(uintptr(unsafe.Pointer(&m)), 0, 0, 0)
		if int32(r) == 0 {
			break
		}
		procTranslateMessage.Call(uintptr(unsafe.Pointer(&m)))
		procDispatchMessageW.Call(uintptr(unsafe.Pointer(&m)))
	}
}

func centerWindow(hwnd uintptr, w, h int32) {
	sw, _, _ := procGetSystemMetrics.Call(smCxScreen)
	sh, _, _ := procGetSystemMetrics.Call(smCyScreen)
	x := (int32(sw) - w) / 2
	y := (int32(sh) - h) / 2
	procMoveWindow.Call(hwnd, uintptr(x), uintptr(y), uintptr(w), uintptr(h), 1)
}

func createWindow(className, title string, w, h int32, wndProc uintptr) (uintptr, error) {
	cn := toUTF16Ptr(className)
	wc := wndClassExW{
		cbSize:        uint32(unsafe.Sizeof(wndClassExW{})),
		lpfnWndProc:   wndProc,
		hInstance:     hInstance(),
		hbrBackground: 6,
		lpszClassName: cn,
	}

	r, _, _ := procRegisterClassExW.Call(uintptr(unsafe.Pointer(&wc)))
	if r == 0 {
	}

	hwnd, _, err := procCreateWindowExW.Call(
		0,
		uintptr(unsafe.Pointer(cn)),
		uintptr(unsafe.Pointer(toUTF16Ptr(title))),
		wsOverlapped|wsCaption|wsSysMenu|wsMinimizeBox|wsVisible,
		0,
		0,
		uintptr(w),
		uintptr(h),
		0,
		0,
		hInstance(),
		0,
	)
	if hwnd == 0 {
		return 0, err
	}

	centerWindow(hwnd, w, h)
	procShowWindow.Call(hwnd, swShow)
	procUpdateWindow.Call(hwnd)
	return hwnd, nil
}

type updateResult struct {
	err             error
	rollbackApplied bool
	backupDir       string
	rollbackOK      bool
	targetVersion   string
}

func isCanceled() bool { return gCancelRequested.Load() }

func main() {
	runtime.LockOSThread()

	installDir := flag.String("install-dir", "", "install directory of CrealityPrint")
	currentVer := flag.String("current-version", "", "current version string")
	manualURL := flag.String("manual-url", "", "manual download URL")
	darkMode := flag.Int("dark-mode", -1, "0=light, 1=dark, -1=auto")
	language := flag.String("language", "en", "language: zh_CN or en")
	dataDir := flag.String("data-dir", "", "CrealityPrint data directory for writing ota_result.json")
	// forceFull: hotfix update where the target shares the same major.minor.patch as the
	// installed version (only the 4th build field differs). Since the nupkg name only
	// carries the 3-part version, the updater would normally skip a same-3-part package;
	// this flag forces it to install the full package for that version.
	forceFull := flag.String("force-full", "", "force install full package for same-3-part version (1 to enable)")
	flag.Parse()

	if *installDir == "" || *currentVer == "" {
		fmt.Println("missing --install-dir or --current-version")
		os.Exit(1)
	}

	exePath, err := os.Executable()
	if err != nil {
		fmt.Println("get executable path failed:", err)
		os.Exit(1)
	}
	baseDir := filepath.Dir(exePath)

	logDir := filepath.Join(baseDir, "logs")

	if err := os.MkdirAll(logDir, 0755); err != nil {
		fmt.Println("create log dir failed:", err)
		os.Exit(1)
	}

	logFile, err := os.Create(filepath.Join(logDir, "updater_"+time.Now().Format("20060102_150405")+".log"))
	if err != nil {
		fmt.Println("create log file failed:", err)
		os.Exit(1)
	}
	defer logFile.Close()
	log := func(args ...interface{}) {
		s := fmt.Sprintln(args...)
		logFile.WriteString(time.Now().Format("2006-01-02 15:04:05.000 "))
		logFile.WriteString(s)
	}

	log("base dir:", baseDir)
	log("install dir:", *installDir)
	log("current version:", *currentVer)
	log("manual url:", *manualURL)
	log("force full:", *forceFull)
	gThemeMode.Store(int32(*darkMode))
	gLanguage = *language

	gManualURL.Store(*manualURL)
	gProgressText.Store(getText("install_progress", 0))
	gCancelRequested.Store(false)

	initCommonControls()

	// Query system DPI before creating the window so we can pass a properly scaled size.
	sysDPI := getWindowDpi(0)
	winW := scaleDPI(600, sysDPI)
	winH := scaleDPI(260, sysDPI)

	forceFullEnabled := strings.TrimSpace(*forceFull) == "1"
	log("force full:", forceFullEnabled)

	_, winErr := createWindow("CrealityUpdaterInstallWindow", getText("installing_update"), winW, winH, syscall.NewCallback(installWndProc))
	if winErr != nil {
		res := runUpdateWithUI(log, baseDir, *installDir, *currentVer, forceFullEnabled)
		if res.err != nil {
			log("Update failed with error:", res.err, "rollbackApplied:", res.rollbackApplied, "rollbackOK:", res.rollbackOK, "backupDir:", res.backupDir)
			log("[OTA_ANALYTICS] (no-window path) Update failed, writing ota_result.json with result=fail")
			writeOtaResult(*dataDir, "fail", *currentVer, "")
			messageBox(formatIncompleteMessage(res), getText("update_incomplete"), mbOK|mbIconError|mbSystemModal)
			os.Exit(1)
		}
		log("[OTA_ANALYTICS] (no-window path) Update succeeded, writing ota_result.json with result=success, to=", res.targetVersion)
		writeOtaResult(*dataDir, "success", *currentVer, res.targetVersion)
		launchCrealityPrint(log, *installDir)
		os.Exit(0)
	}

	done := make(chan updateResult, 1)
	go func() {
		res := runUpdateWithUI(log, baseDir, *installDir, *currentVer, forceFullEnabled)
		done <- res
		requestInstallClose()
	}()

	runWindowLoop()

	res := <-done
	if res.err == nil {
		log("[OTA_ANALYTICS] Update succeeded, writing ota_result.json with result=success, from=", *currentVer, ", to=", res.targetVersion)
		writeOtaResult(*dataDir, "success", *currentVer, res.targetVersion)
		launchCrealityPrint(log, *installDir)
		os.Exit(0)
	}
	if errors.Is(res.err, errCanceled) {
		log("[OTA_ANALYTICS] Update canceled by user, writing ota_result.json with result=cancel, from=", *currentVer)
		writeOtaResult(*dataDir, "cancel", *currentVer, "")
		os.Exit(0)
	}

	log("Update failed with error:", res.err, "rollbackApplied:", res.rollbackApplied, "rollbackOK:", res.rollbackOK, "backupDir:", res.backupDir)
	log("[OTA_ANALYTICS] Update failed, writing ota_result.json with result=fail, from=", *currentVer)
	writeOtaResult(*dataDir, "fail", *currentVer, "")
	gIncompleteMessage.Store(formatIncompleteMessage(res))
	_, _ = createWindow("CrealityUpdaterIncompleteWindow", getText("update_incomplete"), winW, winH, syscall.NewCallback(incompleteWndProc))
	runWindowLoop()
	os.Exit(1)
}

func formatInstallStatus(percent int) string {
	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	return getText("install_progress", percent)
}

func setInstallProgress(percent int) {
	postInstallProgress(percent, formatInstallStatus(percent))
}

func setInstallStage(log func(args ...interface{}), stage string, percent int) {
	if strings.TrimSpace(stage) == "" {
		setInstallProgress(percent)
		return
	}

	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	postInstallProgress(percent, getText("install_progress", percent))
}

func isAccessDeniedError(err error) bool {
	if err == nil {
		return false
	}
	var pe *os.PathError
	if errors.As(err, &pe) {
		if errno, ok := pe.Err.(syscall.Errno); ok {
			if errno == syscall.ERROR_ACCESS_DENIED || errno == windowsErrSharingViolation || errno == windowsErrLockViolation {
				return true
			}
		}
	}
	s := strings.ToLower(err.Error())
	return strings.Contains(s, "access is denied") || strings.Contains(s, "sharing violation") || strings.Contains(s, "used by another process")
}

func renameWithRetry(log func(args ...interface{}), src, dst string, timeout time.Duration, unlockDir string) error {
	deadline := time.Now().Add(timeout)
	var lastErr error
	lastUnlock := time.Time{}
	unlockCount := 0
	for {
		if isCanceled() {
			return errCanceled
		}
		err := os.Rename(src, dst)
		if err == nil {
			return nil
		}
		lastErr = err
		if unlockDir != "" && isAccessDeniedError(err) {
			if unlockCount < 3 && (lastUnlock.IsZero() || time.Since(lastUnlock) > 2*time.Second) {
				unlockCount++
				lastUnlock = time.Now()
				releaseInstallDirLocks(log, unlockDir)
			}
		}
		if !isAccessDeniedError(err) || time.Now().After(deadline) {
			return lastErr
		}
		time.Sleep(500 * time.Millisecond)
	}
}

func getText(key string, args ...interface{}) string {
	if gLanguage == "zh_CN" {
		switch key {
		case "install_progress":
			return fmt.Sprintf("安装中 (%d%%已完成)", args...)
		case "install_progress_stage":
			return fmt.Sprintf("%s (%d%%已完成)", args...)
		case "canceling":
			return "正在取消..."
		case "canceling_status":
			return "正在取消..."
		case "cancel":
			return "取消"
		case "installing_update":
			return "正在安装更新"
		case "update_incomplete":
			return "更新未完成"
		case "manual_download":
			return "手动下载"
		case "confirm":
			return "确认"
		case "preparing":
			return "准备中"
		case "preparing_install":
			return "准备安装"
		case "copying_files":
			return "复制文件中"
		case "switching_version":
			return "正在切换版本"
		case "finishing_install":
			return "正在完成安装"
		case "install_complete":
			return "安装完成"
		case "update_exception":
			return "更新过程中出现异常。程序已恢复到原有稳定版本，可继续正常使用。"
		case "incomplete_rollback_ok":
			return "更新未完成。程序已恢复到原有稳定版本，可继续正常使用。你也可以点击" + quote("手动下载") + "重新安装最新版本。"
		case "incomplete_rollback_fail_with_backup":
			return "更新未完成，且自动恢复失败。\r\n请尝试手动恢复备份目录：\r\n" + fmt.Sprintf("%v", args[0]) + "\r\n或点击" + quote("手动下载") + "重新安装最新版本。"
		case "incomplete_rollback_fail_no_backup":
			return "更新未完成，且自动恢复失败。请点击" + quote("手动下载") + "重新安装最新版本。"
		case "incomplete_access_denied":
			return "更新未完成。当前安装目录正在被占用。\r\n请先退出 CrealityPrint（含后台进程），再点击" + quote("Install Now") + "重试；或点击" + quote("手动下载") + "重新安装最新版本。"
		case "incomplete_generic":
			return "更新未完成。请点击" + quote("手动下载") + "重新安装最新版本。"
		case "update_incomplete_title":
			return "更新未完成。程序已恢复到原有稳定版本，可继续正常使用。你也可以点击" + quote("手动下载") + "重新安装最新版本。"
		case "update_incomplete_fail_title":
			return "更新未完成，且自动恢复失败。\r\n请尝试手动恢复备份目录：\r\n" + fmt.Sprintf("%v", args[0]) + "\r\n或点击" + quote("手动下载") + "重新安装最新版本。"
		case "update_incomplete_access_title":
			return "更新未完成。当前安装目录正在被占用。\r\n请先退出 CrealityPrint（含后台进程），再点击" + quote("Install Now") + "重试；或点击" + quote("手动下载") + "重新安装最新版本。"
		case "update_incomplete_generic_title":
			return "更新未完成。请点击" + quote("手动下载") + "重新安装最新版本。"
		}
	}

	switch key {
	case "install_progress":
		return fmt.Sprintf("Installing (%d%% complete)", args...)
	case "install_progress_stage":
		return fmt.Sprintf("%s (%d%% complete)", args...)
	case "canceling":
		return "Canceling..."
	case "canceling_status":
		return "Canceling..."
	case "cancel":
		return "Cancel"
	case "installing_update":
		return "Installing Update"
	case "update_incomplete":
		return "Update Incomplete"
	case "manual_download":
		return "Manual Download"
	case "confirm":
		return "Confirm"
	case "preparing":
		return "Preparing"
	case "preparing_install":
		return "Preparing to install"
	case "copying_files":
		return "Copying files"
	case "switching_version":
		return "Switching version"
	case "finishing_install":
		return "Finishing installation"
	case "install_complete":
		return "Installation complete"
	case "update_exception":
		return "An exception occurred during the update. The program has been restored to the previous stable version and can continue to be used normally."
	case "incomplete_rollback_ok":
		return "Update incomplete. The program has been restored to the previous stable version and can continue to be used normally. You can also click " + quote("Manual Download") + " to reinstall the latest version."
	case "incomplete_rollback_fail_with_backup":
		return "Update incomplete and automatic restore failed.\r\nPlease try to manually restore the backup directory:\r\n" + fmt.Sprintf("%v", args[0]) + "\r\nor click " + quote("Manual Download") + " to reinstall the latest version."
	case "incomplete_rollback_fail_no_backup":
		return "Update incomplete and automatic restore failed. Please click " + quote("Manual Download") + " to reinstall the latest version."
	case "incomplete_access_denied":
		return "Update incomplete. The installation directory is currently in use.\r\nPlease exit CrealityPrint (including background processes) first, then click " + quote("Install Now") + " to retry; or click " + quote("Manual Download") + " to reinstall the latest version."
	case "incomplete_generic":
		return "Update incomplete. Please click " + quote("Manual Download") + " to reinstall the latest version."
	case "update_incomplete_title":
		return "Update incomplete. The program has been restored to the previous stable version and can continue to be used normally. You can also click " + quote("Manual Download") + " to reinstall the latest version."
	case "update_incomplete_fail_title":
		return "Update incomplete and automatic restore failed.\r\nPlease try to manually restore the backup directory:\r\n" + fmt.Sprintf("%v", args[0]) + "\r\nor click " + quote("Manual Download") + " to reinstall the latest version."
	case "update_incomplete_access_title":
		return "Update incomplete. The installation directory is currently in use.\r\nPlease exit CrealityPrint (including background processes) first, then click " + quote("Install Now") + " to retry; or click " + quote("Manual Download") + " to reinstall the latest version."
	case "update_incomplete_generic_title":
		return "Update incomplete. Please click " + quote("Manual Download") + " to reinstall the latest version."
	}
	return key
}

func quote(s string) string {
	return fmt.Sprintf("\"%s\"", s)
}

func formatIncompleteMessage(res updateResult) string {
	return getText("update_exception")
}

// writeOtaResult writes a JSON marker file so that CrealityPrint can report
// the OTA update outcome on next startup.
func writeOtaResult(dir, result, fromVersion, toVersion string) {
	if strings.TrimSpace(dir) == "" {
		fmt.Println("[OTA_ANALYTICS] writeOtaResult: data-dir is empty, skipping")
		return
	}
	data := map[string]interface{}{
		"result":       result,
		"from_version": fromVersion,
		"to_version":   toVersion,
		"timestamp":    time.Now().Unix(),
	}
	b, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		fmt.Println("[OTA_ANALYTICS] writeOtaResult: json marshal failed:", err)
		return
	}
	_ = os.MkdirAll(dir, 0755)
	outPath := filepath.Join(dir, "ota_result.json")
	if err := os.WriteFile(outPath, b, 0644); err != nil {
		fmt.Println("[OTA_ANALYTICS] writeOtaResult: write file failed:", outPath, err)
	} else {
		fmt.Println("[OTA_ANALYTICS] writeOtaResult: wrote", outPath, "result=", result)
	}
}

func runUpdateWithUI(log func(args ...interface{}), baseDir, installDir, currentVer string, forceFull bool) updateResult {
	setInstallStage(log, "Preparing", 0)

	releasesPath := filepath.Join(baseDir, "RELEASES")
	entries, err := parseReleases(releasesPath)
	if err != nil {
		log("parse RELEASES failed:", err)
		return updateResult{err: err}
	}
	if len(entries) == 0 {
		log("no entries in RELEASES")
		return updateResult{err: nil}
	}

	versionInfos := buildVersionInfos(entries)
	chain, err := planUpgradeChain(versionInfos, currentVer, forceFull)
	if err != nil {
		log("plan upgrade chain failed:", err)
		return updateResult{err: err}
	}
	if len(chain) == 0 {
		log("already up to date")
		return updateResult{err: nil}
	}

	log("planned upgrade chain:")
	for _, v := range chain {
		hasDelta := v.Delta != nil
		hasFull := v.Full != nil
		log("  version:", v.Version, "delta:", hasDelta, "full:", hasFull)
	}
	finalVersion := chain[len(chain)-1].Version

	setInstallStage(log, "Preparing to install", 5)
	targetTree, err := buildTargetTree(log, baseDir, installDir, currentVer, chain)
	if err != nil {
		log("build target tree failed:", err)
		return updateResult{err: err}
	}
	if targetTree == "" {
		log("no target tree built")
		return updateResult{err: nil}
	}

	if isCanceled() {
		return updateResult{err: errCanceled}
	}

	installPath := installDir
	parentDir := filepath.Dir(installPath)
	appName := filepath.Base(installPath)
	ts := time.Now().Format("20060102_150405")

	newDir := filepath.Join(parentDir, appName+"_updating_"+ts)
	backupDir := filepath.Join(parentDir, appName+"_"+ts+"_back")

	os.RemoveAll(newDir)
	if err := os.MkdirAll(newDir, 0755); err != nil {
		log("mkdir new version dir failed:", err)
		return updateResult{err: err}
	}

	setInstallStage(log, "Copying files", 30)
	log("copying target tree to", newDir)
	if err := copyDirWithProgressStage(log, targetTree, newDir, 30, 85, "Copying files"); err != nil {
		log("copy target tree failed:", err)
		os.RemoveAll(newDir)
		return updateResult{err: err}
	}

	if _, err := os.Stat(filepath.Join(newDir, "CrealityPrint.exe")); err != nil {
		log("CrealityPrint.exe not found in new version dir:", err)
		os.RemoveAll(newDir)
		return updateResult{err: err}
	}

	if isCanceled() {
		os.RemoveAll(newDir)
		return updateResult{err: errCanceled}
	}

	setInstallStage(log, "Switching version", 86)
	log("renaming install dir to backup", backupDir)
	if err := renameWithRetry(log, installPath, backupDir, 60*time.Second, installPath); err != nil {
		log("rename install dir to backup failed:", err)
		os.RemoveAll(newDir)
		return updateResult{err: err}
	}
	rollbackApplied := true

	if isCanceled() {
		log("canceled after backup, rolling back")
		_ = os.RemoveAll(installPath)
		_ = os.Rename(backupDir, installPath)
		_ = os.RemoveAll(newDir)
		return updateResult{err: errCanceled, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true}
	}

	setInstallStage(log, "Switching version", 90)
	log("renaming new dir to install dir", installPath)
	if isCanceled() {
		log("canceled during switch, rolling back")
		_ = os.RemoveAll(installPath)
		_ = os.Rename(backupDir, installPath)
		_ = os.RemoveAll(newDir)
		return updateResult{err: errCanceled, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true}
	}
	if err := renameWithRetry(log, newDir, installPath, 30*time.Second, ""); err != nil {
		if errors.Is(err, errCanceled) {
			log("canceled during switch, rolling back")
			_ = os.RemoveAll(installPath)
			_ = os.Rename(backupDir, installPath)
			_ = os.RemoveAll(newDir)
			return updateResult{err: errCanceled, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true}
		}
		log("rename new dir to install dir failed, try copy then rollback:", err)
		if errMk := os.MkdirAll(installPath, 0755); errMk != nil {
			log("mkdir install dir for copy failed:", errMk)
			_ = os.RemoveAll(newDir)
			if err2 := os.Rename(backupDir, installPath); err2 != nil {
				log("rollback failed, manual restore needed:", err2)
				return updateResult{err: errMk, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: false}
			}
			return updateResult{err: errMk, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true}
		}

		if errCopy := copyDirWithProgressStage(log, newDir, installPath, 90, 95, "Copying files"); errCopy != nil {
			log("copy new dir to install dir failed, try rollback:", errCopy)
			_ = os.RemoveAll(installPath)
			_ = os.RemoveAll(newDir)
			if err2 := os.Rename(backupDir, installPath); err2 != nil {
				log("rollback failed, manual restore needed:", err2)
				return updateResult{err: errCopy, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: false}
			}
			return updateResult{err: errCopy, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true}
		}

		log("copy new dir to install dir succeeded after rename failure")
		_ = os.RemoveAll(newDir)
	}

	setInstallStage(log, "Finishing installation", 98)
	log("update succeeded from", currentVer, "to", finalVersion)

	// Preserve the original NSIS-generated Uninstall.exe from the backup so
	// that "Programs and Features" uninstall keeps working after a hot-update.
	// The new package intentionally does NOT ship Uninstall.exe (it is excluded
	// from the nupkg), so we restore it from the backup before wiping it.
	preserveUninstallerFromBackup(log, backupDir, installPath)

	if err := os.RemoveAll(backupDir); err != nil {
		log("remove backup dir failed:", err)
	}

	// Clean up cache directory after successful installation
	// Delete all files and directories except updater.exe and logs folder
	cacheEntries, err := os.ReadDir(baseDir)
	if err != nil {
		log("read cache dir failed:", err)
	} else {
		for _, entry := range cacheEntries {
			name := entry.Name()
			path := filepath.Join(baseDir, name)

			if entry.IsDir() {
				// Keep logs directory
				if strings.ToLower(name) == "logs" {
					continue
				}
				// Delete other directories
				if err := os.RemoveAll(path); err != nil {
					log("remove directory failed:", path, err)
				} else {
					log("directory removed:", path)
				}
			} else {
				// Keep updater.exe
				if strings.ToLower(name) == "updater.exe" {
					continue
				}
				// Delete other files
				if err := os.Remove(path); err != nil {
					log("remove file failed:", path, err)
				} else {
					log("file removed:", path)
				}
			}
		}
	}

	setInstallStage(log, "Installation complete", 100)
	return updateResult{err: nil, rollbackApplied: rollbackApplied, backupDir: backupDir, rollbackOK: true, targetVersion: finalVersion}
}

// preserveUninstallerFromBackup copies Uninstall.exe from the backup directory
// back into the new installation directory.  This is called after a successful
// directory-swap update so that the original NSIS-generated uninstaller is not
// lost: the nupkg intentionally excludes Uninstall.exe to avoid overwriting it,
// but the swap replaces the whole directory tree, so we must restore it
// explicitly before the backup is wiped.
//
// The function is tolerant of:
//   - missing backup (first install, not an update)
//   - missing Uninstall.exe in backup (package already excluded it earlier)
//   - case-insensitive file-system: tries "Uninstall.exe" then "uninstall.exe"
func preserveUninstallerFromBackup(log func(args ...interface{}), backupDir, installDir string) {
	candidates := []string{"Uninstall.exe", "uninstall.exe"}
	for _, name := range candidates {
		src := filepath.Join(backupDir, name)
		if _, err := os.Stat(src); err != nil {
			continue // not found under this name, try next
		}
		dst := filepath.Join(installDir, "Uninstall.exe")
		if _, err := os.Stat(dst); err == nil {
			log("Uninstall.exe already present in new install dir, skipping restore")
			return
		}
		if err := copyFile(src, dst); err != nil {
			log("WARN: failed to restore Uninstall.exe from backup:", err)
		} else {
			log("restored Uninstall.exe from backup to", dst)
		}
		return
	}
	log("Uninstall.exe not found in backup dir, nothing to restore")
}

// copyFile copies src to dst, creating dst and its parent directories as needed.
func copyFile(src, dst string) error {
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return err
	}
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer out.Close()
	if _, err := io.Copy(out, in); err != nil {
		return err
	}
	return out.Sync()
}

func countFiles(root string) (int64, error) {
	var count int64
	err := filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if isCanceled() {
			return errCanceled
		}
		if info.IsDir() {
			return nil
		}
		count++
		return nil
	})
	if err != nil {
		return 0, err
	}
	return count, nil
}

func countBytes(root string) (int64, error) {
	var total int64
	err := filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if info.IsDir() {
			return nil
		}
		total += info.Size()
		return nil
	})
	if err != nil {
		return 0, err
	}
	return total, nil
}

func copyDirWithProgress(src, dst string, startPercent, endPercent int) error {
	total, err := countBytes(src)
	if err != nil {
		return err
	}
	if total <= 0 {
		copyErr := copyDir(src, dst)
		if copyErr == nil {
			setInstallProgress(endPercent)
		}
		return copyErr
	}

	var done int64
	lastUpdate := time.Now()
	updateProgress := func() {
		p := startPercent + int((done*int64(endPercent-startPercent))/total)
		setInstallProgress(p)
	}
	updateProgress()

	err = filepath.Walk(src, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if isCanceled() {
			return errCanceled
		}
		rel, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}
		target := filepath.Join(dst, rel)
		if info.IsDir() {
			return os.MkdirAll(target, info.Mode())
		}
		if err := os.MkdirAll(filepath.Dir(target), 0755); err != nil {
			return err
		}
		in, err := os.Open(path)
		if err != nil {
			return err
		}
		out, err := os.Create(target)
		if err != nil {
			in.Close()
			return err
		}
		buf := make([]byte, 128*1024)
		for {
			if isCanceled() {
				in.Close()
				out.Close()
				return errCanceled
			}
			n, rerr := in.Read(buf)
			if n > 0 {
				if _, werr := out.Write(buf[:n]); werr != nil {
					in.Close()
					out.Close()
					return werr
				}
				done += int64(n)
				if done == total || time.Since(lastUpdate) >= 120*time.Millisecond {
					lastUpdate = time.Now()
					updateProgress()
				}
			}
			if rerr != nil {
				if errors.Is(rerr, io.EOF) {
					break
				}
				in.Close()
				out.Close()
				return rerr
			}
		}
		in.Close()
		out.Close()
		if err != nil {
			return err
		}
		return nil
	})
	if err != nil {
		return err
	}
	setInstallProgress(endPercent)
	return nil
}

func copyDirWithProgressStage(log func(args ...interface{}), src, dst string, startPercent, endPercent int, stage string) error {
	total, err := countBytes(src)
	if err != nil {
		return err
	}
	if total <= 0 {
		copyErr := copyDir(src, dst)
		if copyErr == nil {
			setInstallStage(log, stage, endPercent)
		}
		return copyErr
	}

	var done int64
	lastUpdate := time.Now()
	updateProgress := func() {
		p := startPercent + int((done*int64(endPercent-startPercent))/total)
		setInstallStage(log, stage, p)
	}
	updateProgress()

	err = filepath.Walk(src, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if isCanceled() {
			return errCanceled
		}
		rel, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}
		target := filepath.Join(dst, rel)
		if info.IsDir() {
			return os.MkdirAll(target, info.Mode())
		}
		if err := os.MkdirAll(filepath.Dir(target), 0755); err != nil {
			return err
		}
		in, err := os.Open(path)
		if err != nil {
			return err
		}
		out, err := os.Create(target)
		if err != nil {
			in.Close()
			return err
		}
		buf := make([]byte, 128*1024)
		for {
			if isCanceled() {
				in.Close()
				out.Close()
				return errCanceled
			}
			n, rerr := in.Read(buf)
			if n > 0 {
				if _, werr := out.Write(buf[:n]); werr != nil {
					in.Close()
					out.Close()
					return werr
				}
				done += int64(n)
				if done == total || time.Since(lastUpdate) >= 120*time.Millisecond {
					lastUpdate = time.Now()
					updateProgress()
				}
			}
			if rerr != nil {
				if errors.Is(rerr, io.EOF) {
					break
				}
				in.Close()
				out.Close()
				return rerr
			}
		}
		in.Close()
		out.Close()
		if err != nil {
			return err
		}
		return nil
	})
	if err != nil {
		return err
	}
	setInstallStage(log, stage, endPercent)
	return nil
}

func parseReleases(path string) ([]ReleaseEntry, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	var res []ReleaseEntry
	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		parts := strings.Fields(line)
		if len(parts) < 2 {
			continue
		}
		var filename string
		for _, p := range parts {
			if strings.HasSuffix(strings.ToLower(p), ".nupkg") {
				filename = p
				break
			}
		}
		if filename == "" {
			continue
		}
		lower := strings.ToLower(filename)
		isFull := strings.Contains(lower, "-full")
		version := extractVersionFromFileName(filename)
		res = append(res, ReleaseEntry{
			FileName: filename,
			Version:  version,
			IsFull:   isFull,
		})
	}
	return res, scanner.Err()
}

func buildVersionInfos(entries []ReleaseEntry) []VersionInfo {
	m := make(map[string]*VersionInfo)
	for i := range entries {
		e := &entries[i]
		v := e.Version
		if v == "" {
			continue
		}
		info, ok := m[v]
		if !ok {
			info = &VersionInfo{Version: v}
			m[v] = info
		}
		lower := strings.ToLower(e.FileName)
		if strings.Contains(lower, "-full") {
			info.Full = e
		} else if strings.Contains(lower, "-delta") {
			info.Delta = e
		}
	}
	var res []VersionInfo
	for _, v := range m {
		res = append(res, *v)
	}
	sort.Slice(res, func(i, j int) bool {
		return versionGreater(res[j].Version, res[i].Version)
	})
	return res
}

func planUpgradeChain(infos []VersionInfo, current string, forceFull bool) ([]VersionInfo, error) {
	var res []VersionInfo

	// Extract 3-part version from current version for comparison
	// This handles cases where current version has 4 parts (e.g., 7.0.1.4212)
	currentMajor, currentMinor, currentPatch, _ := splitVersion(current)
	currentBase := fmt.Sprintf("%d.%d.%d", currentMajor, currentMinor, currentPatch)

	for _, info := range infos {
		// Extract 3-part version from info version for comparison
		infoMajor, infoMinor, infoPatch, _ := splitVersion(info.Version)
		infoBase := fmt.Sprintf("%d.%d.%d", infoMajor, infoMinor, infoPatch)

		if infoBase == currentBase {
			// Same 3-part version. Normally this is the current version placeholder row
			// and is skipped. For a hotfix update (only the 4th build field differs) the
			// nupkg name still carries the same 3-part version, so force-install its full
			// package. Delta cannot be used here because it patches against a 3-part
			// baseline that is indistinguishable from the current install.
			if forceFull && info.Full != nil {
				res = append(res, VersionInfo{Version: info.Version, Full: info.Full})
			}
			continue
		}
		// Collect all versions newer than current
		if versionGreater(infoBase, currentBase) {
			res = append(res, info)
		}
	}

	return res, nil
}

func extractVersionFromFileName(name string) string {
	base := filepath.Base(name)
	base = strings.TrimSuffix(base, ".nupkg")

	lower := strings.ToLower(base)
	if strings.HasSuffix(lower, "-full") {
		base = base[:len(base)-len("-full")]
		lower = lower[:len(lower)-len("-full")]
	} else if strings.HasSuffix(lower, "-delta") {
		base = base[:len(base)-len("-delta")]
		lower = lower[:len(lower)-len("-delta")]
	}

	start := -1
	for i, ch := range base {
		if ch >= '0' && ch <= '9' {
			start = i
			break
		}
	}
	if start == -1 || start >= len(base) {
		return ""
	}
	return base[start:]
}

func versionGreater(a, b string) bool {
	am, an, ap, aextra := splitVersion(a)
	bm, bn, bp, bextra := splitVersion(b)
	if am != bm {
		return am > bm
	}
	if an != bn {
		return an > bn
	}
	if ap != bp {
		return ap > bp
	}
	if aextra == bextra {
		return false
	}
	if aextra == "" {
		return true
	}
	if bextra == "" {
		return false
	}
	return aextra > bextra
}

func splitVersion(v string) (int, int, int, string) {
	v = strings.TrimSpace(v)
	if v == "" {
		return 0, 0, 0, ""
	}
	body := v
	extra := ""
	if i := strings.Index(body, "-"); i >= 0 {
		extra = body[i+1:]
		body = body[:i]
	}
	parts := strings.Split(body, ".")
	parse := func(s string) int {
		if s == "" {
			return 0
		}
		n, err := strconv.Atoi(s)
		if err != nil {
			return 0
		}
		return n
	}
	maj, min, patch := 0, 0, 0
	if len(parts) > 0 {
		maj = parse(parts[0])
	}
	if len(parts) > 1 {
		min = parse(parts[1])
	}
	if len(parts) > 2 {
		patch = parse(parts[2])
	}
	return maj, min, patch, strings.ToLower(extra)
}

func safeVersionString(v string) string {
	s := strings.ReplaceAll(v, ":", "_")
	s = strings.ReplaceAll(s, "/", "_")
	s = strings.ReplaceAll(s, "\\", "_")
	return s
}

func findFullNupkgForVersion(baseDir, version string) string {
	patterns := []string{
		filepath.Join(baseDir, "*"+version+"*-full.nupkg"),
		filepath.Join(baseDir, "*"+version+"*full.nupkg"),
	}
	best := ""
	var bestTime time.Time
	for _, pat := range patterns {
		matches, _ := filepath.Glob(pat)
		for _, m := range matches {
			fi, err := os.Stat(m)
			if err != nil {
				continue
			}
			if best == "" || fi.ModTime().After(bestTime) {
				best = m
				bestTime = fi.ModTime()
			}
		}
	}
	return best
}

func buildTargetTree(log func(args ...interface{}), baseDir, installDir, currentVer string, chain []VersionInfo) (string, error) {
	if len(chain) == 0 {
		return "", nil
	}
	finalVersion := chain[len(chain)-1].Version
	root := filepath.Join(baseDir, "process_full", safeVersionString(finalVersion))
	if err := os.RemoveAll(root); err != nil {
		return "", err
	}
	if err := os.MkdirAll(root, 0755); err != nil {
		return "", err
	}

	// The base tree (a copy of the current install) is only needed when the first step
	// in the chain is a delta patch, which is applied on top of it. If the first step is
	// a full package, it wipes and re-extracts root anyway, so copying the base is wasted
	// work and can fail (e.g. when the install dir contains junctions/symlinks such as a
	// linked "resources" folder in a dev build). Skip the base copy in that case.
	needBase := len(chain) > 0 && chain[0].Delta != nil
	if needBase {
		log("copy base from install dir:", installDir)
		if err := copyDirWithProgressStage(log, installDir, root, 8, 20, "Building new version files"); err != nil {
			baseFull := ""
			if strings.TrimSpace(currentVer) != "" {
				baseFull = findFullNupkgForVersion(baseDir, currentVer)
			}
			if baseFull == "" {
				return "", err
			}
			setInstallProgress(10)
			log("copy base failed, fallback to extract base current full:", baseFull, "err:", err)
			if err2 := extractNupkgLibNet45(baseFull, root); err2 != nil {
				return "", err2
			}
		}
	} else {
		log("first chain step is a full package, skip copying base tree")
	}

	for _, info := range chain {
		if isCanceled() {
			return "", errCanceled
		}
		if info.Delta != nil {
			setInstallProgress(20)
			nupkgPath := filepath.Join(baseDir, info.Delta.FileName)
			log("apply delta:", nupkgPath)
			if err := applyDeltaNupkg(log, nupkgPath, root); err != nil {
				fullPath := ""
				if info.Full != nil {
					fullPath = filepath.Join(baseDir, info.Full.FileName)
				} else {
					fullPath = findFullNupkgForVersion(baseDir, info.Version)
				}
				if fullPath == "" {
					return "", err
				}
				log("apply delta failed, fallback to extract full:", fullPath, "err:", err)
				if err2 := os.RemoveAll(root); err2 != nil {
					return "", err2
				}
				if err2 := os.MkdirAll(root, 0755); err2 != nil {
					return "", err2
				}
				if err2 := extractNupkgLibNet45(fullPath, root); err2 != nil {
					return "", err2
				}
			}
		} else if info.Full != nil {
			setInstallProgress(12)
			if err := os.RemoveAll(root); err != nil {
				return "", err
			}
			if err := os.MkdirAll(root, 0755); err != nil {
				return "", err
			}
			fullPath := filepath.Join(baseDir, info.Full.FileName)
			log("extract full:", fullPath)
			if err := extractNupkgLibNet45(fullPath, root); err != nil {
				return "", err
			}
		} else {
			return "", fmt.Errorf("no package for version %s", info.Version)
		}
	}
	setInstallProgress(28)
	return root, nil
}

func readShasumFile(f *zip.File) (string, int64, error) {
	rc, err := f.Open()
	if err != nil {
		return "", 0, err
	}
	defer rc.Close()
	data, err := io.ReadAll(rc)
	if err != nil {
		return "", 0, err
	}
	line := strings.TrimSpace(string(data))
	parts := strings.Fields(line)
	if len(parts) < 2 {
		return "", 0, fmt.Errorf("invalid shasum file %s", f.Name)
	}
	shaRaw := strings.TrimPrefix(parts[0], "\uFEFF")
	shaHex := make([]byte, 0, 40)
	for i := 0; i < len(shaRaw) && len(shaHex) < 40; i++ {
		c := shaRaw[i]
		if (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') {
			shaHex = append(shaHex, c)
		}
	}
	sha := shaRaw
	if len(shaHex) == 40 {
		sha = string(shaHex)
	}
	sizeIndex := len(parts) - 1
	size, err := strconv.ParseInt(parts[sizeIndex], 10, 64)
	if err != nil {
		return "", 0, err
	}
	return sha, size, nil
}

func applyDeltaNupkg(log func(args ...interface{}), nupkgPath, root string) error {
	r, err := zip.OpenReader(nupkgPath)
	if err != nil {
		return err
	}
	defer r.Close()
	filesByName := make(map[string]*zip.File)
	for _, f := range r.File {
		filesByName[f.Name] = f
	}
	prefix := "lib/net45/"
	keep := make(map[string]bool)
	patchedCount := 0
	candidateCount := 0
	criticalBases := map[string]struct{}{
		"CrealityPrint.exe":        {},
		"CrealityPrint_Slicer.dll": {},
	}
	criticalSeen := make(map[string]bool, len(criticalBases))
	criticalPatched := make(map[string]bool, len(criticalBases))
	criticalErr := make(map[string]error, len(criticalBases))
	skippedEmptyPatch := 0
	skippedInvalidShasum := 0
	skippedPatchApply := 0
	skippedStat := 0
	skippedSizeMismatch := 0
	skippedShaMismatch := 0
	logLimit := 50
	logged := 0
	for _, f := range r.File {
		if isCanceled() {
			return errCanceled
		}
		name := f.Name
		if !strings.HasPrefix(name, prefix) {
			continue
		}
		rel := strings.TrimPrefix(name, prefix)
		if !strings.HasSuffix(strings.ToLower(rel), ".shasum") {
			continue
		}
		targetRel := strings.TrimSuffix(rel, ".shasum")

		// Skip ExecutionStub files - they are generated by Squirrel installer and not part of NSIS package
		baseName := filepath.Base(targetRel)
		if strings.Contains(strings.ToLower(baseName), "executionstub") {
			if logged < logLimit {
				log("delta skip ExecutionStub patch:", targetRel)
				logged++
			}
			continue
		}
		key := filepath.ToSlash(targetRel)
		keep[key] = true
		oldPath := filepath.Join(root, targetRel)
		base := filepath.Base(oldPath)
		_, isCritical := criticalBases[base]
		patchCandidates := make([]struct {
			name string
			f    *zip.File
		}, 0, 2)
		for _, pn := range []string{prefix + targetRel + ".diff", prefix + targetRel + ".bsdiff"} {
			pf, ok := filesByName[pn]
			if !ok {
				continue
			}
			lowerPn := strings.ToLower(pn)
			minSize := uint64(1)
			if strings.HasSuffix(lowerPn, ".diff") {
				minSize = 4
			} else if strings.HasSuffix(lowerPn, ".bsdiff") {
				minSize = 8
			}
			if pf.UncompressedSize64 < minSize {
				skippedEmptyPatch++
				if logged < logLimit {
					log("delta skip empty patch:", pn)
					logged++
				}
				continue
			}
			patchCandidates = append(patchCandidates, struct {
				name string
				f    *zip.File
			}{name: pn, f: pf})
		}
		if len(patchCandidates) == 0 {
			continue
		}
		candidateCount++
		if isCritical {
			criticalSeen[base] = true
		}
		sha, size, err := readShasumFile(f)
		if err != nil {
			skippedInvalidShasum++
			if isCritical {
				criticalSeen[base] = true
				criticalErr[base] = err
			}
			if logged < logLimit {
				log("delta skip invalid shasum:", f.Name, "err:", err)
				logged++
			}
			continue
		}
		tmpPath := oldPath + ".new"
		applyPatch := func(pf *zip.File) error {
			pr, err := pf.Open()
			if err != nil {
				return err
			}
			header := make([]byte, 8)
			n, _ := io.ReadFull(pr, header)
			_ = pr.Close()

			isBsdiff := n == 8 && string(header[:8]) == "BSDIFF40"
			pr2, err := pf.Open()
			if err != nil {
				return err
			}
			defer pr2.Close()
			if isBsdiff {
				return applyBsdiffFile(oldPath, tmpPath, pr2)
			}
			return applyMsDiffFile(oldPath, tmpPath, pr2)
		}

		var lastErr error
		applied := false
		for _, pc := range patchCandidates {
			if err := applyPatch(pc.f); err == nil {
				applied = true
				break
			} else {
				lastErr = err
				_ = os.Remove(tmpPath)
				if logged < logLimit {
					log("delta patch failed:", targetRel, "patch:", pc.name, "err:", err)
					logged++
				}
			}
		}
		if !applied {
			if fullF, ok := filesByName[prefix+targetRel]; ok && !fullF.FileInfo().IsDir() && fullF.UncompressedSize64 > 0 {
				rc, err := fullF.Open()
				if err == nil {
					out, err2 := os.Create(tmpPath)
					if err2 == nil {
						_, err3 := io.Copy(out, cancelReader{r: rc})
						_ = out.Close()
						_ = rc.Close()
						if err3 == nil {
							applied = true
							if logged < logLimit {
								log("delta fallback to full file:", targetRel)
								logged++
							}
						} else {
							_ = os.Remove(tmpPath)
							lastErr = err3
						}
					} else {
						_ = rc.Close()
						lastErr = err2
					}
				} else {
					lastErr = err
				}
			}
		}
		if !applied {
			_ = os.Remove(tmpPath)
			skippedPatchApply++
			if isCritical && lastErr != nil {
				criticalErr[base] = lastErr
			}
			continue
		}
		fi, err := os.Stat(tmpPath)
		if err != nil {
			os.Remove(tmpPath)
			skippedStat++
			return err
		}
		if fi.Size() != size {
			os.Remove(tmpPath)
			skippedSizeMismatch++
			if isCritical {
				criticalErr[base] = fmt.Errorf("size mismatch got %d want %d", fi.Size(), size)
			}
			if logged < logLimit {
				log("delta size mismatch:", targetRel, "got:", fi.Size(), "want:", size)
				logged++
			}
			continue
		}
		fNew, err := os.Open(tmpPath)
		if err != nil {
			os.Remove(tmpPath)
			return err
		}
		h := sha1.New()
		if _, err := io.Copy(h, cancelReader{r: fNew}); err != nil {
			fNew.Close()
			os.Remove(tmpPath)
			return err
		}
		fNew.Close()
		gotSha := fmt.Sprintf("%x", h.Sum(nil))
		if !strings.EqualFold(gotSha, sha) {
			os.Remove(tmpPath)
			skippedShaMismatch++
			if isCritical {
				criticalErr[base] = fmt.Errorf("sha mismatch got %s want %s", gotSha, sha)
			}
			if logged < logLimit {
				log("delta sha mismatch:", targetRel, "got:", gotSha, "want:", sha)
				logged++
			}
			continue
		}
		if err := os.Remove(oldPath); err != nil && !os.IsNotExist(err) {
			os.Remove(tmpPath)
			return err
		}
		if err := os.Rename(tmpPath, oldPath); err != nil {
			os.Remove(tmpPath)
			return err
		}
		patchedCount++
		if isCritical {
			criticalPatched[base] = true
		}
	}
	for _, f := range r.File {
		if isCanceled() {
			return errCanceled
		}
		name := f.Name
		if !strings.HasPrefix(name, prefix) {
			continue
		}
		if f.FileInfo().IsDir() {
			continue
		}
		rel := strings.TrimPrefix(name, prefix)
		lower := strings.ToLower(rel)
		if strings.HasSuffix(lower, ".shasum") || strings.HasSuffix(lower, ".diff") || strings.HasSuffix(lower, ".bsdiff") {
			continue
		}
		key := filepath.ToSlash(rel)
		keep[key] = true
		targetPath := filepath.Join(root, rel)
		if err := os.MkdirAll(filepath.Dir(targetPath), 0755); err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			return err
		}
		out, err := os.Create(targetPath)
		if err != nil {
			rc.Close()
			return err
		}
		if _, err := io.Copy(out, cancelReader{r: rc}); err != nil {
			rc.Close()
			out.Close()
			return err
		}
		rc.Close()
		out.Close()
	}
	err = filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if isCanceled() {
			return errCanceled
		}
		if info.IsDir() {
			return nil
		}
		rel, err := filepath.Rel(root, path)
		if err != nil {
			return err
		}
		rel = filepath.ToSlash(rel)
		lower := strings.ToLower(rel)
		if !strings.HasSuffix(lower, ".exe") && !strings.HasSuffix(lower, ".dll") {
			return nil
		}
		if !keep[rel] {
			if err := os.Remove(path); err != nil && !os.IsNotExist(err) {
				return err
			}
		}
		return nil
	})
	if err != nil {
		return err
	}
	if candidateCount > 0 || skippedEmptyPatch > 0 || skippedInvalidShasum > 0 {
		log("delta summary:",
			"candidates", candidateCount,
			"patched", patchedCount,
			"emptyPatch", skippedEmptyPatch,
			"invalidShasum", skippedInvalidShasum,
			"patchFail", skippedPatchApply,
			"sizeMismatch", skippedSizeMismatch,
			"shaMismatch", skippedShaMismatch,
		)
	}
	if candidateCount > 0 && patchedCount == 0 {
		return errDeltaNoop
	}
	if len(criticalSeen) > 0 {
		var failed []string
		for base := range criticalSeen {
			if criticalPatched[base] {
				continue
			}
			if e := criticalErr[base]; e != nil {
				failed = append(failed, base+": "+e.Error())
			} else {
				failed = append(failed, base)
			}
		}
		if len(failed) > 0 {
			return fmt.Errorf("%w: %s", errDeltaCriticalNotPatched, strings.Join(failed, "; "))
		}
	}
	return nil
}

func applyBsdiffFile(oldPath, newPath string, patch io.Reader) error {
	patchData, err := io.ReadAll(cancelReader{r: patch})
	if err != nil {
		return err
	}
	if len(patchData) == 0 {
		return binarydist.ErrCorrupt
	}

	oldFile, err := os.Open(oldPath)
	if err != nil {
		return err
	}
	defer oldFile.Close()

	newFile, err := os.Create(newPath)
	if err != nil {
		return err
	}
	err = binarydist.Patch(oldFile, newFile, bytes.NewReader(patchData))
	_ = newFile.Close()
	if err == nil {
		return nil
	}
	_ = os.Remove(newPath)

	if !libbzip2Available() {
		return err
	}

	msg := strings.ToLower(err.Error())
	isLikelyBzip2CompatIssue := strings.Contains(msg, "deprecated") ||
		strings.Contains(msg, "random") ||
		strings.Contains(msg, "corrupt patch") ||
		errors.Is(err, binarydist.ErrCorrupt)
	if !isLikelyBzip2CompatIssue {
		return err
	}

	oldData, err2 := os.ReadFile(oldPath)
	if err2 != nil {
		return err
	}
	newData, err2 := bspatchLibBzip2(oldData, patchData)
	if err2 != nil {
		return fmt.Errorf("bsdiff patch failed: %v; libbzip2 patch failed: %v", err, err2)
	}
	if err2 := os.WriteFile(newPath, newData, 0644); err2 != nil {
		return err2
	}
	return nil
}

func applyMsDiffFile(oldPath, newPath string, patch io.Reader) error {
	oldData, err := os.ReadFile(oldPath)
	if err != nil {
		return err
	}
	patchData, err := io.ReadAll(patch)
	if err != nil {
		return err
	}
	if len(patchData) == 0 {
		return errors.New("empty msdiff patch")
	}

	sig := ""
	if len(patchData) >= 4 {
		sig = string(patchData[:4])
	}

	inOld := deltaInput{editable: 0}
	if len(oldData) > 0 {
		inOld.lpStart = uintptr(unsafe.Pointer(&oldData[0]))
		inOld.uSize = uintptr(len(oldData))
	}
	inPatch := deltaInput{editable: 0}
	if len(patchData) > 0 {
		inPatch.lpStart = uintptr(unsafe.Pointer(&patchData[0]))
		inPatch.uSize = uintptr(len(patchData))
	}

	var out deltaOutput
	applyWithFlags := func(flags uintptr) (bool, error) {
		r, _, e := procApplyDeltaB.Call(
			flags,
			uintptr(unsafe.Pointer(&inOld)),
			uintptr(unsafe.Pointer(&inPatch)),
			uintptr(unsafe.Pointer(&out)),
		)
		if r != 0 {
			return true, nil
		}
		if e != nil && e != syscall.Errno(0) {
			return false, e
		}
		return false, errors.New("ApplyDeltaB failed")
	}

	var lastErr error
	flagsList := []uintptr{0}
	if sig == "PA19" {
		flagsList = []uintptr{1, 0}
	} else if sig != "PA30" && sig != "PA19" {
		flagsList = []uintptr{0, 1}
	}

	ok := false
	for _, flags := range flagsList {
		if applied, e := applyWithFlags(flags); applied {
			ok = true
			break
		} else {
			lastErr = e
		}
	}
	if !ok {
		return fmt.Errorf("ApplyDeltaB failed (sig=%q): %w", sig, lastErr)
	}
	defer procDeltaFree.Call(out.lpStart)

	var buf []byte
	if out.uSize > 0 && out.lpStart != 0 {
		buf = unsafe.Slice((*byte)(unsafe.Pointer(out.lpStart)), int(out.uSize))
	}

	if err := os.WriteFile(newPath, buf, 0644); err != nil {
		return err
	}
	return nil
}

func extractNupkgLibNet45(nupkgPath, destRoot string) error {
	r, err := zip.OpenReader(nupkgPath)
	if err != nil {
		return err
	}
	defer r.Close()

	prefix := "lib/net45/"
	for _, f := range r.File {
		if isCanceled() {
			return errCanceled
		}
		name := f.Name
		if !strings.HasPrefix(name, prefix) {
			continue
		}
		rel := strings.TrimPrefix(name, prefix)
		if rel == "" {
			continue
		}
		targetPath := filepath.Join(destRoot, rel)
		if f.FileInfo().IsDir() {
			if err := os.MkdirAll(targetPath, 0755); err != nil {
				return err
			}
			continue
		}
		if err := os.MkdirAll(filepath.Dir(targetPath), 0755); err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			return err
		}
		out, err := os.Create(targetPath)
		if err != nil {
			rc.Close()
			return err
		}
		_, err = io.Copy(out, cancelReader{r: rc})
		rc.Close()
		out.Close()
		if err != nil {
			return err
		}
	}
	return nil
}

func copyDir(src, dst string) error {
	return filepath.Walk(src, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if isCanceled() {
			return errCanceled
		}
		rel, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}
		target := filepath.Join(dst, rel)
		if info.IsDir() {
			return os.MkdirAll(target, info.Mode())
		}
		if err := os.MkdirAll(filepath.Dir(target), 0755); err != nil {
			return err
		}
		in, err := os.Open(path)
		if err != nil {
			return err
		}
		out, err := os.Create(target)
		if err != nil {
			in.Close()
			return err
		}
		_, err = io.Copy(out, cancelReader{r: in})
		in.Close()
		out.Close()
		return err
	})
}
