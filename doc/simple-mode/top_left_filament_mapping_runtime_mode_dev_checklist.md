# 宸︿笂瑙掕€楁潗鏄犲皠杩愯鏃舵ā寮忓紑鍙戞竻鍗?

## 1. 鏂囨。瀹氫綅

鏈枃妗ｇ敤浜庢壙鎺モ€滃乏涓婅鍦烘櫙鑰楁潗涓庢槧灏勬ā鍧椻€濈殑杩愯鏃舵ā寮忓疄鐜拌璁°€?

褰撳墠闃舵鐩爣鏄細

1. **鍏堜笉淇敼** `ImGuiFilamentPanel` 鐜版湁瀹炵幇
2. 灏嗘湭鏉ュ疄鐜伴渶瑕佹柊澧炵殑妯″紡鏋氫妇銆佺姸鎬佸瓧娈点€佸垽鏂嚱鏁般€佹覆鏌撳垎鍙戠粨鏋勶紝鍏堟敹鏁涙垚涓€浠界嫭绔嬪紑鍙戞竻鍗?
3. 鍚庣画濡傛灉纭鏂瑰悜绋冲畾锛屽啀鎶婃湰鏂囦欢涓殑鍐呭閫愭鍚堝苟鍒扮湡瀹炰唬鐮佷腑

褰撳墠涓嶆秹鍙婏細

1. 鐩存帴鏀瑰姩 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
2. 鐩存帴鏀瑰姩 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
3. 鐩存帴鏀瑰姩鐜版湁 popup 閫昏緫

## 2. 鐩爣闂

宸︿笂瑙掓ā鍧楃殑灞曠ず鏂瑰紡锛屼笉鍙彇鍐充簬鍦烘櫙棰滆壊鏁伴噺锛岃繕鍙栧喅浜庤澶囪兘鍔涗笌鍦ㄧ嚎鐘舵€併€?

鍥犳瀹炵幇灞傞潰闇€瑕佸厛瑙ｅ喅涓€涓粺涓€闂锛?

**杩愯鏃跺浣曞厛鍒ゅ畾褰撳墠搴旇钀藉埌鍝竴绉?UI 妯″紡锛屽啀杩涘叆瀵瑰簲鐨勬覆鏌撹矾寰勩€?*

褰撳墠鏀舵暃鐨?3 绉嶄富妯″紡锛?

1. `MultiColorOnline`
2. `SingleColorDevice`
3. `MultiColorOffline`

## 3. 寤鸿鏂板鐨勮繍琛屾椂妯″紡鏋氫妇

寤鸿鍏堝湪鏈潵瀹炵幇涓紩鍏ヤ竴涓嫭绔嬫灇涓撅紝渚嬪锛?

```cpp
enum class ResidentFilamentUiMode
{
    MultiColorOnline = 0,
    SingleColorDevice,
    MultiColorOffline
};
```

绾︽潫锛?

1. 妯″紡鏋氫妇鍙〃杈惧乏涓婅妯″潡鐨勪富 UI 褰㈡€?
2. 涓嶅湪杩欎釜鏋氫妇閲屾贩鍏?popup 鏄惁鎵撳紑銆佹槸鍚︽湁鎺ㄨ崘銆佹槸鍚︽湁缂撳瓨缁撴灉绛夋绾х姸鎬?
3. 褰撳墠闃舵涓嶅缓璁户缁墿灞曟洿澶氬苟鍒楁ā寮?

## 4. 寤鸿鏂板鐨勮緭鍏ヤ俊鍙风粨鏋?

寤鸿鍚庣画瀹炵幇涓紝鎶婅繍琛屾椂鍒ゆ柇杈撳叆缁熶竴鏀舵暃鎴愪竴涓粨鏋勶紝渚嬪锛?

```cpp
struct ResidentFilamentRuntimeSignals
{
    bool device_is_online = false;
    bool device_supports_multi_color = false;
    bool device_materials_available = false;
    int  scene_color_count = 0;
    bool has_cached_mapping_result = false;
};
```

### 4.1 瀛楁璇存槑

`device_is_online`

1. 褰撳墠璁惧鏄惁鍦ㄧ嚎
2. 褰撳墠鏄惁鑳界户缁繘琛岃澶囦晶鑰楁潗璇诲彇

`device_supports_multi_color`

1. 褰撳墠璁惧鏄惁鍏峰澶氳壊 / 澶氳€楁潗鏄犲皠鑳藉姏
2. 杩欐槸璁惧鑳藉姏锛屼笉鏄綋鍓嶆槸鍚﹁浜嗗涓€楁潗

`device_materials_available`

1. 褰撳墠鏄惁鎷垮埌浜嗗彲淇＄殑璁惧鑰楁潗鍒楄〃
2. 涓昏鐢ㄤ簬鍦ㄧ嚎澶氳壊妯″紡鍐呴儴鍒ゆ柇 popup 鍜屾帹鑽愰€昏緫鏄惁鍙敤

`scene_color_count`

1. 褰撳墠鍦烘櫙棰滆壊鏁伴噺
2. 涓嶇洿鎺ュ喅瀹氫富妯″紡锛屼絾褰卞搷鍗曡壊妯″紡涓嬬殑鎽樿鍜岄瑙堣涔?

`has_cached_mapping_result`

1. 绂荤嚎鏃舵槸鍚﹀瓨鍦ㄥ彲浠ュ弬鑰冩樉绀虹殑鍘嗗彶鏄犲皠缁撴灉
2. 鍙奖鍝嶇绾挎ā寮忓唴閮ㄥ睍绀猴紝涓嶆敼鍙樹富妯″紡

## 5. 寤鸿鏂板鐨勪富鍒ゆ柇鍑芥暟

寤鸿鏈潵瀹炵幇涓彁渚涗竴涓彧璐熻矗鈥滀富妯″紡鍒ゆ柇鈥濈殑绾嚱鏁帮紝渚嬪锛?

```cpp
ResidentFilamentUiMode resolve_resident_filament_ui_mode(
    const ResidentFilamentRuntimeSignals& signals);
```

寤鸿閫昏緫锛?

```cpp
ResidentFilamentUiMode resolve_resident_filament_ui_mode(
    const ResidentFilamentRuntimeSignals& signals)
{
    if (!signals.device_is_online)
        return ResidentFilamentUiMode::MultiColorOffline;

    if (!signals.device_supports_multi_color)
        return ResidentFilamentUiMode::SingleColorDevice;

    return ResidentFilamentUiMode::MultiColorOnline;
}
```

绾︽潫锛?

1. 杩欎釜鍑芥暟鍙仛妯″紡鍒ゆ柇
2. 涓嶅湪杩欓噷鍐冲畾 banner 鏂囨
3. 涓嶅湪杩欓噷鍐冲畾 popup 榛樿绛涢€?
4. 涓嶅湪杩欓噷鍐冲畾琛岀姸鎬佹枃妗?

## 6. 寤鸿鏂板鐨勬ā寮忓唴灞曠ず鐘舵€佺粨鏋?

涓绘ā寮?resolve 涔嬪悗锛岃繕闇€瑕佹ā寮忓唴鐨勫睍绀鸿緭鍏ャ€?

寤鸿鍚庣画瀹炵幇涓啀寮曞叆涓€涓洿鍋忔覆鏌撴€佺殑缁撴瀯锛屼緥濡傦細

```cpp
struct ResidentFilamentUiState
{
    ResidentFilamentUiMode mode = ResidentFilamentUiMode::MultiColorOnline;
    bool show_selector_dropdown = false;
    bool selector_interactable = false;
    bool show_unified_output_card = false;
    bool show_cached_mapping_hint = false;
    bool show_offline_banner = false;
};
```

### 6.1 璁捐鎰忓浘

杩欐牱鍋氱殑鐩殑锛屾槸鎶婏細

1. 鈥滃綋鍓嶆槸浠€涔堟ā寮忊€?
2. 鈥滃綋鍓嶈繖涓ā寮忛噷锛屽摢浜涘眬閮ㄥ厓绱犺鏄剧ず鈥?

鎷嗘垚涓ゅ眰銆?

閬垮厤鍚庣画娓叉煋浠ｇ爜閲岀洿鎺ュ嚭鐜板ぇ閲忥細

```cpp
if (!device_is_online && has_cached_mapping_result && ...)
```

杩欐牱鐨勬贩鍚堝垽鏂€?

## 7. 寤鸿鏂板鐨勮绾ф樉绀哄瓧娈?

宸︿笂瑙掗€愯娓叉煋鏃讹紝寤鸿鍚庣画琛ラ綈涓€缁勮绾?view model 瀛楁锛岃€屼笉鏄湪 `render_item()` 鍐呴浂鏁ｆ嫾鎺ャ€?

渚嬪锛?

```cpp
struct ResidentFilamentRowViewModel
{
    int         index = 0;
    ImVec4      scene_color = ImVec4(1, 1, 1, 1);
    std::string scene_label;
    std::string row_status_text;

    bool        show_selector = false;
    bool        selector_enabled = false;
    bool        selector_show_chevron = false;

    std::string target_slot_label;
    std::string target_material_type;
    ImVec4      target_material_color = ImVec4(1, 1, 1, 1);

    bool        is_using_cached_target = false;
    bool        is_unified_output = false;
};
```

### 7.1 涓轰粈涔堝缓璁崟鐙缓琛岀骇 view model

鍘熷洜锛?

1. `MultiColorOnline`銆乣SingleColorDevice`銆乣MultiColorOffline` 涓夌妯″紡涓嬶紝鍚屼竴琛屽彸渚х殑缁撴瀯宸紓寰堝ぇ
2. 濡傛灉娌℃湁琛岀骇 view model锛屾覆鏌撳嚱鏁伴噷浼氬婊℃潯浠跺垎鏀?
3. 鍏堟妸鈥滆琛岀幇鍦ㄥ簲璇ラ暱浠€涔堟牱鈥濇暣鐞嗗嚭鏉ワ紝娓叉煋浼氭洿绋?

## 8. 寤鸿鏂板鐨勬覆鏌撳垎鍙戝嚱鏁?

寤鸿鏈潵涓嶈鍙繚鐣欎竴涓ぇ鑰屽叏鐨?`render_item()`锛岃€屾槸鍏堝仛妯″紡鍒嗗彂锛屽啀杩涘叆瀵瑰簲娓叉煋璺緞銆?

渚嬪锛?

```cpp
void render_resident_filament_mapping(const ResidentFilamentUiState& ui_state);
```

鍐呴儴寤鸿鍒嗗彂涓猴細

```cpp
switch (ui_state.mode) {
case ResidentFilamentUiMode::MultiColorOnline:
    render_multicolor_online_mode();
    break;
case ResidentFilamentUiMode::SingleColorDevice:
    render_single_color_device_mode();
    break;
case ResidentFilamentUiMode::MultiColorOffline:
    render_multicolor_offline_mode();
    break;
}
```

## 9. 鍚勬ā寮忓缓璁媶鍑虹殑娓叉煋鍑芥暟

### 9.1 澶氳壊璁惧鍦ㄧ嚎

寤鸿鍑芥暟锛?

