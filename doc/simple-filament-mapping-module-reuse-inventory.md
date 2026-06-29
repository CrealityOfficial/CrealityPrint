# simple 鐩綍鑰楁潗鏄犲皠妯″潡澶嶇敤娓呭崟

## 1. 鏂囨。鐩殑

鏈枃閽堝 `src/slic3r/GUI/simple` 鐩綍涓師鏈负 ImGui 甯搁┗鑰楁潗鏄犲皠鐣岄潰鍑嗗鐨勪竴缁勬ā鍧楋紝鍥炵瓟涓変釜闂锛?
1. 鍝簺鏂囦欢鍙互鐩存帴澶嶇敤鍒?AI 鐗?Vue 鍙戦€佸崱鐗?2. 鍝簺鏂囦欢閫傚悎鏀归€犲悗澶嶇敤
3. 鍝簺鏂囦欢鍙€傚悎浣滀负鍙傝€冿紝涓嶅缓璁繘鍏?AI 鍙戦€佷富閾捐矾

鍚屾椂锛屾湰鏂囬『甯︾粰鍑轰竴鐗?`AISendWorkflowService` 鐨勬帴鍙ｈ崏妗堬紝浣滀负鍚庣画 C++ 宸ヤ綔娴佹娊璞＄殑钀藉湴鏂瑰悜銆?
## 2. 鎬讳綋缁撹

杩欐壒鏂囦欢骞朵笉鍙槸 ImGui UI 浠ｇ爜锛岄噷闈㈢‘瀹炴湁涓€鎵瑰緢鏈変环鍊肩殑绠楁硶涓庨鍩熸ā鍨嬨€?
鏈€鏍稿績鐨勫垽鏂槸锛?
- `match_color.*` 鍜?`ThumbnailDataRecolor.*` 鏄渶鍊煎緱鐩存帴澶嶇敤鐨勭畻娉曟牳蹇?- `ResidentFilamentMappingLogic.*`銆乣ResidentFilamentMappingPopupPolicy.*`銆乣ResidentFilamentMappingAdapter.*` 閫傚悎鎶芥帀 ImGui 渚濊禆鍚庣户缁鐢?- `ResidentFilamentMappingView.*`銆乣ImGuiThumbnailPreview.*`銆乣ImGuiSpoolWidget.*` 涓昏鏄?ImGui/GL 娓叉煋灞傦紝涓嶉€傚悎鐩存帴杩涘叆 Vue 鍗＄墖鏂规

涓€鍙ヨ瘽姒傛嫭锛?
`绠楁硶鍜岄鍩熺姸鎬佸彲浠ュ鐢紝ImGui 瑙嗗浘澹冲瓙涓嶈澶嶇敤銆俙

## 3. 妯″潡鍒嗙骇娓呭崟

## 3.1 鐩存帴澶嶇敤

### A. `match_color.hpp` / `match_color.cpp`

鎺ㄨ崘绛夌骇锛歚鐩存帴澶嶇敤`

鍘熷洜锛?
- 杩欑粍鏂囦欢鏄函 C++ 棰滆壊鍖归厤绠楁硶锛屽熀鏈笉渚濊禆 ImGui UI 灞?- 鏁版嵁杈撳叆杈撳嚭娓呮櫚
- 宸茬粡瀹炵幇浜嗘ā鍨嬮鑹蹭笌璁惧鑰楁潗棰滆壊鐨勮嚜鍔ㄥ尮閰?- 宸茬粡鍐呯疆棰滆壊璺濈璁＄畻涓庡尮閰嶄紭鍏堢骇瑙勫垯

鏍稿績鑳藉姏锛?
- 璁惧鑰楁潗鏁版嵁缁撴瀯锛歚DeviceBoxColorInfo`
- 妯″瀷棰滆壊鏁版嵁缁撴瀯锛歚ModelColor`
- 鍖归厤缁撴灉鏁版嵁缁撴瀯锛歚MatchResult`
- 鏍稿績绠楁硶锛歚getColorMatchInfo(...)`

鍏抽敭浣嶇疆锛?
- [match_color.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.hpp:8)
- [match_color.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.hpp:27)
- [match_color.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.hpp:34)
- [match_color.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.hpp:53)
- [match_color.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.cpp:54)
- [match_color.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.cpp:131)
- [match_color.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/match_color.cpp:190)

