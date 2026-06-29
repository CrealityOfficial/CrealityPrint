# 绠€鏄撴ā寮忓乏涓婅鑰楁潗鏄犲皠闈㈡澘缂╂斁璁捐

## 1. 鑳屾櫙涓庣洰鏍?
褰撳墠绠€鏄撴ā寮忎腑鐨勫父椹昏€楁潗鏄犲皠闈㈡澘鐢?`GLCanvas3D::_compute_resident_filament_rect_simple()` 鐩存帴杩斿洖鍥哄畾鐭╁舰锛岄殢鍚庡湪 `GLCanvas3D::_render_resident_filament_panel_simple()` 涓寜璇ョ煩褰㈡覆鏌撱€?
鐜扮姸鐗瑰緛锛?
- 闈㈡澘浣嶇疆鍥哄畾鍦ㄥ乏涓婅銆?- 闈㈡澘瀹介珮鍥哄畾锛屼笉鑳芥寜鐢ㄦ埛涔犳儻璋冩暣銆?- 闈㈡澘鍐呴儴 `ResidentFilamentMappingView::render_panel()` 宸茬粡鎸夌埗瀹瑰櫒鍓╀綑绌洪棿鑷€傚簲甯冨眬锛屽洜姝ゆ洿閫傚悎鍦ㄥ灞傚仛缂╂斁锛岃€屼笉鏄厛閲嶆瀯鍐呴儴鍗＄墖甯冨眬銆?
鏈璁＄殑鐩爣鏄細

- 涓哄乏涓婅鑰楁潗鏄犲皠闈㈡澘鎻愪緵涓婁笅宸﹀彸鍥涜竟缂╂斁鑳藉姏銆?- 淇濇寔褰撳墠鍐呴儴甯冨眬缁撴瀯涓嶅彉锛屽彧璋冩暣澶栧眰闈㈡澘鐭╁舰銆?- 鏀寔灏哄鍜屼綅缃寔涔呭寲锛岄噸鍚悗鎭㈠鐢ㄦ埛涓婃璋冩暣缁撴灉銆?- 淇濊瘉鍦ㄥ皬绐楀彛銆佺缉鏀炬瘮渚嬪彉鍖栥€佸満鏅垏鎹㈡椂涓嶅嚭鐜伴潰鏉胯窇椋炴垨灏哄澶辨帶銆?
鏈璁捐鑼冨洿锛?
- 鍙仛甯搁┗鑰楁潗鏄犲皠闈㈡澘澶栨 resize銆?- 涓嶅仛鍐呴儴鍒楄〃鍖?/ 棰勮鍖虹殑 split 姣斾緥鎷栨嫿銆?- 涓嶅仛鍥涜缂╂斁锛屽洓杈圭缉鏀句綔涓虹涓€闃舵瀹炵幇銆?
## 2. 褰撳墠浠ｇ爜钀界偣

鏍稿績鏂囦欢涓庡嚱鏁帮細

- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`
  - `_compute_resident_filament_rect_simple()`
  - `_render_resident_filament_panel_simple(const ImVec4& rect)`
  - `_render_overlays_easymode()`
- `src/slic3r/GUI/GLCanvas3D.hpp`
  - easy mode 鐩稿叧鎴愬憳鍖?  - `_compute_resident_filament_rect_simple()` 鍜?`_render_resident_filament_panel_simple()` 鐨勫０鏄?- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
  - `ImGuiFilamentPanel::Render()`
- `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.cpp`
  - `render_panel()`
  - `render_row()`
  - `render_preview_card()`

鑱岃矗鍒嗗伐锛?
- `GLCanvas3DSimple.cpp` 璐熻矗闈㈡澘澶栨鐨勭煩褰€佸畾浣嶅拰 overlay 灞備氦浜掋€?- `ImGuiFilamentPanel.cpp` 璐熻矗闈㈡澘鍐呭鏋勫缓涓庡洖璋冿紝涓嶅簲鎵挎媴瀹夸富鐭╁舰 resize 鑱岃矗銆?- `ResidentFilamentMappingView.cpp` 璐熻矗鍐呭缁樺埗锛岀户缁寜鐖跺鍣ㄨ嚜閫傚簲銆?
鍥犳 resize 閫昏緫搴旀斁鍦?`GLCanvas3D` easy mode overlay 灞傘€?
## 3. 璁捐鎬昏

鏁翠綋鏂规鍒嗕笁灞傦細

1. 榛樿鐭╁舰璁＄畻灞?   - 淇濈暀鐜版湁榛樿甯冨眬閫昏緫銆?   - 褰撶敤鎴蜂粠鏈皟鏁存椂锛岀户缁娇鐢ㄩ粯璁ょ煩褰€?
2. 鐢ㄦ埛瑕嗙洊鐭╁舰灞?   - 褰撶敤鎴烽€氳繃鎷栨嫿璋冩暣鍚庯紝璁板綍鐢ㄦ埛鐭╁舰銆?   - 鍚庣画娓叉煋浼樺厛浣跨敤鐢ㄦ埛鐭╁舰銆?
3. 浜や簰涓庣害鏉熷眰
   - 鍩轰簬鍥涜竟鐑尯鍋?resize hit-test銆?   - 鎷栨嫿涓疄鏃惰绠楀€欓€夌煩褰€?   - 姣忓抚鍋?clamp锛岀‘淇濈煩褰㈣惤鍦ㄥ悎娉曞尯鍩熴€?   - 榧犳爣閲婃斁鍚庡啀鎸佷箙鍖栧埌 `AppConfig`銆?
## 4. 鎴愬憳鍙橀噺璁捐

### 4.1 寤鸿鏂板鐨勬暟鎹粨鏋?
寤鸿鍦?`GLCanvas3D.hpp` 鐨?easy mode 鎴愬憳鍖洪檮杩戞柊澧炰互涓嬬粨鏋勶細

```cpp
enum class ResidentFilamentResizeEdge : unsigned int {
    None   = 0,
    Left   = 1 << 0,
    Right  = 1 << 1,
    Top    = 1 << 2,
    Bottom = 1 << 3
};