```cpp
void render_multicolor_online_mode();
void render_multicolor_online_row(const ResidentFilamentRowViewModel& row);
```

鑱岃矗锛?

1. 娓叉煋閫愯鏄犲皠 selector
2. 鍏佽鎵撳紑 popup
3. 鍏佽瀹炴椂鍒锋柊棰勮

### 9.2 鍗曡壊璁惧

寤鸿鍑芥暟锛?

```cpp
void render_single_color_device_mode();
void render_single_color_output_card();
void render_single_color_scene_row(const ResidentFilamentRowViewModel& row);
```

鑱岃矗锛?

1. 娓叉煋缁熶竴杈撳嚭鑰楁潗鎽樿鍗?
2. 鍦烘櫙棰滆壊琛屽彧鍋氳鏄庡睍绀?
3. 涓嶅嚭鐜板€欓€?popup 鍏ュ彛

### 9.3 澶氳壊璁惧绂荤嚎

寤鸿鍑芥暟锛?

```cpp
void render_multicolor_offline_mode();
void render_multicolor_offline_row(const ResidentFilamentRowViewModel& row);
```

鑱岃矗锛?

1. 淇濈暀閫愯楠ㄦ灦
2. selector 璧扮鐢ㄦ€?
3. 鍙睍绀轰笂娆℃槧灏勭粨鏋滄垨绌哄崰浣?
4. 涓嶅厑璁告墦寮€ popup

## 10. 寤鸿鏂板鐨勮緟鍔╁垽鏂嚱鏁?

寤鸿鍚庣画鏂板涓€缁勫皬鍑芥暟锛岄伩鍏嶆妸鍒ゆ柇閫昏緫鏁ｈ惤鍦ㄦ覆鏌撲唬鐮侀噷銆?

### 10.1 淇″彿鏀堕泦

```cpp
ResidentFilamentRuntimeSignals collect_resident_filament_runtime_signals();
```

鑱岃矗锛?

1. 浠庡綋鍓嶈澶囥€佸満鏅拰缂撳瓨鏄犲皠缁撴灉涓敹闆嗗垽鏂緭鍏?
2. 涓嶅仛娓叉煋

### 10.2 妯″紡鍐呯姸鎬佺敓鎴?

```cpp
ResidentFilamentUiState build_resident_filament_ui_state(
    const ResidentFilamentRuntimeSignals& signals);
```

鑱岃矗锛?

1. 灏嗕富妯″紡涓庡眬閮ㄦ樉绀哄紑鍏虫暣鍚堟垚娓叉煋鎬?
2. 涓嶅仛鍏蜂綋缁樺埗

### 10.3 琛岀骇鏁版嵁鏋勫缓

```cpp
std::vector<ResidentFilamentRowViewModel> build_resident_filament_rows(
    const ResidentFilamentUiState& ui_state);
```

鑱岃矗锛?

1. 涓哄綋鍓嶆ā寮忕敓鎴愭瘡涓€琛岃鎬庝箞鏄剧ず鐨勬暟鎹?
2. 灏嗏€滄槸鍚︽樉绀?selector鈥濃€滄槸鍚︽樉绀虹粺涓€杈撳嚭鈥濃€滄槸鍚﹀睍绀虹紦瀛樼洰鏍団€濇彁鍓嶆暣鐞嗗ソ

## 11. 鎺ㄨ崘鐨勮皟鐢ㄩ『搴?

寤鸿鍚庣画鐪熷疄瀹炵幇鏃舵寜浠ヤ笅椤哄簭缁勭粐锛?

1. 鏀堕泦杩愯鏃惰緭鍏ヤ俊鍙?
2. resolve 涓?UI 妯″紡
3. 鐢熸垚褰撳墠妯″潡 UI state
4. 鐢熸垚琛岀骇 view model
5. 鎸夋ā寮忓垎鍙戞覆鏌?
6. 妯″紡鍐呴儴鍐嶅鐞?popup銆侀瑙堝拰灞€閮ㄥ姩浣?

瀵瑰簲浼唬鐮侊細

```cpp
auto signals = collect_resident_filament_runtime_signals();
auto mode = resolve_resident_filament_ui_mode(signals);
auto ui_state = build_resident_filament_ui_state(signals);
ui_state.mode = mode;
auto rows = build_resident_filament_rows(ui_state);
render_resident_filament_mapping(ui_state, rows);
```

## 12. 鍝簺浜嬩欢闇€瑕侀噸鏂?resolve 妯″紡

浠ヤ笅浜嬩欢寤鸿閲嶆柊鎵ц涓€娆?`resolve_resident_filament_ui_mode()`锛?

1. 褰撳墠璁惧鍒囨崲
2. 璁惧鍦ㄧ嚎鐘舵€佸彉鍖?
3. 璁惧鑳藉姏淇℃伅鍙樺寲
4. 璁惧鏂欐灦鏁版嵁浠庝笉鍙敤鍙樹负鍙敤锛屾垨浠庡彲鐢ㄥ彉涓哄け鏁?

浠ヤ笅浜嬩欢閫氬父涓嶉渶瑕佸垏鎹富妯″紡锛屼絾闇€瑕侀噸寤哄綋鍓嶆ā寮忓唴瀹癸細

1. 鍦烘櫙棰滆壊鏂板
2. 鍦烘櫙棰滆壊鍒犻櫎
3. 鍦烘櫙棰滆壊淇敼
4. 鏄犲皠鐩爣鍙樺寲
5. 鎺ㄨ崘鐩爣鍙樺寲

## 13. 褰撳墠闃舵寤鸿

褰撳墠闃舵鏈€鎺ㄨ崘鐨勬帹杩涙柟寮忥細

1. 鍏堟妸鏈枃浠朵腑鐨勬灇涓俱€乻ignals銆乽i state銆乺ow view model 鍛藉悕瀹氫笅鏉?
2. 鏆傛椂涓嶈纰?`ImGuiFilamentPanel` 閲岀殑鐪熷疄娓叉煋
3. 绛変骇鍝佸眰鍒ゆ柇閫昏緫瀹屽叏纭鍚庯紝鍐嶅喅瀹氭槸锛?
   - 鐩存帴骞跺叆 `ImGuiFilamentPanel`
   - 杩樻槸鍏堟娊涓€涓柊鐨?resident mapping helper / presenter

## 14. 鐩稿叧鏂囨。

褰撳墠杩欎唤寮€鍙戞竻鍗曪紝鍩轰簬浠ヤ笅鏂囨。缁х画灞曞紑锛?

1. [top_left_filament_mapping_resident_prd.md](C:/WORK/C3DSlicer/doc/simple-mode/top_left_filament_mapping_resident_prd.md)
2. [top_left_filament_mapping_device_states_temp.html](C:/WORK/C3DSlicer/doc/simple-mode/top_left_filament_mapping_device_states_temp.html)

## 15. 褰撳墠浠ｇ爜钀藉湴鏇存柊

鎴嚦褰撳墠锛屽墠鏂囦腑鐨勨€滆繍琛屾椂妯″紡鍒ゆ柇 + 琛岀骇 view model + 娓叉煋鍒嗗眰鈥濆凡缁忎笉鍐嶅彧鏄鐮旀蹇碉紝simple mode 浠ｇ爜閲屽凡缁忔湁涓€鐗堝彲杩愯鐨勬媶鍒嗗疄鐜般€?

### 15.1 宸叉柊澧炵殑浠ｇ爜鍒嗗眰

褰撳墠 resident mapping 鐩稿叧浠ｇ爜宸叉媶涓轰互涓嬪嚑灞傦細

1. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp`
2. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.cpp`
3. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp`
4. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp`
5. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.hpp`
6. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.cpp`
7. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp`
8. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp`
9. `src/slic3r/GUI/simple/filamentMapping/ImGuiSpoolWidget.hpp`
10. `src/slic3r/GUI/simple/filamentMapping/ImGuiSpoolWidget.cpp`

鑱岃矗绾﹀畾濡備笅锛?

1. `Logic`
   - 璐熻矗 `RuntimeSignals -> UiMode -> UiModel`
   - 涓嶆劅鐭?ImGui 鍏蜂綋缁樺埗缁嗚妭
2. `Adapter`
   - 璐熻矗鎶?`device + item states` 杞垚琛岃緭鍏ャ€乸opup catalog銆乸anel view data
   - 璐熻矗 popup 閫夋嫨缁撴灉鍐欏洖 `ImGuiFilamentItemState`
3. `View`
   - 璐熻矗绾?ImGui 缁樺埗
   - 涓嶇洿鎺ヨ鍙栬澶囨暟鎹紝涓嶇洿鎺ュ喅瀹氫笟鍔¤鍒?
4. `PopupPolicy`
   - 璐熻矗 popup 鍊欓€夌殑绛涢€夈€佸垎缁勩€佺鐢ㄥ拰鍖归厤绛栫暐
   - 璐熻矗杩愯鏃?popup 鍖归厤绛栫暐閫夋嫨
5. `ImGuiSpoolWidget`
   - 璐熻矗鍗风洏鎺т欢鐨勫彲澶嶇敤缁樺埗

### 15.2 杩愯鏃朵富妯″紡宸插搴斿埌鐪熷疄浠ｇ爜

褰撳墠杩愯鏃朵富妯″紡宸茶惤鍒帮細

1. `ResidentFilamentMapping::RuntimeSignals`
2. `ResidentFilamentMapping::UiMode`
3. `ResidentFilamentMapping::build_ui_model(...)`

瀵瑰簲鍏崇郴涓庡墠鏂囦繚鎸佷竴鑷达細

1. `MultiColorOnline`
2. `SingleColorDevice`
3. `MultiColorOffline`

`ImGuiFilamentPanel::Render()` 褰撳墠宸茬粡鍏堟敹闆嗭細

1. `device_is_online`
2. `device_supports_multi_color`
3. `device_materials_available`
4. `scene_color_count`
5. `has_cached_mapping_result`

鍐嶇粺涓€浜ょ粰 `Logic` 灞傜敓鎴愬綋鍓?UI 妯″瀷銆?

### 15.3 popup 鍖归厤瑙勫垯宸叉敹鎴?policy config

褰撳墠 popup 鐨勫尮閰嶈鍒欎笉鍐嶆暎钀藉湪 adapter 鎴?view 鍐呴儴锛岃€屾槸鏀舵暃涓烘樉寮忛厤缃細

```cpp
enum class PopupMatchMode
{
    StrictType = 0,
    LooseType,
    AvailabilityOnly
};
```

瀵瑰簲鏂囦欢锛?

1. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp`
2. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp`

璇箟濡備笅锛?

1. `StrictType`
   - 鎸夎€楁潗绫诲瀷寮哄尮閰?
   - 褰撳墠浣滀负榛樿琛屼负
2. `LooseType`
   - 鍏佽鏇村鏉剧殑绫诲瀷鍏煎
   - 褰撳墠瀹炵幇鍖呭惈澶у皬鍐欏綊涓€銆佸寘鍚叧绯汇€佸熀纭€鏉愭枡瀹舵棌褰掑苟
3. `AvailabilityOnly`
   - 鍙湅鍊欓€夋槸鍚﹀彲鐢?
   - 涓嶅啀鎸夌被鍨嬬鐢?