AI 鍗＄墖涓殑寤鸿鐢ㄩ€旓細

- 鑷姩鑰楁潗鏄犲皠
- 鏄犲皠缁撴灉鎽樿
- 鏄犲皠澶辫触椤硅瘑鍒?- 鍚庣画鈥滃彲閫夋Ы浣嶆帹鑽愨€濈殑鍩虹璇勫垎鏉ユ簮

### B. `ThumbnailDataRecolor.hpp` / `ThumbnailDataRecolor.cpp`

鎺ㄨ崘绛夌骇锛歚鐩存帴澶嶇敤`

鍘熷洜锛?
- 杩欐槸绾浘鍍忛噸鐫€鑹茬畻娉曪紝涓嶄緷璧?ImGui
- 杈撳叆鏄?`ThumbnailData` 鍜屾尋鍑烘満棰滆壊
- 杈撳嚭浠嶆槸 `ThumbnailData`
- 涓?AI 鍗＄墖鈥滈瑙堝浘闅忔槧灏勫彉鍖栬€屽埛鏂扳€濈殑璇夋眰楂樺害涓€鑷?
鏍稿績鑳藉姏锛?
- `RGB8`
- `ThumbnailRecolorParams`
- `recolor_thumbnail_with_no_light(...)`

鍏抽敭浣嶇疆锛?
- [ThumbnailDataRecolor.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ThumbnailDataRecolor.hpp:11)
- [ThumbnailDataRecolor.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ThumbnailDataRecolor.hpp:17)
- [ThumbnailDataRecolor.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ThumbnailDataRecolor.hpp:26)
- [ThumbnailDataRecolor.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ThumbnailDataRecolor.cpp:26)

AI 鍗＄墖涓殑寤鸿鐢ㄩ€旓細

- 鐢熸垚鈥滃綋鍓嶆槧灏勭粨鏋滀笅鈥濈殑鍗曠洏棰勮鍥?- 鐢ㄦ埛鍒囨崲鏄犲皠鍊欓€夊悗鍗虫椂鍒锋柊棰勮鍥?
## 3.2 闇€鏀归€犲鐢?
### A. `ResidentFilamentMappingLogic.hpp` / `ResidentFilamentMappingLogic.cpp`

鎺ㄨ崘绛夌骇锛歚闇€鏀归€犲鐢╜

鍘熷洜锛?
- 杩欑粍浠ｇ爜鏈川涓婂凡缁忓湪鍋氣€滄槧灏勯潰鏉跨殑鐘舵€佸缓妯♀€?- 鍏朵腑鐨?`RuntimeSignals`銆乣RowInput`銆乣UiModel` 寰堟帴杩戜竴灞傞鍩?ViewModel
- 浣嗙幇鍦ㄤ粛娣峰叆浜嗭細
  - `ImVec4`
  - 闈㈠悜 ImGui 鐨勬枃妗堜笌灞曠ず璇箟
  - 闈㈠悜甯搁┗闈㈡澘鐨?UI mode 瀹氫箟

鏍稿績鑳藉姏锛?
- UI 妯″紡鍒ゅ畾锛歚resolve_ui_mode(...)`
- 鐘舵€佹ā鍨嬬敓鎴愶細`build_ui_model(...)`

鍏抽敭浣嶇疆锛?
- [ResidentFilamentMappingLogic.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp:30)
- [ResidentFilamentMappingLogic.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp:45)
- [ResidentFilamentMappingLogic.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp:64)
- [ResidentFilamentMappingLogic.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp:82)
- [ResidentFilamentMappingLogic.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.cpp:9)
- [ResidentFilamentMappingLogic.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingLogic.cpp:55)

寤鸿鏀归€犳柟鍚戯細

- 鎶?`ImVec4` 鏀规垚閫氱敤棰滆壊缁撴瀯鎴栧崄鍏繘鍒跺瓧绗︿覆
- 鎶?`UiMode` 鏀归€犳垚 AI 鍗＄墖鍙悊瑙ｇ殑鐘舵€佽涔?- 鎶婁腑鏂囨枃妗堜粠閫昏緫灞傚墺绂伙紝鍙樻垚鐘舵€佺爜鎴?message key

鏀归€犲悗鏇撮€傚悎鐨勫懡鍚嶅彲浠ユ槸锛?
- `FilamentMappingStateBuilder`
- `AISendMappingViewModelBuilder`