struct ResidentFilamentPanelLayoutState {
    bool  loaded_from_config = false;
    bool  user_override      = false;
    float x                  = 0.0f; // logical px, unscaled
    float y                  = 0.0f; // logical px, unscaled
    float w                  = 0.0f; // logical px, unscaled
    float h                  = 0.0f; // logical px, unscaled
};

struct ResidentFilamentPanelResizeState {
    ResidentFilamentResizeEdge hot_edge    = ResidentFilamentResizeEdge::None;
    ResidentFilamentResizeEdge active_edge = ResidentFilamentResizeEdge::None;
    bool   dragging                        = false;
    ImVec2 drag_start_mouse                = ImVec2(0.0f, 0.0f);
    ImVec4 drag_start_rect                 = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // scaled px
    ImVec4 live_rect                       = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // scaled px
    bool   dirty_since_mouse_down          = false;
};
```

### 4.2 寤鸿鏂板鎴愬憳鍙橀噺

寤鸿鍦?`GLCanvas3D` 涓柊澧烇細

```cpp
ResidentFilamentPanelLayoutState m_resident_filament_layout_state;
ResidentFilamentPanelResizeState m_resident_filament_resize_state;
```

### 4.3 涓轰粈涔堣繖鏍锋媶

`LayoutState` 涓?`ResizeState` 鍒嗙鏈夊嚑涓ソ澶勶細

- `LayoutState` 琛ㄧず鈥滄渶缁堝竷灞€缁撴灉鈥濓紝鍙寔涔呭寲銆?- `ResizeState` 琛ㄧず鈥滄湰娆￠紶鏍囦氦浜掕繃绋嬩腑鐨勪复鏃剁姸鎬佲€濓紝涓嶅彲鎸佷箙鍖栥€?- 鍦烘櫙鍒锋柊鎴?easy mode 鍒囨崲鏃讹紝鍙渶淇濈暀 `LayoutState`锛岄噸缃?`ResizeState`銆?- 鍚庣画濡傛灉闇€瑕佸鍔犫€滄仮澶嶉粯璁も€濄€佲€滃弻鍑昏竟妗嗗浣嶁€濄€佲€滃惛闄勫埌榛樿灏哄鈥濈瓑琛屼负锛屼篃涓嶄細姹℃煋鏈€缁堝竷灞€鏁版嵁銆?
### 4.4 logical px 瀛樺偍绾﹀畾

寤鸿 `LayoutState` 涓殑 `x/y/w/h` 瀛樺偍涓衡€滄湭涔?`get_scale()` 鐨勯€昏緫鍍忕礌鍊尖€濄€?
鍘熷洜锛?
- 褰撳墠 easy mode UI 骞挎硾渚濊禆 `view_scale` 鎴?`get_scale()`銆?- 濡傛灉鐩存帴瀛樺睆骞曞儚绱犲€硷紝鍦ㄧ郴缁?DPI 鍙樺寲銆佺獥鍙ｈ法灞忔垨杩愯鏃?scale 鍙樺寲鏃讹紝鐢ㄦ埛甯冨眬浼氬彉褰€?- 瀛?logical px 鍚庯紝鍦ㄥ疄闄呰绠楁椂缁熶竴涔樹互 `scale`锛屾洿绋炽€?
鎹㈢畻瑙勫垯锛?
- 璇婚厤缃悗鍙備笌娓叉煋锛歚scaled = logical * scale`
- 浜や簰缁撴潫鍥炲啓閰嶇疆锛歚logical = scaled / scale`

## 5. 闇€瑕佹柊澧炵殑鏂规硶璁捐

寤鸿鍦?`GLCanvas3D.hpp` 涓鍔犱互涓嬪０鏄庯細

```cpp
ImVec4 _compute_resident_filament_rect_default_simple() const;
ImVec4 _clamp_resident_filament_rect_simple(const ImVec4& rect, ResidentFilamentResizeEdge active_edge) const;
void   _load_resident_filament_layout_simple();
void   _save_resident_filament_layout_simple() const;
void   _reset_resident_filament_layout_simple();
ResidentFilamentResizeEdge _hit_test_resident_filament_resize_edge_simple(const ImVec4& rect, const ImVec2& mouse_pos) const;
void   _update_resident_filament_resize_state_simple(const ImVec4& current_rect);
void   _render_resident_filament_resize_handles_simple(const ImVec4& rect) const;
```

鑱岃矗寤鸿锛?
- `_compute_resident_filament_rect_default_simple()`
  - 杩斿洖褰撳墠榛樿鐭╁舰銆?  - 鎶婄幇鏈?`_compute_resident_filament_rect_simple()` 鐨勯粯璁よ绠楅€昏緫鎸埌杩欓噷銆?
- `_compute_resident_filament_rect_simple()`
  - 鎴愪负缁熶竴鍑哄彛銆?  - 鎸夆€滈粯璁ょ煩褰?+ 鐢ㄦ埛瑕嗙洊 + clamp鈥濊繑鍥炴渶缁堢煩褰€?
- `_load_resident_filament_layout_simple()`
  - 浠?`AppConfig` 璇荤敤鎴峰竷灞€銆?  - 鍙渶鍦?easy mode 鍒濆鍖栨椂鎴栭娆℃覆鏌撳墠鎵ц涓€娆°€?
- `_save_resident_filament_layout_simple()`
  - 鎶婂綋鍓?`LayoutState` 鍐欏叆 `AppConfig`銆?  - 鍙湪鎷栨嫿缁撴潫鏃惰皟鐢ㄣ€?
- `_reset_resident_filament_layout_simple()`
  - 娓呮帀鐢ㄦ埛瑕嗙洊锛屾仮澶嶉粯璁ゃ€?  - 绗竴闃舵鍙互鍙湪浠ｇ爜閲岄鐣欙紝涓嶅己鍒跺厛鎸?UI 鍏ュ彛銆?
- `_hit_test_resident_filament_resize_edge_simple(...)`
  - 缁欏畾褰撳墠 rect 鍜岄紶鏍囩偣锛岃繑鍥炲綋鍓嶅懡涓殑杈广€?
- `_update_resident_filament_resize_state_simple(...)`
  - 澶勭悊 hover銆佹寜涓嬨€佹嫋鎷姐€侀噴鏀惧畬鏁寸姸鎬佹満銆?
- `_render_resident_filament_resize_handles_simple(...)`
  - 璐熻矗杈规鐑尯楂樹寒鍜岃皟璇曞彲瑙嗗寲銆?  - 绗竴闃舵鍙互鍙仛 hover 楂樹寒锛屼笉蹇呭仛澶嶆潅瑁呴グ銆?
## 6. 鎷栨嫿鐘舵€佹満璁捐

### 6.1 鐘舵€佸畾涔?
鎺ㄨ崘閲囩敤浠ヤ笅鐘舵€佽涔夛細

- `Idle`
  - 褰撳墠鏃犲懡涓竟锛屼笖鏃犳嫋鎷姐€?- `Hot`
  - 榧犳爣鍛戒腑鏌愭潯杈癸紝浣嗗皻鏈寜涓嬨€?- `Dragging`
  - 宸插湪鏌愭潯杈逛笂鎸変笅骞跺紑濮嬫嫋鍔ㄣ€?
鍙敱 `hot_edge`銆乣active_edge` 鍜?`dragging` 缁勫悎琛ㄨ揪锛屼笉蹇呴澶栧紩鍏?enum銆?
### 6.2 浜や簰娴佺▼