### 15.4 杩愯鏃?popup 绛栫暐宸茶嚜鍔ㄩ€夋嫨

褰撳墠宸叉柊澧烇細

```cpp
PopupMatchPolicyConfig popup_policy_config_for_runtime(
    const DM::Device& device,
    ImGuiFilamentPanel::Mode mode,
    const ResidentFilamentRuntimeSignals& signals);
```

褰撳墠鑷姩瑙勫垯涓猴細

1. 璁惧绂荤嚎锛屾垨褰撳墠鏈鍙栧埌鍙敤璁惧鑰楁潗
   - 浣跨敤 `AvailabilityOnly`
2. 澶栨寕鏂欐灦妯″紡锛屾垨褰撳墠璁惧涓嶆槸澶氳壊璁惧
   - 浣跨敤 `LooseType`
3. 澶氳壊鍦ㄧ嚎妯″紡涓嬶細
   - 鑻ュ瓨鍦ㄥ閮ㄦ潵婧愭枡鐩掞紙`box_type == 1 / 2`锛夛紝浣跨敤 `LooseType`
   - 鍚﹀垯浣跨敤 `StrictType`

璇存槑锛?

1. 杩欏眰瑙勫垯褰撳墠鐢?controller 璋冪敤锛屼絾瑙勫垯鏈韩浠嶆斁鍦?`PopupPolicy` 涓?
2. 鍚庣画濡傛灉浜у搧甯屾湜璋冩暣鈥滄洿涓ユ牸鈥濇垨鈥滄洿瀹芥澗鈥濈殑鍖归厤锛屽彧闇€瑕佹敼 policy锛屼笉闇€瑕佸啀鏀?view

### 15.5 涓庢湰鏂囨棭鏈熺害鏉熺殑鍏崇郴

鏈枃鏈€寮€濮嬪啓鐨勬槸鈥滃厛涓嶄慨鏀?`ImGuiFilamentPanel` 鐪熷疄瀹炵幇鈥濄€傝繖涓€鏉″湪棰勭爺闃舵鎴愮珛锛屼絾褰撳墠宸茶繘鍏ョ湡瀹炰唬鐮佽惤鍦伴樁娈碉紝鍥犳杩欓噷琛ュ厖鏂扮殑缁撹锛?

1. `ImGuiFilamentPanel` 宸茬粡寮€濮嬫帴鍏ヤ笂杩板垎灞?
2. 褰撳墠鏂瑰悜涓嶆槸鈥滅户缁叏閮ㄥ爢鍥?`ImGuiFilamentPanel`鈥?
3. 褰撳墠鏂瑰悜鏄 `ImGuiFilamentPanel` 閫愭閫€鍖栦负 controller / orchestrator
4. 鍚庣画鏂板瑙勫垯锛屼紭鍏堢户缁惤鍒?`Logic / Adapter / PopupPolicy / View`锛岃€屼笉鏄洖鍒板ぇ鍑芥暟鍐呬复鏃舵嫾鎺?
## 16. 褰撳墠 popup 鐪熷疄瀹炵幇琛ュ厖

鏈珷鐢ㄤ簬鎶婂綋鍓嶅凡钀藉湴鐨?popup 缁撴瀯鍙嶅啓涓哄紑鍙戠害鏉燂紝鏂逛究鍚庣画缁х画娌垮悓涓€鏂瑰悜杩唬銆?

### 16.1 褰撳墠 popup 淇℃伅缁撴瀯

褰撳墠浠ｇ爜涓殑 popup 宸叉寜浠ヤ笅缁撴瀯瀹炵幇锛?

1. 澶撮儴鎽樿鍖?
   - 鏍囬
   - 褰撳墠鍦烘櫙棰滆壊鍚嶇О
   - 鍙充晶鍦烘櫙棰滆壊鍧?
2. 鏉ユ簮绛涢€?chip 琛?
   - 鏉ヨ嚜 popup group labels
   - 褰撳墠鍙睍绀哄疄闄呮潵婧愮粍锛屼笉棰濆鍚堟垚 `鎺ㄨ崘` / `鍏ㄩ儴`
3. 鍗曟潵婧愬€欓€夌煩闃靛尯
   - 鍥哄畾楂樺害
   - 鍐呴儴婊氬姩
   - 涓ゅ垪鍗风洏鍗?

寮€鍙戠害鏉燂細

1. 涓嶅啀鍥為€€涓衡€滄墍鏈夋潵婧愮粍鍚屾椂灞曞紑鈥濈殑绾靛悜闀块潰鏉?
2. 澶氭潵婧愬満鏅紭鍏堥€氳繃 chip 鍒囨崲鏀剁獎
3. 澶氬€欓€夊満鏅紭鍏堥€氳繃鍐呭鍖烘粴鍔ㄦ帶楂?

### 16.2 褰撳墠 popup 榛樿钀界偣瑙勫垯

褰撳墠瀹炵幇閲囩敤浠ヤ笅榛樿瑙勫垯锛?

1. 鎵撳紑鏌愰鑹查」 popup 鏃讹紝浼樺厛瀹氫綅鍒板綋鍓嶅凡閫夌洰鏍囨墍鍦ㄦ潵婧愮粍
2. 鑻ュ綋鍓嶈娌℃湁宸查€夌洰鏍囷紝鍒欓粯璁ら€変腑绗竴涓潵婧愮粍
3. 鐢ㄦ埛鍒囨崲鏉ユ簮 chip 鍚庯紝浠呭埛鏂板綋鍓嶅€欓€夌煩闃?

寮€鍙戠害鏉燂細

1. 杩欎釜榛樿瑙勫垯搴旇涓哄綋鍓嶅凡钀藉湴鍩虹嚎
2. 鑻ュ悗缁琛モ€滆蹇嗕笂娆＄瓫閫夆€濓紝涔熶笉搴旇鐩栤€滃綋鍓嶅凡閫夋潵婧愪紭鍏堚€濊繖鏉′富瑙勫垯

### 16.3 褰撳墠鍏变韩鍗风洏缁勪欢绾︽潫

褰撳墠鍏变韩鍗风洏缁樺埗宸叉敹鏁涗负锛?

1. `ImGuiSpoolWidget` 璐熻矗缁熶竴缁樺埗
2. 閫氳繃鍚庤疆 / 鍗蜂笣 / 鍓嶈疆涓夊眰 SVG 璧勬簮缁勫悎娓叉煋
3. selector 涓?popup 浣跨敤鍚屼竴濂楀嵎鐩樿瑙夎祫婧?

寮€鍙戠害鏉燂細

1. 鍗风洏瑙傛劅璋冩暣浼樺厛杩涘叆 `ImGuiSpoolWidget` 鎴栬祫婧愭枃浠?
2. 涓嶅湪 `ResidentFilamentMappingView` 涓负 popup 鍗曠嫭鍐嶉€犱竴濂楀嵎鐩樻牱寮?

### 16.4 褰撳墠 selector 甯冨眬绾︽潫

褰撳墠 selector 鍙充晶淇℃伅鍖哄凡鏀舵暃涓猴細

1. 妲戒綅鏍囩涓庢潗鏂欑被鍨嬬珫鐩存帓甯?
2. 鏂囨湰鍖哄乏瀵归綈
3. 涓嬫媺绠ご鐙珛鏀惧湪鏈€鍙充晶

寮€鍙戠害鏉燂細

1. 鍚庣画鑻ョ户缁敹瑙嗚锛屽簲淇濇寔鏂囧瓧鍖轰笌绠ご鍖哄垎绂?
2. 涓嶅缓璁啀鎶婄澶村祵鍥炴枃瀛楀尯鍐呴儴

### 16.5 褰撳墠涓诲叆鍙ｄ笌椤堕儴宸ュ叿鏍忓叧绯?

褰撳墠瀹炵幇琛ュ厖濡備笅锛?

1. 宸︿笂 resident mapping panel 宸蹭綔涓?simple mode 涓嬬殑涓昏鑰楁潗灞曠ず涓庢槧灏勫叆鍙?
2. 椤堕儴涓诲伐鍏锋爮涓殑 `filament / 鑰楁潗` 鍙鎸夐挳宸茬Щ闄?
3. 琛屽唴 selector popup 鎴愪负褰撳墠鍞竴瀵瑰鏆撮湶鐨勬槧灏勮皟鏁村叆鍙?

寮€鍙戠害鏉燂細

1. 鍚庣画涓嶈鍐嶉粯璁ゆ仮澶嶁€滈《閮ㄦ寜閽?+ 甯搁┗闈㈡澘鈥濆弻鍏ュ彛
2. 鑻ユ棫閫昏緫浠嶄繚鐣?`open_filament_toolbar_popup()` 涔嬬被鍏煎璋冪敤锛屽簲瑙嗕负鍘嗗彶鍏煎鑳藉姏锛屼笉浠ｈ〃褰撳墠涓讳氦浜掑叆鍙?

### 16.6 褰撳墠 popup 鎮诞棰勮瀹炵幇绾︽潫

褰撳墠 popup 宸叉敮鎸佸€欓€夐」 hover preview锛岃涓哄涓嬶細

1. 鎮诞鍒版煇涓?enabled 鍊欓€夊嵎鐩樺崱鏃?
   - 宸︿晶棰勮鍥句复鏃惰鐩栦负璇ュ€欓€夋槧灏勮壊
2. hover 绂诲紑銆乸opup 鍏抽棴銆佹垨褰撳墠甯ф湭鍛戒腑浠讳綍鍊欓€夋椂
   - 涓存椂瑕嗙洊鑷姩娓呴櫎
3. 鐐瑰嚮鍊欓€夐」鏃?
   - 鎵嶆墽琛岀湡瀹炴槧灏勬彁浜?

褰撳墠瀹炵幇鍒嗗眰锛?

1. `ResidentFilamentMappingView`
   - 鍙戝嚭 `on_hover_option(item_index, selection_token)` 鍥炶皟
2. `ResidentFilamentMappingAdapter`
   - 閫氳繃 `find_popup_selection_payload(...)` 瑙ｆ瀽 token 瀵瑰簲鍊欓€夋暟鎹?
3. `ImGuiFilamentPanel`
   - 缁存姢甯х骇 `hover preview override`
   - 鍦ㄧ缉鐣ュ浘缁樺埗鏃朵紭鍏堣鍙栬涓存椂瑕嗙洊鑹?

棰勮鏁版嵁浼樺厛绾э細

1. 鑻ュ瓨鍦?active hover override
   - 浣跨敤 hover 瀵瑰簲棰滆壊
2. 鍚﹀垯
   - 浣跨敤褰撳墠瀹為檯宸叉槧灏勯鑹?
3. 鑻ュ疄闄呮槧灏勭己澶?
   - 鍥為€€鍒板満鏅鑹茶嚜韬?

寮€鍙戠害鏉燂細

1. hover preview 蹇呴』淇濇寔闈炴彁浜ゃ€侀潪鍐欏洖
2. 涓嶄负 hover preview 棰濆鎸佷箙鍖栫姸鎬?
3. 浼樺厛淇濇寔褰撳墠鈥滄寜甯ч噸寤恒€佹湭鍛戒腑鍗冲洖閫€鈥濈殑杞婚噺瀹炵幇