### B. `ResidentFilamentMappingPopupPolicy.hpp` / `ResidentFilamentMappingPopupPolicy.cpp`

鎺ㄨ崘绛夌骇锛歚闇€鏀归€犲鐢╜

鍘熷洜锛?
- 杩欑粍浠ｇ爜鐨勭湡姝ｄ环鍊间笉鍦?popup锛岃€屽湪鈥滃€欓€夐」鍖归厤绛栫暐鈥?- 瀹冨畾涔変簡锛?  - `StrictType`
  - `LooseType`
  - `AvailabilityOnly`
- 闈炲父閫傚悎 AI 鍗＄墖閲屸€滅粰鏌愪釜鎸ゅ嚭鏈哄睍绀哄摢浜涘彲閫夎€楁潗妲戒綅鈥?
闂鍦ㄤ簬锛?
- 褰撳墠鍑芥暟绛惧悕渚濊禆 `ImGuiFilamentItemState`
- 杩斿洖鍊间緷璧?`ResidentFilamentMappingView::PopupGroupViewModel`

鎵€浠ュ畠鐩墠杩樻槸鈥滅瓥鐣?+ ImGui ViewModel 鏋勯€犫€濈殑娣峰悎浣撱€?
鍏抽敭浣嶇疆锛?
- [ResidentFilamentMappingPopupPolicy.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp:14)
- [ResidentFilamentMappingPopupPolicy.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp:26)
- [ResidentFilamentMappingPopupPolicy.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp:35)
- [ResidentFilamentMappingPopupPolicy.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp:40)
- [ResidentFilamentMappingPopupPolicy.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp:55)
- [ResidentFilamentMappingPopupPolicy.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp:96)
- [ResidentFilamentMappingPopupPolicy.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp:118)
- [ResidentFilamentMappingPopupPolicy.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp:132)

寤鸿鏀归€犳柟鍚戯細

- 鎻愬彇鍑虹函绛栫暐鍑芥暟灞?- 杈撳叆鏀逛负 AI 鑷繁鐨?`SceneItem` / `MappingOption`
- 杈撳嚭鏀逛负閫氱敤鍊欓€夐」鍒楄〃锛岃€屼笉鏄?ImGui popup 鍒嗙粍妯″瀷

鏀归€犲悗寤鸿鎷嗘垚涓ゅ眰锛?
- `FilamentMappingMatchPolicy`
- `FilamentMappingOptionBuilder`

### C. `ResidentFilamentMappingAdapter.hpp` / `ResidentFilamentMappingAdapter.cpp`

鎺ㄨ崘绛夌骇锛歚闇€鏀归€犲鐢╜

鍘熷洜锛?
- 杩欎竴灞傞噷鏈変笉灏戝緢瀹炵敤鐨勬暟鎹閰嶉€昏緫
- 浣嗗畠鍚屾椂鎶婏細
  - 璁惧鏁版嵁鎻愬彇
  - 鍦烘櫙鏉＄洰鎻愬彇
  - popup 鍊欓€夐」鏋勯€?  - ImGui 闈㈡澘 ViewData 鏋勯€?  - 浜や簰閫夋嫨鍥炲啓

鍏ㄩ儴鎻夊湪涓€璧蜂簡

鍏朵腑鍊煎緱淇濈暀鐨勫嚱鏁帮細

- `device_has_available_materials(...)`
- `has_cached_mapping_result(...)`
- `collect_unified_output_input(...)`
- `collect_row_inputs_from_items(...)`
- `build_popup_option_catalog(...)`
- `apply_popup_selection(...)`
- `find_popup_selection_payload(...)`

鍏抽敭浣嶇疆锛?
- [ResidentFilamentMappingAdapter.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp:45)
- [ResidentFilamentMappingAdapter.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp:49)
- [ResidentFilamentMappingAdapter.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp:55)
- [ResidentFilamentMappingAdapter.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp:76)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:274)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:291)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:298)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:356)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:377)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:495)
- [ResidentFilamentMappingAdapter.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp:507)

涓嶅缓璁洿鎺ュ鐢ㄧ殑鍑芥暟锛?
- `build_panel_view_data(...)`