姣忓抚鎵ц椤哄簭寤鸿濡備笅锛?
1. 鍏堣绠楀綋鍓嶆渶缁?rect銆?2. 鍩轰簬褰撳墠 rect 鍋氳竟缂?hit-test銆?3. 鑻ユ湭鎷栨嫿锛屾牴鎹懡涓竟璁剧疆榧犳爣鏍峰紡銆?4. 鑻ュ湪杈圭紭鎸変笅宸﹂敭锛岃繘鍏ユ嫋鎷界姸鎬併€?5. 鎷栨嫿涓牴鎹紶鏍?delta 鐢熸垚鍊欓€?rect銆?6. 瀵瑰€欓€?rect 鍋?clamp銆?7. 鎶?clamp 鍚庣粨鏋滃啓鍏?`live_rect` 鍜?`LayoutState`銆?8. 榧犳爣閲婃斁鏃堕€€鍑烘嫋鎷斤紝鑻ュ昂瀵告湁鍙樺垯鎸佷箙鍖栥€?
### 6.3 hit-test 瑙勫垯

绗竴闃舵鍙仛鍥涜竟锛屼笉鍋氬洓瑙掋€?
寤鸿鐑尯鍘氬害锛?
- `edge_hit_thickness = 6.0f * scale`

鍛戒腑浼樺厛绾у缓璁細