### 16.7 褰撳墠澧炲垹棰滆壊鑳藉姏杈圭晫

闈㈠悜鏂版墜鐨?simple mode锛屽綋鍓嶈兘鍔涜竟鐣屾敹鏁涗负锛?

1. 鏀寔 `娣诲姞棰滆壊`
2. 鏀寔 `鍒犻櫎璇ラ鑹瞏
3. 涓嶆敮鎸佸湪 resident mapping 涓毚闇?`鍚堝苟鍒?..`

寮€鍙戠害鏉燂細

1. selector popup 鍙礋璐ｆ槧灏勫€欓€夐€夋嫨
2. 涓嶅湪 selector popup 鍐呯户缁爢鍙?`鍚堝苟` 绛夊鏉傞鑹茬粨鏋勬搷浣?
3. 鑻ユ枃妗ｅ墠鏂囦粛鏈夆€滅畝鏄撴ā寮?popup 鍐呮敮鎸佸悎骞垛€濈殑鏃ц鎯筹紝浠ユ湰鑺備负鍑?

### 16.8 褰撳墠澧炲垹鍏ュ彛寤鸿

褰撳墠寤鸿鍏ュ彛濡備笅锛?

1. 鏍囬琛屽彸渚ф斁缃?`+ 娣诲姞棰滆壊`
2. 琛岀骇鎻愪緵 `鍒犻櫎璇ラ鑹瞏
3. 涓嶅啀鎻愪緵鍏ㄥ眬 `- 鍒犻櫎鏈€鍚庝竴涓鑹瞏 浣滀负 resident mapping 涓诲叆鍙?

寮€鍙戠害鏉燂細

1. `娣诲姞棰滆壊` 灞炰簬椤甸潰绾у姩浣滐紝鍥犳鏀惧湪鏍囬琛?
2. `鍒犻櫎璇ラ鑹瞏 灞炰簬琛岀骇鍔ㄤ綔锛屽洜姝ゅ繀椤荤粦瀹氬埌鍏蜂綋棰滆壊椤?
3. 涓嶈璁╃敤鎴疯瑙ｂ€滃垹闄ゆ寜閽€濅細鍒犻櫎鍒殑琛屾垨鏈€鍚庝竴琛?

### 16.9 褰撳墠鍒犻櫎瀹炵幇寤鸿

褰撳墠 resident mapping 鑻ヨ琛ラ綈琛岀骇鍒犻櫎锛屽缓璁寜浠ヤ笅鏂瑰悜瀹炵幇锛?

1. 鍒犻櫎鍔ㄤ綔鎸夊綋鍓嶈绱㈠紩鎵ц
2. 鍒犻櫎瀹屾垚鍚庨噸寤?resident rows銆乻ummary銆乸review
3. 鑻ュ垹闄ら」 popup 姝ｅ湪鎵撳紑锛屽垯绔嬪嵆鍏抽棴
4. 鑻ュ垹闄ら」瀛樺湪 hover preview override锛屽垯绔嬪嵆娓呯┖

瀹炵幇寤鸿锛?

1. 鐜版湁 `remove_last_filament()` 浠呬繚鐣欑粰鏃х殑鈥滃垹闄ゆ渶鍚庝竴涓€濊矾寰?
2. resident mapping 琛岀骇鍒犻櫎涓嶅缓璁鐢ㄢ€減op_back + refresh鈥濇€濊矾
3. 浼樺厛妗ユ帴宸叉湁鎴愮啛搴曞眰锛?
   - `Sidebar::delete_filament(filament_id, replace_filament_id = -1)`

淇濇姢瑙勫垯锛?

1. 褰撳彧鍓?1 涓満鏅鑹叉椂锛岀鐢ㄥ垹闄?
2. 鍒犻櫎鍓嶇粰鍑鸿交閲忕‘璁ゆ垨椋庨櫓鎻愮ず
3. 鏈樁娈典笉澧炲姞鈥滃垹闄ゅ悗鏀逛负鍚堝苟鈥濆垎鏀?

### 16.10 `+ 娣诲姞棰滆壊` 鎺т欢钀戒綅寤鸿

寤鸿鍦?resident mapping 鏍囬琛屽鍔犱竴涓彸瀵归綈鎸夐挳锛?

1. 鏂囨
   - `+ 娣诲姞棰滆壊`
2. 钀戒綅
   - 涓?`褰撳墠鍦烘櫙棰滆壊 / N 涓鑹瞏 鍚屼竴琛?
   - 鍙冲榻?
3. 琛屼负
   - 鐐瑰嚮鍚庡叧闂綋鍓?selector popup
   - 鎵撳紑鐜版湁 add color / material 閫夋嫨娴佺▼

瀹炵幇绾︽潫锛?

1. 璇ユ寜閽睘浜庨〉闈㈢骇鍔ㄤ綔锛屼笉璺熼殢鍏蜂綋琛屾粴鍔ㄥ埌 popup 鍐呴儴
2. 鍒拌揪鏈€澶?filament 鏁伴噺鏃剁疆鐏?
3. 涓嶅啀棰濆鎻愪緵涓€涓叏灞€ `-` 鎸夐挳浣滀负 resident 涓诲叆鍙?

### 16.11 琛岀骇鍒犻櫎鍏ュ彛鎺т欢寤鸿

寤鸿缁欐瘡涓?row 澧炲姞涓€涓嫭绔嬬殑鍒犻櫎鍏ュ彛锛?

1. 钀戒綅
   - 琛屽鍣ㄥ彸涓婅
   - 鐙珛浜?selector 鐑尯
2. 褰㈡€?
   - 灏忓昂瀵?icon button
   - 榛樿寮卞寲锛宧over / focused 鏃跺寮?
3. 琛屼负
   - 鐐瑰嚮鍚庝粎瀵瑰綋鍓?row 鐢熸晥
   - 涓嶆墦寮€ selector popup

瀹炵幇绾︽潫锛?

1. 鍒犻櫎 icon 鐨勭偣鍑荤儹鍖轰笉瑕佷笌 selector 鐑尯閲嶅彔
2. 琛岀骇鍒犻櫎涓嶅缓璁寕鍒?selector popup 鍐?
3. 褰撲粎鍓?1 琛屾椂锛屾寜閽鐢?

寤鸿鐘舵€佸瓧娈碉細

1. `can_delete_row`
2. `pending_delete_row_index`
3. `show_delete_confirm`

### 16.12 鍒犻櫎纭寮瑰眰寤鸿

寤鸿琛ヤ竴涓交閲?modal / confirm popup锛?

1. 鏍囬
   - `鍒犻櫎璇ラ鑹诧紵`
2. 涓绘枃妗?
   - `鍒犻櫎鍚庯紝杩欎釜鍦烘櫙棰滆壊灏嗕粠褰撳墠鎵撳嵃浠诲姟涓Щ闄わ紝鏄犲皠鍏崇郴鍜岄瑙堜細鍚屾鏇存柊銆俙
3. 杈呭姪鏂囨
   - `姝ゆ搷浣滀笉浼氬垹闄よ澶囦腑鐨勮€楁潗锛屽彧浼氬垹闄ゅ綋鍓嶅満鏅鑹层€俙
4. 鎸夐挳
   - `鍙栨秷`
   - `鍒犻櫎`

瀹炵幇绾︽潫锛?

1. 榛樿鐒︾偣搴斿湪 `鍙栨秷`
2. `鍒犻櫎` 浣跨敤鍗遍櫓鎬佹寜閽壊
3. `Esc`銆佸彸涓婅鍏抽棴銆佺偣鍑诲彇娑堥兘搴旂粓姝㈡湰娆″垹闄?

纭鍒犻櫎鍚庣殑鑱斿姩锛?

1. 璋冪敤鎸夌储寮曞垹闄ら€昏緫
2. 娓呯┖ `pending_delete_row_index`
3. 鍏抽棴褰撳墠琛?popup
4. 娓呯┖璇ヨ hover preview override
5. 閲嶅缓 panel view data
## 16. 澶氱洏鍦烘櫙涓嬬殑杩愯鏃舵樉绀鸿鍒欒ˉ鍏?

鏈妭琛ュ厖褰撳墠鐩?/ 鍏ㄤ换鍔′袱灞備俊鎭粨鏋勶紝浣滀负 resident mapping 鍦ㄥ鐩樺満鏅笅鐨勭粺涓€杩愯鏃惰鍒欍€?

### 16.1 鏍稿績鍘熷垯

澶氱洏鍦烘櫙涓嬶紝宸︿笂瑙?resident mapping 妯″潡榛樿鏈嶅姟浜庯細

**褰撳墠閫変腑鐩?*

涓嶉粯璁ゆ湇鍔′簬鏁翠釜浠诲姟鐨勫叏閮ㄩ鑹查泦鍚堛€?

鍥犳杩愯鏃堕渶瑕佹槑纭尯鍒嗕袱绫绘暟鎹細

1. `current_plate_rows`
2. `other_plates_rows` 鎴?`task_level_rows_except_current`

### 16.2 寤鸿鏂板鐨勭洏绾ц緭鍏ヤ俊鍙?

寤鸿鍦ㄧ幇鏈夎繍琛屾椂杈撳叆淇″彿涔嬪锛岃ˉ鍏呬竴缁勭洏绾ц緭鍏ワ細

```cpp
struct ResidentFilamentPlateSignals
{
    int  current_plate_index = -1;
    int  current_plate_color_count = 0;
    int  task_level_color_count = 0;
    bool has_other_plate_colors = false;
    int  other_plate_color_count = 0;
    bool current_plate_has_unmapped_rows = false;
    bool other_plates_have_unmapped_rows = false;
};
```

### 16.3 瀛楁璇箟

`current_plate_index`

1. 褰撳墠涓荤敾甯冮€変腑鐨勭洏绱㈠紩
2. 鐢ㄤ簬椹卞姩宸︿晶鍒楄〃涓庨瑙堢殑涓讳笂涓嬫枃

`current_plate_color_count`

1. 褰撳墠鐩樺疄闄呬娇鐢ㄥ埌鐨勯鑹叉暟閲?
2. 鐢ㄤ簬鏍囬鍓枃妗堜笌褰撳墠鐩樺垪琛ㄦ爣棰?

`task_level_color_count`

1. 鏁翠釜浠诲姟鎵€鏈夌洏鍚堝苟鍚庣殑鍘婚噸棰滆壊鏁?
2. 鐢ㄤ簬鏍囬鍓枃妗堜腑鐨勫叏浠诲姟璁℃暟

`has_other_plate_colors`

1. 闄ゅ綋鍓嶇洏澶栵紝鏄惁杩樻湁鍏朵粬鐩橀鑹查渶瑕佸睍绀?
2. 鐢ㄤ簬鍐冲畾鏄惁鏄剧ず鎶樺彔鍖哄叆鍙?

`other_plate_color_count`

1. 闄ゅ綋鍓嶇洏澶栫殑鍏朵粬鐩橀鑹叉暟
2. 鐢ㄤ簬鎶樺彔鍖烘憳瑕佹枃妗堬紝渚嬪 `杩樻湁 3 涓鑹瞏