鍥犱负瀹冪殑杈撳嚭鏄?`PanelViewData`锛屽凡缁忔槑鏄炬槸 ImGui 涓撶敤瑙嗗浘妯″瀷銆?
寤鸿鏀归€犳柟鍚戯細

- 淇濈暀鈥滄暟鎹噰闆嗏€濆拰鈥滈€夋嫨搴旂敤鈥濈殑閮ㄥ垎
- 鍒犳帀瀵?`PanelViewData` 鐨勪緷璧?- 鍗曠嫭鎶芥垚锛?  - `MappingSourceCollector`
  - `MappingOptionCatalogBuilder`
  - `MappingSelectionApplier`

### D. `ImGuiFilamentPanel.hpp` 涓殑杞婚噺鐘舵€佹ā鍨?
鎺ㄨ崘绛夌骇锛歚闇€鏀归€犲鐢╜

鍘熷洜锛?
- `ImGuiFilamentItemState` 鏄竴涓瘮杈冭交鐨勪腑闂存€侊紝瀛楁涓嶅
- 瀵规垜浠瘑鍒€滃綋鍓嶅満鏅殑鎸ゅ嚭鏈烘潯鐩姸鎬佲€濆緢鏈夊弬鑰冧环鍊?
鍏抽敭浣嶇疆锛?
- [ImGuiFilamentPanel.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp:23)

涓嶈繃瀹冪殑闂涔熷緢鏄庢樉锛?
- 鐢ㄤ簡 `ImVec4`
- 鍛藉悕涓婂甫鏈?ImGui 鐥曡抗
- 渚濈劧鏄潰鏉挎€侊紝涓嶆槸 AI 鍗＄墖棰嗗煙鎬?
寤鸿鍋氭硶锛?
- 涓嶇洿鎺ュ鐢ㄧ被鍨嬪悕
- 鍙傝€冨畠鐨勫瓧娈碉紝鏀归€犲嚭 AI 涓撶敤鐨?`AISendSceneItem`

## 3.3 浠呭弬鑰?
### A. `ResidentFilamentMappingView.hpp` / `ResidentFilamentMappingView.cpp`

鎺ㄨ崘绛夌骇锛歚浠呭弬鑰僠

鍘熷洜锛?
- 杩欐槸鍏稿瀷 ImGui View 灞?- `PanelViewData`銆乣PopupOptionViewModel`銆乣render_panel(...)` 閮芥槸涓轰簡 ImGui 鐢婚潰鏈嶅姟
- 瀵?Vue 鍗＄墖娌℃湁鐩存帴澶嶇敤浠峰€?
鍏抽敭浣嶇疆锛?
- [ResidentFilamentMappingView.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.hpp:42)
- [ResidentFilamentMappingView.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ResidentFilamentMappingView.hpp:88)

鍙弬鑰冨唴瀹癸細

- 淇℃伅灞傜骇缁勭粐鏂瑰紡
- 褰撳墠鐩?/ 鍏朵粬鐩樺垎缁勭殑灞曠ず鎬濊矾
- 鏄犲皠鐘舵€佹爣绛捐涔?
### B. `ImGuiThumbnailPreview.hpp` / `ImGuiThumbnailPreview.cpp`

鎺ㄨ崘绛夌骇锛歚浠呭弬鑰僠

鍘熷洜锛?
- 杩欏眰涓昏鏄?OpenGL 绾圭悊绠＄悊銆丗BO銆乻hader銆佺汗鐞嗙紦瀛?- 鏄?ImGui 鍦烘櫙涓嬩负浜嗛珮鏁堟樉绀洪噸鐫€鑹查瑙堝浘鑰屽仛鐨勬覆鏌撲紭鍖栧眰
- Vue 鍗＄墖鍙渶瑕佺粨鏋滃浘锛屼笉闇€瑕佽繖灞?GPU 棰勮鎺т欢

鍏抽敭浣嶇疆锛?
- [ImGuiThumbnailPreview.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiThumbnailPreview.hpp:13)
- [ImGuiThumbnailPreview.hpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiThumbnailPreview.hpp:26)
- [ImGuiThumbnailPreview.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiThumbnailPreview.cpp:227)
- [ImGuiThumbnailPreview.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiThumbnailPreview.cpp:476)

鍙弬鑰冨唴瀹癸細

- 缂撳瓨绛惧悕璁捐
- 浣曟椂閲嶅缓棰勮绾圭悊鐨勫垽瀹氭€濊矾