- 宸︺€佸彸浼樺厛浜庝笂銆佷笅銆?- 鑻ラ紶鏍囧悓鏃跺浜庤竟瑙掗偦鍩燂紝鍙鏇存帴杩戠殑鍗曡竟銆?- 绗竴闃舵涓嶈繘鍏?`ResizeNWSE / ResizeNESW` 绛夎缂╂斁 cursor銆?
杩欐牱鍋氱殑鍘熷洜鏄細

- 鐢ㄦ埛鐩爣鏄帶鍒跺楂橈紝涓嶄竴瀹氶渶瑕佸洓瑙掔缉鏀俱€?- 鍥涜竟閫昏緫鏇寸ǔ锛岃瑙︽洿灏戙€?- 鍚庣画濡傛灉瑕佸姞鍥涜缂╂斁锛屽彲浠ュ湪鐜版湁 hit-test 涓婃墿灞曘€?
### 6.4 cursor 瑙勫垯

寤鸿锛?
- 鍛戒腑 `Left` 鎴?`Right` 鏃舵樉绀?`ImGuiMouseCursor_ResizeEW`
- 鍛戒腑 `Top` 鎴?`Bottom` 鏃舵樉绀?`ImGuiMouseCursor_ResizeNS`

### 6.5 鎷栨嫿閫昏緫

褰撻紶鏍囨寜涓嬫煇鏉¤竟鏃讹細

```cpp
resize_state.dragging = true;
resize_state.active_edge = hot_edge;
resize_state.drag_start_mouse = ImGui::GetMousePos();
resize_state.drag_start_rect = current_rect;
resize_state.live_rect = current_rect;
resize_state.dirty_since_mouse_down = false;
```

鎷栨嫿涓細

```cpp
ImVec2 delta = ImGui::GetMousePos() - drag_start_mouse;
ImVec4 candidate = drag_start_rect;

switch (active_edge) {
case Left:
    candidate.x = drag_start_rect.x + delta.x;
    candidate.z = drag_start_rect.z - delta.x;
    break;
case Right:
    candidate.z = drag_start_rect.z + delta.x;
    break;
case Top:
    candidate.y = drag_start_rect.y + delta.y;
    candidate.w = drag_start_rect.w - delta.y;
    break;
case Bottom:
    candidate.w = drag_start_rect.w + delta.y;
    break;
default:
    break;
}
```

鐒跺悗鍋?clamp锛?
```cpp
candidate = _clamp_resident_filament_rect_simple(candidate, active_edge);
```

鍐嶅皢缁撴灉鍚屾鍒板竷灞€鐘舵€侊細

```cpp
resize_state.live_rect = candidate;
layout_state.user_override = true;
layout_state.x = candidate.x / scale;
layout_state.y = candidate.y / scale;
layout_state.w = candidate.z / scale;
layout_state.h = candidate.w / scale;
resize_state.dirty_since_mouse_down = true;
```

榧犳爣閲婃斁鏃讹細

- `dragging = false`
- `active_edge = None`
- 濡傛灉 `dirty_since_mouse_down == true`锛屾墽琛?`_save_resident_filament_layout_simple()`

### 6.6 涓轰粈涔堜笉瑕佸湪鎷栨嫿涓绻佸啓閰嶇疆

涓嶅缓璁嫋鎷借繃绋嬩腑姣忓抚鍐?`AppConfig`锛屽師鍥犲涓嬶細

- 浼氫骇鐢熶笉蹇呰鐨勯厤缃啓鏀惧ぇ銆?- 瀹规槗閫犳垚閰嶇疆鑴忔爣璁伴绻佹尝鍔ㄣ€?- 鎷栨嫿缁撴潫缁熶竴鎻愪氦鏇寸鍚堜氦浜掕涔夈€?
## 7. clamp 瑙勫垯璁捐

### 7.1 鍘熷垯

clamp 蹇呴』鍚屾椂淇濊瘉锛?
- 闈㈡澘濮嬬粓鍦ㄥ睆骞曞彲瑙佽寖鍥村唴銆?- 闈㈡澘涓嶄細缂╁皬鍒扮牬鍧忓綋鍓嶅崱鐗囨帓鐗堛€?- 闈㈡澘涓嶄細鏀惧ぇ鍒板悶鎺夎繃澶?3D 宸ヤ綔鍖恒€?- 闈㈡澘涓婅竟缂樹笉閬尅椤堕儴涓诲伐鍏锋爮銆?
### 7.2 鍙鍖哄煙瀹氫箟

寤鸿浣跨敤褰撳墠 canvas 鍙鍖哄煙锛?
```cpp
const ImVec2 canvas_sz = ImGui::GetIO().DisplaySize;
```