`current_plate_has_unmapped_rows`

1. 褰撳墠鐩樻槸鍚﹀瓨鍦ㄦ湭鏄犲皠鎴栭珮椋庨櫓琛?
2. 浣滀负鏍囬鍖轰富鎻愮ず浼樺厛绾ф渶楂樼殑寮傚父杈撳叆

`other_plates_have_unmapped_rows`

1. 鍏朵粬鐩樻槸鍚﹀瓨鍦ㄦ湭鏄犲皠鎴栭珮椋庨櫓琛?
2. 浠呭綋褰撳墠鐩樻甯告椂锛屾墠浣滀负寮辨彁閱掓樉绀?

### 16.4 寤鸿鏂板鐨勬暟鎹瀯寤哄嚱鏁?

寤鸿鍦?adapter / model builder 灞傝ˉ鍏咃細

```cpp
ResidentFilamentPlateSignals collect_resident_filament_plate_signals();
std::vector<RowInput> build_current_plate_row_inputs(...);
std::vector<RowInput> build_other_plate_row_inputs(...);
```

鑱岃矗绾︽潫濡備笅锛?

1. `build_current_plate_row_inputs(...)` 鍙敓鎴愬綋鍓嶇洏鐨勯鑹茶
2. `build_other_plate_row_inputs(...)` 鍙敓鎴愬叾浠栫洏棰滆壊琛?
3. 涓嶈鍦?view 灞備复鏃惰繃婊も€滄槸鍚﹀睘浜庡綋鍓嶇洏鈥?

### 16.5 寤鸿鏂板鐨?view data 瀛楁

寤鸿鍦?`PanelViewData` 灞傚鍔犱互涓嬪瓧娈碉細

```cpp
std::string current_plate_title;
std::string current_plate_count_text;
std::string plate_scope_subtitle;
std::string other_plates_fold_label;
bool        show_other_plates_section = false;
bool        other_plates_section_expanded = false;
std::vector<RowViewData> current_plate_rows;
std::vector<RowViewData> other_plate_rows;
```

鍏朵腑锛?

`current_plate_title`

鎺ㄨ崘鍥哄畾涓猴細

`褰撳墠鐩樿€楁潗鏄犲皠`

`plate_scope_subtitle`

鎺ㄨ崘鏍煎紡涓猴細

`褰撳墠鐩?X 涓鑹?/ 鍏ㄤ换鍔?Y 涓鑹瞏

`other_plates_fold_label`

鎺ㄨ崘鏍煎紡涓猴細

`鍏朵粬鐩橀鑹叉槧灏刞

### 16.6 榛樿娓叉煋椤哄簭

澶氱洏鍦烘櫙涓嬬殑娓叉煋椤哄簭寤鸿鍥哄畾涓猴細

1. 鏍囬鍖?
2. 褰撳墠鐩橀鑹插垪琛?
3. 鍏朵粬鐩橀鑹叉姌鍙犲尯
4. 褰撳墠鐩樻墦鍗版晥鏋滈瑙?

瀹炵幇绾︽潫锛?

1. 褰撳墠鐩樺垪琛ㄥ繀椤诲缁堝睍寮€
2. 鍏朵粬鐩橀鑹查粯璁ゆ敹璧?
3. 棰勮濮嬬粓缁戝畾褰撳墠鐩?

### 16.7 鏍囬涓庡壇鏂囨瑙勫垯

鏍囬鍖鸿繍琛屾椂鏂囨寤鸿鎸変互涓嬭鍒欑敓鎴愶細

涓绘爣棰橈細

`褰撳墠鐩樿€楁潗鏄犲皠`

鍓枃妗堬細

`褰撳墠鐩?X 涓鑹?/ 鍏ㄤ换鍔?Y 涓鑹瞏

涓嶈鍐嶅湪澶氱洏鍦烘櫙涓嬬户缁娇鐢ㄦ硾鍖栨爣棰橈細

`鍦烘櫙鑰楁潗鏄犲皠`

### 16.8 鎶樺彔鍖烘樉绀烘潯浠?

鍙湁婊¤冻浠ヤ笅鏉′欢鏃舵墠鏄剧ず鎶樺彔鍖哄叆鍙ｏ細

```cpp
show_other_plates_section = has_other_plate_colors;
```

鎶樺彔鍖烘敹璧锋€佹樉绀猴細

1. 鏍囬 `鍏朵粬鐩橀鑹叉槧灏刞
2. 鍙充晶鎽樿 `杩樻湁 N 涓鑹瞏

鎶樺彔鍖哄睍寮€鍚庯細

1. 鍙樉绀?`other_plate_rows`
2. 涓嶉噸澶嶆覆鏌撳綋鍓嶇洏棰滆壊

### 16.9 寮傚父鎻愮ず浼樺厛绾?

澶氱洏鍦烘櫙涓嬶紝鏍囬鍖?banner 寤鸿鎸夊涓嬩紭鍏堢骇鐢熸垚锛?

1. 褰撳墠鐩樻湭鏄犲皠 / 楂橀闄?
2. 璁惧绂荤嚎
3. 鍗曡壊璁惧
4. 鍏朵粬鐩樻湭澶勭悊

瀵瑰簲鎺ㄨ崘鏂囨锛?

褰撳墠鐩樺紓甯革細

`褰撳墠鐩樿繕鏈?2 涓鑹插緟鏄犲皠`

璁惧绂荤嚎锛?

`璁惧绂荤嚎锛屽綋鍓嶄粎鏄剧ず涓婃鏄犲皠缁撴灉`

鍗曡壊璁惧锛?

`褰撳墠璁惧涓哄崟鑹茶緭鍑猴紝褰撳墠鐩橀鑹插皢缁熶竴鏄犲皠鍒板悓涓€鑰楁潗`

鍏朵粬鐩樺紓甯革細

`鍏朵粬鐩樿繕鏈?2 涓鑹插緟澶勭悊`

鍏朵腑绗?4 绫诲彧鑳藉湪鍓?3 绫婚兘涓嶈Е鍙戞椂鏄剧ず銆?

### 16.10 澶氱洏鍒囨崲鏃剁殑鑱斿姩瑙勫垯

褰撲富鐢诲竷褰撳墠鐩樺垏鎹㈡椂锛宺esident mapping 妯″潡闇€瑕佹墽琛屼竴娆″畬鏁撮噸寤恒€?

寤鸿鏈€灏戞墽琛屼互涓嬫楠わ細

1. 閲嶆柊鏀堕泦 `ResidentFilamentPlateSignals`
2. 閲嶅缓 `current_plate_rows`
3. 閲嶅缓 `other_plate_rows`
4. 閲嶅缓鏍囬鍓枃妗?
5. 閲嶅缓褰撳墠鐩橀瑙?
6. 鍏抽棴鎵€鏈夊綋鍓嶇洏涓婁笅鏂囩浉鍏?popup

浼唬鐮佸缓璁細

```cpp
on_current_plate_changed()
{
    close_resident_mapping_popups();
    auto runtime_signals = collect_resident_filament_runtime_signals();
    auto plate_signals = collect_resident_filament_plate_signals();
    auto ui_model = build_ui_model(runtime_signals, build_current_plate_row_inputs(...), unified_output);
    auto other_rows = build_other_plate_row_inputs(...);
    auto panel_view = build_panel_view_data(...);
    panel_view.current_plate_rows = ...;
    panel_view.other_plate_rows = ...;
}
```

### 16.11 鐘舵€佷繚鎸佸缓璁?

澶氱洏鍒囨崲鏃讹紝寤鸿鍙繚鎸侊細

1. `鍏朵粬鐩橀鑹叉槧灏刞 鎶樺彔鍖虹殑灞曞紑 / 鏀惰捣鐘舵€?

涓嶅缓璁繚鎸侊細

1. 鏌愪釜棰滆壊琛岀殑 popup 鎵撳紑鐘舵€?
2. 鏌愪釜鍊欓€夐」鐨?hover 棰勮鐘舵€?
3. 鏃х洏涓婁笅鏂囦笅鐨勪复鏃堕€夋嫨鐘舵€?

### 16.12 瀹炵幇鍩虹嚎缁撹

澶氱洏鍦烘櫙涓嬬殑瀹炵幇鍩虹嚎鍙互鏀舵暃涓轰竴鍙ヨ瘽锛?

**宸︿笂瑙掍富鍗″彧瑙ｉ噴褰撳墠鐩橈紱鍏ㄤ换鍔′俊鎭€氳繃璁℃暟鍜屾姌鍙犲尯琛ュ厖锛涘綋鍓嶇洏棰勮銆佸綋鍓嶇洏鏄犲皠銆佸綋鍓嶇洏寮傚父鎻愮ず蹇呴』淇濇寔鍚屼竴涓婁笅鏂囥€?*

## 17. 澶氱洏鍦烘櫙鐨?ImGui 钀藉湴浠诲姟娓呭崟

鏈妭灏嗗墠鏂囩殑浜у搧缁撹杩涗竴姝ュ帇缂╂垚涓€鐗堟洿鐩存帴鐨?ImGui 钀藉湴浠诲姟娓呭崟銆?

鐩爣涓嶆槸閲嶆柊璁ㄨ鏂规锛岃€屾槸鏄庣‘锛?

1. 闇€瑕佹柊澧炲摢浜涚姸鎬?
2. 闇€瑕佹敼鍝竴灞?
3. 姣忎竴灞傜殑鑱岃矗杈圭晫鏄粈涔?
4. UI 涓婃湁鍝簺蹇呴』瀹屾垚鐨勪氦浜掕仈鍔?

### 17.1 鎬讳綋钀藉湴鐩爣

鍦?ImGui 钀藉湴鏃讹紝闇€瑕佸疄鐜颁互涓嬫渶缁堟晥鏋滐細

1. 宸︿笂瑙掓爣棰樺尯榛樿琛ㄨ揪褰撳墠鐩橈紝鑰屼笉鏄叏浠诲姟
2. 褰撳墠鐩橀鑹插垪琛ㄥ彧鏄剧ず褰撳墠鐩樹娇鐢ㄥ埌鐨勯鑹?
3. 鍏ㄤ换鍔″叾浣欓鑹查€氳繃鎶樺彔鍖烘樉绀?
4. 鎵撳嵃鏁堟灉棰勮濮嬬粓缁戝畾褰撳墠鐩?
5. 褰撳墠鐩樺垏鎹㈡椂锛屽乏涓婅鏁村崱鍚屾鍒锋柊

### 17.2 ViewModel 灞備换鍔?

#### 17.2.1 `PanelViewData` 缁撴瀯琛ュ厖

闇€瑕佸湪 `PanelViewData` 涓ˉ鍏呬互涓嬪瓧娈碉細

```cpp
std::string current_plate_title;
std::string plate_scope_subtitle;
std::string current_plate_count_text;

bool        show_other_plates_section = false;
bool        other_plates_section_expanded = false;
std::string other_plates_fold_label;
std::string other_plates_fold_count_text;

std::vector<RowViewData> current_plate_rows;
std::vector<RowViewData> other_plate_rows;
```

钀藉湴瑕佹眰锛?