### C. `ImGuiSpoolWidget.cpp`

鎺ㄨ崘绛夌骇锛歚浠呭弬鑰僠

鍘熷洜锛?
- 杩欐槸绾瑙夋帶浠?- 涓嶅睘浜?AI 鍙戦€佸伐浣滄祦鐨勭畻娉曟牳蹇?
鍏抽敭浣嶇疆锛?
- [ImGuiSpoolWidget.cpp](/c:/WORK/C3DSlicer/src/slic3r/GUI/simple/filamentMapping/ImGuiSpoolWidget.cpp:118)

鍙弬鑰冨唴瀹癸細

- 鑰楁潗瑙嗚琛ㄨ揪鏂瑰紡

## 4. 鎺ㄨ崘鐨勫鐢ㄨ惤鐐?
## 4.1 绗竴浼樺厛绾?
寤鸿浼樺厛绾冲叆 AI 鍙戦€佸伐浣滄祦鐨勬ā鍧楋細

- `match_color.*`
- `ThumbnailDataRecolor.*`

杩欎袱缁勫熀鏈凡缁忔槸鎴愮啛绠楁硶妯″潡锛屼笖涓?Vue/UI 鎶€鏈爤鏃犲叧銆?
## 4.2 绗簩浼樺厛绾?
寤鸿鍦ㄥ伐浣滄祦灞傝惤绋冲悗锛屽啀寮€濮嬫娊锛?
- `ResidentFilamentMappingLogic.*`
- `ResidentFilamentMappingPopupPolicy.*`
- `ResidentFilamentMappingAdapter.*`

杩欎笁缁勬洿閫傚悎浣滀负鈥淎I 鏄犲皠棰嗗煙灞傗€濈殑鏉ユ簮浠ｇ爜銆?
## 4.3 涓嶅缓璁湰鏈熸姇鍏?
鏈湡涓嶅缓璁妸绮惧姏鏀惧湪锛?
- `ResidentFilamentMappingView.*`
- `ImGuiThumbnailPreview.*`
- `ImGuiSpoolWidget.*`

鍥犱负杩欎簺涓嶄細鐩存帴鍑忓皯 AI 妯″紡鐨勪笟鍔￠闄╋紝鍙細鎶?UI 鎶€鏈€哄紩杩涙潵銆?
## 5. 闈㈠悜 AI 鍙戦€佸崱鐗囩殑閲嶆瀯寤鸿

寤鸿鍦?`simple` 鑳藉姏鍩虹涓婏紝鏂板涓€灞傚拰 UI 鏃犲叧鐨勯鍩熸ā鍧楋紝渚嬪锛?
- `AISendFilamentMappingTypes`
- `AISendFilamentMatcher`
- `AISendMappingOptionPolicy`
- `AISendThumbnailPreviewService`

鍏朵腑寤鸿鐨勫叧绯绘槸锛?
```mermaid
flowchart LR
    A[褰撳墠鐩樺満鏅暟鎹甝 --> B[AISendFilamentMatcher]
    C[褰撳墠璁惧鑰楁潗鏁版嵁] --> B
    B --> D[鑷姩鏄犲皠缁撴灉]
    C --> E[AISendMappingOptionPolicy]
    D --> E
    E --> F[鍊欓€夐」鍒楄〃]
    D --> G[AISendThumbnailPreviewService]
    G --> H[閲嶇潃鑹查瑙堝浘]
```

## 6. `AISendWorkflowService` 鎺ュ彛鑽夋

## 6.1 璁捐鐩爣

`AISendWorkflowService` 鐨勭洰鏍囦笉鏄仛 UI锛岃€屾槸浣滀负 AI 鍗＄墖鑳屽悗鐨勫伐浣滄祦涓彴锛岀粺涓€绠＄悊锛?
- 褰撳墠鐩?- 褰撳墠璁惧
- 褰撳墠鏄犲皠缁撴灉
- 褰撳墠棰勮鍥?- 褰撳墠鍙戦€佺姸鎬?
## 6.2 寤鸿鐨勮緭鍏ヨ緭鍑烘ā鍨?
### 鍦烘櫙鑰楁潗椤?
```cpp
struct AISendSceneItem {
    int         extruder_id = 0;
    std::string scene_color_hex;
    std::string filament_type;
    double      filament_length = 0.0;
    std::string label;
};
```