寤鸿绾︽潫杈圭晫锛?
- `left_bound   = 8.0f * scale`
- `top_bound    = get_main_toolbar_height() + 14.0f * scale`
- `right_bound  = canvas_sz.x - 8.0f * scale`
- `bottom_bound = canvas_sz.y - 8.0f * scale`

璇存槑锛?
- `top_bound` 缁ф壙褰撳墠榛樿瀹氫綅閫昏緫锛岄伩鍏嶄笂杈圭紭鍘嬩綇椤堕儴宸ュ叿鏍忋€?- 鍙充晶涓嶉澶栦负瀵硅薄鎶藉眽璁╁嚭淇濈暀鍖猴紝淇濇寔瀹炵幇绠€鍗曘€?- AI 鎸夐挳鍦ㄥ彸涓嬭锛屼笉闇€瑕佸崟鐙负宸︿笂瑙掗潰鏉块鐣欓伩璁╁尯銆?
### 7.3 鏈€灏忓搴?
寤鸿鍚嶄箟鏈€灏忓搴︼細

```cpp
min_w = 410.0f * scale;
```

渚濇嵁锛?
- 琛岄」涓瓨鍦ㄥ涓浐瀹氬搴﹀厓绱犮€?- `render_row()` 涓満鏅崱鐗囥€佺澶淬€乻elector銆乥adge銆佸垹闄ゆ寜閽潎鏈夊浐瀹氬昂瀵搞€?- 瀹藉害杩囧皬浼氬鑷?selector 鍗＄墖鍜?badge 鍖轰弗閲嶆尋鍘嬨€?
淇濆畧绛栫暐锛?
```cpp
effective_min_w = min(min_w, right_bound - left_bound);
```

杩欐牱褰撶獥鍙ｆ湰韬お绐勬椂锛屽厑璁搁€€鍖栵紝浣嗕粛淇濊瘉 rect 涓嶈秺鐣屻€?
### 7.4 鏈€灏忛珮搴?
寤鸿鍚嶄箟鏈€灏忛珮搴︼細

```cpp
min_h = 520.0f * scale;
```

渚濇嵁锛?
- 椤堕儴 summary 鍗＄墖銆佸綋鍓嶇洏棰滆壊 header銆佷笁琛岄鑹叉粴鍔ㄥ尯銆佸叾浠栫洏鎶樺彔鍖恒€佸簳閮ㄩ瑙堝尯閮介渶瑕佸熀鏈睍绀虹┖闂淬€?- 鑻ラ珮搴﹀お灏忥紝搴曢儴棰勮鍖轰細澶卞幓浣跨敤浠峰€笺€?
鍚屾牱寤鸿閫€鍖栵細

```cpp
effective_min_h = min(min_h, bottom_bound - top_bound);
```

### 7.5 鏈€澶у搴?
寤鸿鍚嶄箟鏈€澶у搴︼細

```cpp
max_w = min(680.0f * scale, canvas_sz.x * 0.45f);
```

鍘熷洜锛?
- 淇濈暀瓒冲澶х殑 3D 宸ヤ綔鍖恒€?- 閬垮厤鐢ㄦ埛鎶婇潰鏉挎í鍚戞墿鍒拌繃浜庡じ寮犮€?
### 7.6 鏈€澶ч珮搴?
寤鸿鍚嶄箟鏈€澶ч珮搴︼細

```cpp
max_h = bottom_bound - top_bound;
```

鍘熷洜锛?
- 褰撳墠宸︿笂闈㈡澘鍨傜洿鏂瑰悜娌℃湁鍏朵粬蹇呴』閬胯鐨?overlay銆?- 楂樺害鐞嗚涓婂厑璁告媺鍒版帴杩戝簳閮紝鍙涓嶈秺鐣屽嵆鍙€?
### 7.7 鎸夎竟 clamp 瑙勫垯

涓嶅悓杈规嫋鍔ㄦ椂锛屽缓璁繚鎸佲€滃渚ч敋鐐逛笉鍔ㄢ€濄€?
#### 鎷栧乏杈?
- 鍥哄畾鍙宠竟鐣?`right = drag_start_rect.x + drag_start_rect.z`
- 鏂?`x = clamp(candidate.x, left_bound, right - effective_min_w)`
- 鏂?`w = right - x`

#### 鎷栧彸杈?
- 鍥哄畾宸﹁竟鐣?`left = drag_start_rect.x`
- 鏂?`right = clamp(candidate.x + candidate.z, left + effective_min_w, min(left + max_w, right_bound))`
- 鏂?`w = right - left`

#### 鎷栦笂杈?
- 鍥哄畾涓嬭竟鐣?`bottom = drag_start_rect.y + drag_start_rect.w`
- 鏂?`y = clamp(candidate.y, top_bound, bottom - effective_min_h)`
- 鏂?`h = bottom - y`

#### 鎷栦笅杈?
- 鍥哄畾涓婅竟鐣?`top = drag_start_rect.y`
- 鏂?`bottom = clamp(candidate.y + candidate.w, top + effective_min_h, bottom_bound)`
- 鏂?`h = bottom - top`