1. `rows` 涓嶅啀缁х画鎵挎媴鈥滃綋鍓嶇洏 + 鍏朵粬鐩樻贩鍚堝垪琛ㄢ€濈殑鍞竴琛ㄨ揪
2. `current_plate_rows` 涓?`other_plate_rows` 蹇呴』鍦?model builder 闃舵鎷嗗ソ
3. view 灞備笉涓存椂鍋氱洏绾ц繃婊?

#### 17.2.2 鏍囬鍖烘暟鎹敓鎴?

鍦?adapter / model builder 灞傜敓鎴愶細

1. `current_plate_title = 褰撳墠鐩樿€楁潗鏄犲皠`
2. `plate_scope_subtitle = 褰撳墠鐩?X 涓鑹?/ 鍏ㄤ换鍔?Y 涓鑹瞏
3. `current_plate_count_text = X 涓鑹瞏
4. `other_plates_fold_label = 鍏朵粬鐩橀鑹叉槧灏刞
5. `other_plates_fold_count_text = 杩樻湁 N 涓鑹瞏

### 17.3 Adapter / Builder 灞備换鍔?

#### 17.3.1 鏂板鐩樼骇鏁版嵁鏀堕泦

寤鸿鏂板锛?

```cpp
ResidentFilamentPlateSignals collect_resident_filament_plate_signals(...);
```

鑱岃矗锛?

1. 璇嗗埆褰撳墠閫変腑鐩?
2. 缁熻褰撳墠鐩橀鑹叉暟
3. 缁熻鍏ㄤ换鍔″幓閲嶉鑹叉暟
4. 鍒ゆ柇鏄惁瀛樺湪鍏朵粬鐩橀鑹?
5. 鍒ゆ柇褰撳墠鐩樻槸鍚︽湁寮傚父
6. 鍒ゆ柇鍏朵粬鐩樻槸鍚︽湁寮傚父

#### 17.3.2 鎷嗗垎褰撳墠鐩?/ 鍏朵粬鐩橀鑹茶緭鍏?

寤鸿鏂板锛?

```cpp
std::vector<RowInput> build_current_plate_row_inputs(...);
std::vector<RowInput> build_other_plate_row_inputs(...);
```

鑱岃矗锛?

1. `build_current_plate_row_inputs(...)` 鍙骇鍑哄綋鍓嶇洏棰滆壊
2. `build_other_plate_row_inputs(...)` 鍙骇鍑哄叾浠栫洏棰滆壊
3. 涓よ€呭叡浜悓涓€濂楄绾ф瀯寤鸿鍒欙紝浣嗚緭鍏ラ泦鍚堜笉鍚?

#### 17.3.3 `build_panel_view_data(...)` 鎵╁睍

鍦?`build_panel_view_data(...)` 涓ˉ鍏咃細

1. 褰撳墠鐩樻爣棰樹笌鍓爣棰?
2. 褰撳墠鐩樺垪琛?
3. 鍏朵粬鐩樻姌鍙犲尯鏍囬
4. 鍏朵粬鐩樺垪琛?
5. 鎶樺彔鍖烘槸鍚︽樉绀?

### 17.4 View 灞備换鍔?

#### 17.4.1 鏍囬鍖烘覆鏌撴敼閫?

`render_summary_card(...)` 鎴栧搴旀爣棰樺尯鍑芥暟闇€瑕佽皟鏁翠负锛?

1. 涓绘爣棰樻樉绀?`褰撳墠鐩樿€楁潗鏄犲皠`
2. 鍓爣棰樻樉绀?`褰撳墠鐩?X 涓鑹?/ 鍏ㄤ换鍔?Y 涓鑹瞏
3. 璁惧鐘舵€佽兌鍥婄户缁斁鍦ㄦ爣棰樺尯鍙充笂
4. banner 鎸夊綋鍓嶇洏浼樺厛绾ц緭鍑?

#### 17.4.2 褰撳墠鐩樺垪琛ㄦ覆鏌?

`render_rows_scroll(...)` 闇€瑕佹敼閫犳垚鍙覆鏌擄細

```cpp
current_plate_rows
```

鍒楄〃鏍囬鏂囨鏀逛负锛?

`褰撳墠鐩橀鑹瞏

鍙充晶灏忓瓧锛?

`X 涓鑹瞏

#### 17.4.3 鏂板鈥滃叾浠栫洏棰滆壊鏄犲皠鈥濇姌鍙犲尯

闇€瑕佹柊澧炰竴涓笓闂ㄧ殑鎶樺彔鍖烘覆鏌撳嚱鏁帮紝渚嬪锛?

```cpp
void render_other_plates_section(const PanelViewData& view_data, const Callbacks& callbacks);
```

鎶樺彔鍖鸿鍒欙細

1. 鏀惧湪褰撳墠鐩樺垪琛ㄤ笅鏂?
2. 鏀惧湪鎵撳嵃鏁堟灉棰勮涓婃柟
3. 鏀惰捣鎬佸彧鏄剧ず鏍囬涓庢憳瑕佹暟閲?
4. 灞曞紑鎬佸彧鏄剧ず `other_plate_rows`
5. 灞曞紑鍚庝笉閲嶅褰撳墠鐩橀鑹?

#### 17.4.4 棰勮鍖轰繚鎸佸綋鍓嶇洏缁戝畾

`render_preview_card(...)` 涓嶉渶瑕佸垏鎴愬叏浠诲姟鎬昏銆?

钀藉湴瑕佹眰锛?

1. 褰撳墠鐩樺垏鎹㈡椂鍒锋柊棰勮
2. hover 鍊欓€夐」鏃朵粛鍙奖鍝嶅綋鍓嶇洏棰勮
3. 鍏朵粬鐩樻姌鍙犲尯涓殑 hover 涓嶅奖鍝嶅綋鍓嶇洏棰勮锛岄櫎闈炴湭鏉ヤ骇鍝佹槑纭姹傛敮鎸佽法鐩?hover 棰勮

褰撳墠闃舵寤鸿锛?

**鍙厑璁稿綋鍓嶇洏鍒楄〃瑙﹀彂 hover preview**

### 17.5 Runtime State 灞備换鍔?

#### 17.5.1 鎶樺彔鍖哄睍寮€鐘舵€?

闇€瑕佸鍔犱竴涓交閲忚繍琛屾椂鐘舵€侊紝渚嬪锛?

```cpp
bool m_other_plates_section_expanded = false;
```

鑱岃矗锛?

1. 璁板綍鈥滃叾浠栫洏棰滆壊鏄犲皠鈥濇槸鍚﹀睍寮€
2. 褰撳墠鐩樺垏鎹㈠悗淇濇寔璇ョ姸鎬?
3. 涓嶄笌 popup 鐘舵€佹贩鐢?

#### 17.5.2 褰撳墠鐩樺垏鎹㈡椂鐨勯噸缃」

褰撳墠鐩樺垏鎹㈡椂锛屽繀椤婚噸缃細

1. 鎵撳紑鐨?selector popup
2. 鎵撳紑鐨?scene material popup
3. hover preview override
4. 褰撳墠鐩樹笂涓嬫枃涓嬬殑 pending row index

褰撳墠鐩樺垏鎹㈡椂锛屽彲淇濈暀锛?

1. `鍏朵粬鐩橀鑹叉槧灏刞 鎶樺彔鍖哄睍寮€ / 鏀惰捣鐘舵€?

### 17.6 Banner 鐢熸垚浠诲姟

寤鸿鏂板涓€涓洿鏄庣‘鐨?banner 鍐崇瓥鍑芥暟锛屼緥濡傦細

```cpp
ResidentFilamentBannerViewModel build_plate_scope_banner(
    const RuntimeSignals& runtime_signals,
    const ResidentFilamentPlateSignals& plate_signals);
```

杈撳嚭浼樺厛绾у浐瀹氫负锛?

1. 褰撳墠鐩樻湭鏄犲皠 / 楂橀闄?
2. 璁惧绂荤嚎
3. 鍗曡壊璁惧
4. 鍏朵粬鐩樻湭澶勭悊

涓嶈鍦?view 灞傜洿鎺ユ嫾鎺ュ鏉傛潯浠躲€?

### 17.7 Callback 涓庤仈鍔ㄤ换鍔?

#### 17.7.1 褰撳墠鐩樺垏鎹㈣仈鍔?

闇€瑕佹帴鍏ヤ竴涓€滃綋鍓嶇洏鍙樻洿鈥濊Е鍙戠偣銆?

钀藉湴鍚庡簲鎵ц锛?

1. 閲嶆柊鏀堕泦褰撳墠鐩?/ 鍏朵粬鐩樿緭鍏?
2. 閲嶅缓 panel view data
3. 鍒锋柊褰撳墠鐩橀瑙?
4. 鍏抽棴涓庢棫鐩樼浉鍏崇殑 popup

#### 17.7.2 褰撳墠鐩樺垪琛ㄤ笌鎶樺彔鍖虹殑浜や簰闅旂

钀藉湴鏃惰淇濊瘉锛?

1. 褰撳墠鐩樺垪琛ㄤ腑鐨?popup 浠嶆寜鐜版湁浜や簰宸ヤ綔
2. 鍏朵粬鐩樻姌鍙犲尯鑻ユ殏涓嶆敮鎸佷氦浜掞紝鍒欐槑纭彧璇绘樉绀?
3. 涓嶈璁╁叾浠栫洏鍒楄〃鎶㈠崰褰撳墠鐩?hover preview

濡傛灉绗竴闃舵瑕佹帶椋庨櫓锛屽缓璁細

1. 褰撳墠鐩樺垪琛ㄤ繚鎸佸彲浜や簰
2. 鍏朵粬鐩樻姌鍙犲尯鍏堝仛鍙

### 17.8 绗竴闃舵鎺ㄨ崘鎷嗗垎椤哄簭

寤鸿鎸変互涓嬮『搴忓疄鐜帮紝椋庨櫓鏈€浣庯細

#### 绗竴姝?

琛?`ResidentFilamentPlateSignals` 涓庡綋鍓嶇洏 / 鍏朵粬鐩?row builder銆?

#### 绗簩姝?

鎵╁睍 `PanelViewData`锛屾妸褰撳墠鐩樻爣棰樸€佸壇鏍囬銆佷袱缁?rows 鎺ヨ繘鏉ャ€?

#### 绗笁姝?

鏀?view锛?

1. 鏍囬鏀规垚褰撳墠鐩樿涔?
2. 涓诲垪琛ㄥ彧娓叉煋褰撳墠鐩?
3. 鍔犲叾浠栫洏鎶樺彔鍖?

#### 绗洓姝?

鎺ュ綋鍓嶇洏鍒囨崲鑱斿姩锛?

1. 鍒囩洏鍒锋柊鏍囬
2. 鍒囩洏鍒锋柊褰撳墠鐩樺垪琛?
3. 鍒囩洏鍒锋柊棰勮
4. 鍒囩洏鍏抽棴鏃?popup

#### 绗簲姝?

琛ュ紓甯?banner 鐨勫綋鍓嶇洏 / 鍏朵粬鐩樹紭鍏堢骇閫昏緫銆?