### 璁惧鑰楁潗椤?
```cpp
struct AISendDeviceMaterial {
    int         box_type = 0;
    int         box_id = -1;
    int         material_id = 0;
    std::string color_hex;
    std::string filament_type;
    std::string filament_name;
    int         c_id = -1;
    int         rfid_state = 1;
    int         percent = 100;
    double      remaining_length = 0.0;
};
```

### 鏄犲皠缁撴灉椤?
```cpp
struct AISendMappingItem {
    int         extruder_id = 0;
    std::string scene_color_hex;
    std::string filament_type;
    std::string mapped_slot_label;
    std::string mapped_color_hex;
    int         box_id = -1;
    int         material_id = 0;
    int         c_id = -1;
    bool        matched = false;
    std::string reason;
};
```

### 鍊欓€夐」

```cpp
struct AISendMappingOption {
    std::string selection_token;
    std::string slot_label;
    std::string material_label;
    std::string material_type;
    std::string color_hex;
    bool        available = false;
    bool        disabled = false;
};
```

### 蹇収

```cpp
struct AISendCardSnapshot {
    std::string                    card_id;
    int                            selected_plate_index = -1;
    std::vector<int>               available_plate_indices;
    std::vector<AISendMappingItem> mapping_items;
    std::string                    preview_image_base64;
    bool                           can_start_print = false;
    bool                           can_send_only = false;
    std::string                    status;
    std::string                    status_text;
};
```

## 6.3 瀵瑰鎺ュ彛寤鸿

```cpp
class AISendWorkflowService {
public:
    std::string open_send_card_for_current_context();

    AISendCardSnapshot get_snapshot(const std::string& card_id) const;

    AISendCardSnapshot select_plate(const std::string& card_id, int plate_index);

    std::vector<AISendMappingOption> get_mapping_options(
        const std::string& card_id,
        int extruder_id) const;

    AISendCardSnapshot apply_mapping_selection(
        const std::string& card_id,
        int extruder_id,
        const std::string& selection_token);

    AISendCardSnapshot auto_match(const std::string& card_id);

    void start_send_only(const std::string& card_id);

    void start_send_and_print(const std::string& card_id);

    void cancel(const std::string& card_id);
};
```

## 6.4 鍐呴儴渚濊禆寤鸿

### A. 鑷姩鏄犲皠

鍐呴儴璋冪敤椤哄簭寤鸿涓猴細

1. 浠庡綋鍓嶇洏鏋勫缓 `AISendSceneItem`
2. 浠庡綋鍓嶈澶囨瀯寤?`AISendDeviceMaterial`
3. 杞崲涓?`ColorMatch::Device` 涓?`ColorMatch::ModelColor`
4. 璋?`ColorMatch::getColorMatchInfo(...)`
5. 褰掍竴鍖栨垚 `AISendMappingItem`

瀵瑰簲澶嶇敤鏉ユ簮锛?
- `match_color.*`

### B. 鍊欓€夐」鏋勫缓

鍐呴儴璋冪敤椤哄簭寤鸿涓猴細

1. 浠庡綋鍓嶈澶囨潗鏂欏垪琛ㄦ瀯寤?option seeds
2. 鏍规嵁璁惧鍦ㄧ嚎鐘舵€佸拰璁惧妯″紡鍐冲畾绛栫暐
3. 瀵规寚瀹?`extruder_id` 杩囨护鍊欓€夐」
4. 杈撳嚭 `AISendMappingOption`

瀵瑰簲澶嶇敤鏉ユ簮锛?
- `ResidentFilamentMappingPopupPolicy.*`
- `ResidentFilamentMappingAdapter.*` 涓€欓€夐」鐩綍鏋勫缓鎬濊矾

### C. 搴旂敤鏄犲皠

鍐呴儴璋冪敤椤哄簭寤鸿涓猴細

1. 鏍规嵁 `selection_token` 鎵惧埌鐢ㄦ埛閫変腑鐨勫€欓€夐」
2. 鏇存柊鏌愪釜 `AISendMappingItem`
3. 閲嶅缓鏁村紶鍗＄墖蹇収
4. 瑙﹀彂棰勮鍥鹃噸鐫€鑹?
瀵瑰簲澶嶇敤鏉ユ簮锛?
- `ResidentFilamentMappingAdapter::apply_popup_selection(...)`