### 7.8 clamp 鍚庡厹搴?
涓嶈浠庡摢鏉¤竟杩涘叆锛岄兘寤鸿鏈€缁堝啀鍋氫竴娆＄粺涓€鍏滃簳锛?
```cpp
rect.z = clamp(rect.z, effective_min_w, max_w);
rect.w = clamp(rect.w, effective_min_h, max_h);
rect.x = clamp(rect.x, left_bound, right_bound - rect.z);
rect.y = clamp(rect.y, top_bound, bottom_bound - rect.w);
```

## 8. 閰嶇疆鎸佷箙鍖栬璁?
### 8.1 寤鸿閰嶇疆閿?
寤鸿浣跨敤 section 鍖栧瓨鍌紝閬垮厤姹℃煋 `app` 椤跺眰閿細

- section: `easy_mode`
- keys:
  - `resident_filament_panel_user_override`
  - `resident_filament_panel_x`
  - `resident_filament_panel_y`
  - `resident_filament_panel_w`
  - `resident_filament_panel_h`

寤鸿瀛樺偍鍊肩被鍨嬶細

- `user_override` 瀛?`"true"` / `"false"`
- `x/y/w/h` 瀛樺瓧绗︿覆鍖栨诞鐐规暟

### 8.2 璇诲彇绛栫暐

寤鸿鍦?`_init_ui_simple()` 鎴栭娆?`_compute_resident_filament_rect_simple()` 璋冪敤鍓嶈鍙栦竴娆★細

- 鑻ユ湭璇诲彇杩囬厤缃紝鍒欏厛 `_load_resident_filament_layout_simple()`
- 鑻ョ己澶变换涓€鍏抽敭瀛楁锛屽垯璁や负娌℃湁鐢ㄦ埛瑕嗙洊

### 8.3 淇濆瓨绛栫暐

寤鸿鍙湪浠ヤ笅鏃舵満淇濆瓨锛?
- 榧犳爣閲婃斁涓旀嫋鎷借繃绋嬩腑 rect 鏈夊彉鍖?- 鐢ㄦ埛鏄惧紡鎵ц鈥滄仮澶嶉粯璁も€濆悗

### 8.4 閲嶇疆绛栫暐

`_reset_resident_filament_layout_simple()` 寤鸿鍋氫互涓嬪姩浣滐細

- `user_override = false`
- 娓呯┖ `x/y/w/h`
- 鍐欏洖閰嶇疆

绗竴闃舵鍗充究娌℃湁 UI 鍏ュ彛锛屼篃寤鸿鍏堟妸璇ュ嚱鏁拌惤鍑烘潵锛屾柟渚垮悗缁帴鍏ワ細

- 鍙抽敭鑿滃崟
- 鏍囬鏍忓弻鍑?- 璁剧疆椤典腑鐨勨€滄仮澶嶉潰鏉垮竷灞€鈥?
## 9. 鏂囦欢鏀瑰姩璁捐

### 9.1 `src/slic3r/GUI/GLCanvas3D.hpp`

闇€瑕佹敼鍔細

- 鏂板 resize edge enum
- 鏂板 layout / resize state struct
- 鏂板 `m_resident_filament_layout_state`
- 鏂板 `m_resident_filament_resize_state`
- 鏂板鑻ュ共 helper 鍑芥暟澹版槑

鍘熷洜锛?
- 杩欎簺鐘舵€佸睘浜?`GLCanvas3D` overlay 瀹夸富灞傘€?- 涓嶅簲鏀惧叆 `ImGuiFilamentPanel`锛屽惁鍒欒亴璐ｄ笅娌夊埌鍐呭灞傘€?
### 9.2 `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`

闇€瑕佹敼鍔細

- 鎶藉嚭 `_compute_resident_filament_rect_default_simple()`
- 閲嶅啓 `_compute_resident_filament_rect_simple()`
- 鏂板 layout load / save / reset / clamp / hit-test / resize update helper
- 鍦?`_render_resident_filament_panel_simple()` 涓皟鐢?resize 鐘舵€佹洿鏂颁笌杈规鐑尯缁樺埗
- 鍦?`_render_overlays_easymode()` 涓户缁寜缁熶竴鍏ュ彛缁樺埗
- 鍦?`on_easy_mode_switch()` 涓噸缃氦浜掓€侊紝涓嶉噸缃敤鎴峰竷灞€

杩欐槸鏈瀹炵幇涓绘垬鍦恒€?
### 9.3 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

鍘熷垯涓婁笉闇€瑕佹敼鍐呭閫昏緫銆?
鍙€夋敼鍔細

- 濡傛灉鍚庣画瑕佸姞鈥滄爣棰樻爮鍙屽嚮鎭㈠榛樿鈥濇垨鈥滈潰鏉垮ご閮ㄦ樉绀虹缉鏀炬彁绀衡€濓紝鍐嶈€冭檻澧炲姞 callback銆?
鏈绗竴闃舵寤鸿涓嶆敼銆?
### 9.4 `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.cpp`