### 17.9 绗竴闃舵鍙帴鍙楃殑绠€鍖?

涓轰簡鍏堣惤鍦帮紝鍙互鎺ュ彈浠ヤ笅绠€鍖栵細

1. `鍏朵粬鐩橀鑹叉槧灏刞 鍏堜笉鏀寔缂栬緫锛屽彧鍋氬彧璇?
2. 鎶樺彔鍖哄厛涓嶅仛澶嶆潅鍔ㄧ敾锛屽彧鍋氬紑鍏冲垏鎹?
3. 褰撳墠鐩?/ 鍏ㄤ换鍔¤鏁板厛鍩轰簬宸叉湁棰滆壊闆嗗悎缁熻锛屼笉瑕佹眰涓€寮€濮嬪氨鍋氬埌鏈€澶嶆潅鐨勫幓閲嶈涔?

### 17.10 浜や粯瀹屾垚鍒ゅ畾

杩欒疆 ImGui 钀藉湴瀹屾垚鐨勫垽瀹氭爣鍑嗗缓璁负锛?

1. 澶氱洏浠诲姟鏃讹紝宸︿笂瑙掓爣棰樻槑纭负褰撳墠鐩樿涔?
2. 涓诲垪琛ㄥ彧鏄剧ず褰撳墠鐩橀鑹?
3. 鎶樺彔鍖鸿兘鏄剧ず鍏朵粬鐩橀鑹叉憳瑕?
4. 鎵撳嵃鏁堟灉棰勮涓庡綋鍓嶇洏淇濇寔涓€鑷?
5. 褰撳墠鐩樺垏鎹㈠悗锛屽乏渚т俊鎭棤鏃х洏娈嬬暀
6. 褰撳墠鐩樺紓甯镐紭鍏堜簬鍏朵粬鐩樺紓甯告彁绀?

## 18. 鐩存帴瀵瑰簲婧愮爜鏂囦欢鐨勫疄鏂芥竻鍗?

鏈妭灏?`17.x` 杩涗竴姝ュ帇缂╂垚鐩存帴瀵瑰簲婧愮爜鏂囦欢鐨勫疄鏂芥竻鍗曘€?

鐩爣鏄槑纭細

1. 姣忎釜鏂囦欢璐熻矗鍝竴灞?
2. 鍏蜂綋瑕佹柊澧炴垨璋冩暣浠€涔?
3. 鍝簺閫昏緫涓嶈鏀鹃敊灞?

褰撳墠寤鸿涓昏钀藉湪浠ヤ笅 3 涓枃浠讹細

1. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp`
2. `src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.cpp`
3. `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

---

### 18.1 `ResidentFilamentMappingAdapter.cpp` 瀹炴柦娓呭崟

璇ユ枃浠剁殑鑱岃矗搴旂户缁繚鎸佷负锛?

**鎶婁笟鍔¤緭鍏ユ暣鐞嗘垚鍙洿鎺ヤ緵 view 娓叉煋鐨勭粨鏋勫寲鏁版嵁銆?*

涓嶈鍦ㄨ繖閲岀洿鎺ョ敾 UI锛屼篃涓嶈鍦ㄨ繖閲屼繚瀛?popup 鎵撳紑鐘舵€併€?

#### 18.1.1 瑕佹柊澧炵殑鍐呭

寤鸿鍦ㄨ鏂囦欢涓柊澧炴垨鎵╁睍浠ヤ笅鑳藉姏锛?

1. 鏀堕泦鐩樼骇淇″彿
2. 鎷嗗垎褰撳墠鐩橀鑹茶緭鍏?
3. 鎷嗗垎鍏朵粬鐩橀鑹茶緭鍏?
4. 鐢熸垚褰撳墠鐩樻爣棰樸€佸壇鏍囬銆佹姌鍙犲尯鎽樿
5. 鐢熸垚褰撳墠鐩樺紓甯?/ 鍏朵粬鐩樺紓甯告墍闇€鐨?banner 杈撳叆

#### 18.1.2 寤鸿鏂板鍑芥暟

寤鸿鏂板锛?

```cpp
ResidentFilamentPlateSignals collect_resident_filament_plate_signals(...);
std::vector<RowInput> build_current_plate_row_inputs(...);
std::vector<RowInput> build_other_plate_row_inputs(...);
```

濡傛灉涓嶆兂鎶婂嚱鏁板悕鎷嗗緱杩囩粏锛屼篃鍙互鍦ㄧ幇鏈?`build_panel_view_data(...)` 涔嬪墠澧炲姞涓€涓洿涓婃父鐨?builder锛?

```cpp
PlateScopedPanelInput build_plate_scoped_panel_input(...);
```

#### 18.1.3 `build_panel_view_data(...)` 闇€瑕佸仛鐨勪簨

鐜版湁 `build_panel_view_data(...)` 闇€瑕佹墿灞曚负鑳界敓鎴愶細

1. `current_plate_title`
2. `plate_scope_subtitle`
3. `current_plate_count_text`
4. `show_other_plates_section`
5. `other_plates_fold_label`
6. `other_plates_fold_count_text`
7. `current_plate_rows`
8. `other_plate_rows`

#### 18.1.4 杩欎竴灞備笉瑕佸仛鐨勪簨

涓嶈鍦?`ResidentFilamentMappingAdapter.cpp` 閲屽仛锛?

1. 鎶樺彔鍖哄睍寮€ / 鏀惰捣鐘舵€佷繚瀛?
2. popup 鎵撳紑 / 鍏抽棴鎺у埗
3. 褰撳墠鐩樺垏鎹㈠悗鐨?UI reset
4. hover preview override

杩欎簺閮戒笉灞炰簬 adapter 灞傘€?

#### 18.1.5 绗竴闃舵瀹炵幇寤鸿

绗竴闃舵鍙互鍏堜笉鍦ㄨ繖涓€灞傝拷姹傗€滄渶瀹岀編鐨勫叏浠诲姟棰滆壊鍘婚噸鈥濄€?

浼樺厛淇濊瘉锛?

1. 褰撳墠鐩橀鑹查泦鍚堟纭?
2. 鍏朵粬鐩橀鑹查泦鍚堟纭?
3. 鏍囬涓庢姌鍙犲尯鎽樿鑳芥纭窡闅忓綋鍓嶇洏鍙樺寲

---

### 18.2 `ResidentFilamentMappingView.cpp` 瀹炴柦娓呭崟

璇ユ枃浠剁殑鑱岃矗搴旂户缁繚鎸佷负锛?

**鍙礋璐ｆ妸 `PanelViewData` 娓叉煋鎴?ImGui 鐣岄潰銆?*

涓嶈鍦ㄨ繖閲屽仛涓氬姟鍒ゆ柇锛屼笉瑕佸湪杩欓噷鑷繁璁＄畻鈥滃摢浜涢鑹插睘浜庡綋鍓嶇洏鈥濄€?

#### 18.2.1 鏍囬鍖烘敼閫?

闇€瑕佹敼閫犳爣棰?/ 鎽樿鍖虹殑娓叉煋鍑芥暟锛屼娇鍏舵敮鎸侊細

1. 涓绘爣棰樻樉绀?`褰撳墠鐩樿€楁潗鏄犲皠`
2. 鍓爣棰樻樉绀?`褰撳墠鐩?X 涓鑹?/ 鍏ㄤ换鍔?Y 涓鑹瞏
3. 璁惧鐘舵€佽兌鍥婄户缁繚鐣?
4. banner 浼樺厛鏄剧ず褰撳墠鐩樺紓甯革紝鍐嶉檷绾ф樉绀哄叾浠栫洏寮傚父

瀵瑰簲鍑芥暟閫氬父鏄細

1. `render_summary_card(...)`
2. 鎴栦笌鍏剁瓑浠风殑鏍囬娓叉煋鍑芥暟

#### 18.2.2 涓诲垪琛ㄦ敼閫?

褰撳墠涓诲垪琛ㄦ覆鏌撳嚱鏁伴渶瑕佷粠锛?

```cpp
render_rows_scroll(view_data.rows, ...)
```

鏀逛负鏄庣‘娓叉煋锛?

```cpp
render_current_plate_rows(view_data.current_plate_rows, ...)
```

鎴栬€呭湪鐜版湁 `render_rows_scroll(...)` 涓敼鎴愯鍙栵細

```cpp
view_data.current_plate_rows
```

鏍稿績瑕佹眰锛?

1. 褰撳墠鐩樺垪琛ㄥ缁堝睍寮€
2. 褰撳墠鐩樺垪琛ㄥ彧鏄剧ず褰撳墠鐩橀鑹?

#### 18.2.3 鏂板鈥滃叾浠栫洏棰滆壊鏄犲皠鈥濇姌鍙犲尯

寤鸿鏂板涓€涓嫭绔嬪嚱鏁帮紝渚嬪锛?

```cpp
void render_other_plates_section(const PanelViewData& view_data, const Callbacks& callbacks);
```

鍔熻兘瑕佹眰锛?

1. 鏀惰捣鎬佹樉绀烘爣棰樹笌鎽樿
2. 灞曞紑鎬佹樉绀?`other_plate_rows`
3. 鎶樺彔鍖轰綅浜庡綋鍓嶇洏鍒楄〃涓嬫柟銆侀瑙堝尯涓婃柟
4. 绗竴闃舵鍙厛鍙灞曠ず

#### 18.2.4 棰勮鍖轰繚鎸佸綋鍓嶇洏缁戝畾

`render_preview_card(...)` 闇€瑕佺户缁繚鎸佲€滃綋鍓嶇洏棰勮鈥濈殑璇箟銆?

杩欓噷瑕佹敞鎰忥細

1. 涓嶈鍥犱负鏈?`other_plate_rows` 灏辨妸棰勮鍙樻垚鍏ㄤ换鍔￠瑙?
2. 褰撳墠鐩樺垏鎹㈠悗锛岄瑙堝尯瑕佸拰鏍囬銆佸壇鏍囬銆佸綋鍓嶇洏鍒楄〃鍚屾

#### 18.2.5 View 灞備笉瑕佸仛鐨勪簨

涓嶈鍦?`ResidentFilamentMappingView.cpp` 閲屽仛锛?

1. 鍒ゆ柇褰撳墠鐩?index
2. 杩囨护褰撳墠鐩?/ 鍏朵粬鐩橀鑹?
3. 缁熻鍏ㄤ换鍔￠鑹叉暟
4. 鍐冲畾 banner 浼樺厛绾?

杩欎簺閮藉簲璇ュ湪 adapter / builder 灞傛彁鍓嶅噯澶囧ソ銆?

#### 18.2.6 绗竴闃舵瀹炵幇寤鸿

绗竴闃舵寤鸿鍏堝仛锛?

1. 鏍囬鍖哄垏褰撳墠鐩樿涔?
2. 涓诲垪琛ㄥ垏鎴愬綋鍓嶇洏
3. 鏂板涓€涓渶杞婚噺鐨勫叾浠栫洏鎶樺彔鍖?

鍏堜笉瑕佷竴寮€濮嬪氨鍦?view 灞傚仛澶鏉傜殑浜や簰鍔ㄦ晥銆?

---

### 18.3 `ImGuiFilamentPanel.cpp` 瀹炴柦娓呭崟