### D. 棰勮鍥鹃噸寤?
鍐呴儴璋冪敤椤哄簭寤鸿涓猴細

1. 鍙栧綋鍓嶇洏 `lit_thumbnail`
2. 鍙栧綋鍓嶇洏 `no_light_thumbnail`
3. 鏍规嵁褰撳墠鏄犲皠缁撴灉鐢熸垚 extruder colors
4. 璋?`recolor_thumbnail_with_no_light(...)`
5. 杞垚 base64 鍥惧儚瀛楃涓插啓鍥炲揩鐓?
瀵瑰簲澶嶇敤鏉ユ簮锛?
- `ThumbnailDataRecolor.*`

## 6.5 涓庡彂閫佸伐浣滄祦鐨勮鎺ョ偣

寤鸿 `AISendWorkflowService` 涓嶇洿鎺ラ噸鍐欏彂閫侀€昏緫锛岃€屾槸鎶婃槧灏勭粨鏋滄暣鐞嗘垚涓撲笟妯″紡搴曞眰鍙帴鍙楃殑缁撴瀯锛屽啀缁х画璋冪敤鐜版湁鍙戦€佽矾寰勶細

- 鍗曠洏 GCode 涓婁紶
- 寮€濮嬫墦鍗?- 鍙栨秷鍙戦€?
寤鸿鏂板涓や釜鍐呴儴妗ユ帴鍑芥暟锛?
```cpp
void dispatch_send_gcode(const std::string& card_id);
void dispatch_start_print(const std::string& card_id);
```

鍏朵腑锛?
- `dispatch_send_gcode(...)` 澶嶇敤褰撳墠鎴愮啛鐨?`send_gcode` 閾捐矾
- `dispatch_start_print(...)` 澶嶇敤 `send_start_print_cmd` 閾捐矾

## 7. 鎺ㄨ崘鐨勫疄鏂介『搴?
寤鸿鎸変笅闈㈤『搴忚惤鍦帮細

1. 鍏堟妸 `match_color.*` 鎺ュ叆 AI 鍙戦€佸伐浣滄祦
2. 鍐嶆妸 `ThumbnailDataRecolor.*` 鎺ュ叆棰勮鍥惧埛鏂?3. 鍐嶆娊 `PopupPolicy` 鎴?AI 鐗堝€欓€夐」瑙勫垯
4. 鏈€鍚庡啀鑰冭檻浠?`ResidentFilamentMappingLogic.*` 涓彁鐐肩姸鎬佹憳瑕佺敓鎴愬櫒

杩欐牱鍋氱殑濂藉鏄細

- 鍏堟嬁鍒拌嚜鍔ㄦ槧灏?- 鍐嶆嬁鍒伴瑙堣仈鍔?- 鍐嶆嬁鍒颁汉宸ヤ慨姝?- 鏈€鍚庡啀鍋氱姸鎬佽〃杈剧殑绮剧粏鍖?
## 8. 鏈€缁堝缓璁?
濡傛灉浠庘€滄€т环姣斺€濊搴︾湅锛岃繖鎵?`simple` 鐩綍浠ｇ爜鏈€鍊煎緱绔嬪嵆绾冲叆 AI 鏂规鐨勶紝灏辨槸锛?
- `match_color.*`
- `ThumbnailDataRecolor.*`

濡傛灉浠庘€滀腑鏈熸紨杩涒€濊搴︾湅锛屾渶鍊煎緱缁х画鎶借薄鐨勶紝灏辨槸锛?
- `ResidentFilamentMappingPopupPolicy.*`
- `ResidentFilamentMappingLogic.*`
- `ResidentFilamentMappingAdapter.*`

鑰?ImGui 鐩稿叧瑙嗗浘鏂囦欢寤鸿鍋滅暀鍦ㄥ弬鑰冨眰锛屼笉瑕佹妸瀹冧滑鐩存帴甯﹁繘 Vue 鍗＄墖鏂规銆?
涓€鍙ヨ瘽鎬荤粨锛?
`鎶?simple 鐩綍閲岀殑鏄犲皠绠楁硶鍗囩骇鎴?AI 宸ヤ綔娴佽兘鍔涳紝涓嶈鎶?simple 鐩綍閲岀殑 ImGui 浜や簰褰㈡€佹惉杩涙潵銆俙