鍘熷垯涓婁笉闇€瑕佹敼甯冨眬閫昏緫銆?
鍘熷洜锛?
- 褰撳墠鍐呭宸叉寜鐖剁獥鍙ｅ墿浣欑┖闂磋嚜閫傚簲銆?- 鑻ュ涓?rect 鍙彉锛屽唴閮ㄤ細鑷劧闅忎箣缂╂斁銆?
鏈绗竴闃舵寤鸿涓嶆敼銆?
## 10. 鍒嗘鎻愪氦寤鸿

寤鸿鎷嗘垚 4 涓彁浜わ紝闄嶄綆鍥炲綊椋庨櫓銆?
### 鎻愪氦 1锛氬涓诲眰鐘舵€佷笌榛樿鐭╁舰閲嶆瀯

鏀瑰姩鐩爣锛?
- 鍦?`GLCanvas3D.hpp` 鍔犵姸鎬佺粨鏋勫拰鎴愬憳銆?- 鍦?`GLCanvas3DSimple.cpp` 涓媶鍑洪粯璁ょ煩褰㈠嚱鏁般€?- `_compute_resident_filament_rect_simple()` 鏀逛负缁熶竴鍑哄彛锛屼絾鍏堜笉鎺ユ嫋鎷姐€?
棰勬湡缁撴灉锛?
- 鍔熻兘琛屼负涓嶅彉銆?- 浠ｇ爜缁撴瀯宸茬粡涓虹敤鎴疯鐩栫煩褰㈠仛鍑嗗銆?
### 鎻愪氦 2锛歳esize 鐘舵€佹満涓庡洓杈规嫋鎷?
鏀瑰姩鐩爣锛?
- 瀹屾垚 hit-test
- 瀹屾垚 hover cursor
- 瀹屾垚鎸夎竟鎷栨嫿鏇存柊 rect
- 瀹屾垚 clamp

棰勬湡缁撴灉锛?
- 闈㈡澘鍙€氳繃鍥涜竟鎷栧姩鏀瑰彉浣嶇疆鍜屽昂瀵搞€?- 浣嗘鏃跺彲鏆備笉鍋氶厤缃寔涔呭寲銆?
### 鎻愪氦 3锛氶厤缃寔涔呭寲涓庢仮澶嶉粯璁?
鏀瑰姩鐩爣锛?
- 浠?`AppConfig` 璇诲彇 / 鍐欏洖甯冨眬
- easy mode 閲嶈繘鍚庢仮澶嶇敤鎴峰竷灞€
- 澧炲姞 `_reset_resident_filament_layout_simple()`

棰勬湡缁撴灉锛?
- 鐢ㄦ埛鎷栨嫿缁撴灉鍦ㄩ噸鍚悗鍙繚鐣欍€?- 浠ｇ爜鍏峰鎭㈠榛樿鐨勫熀纭€鑳藉姏銆?
### 鎻愪氦 4锛氫綋楠屾墦纾ㄤ笌鍥炲綊淇

鏀瑰姩鐩爣锛?
- 杈圭紭 hover 楂樹寒
- 灏忕獥鍙ｅ満鏅井璋?- 鍦烘櫙鍒囨崲 / 妯″紡鍒囨崲 / DPI 鍙樺寲鍥炲綊
- 蹇呰鏃惰ˉ涓€涓仮澶嶉粯璁ゅ叆鍙?
棰勬湡缁撴灉锛?
- 琛屼负绋冲畾锛屽彲浜や粯娴嬭瘯銆?
## 11. 浼唬鐮佸缓璁?
```cpp
ImVec4 GLCanvas3D::_compute_resident_filament_rect_simple() const
{
    const float scale = get_scale();
    ImVec4 rect = _compute_resident_filament_rect_default_simple();

    if (!m_resident_filament_layout_state.loaded_from_config)
        const_cast<GLCanvas3D*>(this)->_load_resident_filament_layout_simple();

    if (m_resident_filament_layout_state.user_override) {
        rect.x = m_resident_filament_layout_state.x * scale;
        rect.y = m_resident_filament_layout_state.y * scale;
        rect.z = m_resident_filament_layout_state.w * scale;
        rect.w = m_resident_filament_layout_state.h * scale;
    }

    return _clamp_resident_filament_rect_simple(rect, ResidentFilamentResizeEdge::None);
}
```

```cpp
void GLCanvas3D::_render_resident_filament_panel_simple(const ImVec4& rect)
{
    _update_resident_filament_resize_state_simple(rect);

    ImVec4 final_rect = rect;
    if (m_resident_filament_resize_state.dragging)
        final_rect = m_resident_filament_resize_state.live_rect;

    // 缁х画娌跨敤鐜版湁 Begin + Render 娴佺▼
    // render panel with final_rect

    _render_resident_filament_resize_handles_simple(final_rect);
}
```