璇ユ枃浠跺綋鍓嶆洿鍍?controller / runtime orchestration 灞傘€?

瀹冨簲璐熻矗锛?

1. 鏀堕泦褰撳墠杩愯鏃朵笂涓嬫枃
2. 璋?adapter 鐢熸垚 view data
3. 璋?view 娓叉煋
4. 绠＄悊 popup銆乭over preview銆佸垏鐩樿仈鍔ㄣ€佹姌鍙犵姸鎬佺瓑杩愯鏃剁姸鎬?

#### 18.3.1 瑕佹柊澧炴垨缁存姢鐨勮繍琛屾椂鐘舵€?

寤鸿鍦ㄨ鏂囦欢涓柊澧炴垨缁存姢锛?

```cpp
bool m_other_plates_section_expanded = false;
int  m_last_current_plate_index = -1;
```

鑱岃矗锛?

1. 璁板綍鍏朵粬鐩樻姌鍙犲尯灞曞紑鐘舵€?
2. 妫€娴嬪綋鍓嶇洏鏄惁鍙戠敓鍒囨崲

濡傛灉涓嶆兂鏀惧埌鎴愬憳鍙橀噺锛屼篃鍙互鍏堟斁鍦?`transient_ui_state()` 閲岋紝浣嗚涔変笂鏈€濂藉尯鍒嗭細

1. 鐭湡 popup 鐘舵€?
2. 鍙法甯т繚鐣欑殑鎶樺彔鐘舵€?

#### 18.3.2 褰撳墠鐩樺垏鎹㈡娴?

闇€瑕佸湪 `Render()` 鎴栧叾涓婃父璋冪敤鐐归噷鍔犲叆鈥滃綋鍓嶇洏鏄惁鍒囨崲鈥濈殑妫€娴嬨€?

寤鸿娴佺▼锛?

1. 璇诲彇褰撳墠鐩?index
2. 涓?`m_last_current_plate_index` 姣旇緝
3. 鑻ュ彂鐢熷彉鍖栵紝鎵ц涓€娆?resident mapping 涓婁笅鏂囬噸缃?

閲嶇疆鍐呭寤鸿鍖呮嫭锛?

1. 鍏抽棴 selector popup
2. 鍏抽棴 scene material popup
3. 娓呯┖ hover preview override
4. 娓呯┖鏃х洏鐩稿叧 pending row index
5. 鏇存柊 `m_last_current_plate_index`

#### 18.3.3 ViewData 缁勮鍏ュ彛

鍦?`Render()` 涓紝闇€瑕佹妸褰撳墠鐩?/ 鍏朵粬鐩樹袱缁勬暟鎹兘鎺ュ埌 `build_panel_view_data(...)` 鐨勮緭鍏ヤ笂銆?

杩欎竴灞傜殑閲嶇偣鏄細

1. 璐熻矗鏀堕泦鈥滃綋鍓嶇洏鏄皝鈥?
2. 璐熻矗鎶婂綋鍓嶇洏涓婁笅鏂囧杺缁?adapter
3. 涓嶇洿鎺ユ墜鍐?view 灞傜殑鐩樼骇杩囨护閫昏緫

#### 18.3.4 鎶樺彔鍖虹姸鎬佸洖浼?

鑻?`ResidentFilamentMappingView.cpp` 鏂板鎶樺彔鍖轰氦浜掞紝鍒?`ImGuiFilamentPanel.cpp` 闇€瑕佽礋璐ｏ細

1. 鎺ユ敹鎶樺彔鍖哄睍寮€ / 鏀惰捣鍥炶皟
2. 鏇存柊 `m_other_plates_section_expanded`
3. 鎶婅鐘舵€佸啀浼犲洖 `PanelViewData`

#### 18.3.5 hover preview 浣滅敤鍩熺害鏉?

杩欎竴灞傝淇濊瘉锛?

1. 褰撳墠鐩樹富鍒楄〃鐨?hover preview 浠嶆湁鏁?
2. 鍏朵粬鐩樻姌鍙犲尯濡傛灉绗竴闃舵鏄彧璇伙紝灏变笉瑕佽Е鍙?hover preview
3. 鍗充娇鏈潵鍏朵粬鐩樻敮鎸?hover锛屼篃瑕佹槑纭槸鍚﹀厑璁稿叾瑕嗙洊褰撳墠鐩橀瑙?

绗竴闃舵鎺ㄨ崘锛?

**鍙湁褰撳墠鐩樺垪琛ㄨ兘椹卞姩 hover preview**

#### 18.3.6 杩欎竴灞備笉瑕佸仛鐨勪簨

涓嶈鍦?`ImGuiFilamentPanel.cpp` 涓‖缂栫爜锛?

1. 褰撳墠鐩樻爣棰樺瓧绗︿覆鎷兼帴缁嗚妭
2. 褰撳墠鐩?/ 鍏ㄤ换鍔¤鏁版牸寮忓寲缁嗚妭
3. 鍏朵粬鐩橀鑹茶繃婊よ鍒?

杩欎簺浠嶅簲灏介噺鐣欏湪 adapter / builder 灞傘€?

#### 18.3.7 绗竴闃舵鎺ㄨ崘鏀瑰姩椤哄簭

`ImGuiFilamentPanel.cpp` 鐨勬帹鑽愭敼鍔ㄩ『搴忓涓嬶細

1. 澧炲姞褰撳墠鐩樺垏鎹㈡娴?
2. 澧炲姞鎶樺彔鍖虹姸鎬佷繚瀛?
3. 璋冩暣 `Render()` 涓殑 view data 缁勮鍏ュ彛
4. 琛?popup reset 涓?hover preview reset
5. 鏈€鍚庢帴 view 灞傜殑鏂版姌鍙犲尯鍥炶皟

---

### 18.4 杩?3 涓枃浠剁殑鑱岃矗杈圭晫鎬荤粨

#### `ResidentFilamentMappingAdapter.cpp`

璐熻矗锛?

1. 涓氬姟杈撳叆鏁寸悊
2. 褰撳墠鐩?/ 鍏朵粬鐩樻暟鎹媶鍒?
3. 鏍囬銆佸壇鏍囬銆佹姌鍙犲尯鎽樿鐢熸垚

涓嶈礋璐ｏ細

1. popup 鐘舵€?
2. 鎶樺彔鐘舵€?
3. 鍏蜂綋 ImGui 缁樺埗

#### `ResidentFilamentMappingView.cpp`

璐熻矗锛?

1. 鏍囬鍖虹粯鍒?
2. 褰撳墠鐩樺垪琛ㄧ粯鍒?
3. 鍏朵粬鐩樻姌鍙犲尯缁樺埗
4. 棰勮鍗＄粯鍒?

涓嶈礋璐ｏ細

1. 褰撳墠鐩?/ 鍏朵粬鐩樺垽瀹?
2. 鏁版嵁缁熻
3. popup 鐢熷懡鍛ㄦ湡

#### `ImGuiFilamentPanel.cpp`

璐熻矗锛?

1. 杩愯鏃朵笂涓嬫枃鏀堕泦
2. 褰撳墠鐩樺垏鎹㈡娴?
3. popup / hover / fold state 绠＄悊
4. adapter 涓?view 涔嬮棿鐨勭紪鎺?

涓嶈礋璐ｏ細

1. 澶嶆潅瀛楃涓叉牸寮忓寲
2. 琛岀骇鏄剧ず鏁版嵁鐨勬牳蹇冪粍瑁?
3. 浣庡眰 UI 缁撴瀯缁嗚妭鍒ゆ柇

---

### 18.5 鎺ㄨ崘鐨勫疄闄呭紑宸ラ『搴?

濡傛灉鎸夋渶灏忛闄╁疄鏂斤紝寤鸿鐪熷疄寮€宸ラ『搴忓氨鏄細

1. 鍏堟敼 `ResidentFilamentMappingAdapter.cpp`
   - 鎶婂綋鍓嶇洏 / 鍏朵粬鐩樻暟鎹媶鍑烘潵
2. 鍐嶆敼 `ResidentFilamentMappingView.cpp`
   - 璁?view 鑳藉悆涓ょ粍 rows
3. 鏈€鍚庢敼 `ImGuiFilamentPanel.cpp`
   - 鎺ュ綋鍓嶇洏鍒囨崲銆佹姌鍙犵姸鎬併€乸opup reset

杩欐牱鍙互閬垮厤涓€寮€濮嬪氨鍦?controller 灞傚啓澶涓存椂鍒ゆ柇锛屾妸缁撴瀯鍐欎贡銆?

### 18.6 ???????????? ImGui ????

#### 18.6.1 ??????

?? resident mapping ?????????????

1. `current_plate_rows`
2. `other_plate_rows`

??????????????????????????????

1. ??????????????
2. ???????????????
3. ????????????????????????

#### 18.6.2 ??????

??? `PanelViewData` / adapter ??????? 3 ??

```cpp
std::vector<RowViewData> current_plate_rows;
std::vector<RowViewData> inactive_global_rows;
std::vector<RowViewData> non_current_rows;
```

???????

1. `current_plate_rows`
   - ????????????
   - ???????

2. `inactive_global_rows`
   - ????????????????
   - ????????????????????
   - ????????????????????

3. `non_current_rows`
   - view ?????????????????????
   - ??? adapter ????
   - ???? `inactive_global_rows` ??????????????????????

#### 18.6.3 Adapter ???

`ResidentFilamentMappingAdapter.cpp` ????????????

1. ?????????????
2. ????????????????
3. ????? - ??????
4. ?????? `inactive_global_rows`
5. ???????????????? `non_current_rows`
6. ???
   - ????????
   - ???????????
   - ??????????????

?????

1. ?????????? adapter
2. view ????????
3. panel/controller ?????????????????

#### 18.6.4 View ???

`ResidentFilamentMappingView.cpp` ????????????

1. ?????? `current_plate_rows`
2. ??????? `??????`
3. ??????? `non_current_rows`
4. ??????? `non_current_rows.size()` ? adapter ?????
5. ?????????????????? `non_current_rows` ??????

view ????????

1. ????????????
2. ???????????
3. ??????????????

#### 18.6.5 Panel / Controller ???

`ImGuiFilamentPanel.cpp` ???????????????????

1. ? `Render()` ???????????? adapter
2. ?? adapter ?? 3 ? rows
3. ??????????????????????
4. ??????????????
   - ??? rows ??
   - `inactive_global_rows` ??
   - `non_current_rows` ??
   - ??????
   - ???????

#### 18.6.6 ????????

??????????

1. ?? `ResidentFilamentMappingAdapter.cpp`
   - ?? `inactive_global_rows`
   - ?? `non_current_rows`
   - ????????
2. ?? `ResidentFilamentMappingView.hpp / .cpp`
   - ????????????????????????`
   - ?????? `non_current_rows`
3. ??? `ImGuiFilamentPanel.cpp`
   - ?????????????????? 3 ? rows ????

#### 18.6.7 ??????

?????????????????????????

1. `current_plate_rows` = ?????????
2. `inactive_global_rows` = ?????????????
3. `non_current_rows` = ?????????
4. ???????? `??????`