## 12. 椋庨櫓鐐逛笌瑙勯伩绛栫暐

### 椋庨櫓 1锛氭嫋鎷戒笌 ImGui Window 鏈韩鐨?hover / capture 鍐茬獊

瑙勯伩锛?
- 杈圭紭鐑尯搴旀斁鍦ㄧ獥鍙?Begin 涔嬪悗绔嬪嵆澶勭悊銆?- 浼樺厛浣跨敤 `ImGui::IsMouseClicked()`銆乣ImGui::IsMouseDown()`銆乣ImGui::IsMouseReleased()` 鍜岀粷瀵瑰潗鏍囧垽鏂€?- 鑻ュ懡涓竟缂樹笖杩涘叆鎷栨嫿锛岀洿鎺ヨ缃紶鏍囨牱寮忥紝涓嶄緷璧栧瓙鎺т欢 hover銆?
### 椋庨櫓 2锛氬唴閮ㄥ唴瀹瑰湪鏋佺獎灏哄涓嬮噸鍙?
瑙勯伩锛?
- 閫氳繃 `min_w` 鎻愬墠鎸′綇銆?- 绗竴闃舵涓嶈杩芥眰鏃犻檺缂╁皬銆?
### 椋庨櫓 3锛欴PI 鍙樺寲鍚庡竷灞€寮傚父

瑙勯伩锛?
- 閰嶇疆鎸佷箙鍖栧瓨 logical px锛屼笉瀛樻渶缁堝睆骞曞儚绱犮€?
### 椋庨櫓 4锛氬垏鎹?easy mode 鏃惰娓呯┖鐢ㄦ埛甯冨眬

瑙勯伩锛?
- `on_easy_mode_switch()` 鍙噸缃氦浜掓€侊紝涓嶆竻 `LayoutState`
- 浠呭湪鏄惧紡鈥滄仮澶嶉粯璁も€濇椂娓呴櫎鐢ㄦ埛甯冨眬

## 13. 楠屾敹寤鸿

鑷冲皯楠岃瘉浠ヤ笅鍦烘櫙锛?
1. 榛樿杩涘叆 easy mode锛岄潰鏉夸綅缃笌褰撳墠鐗堟湰涓€鑷淬€?2. 鍙宠竟鎷栨嫿鍙樺锛屽唴閮ㄥ唴瀹规甯告墿灞曘€?3. 宸﹁竟鎷栨嫿鏃讹紝鍙宠竟閿氱偣淇濇寔涓嶅姩銆?4. 涓婅竟鎷栨嫿鏃讹紝搴曡竟淇濇寔涓嶅姩锛岄《閮ㄤ笉瓒婅繃涓诲伐鍏锋爮銆?5. 涓嬭竟鎷栨嫿鏃讹紝棰勮鍖洪殢楂樺害澧炲姞鑷劧鎵╁睍銆?6. 缂╁埌鏈€灏忓昂瀵告椂锛屼笉瓒婄晫銆佷笉闂儊銆?7. 鏀惧埌杈冨ぇ灏哄鏃讹紝涓嶈秴鍑虹敾甯冭寖鍥淬€?8. 閲嶅惎杞欢鍚庢仮澶嶄笂娆″竷灞€銆?9. 鍦烘櫙 reload銆佸垏鎹㈢洏銆佸垏鎹㈣澶囧悗甯冨眬涓嶄涪銆?10. 閫€鍑?easy mode 鍐嶅洖鏉ワ紝甯冨眬浠嶅瓨鍦ㄣ€?
## 14. 鎺ㄨ崘瀹炴柦椤哄簭

鎺ㄨ崘鐩存帴鎸変互涓嬮『搴忓姩鎵嬶細

1. 鍏堥噸鏋勯粯璁?rect 璁＄畻锛屽缓绔?`LayoutState`
2. 鍐嶅疄鐜板洓杈?hit-test 鍜屾嫋鎷?3. 鍐嶈ˉ clamp
4. 鏈€鍚庢帴 `AppConfig` 鎸佷箙鍖?
杩欐牱鍋氱殑濂藉鏄細

- 姣忎竴姝ラ兘鍙崟鐙繍琛岄獙璇?- 鍑洪棶棰樻椂瀹规槗鍥為€€
- 涓嶄細鎶婁氦浜掋€佸竷灞€銆侀厤缃笁浠朵簨鑰﹀湪涓€鍧楄皟璇?
---

杩欎唤鏂囨。瀵瑰簲鐨勬槸绗竴闃舵鈥滃妗嗗洓杈圭缉鏀锯€濊璁°€? 
濡傛灉鍚庣画纭杩橀渶瑕佲€滈潰鏉垮唴閮ㄥ垪琛ㄥ尯 / 棰勮鍖烘瘮渚嬫嫋鎷解€濓紝寤鸿鍗曠嫭璧风浜屼唤璁捐锛屼笉瑕佸拰鏈瀹夸富灞?resize 娣峰湪涓€璧枫€?
