# 鑰楁潗鏄犲皠閫昏緫姊崇悊

## 1. 鑼冨洿

鏈枃鍙垎鏋愬綋鍓嶅伐浣滃尯閲屽凡缁忚惤鍦扮殑鈥滆€楁潗鏄犲皠鈥濈浉鍏冲疄鐜帮紝閲嶇偣瑕嗙洊涓嬮潰鍑犳潯閾捐矾锛?

- 绠€鍗曟ā寮忛〉闈腑鐨?ImGui 鑰楁潗鏄犲皠闈㈡澘
- 棰滆壊/绫诲瀷鍖归厤绠楁硶
- 鎵撳嵃涓嬪彂鏃剁殑 `color_match_info` 缁勮
- AI Chat / `CxAgent` 瀵光€滄墦寮€鑰楁潗鏄犲皠鈥濈殑鎺ュ叆鐐?
- 鏃х増 wxWidgets 渚ц竟鏍忚€楁潗鏄犲皠鍏ュ彛

鏍稿績鏂囦欢锛?

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
- `src/slic3r/GUI/simple/filamentMapping/match_color.cpp`
- `src/slic3r/GUI/simple/filamentMapping/match_color.hpp`
- `src/slic3r/GUI/simple/filamentSimplePage.cpp`
- `src/slic3r/GUI/Plater.cpp`
- `src/slic3r/GUI/print_manage/data/DataType.hpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/simple/bridge/CxAgentClientBridge.cpp`

## 2. 鎬讳綋缁撴瀯

褰撳墠椤圭洰閲屽疄闄呬笂骞跺瓨涓ゅ鑰楁潗鏄犲皠瀹炵幇锛?

### 2.1 鏂板疄鐜帮細ImGui 绠€鍗曟ā寮忔槧灏?

鍏ュ彛鍦?`GLCanvas3D::_draw_filament_menu_contents()`锛屾枃浠讹細

- `src/slic3r/GUI/simple/filamentSimplePage.cpp`

杩欎竴濂楄礋璐ｏ細

- 鏍规嵁褰撳墠璁惧鍒ゆ柇鏄剧ず `CFS` 杩樻槸 `External Spool`
- 灏嗗満鏅噷鐨勬尋鍑烘満妲戒綅娓叉煋涓轰竴缁勫僵鑹插崱鐗?
- 鑷姩鏍规嵁璁惧鑰楁潗鍋氬尮閰?
- 鍏佽鐢ㄦ埛鎵嬪姩鐐瑰紑姣忎釜鍗＄墖鐨勬槧灏勫脊灞?
- 棰勮鍖归厤鍚庣殑缂╃暐鍥剧潃鑹?
- 鍦ㄦ墦鍗版椂瀵煎嚭 `color_match_info`

### 2.2 鏃у疄鐜帮細wx Sidebar / RemotePrint 鏄犲皠

鍏ュ彛涓昏鍦細

- `src/slic3r/GUI/Plater.cpp`
- `src/slic3r/GUI/print_manage/PrinterBoxFilamentPanel.cpp`
- `src/slic3r/GUI/print_manage/MaterialMapPanel.cpp`

杩欎竴濂椾粛鐒跺湪 sidebar / send-to-printer / cloud upload 绛夎矾寰勯噷浣跨敤銆?

### 2.3 褰撳墠鐜扮姸

绠€鍗曟ā寮?UI 宸茬粡涓昏渚濊禆 `ImGuiFilamentPanel`锛屼絾 AI Chat 鐨?`open_filament_mapping` 宸ュ叿浠嶇劧瑙﹀彂鐨勬槸鏃т簨浠讹細

- `MCPChatPanel.cpp` 涓彂 `EVT_ON_MAPPING_DEVICE_FILAMENT`
- `Plater.cpp` 涓?`Sidebar::on_mapping_device_filament()`
- 鏈€缁堣繘鏃х殑 `PrinterBoxFilamentPanel::on_auto_device_filament_mapping()`

涔熷氨鏄锛?

- 鐢ㄦ埛鍦ㄧ畝鍗曟ā寮忛噷鐪嬪埌鐨勬槸鏂?ImGui 闈㈡澘
- AI 瑙﹀彂鈥滄墦寮€鑰楁潗鏄犲皠鈥濇椂锛屼粛鐒舵洿鍋忓悜鏃ц矾寰?

杩欐槸褰撳墠瀹炵幇閲屾渶閲嶈鐨勨€滃叆鍙ｄ笉缁熶竴鈥濋棶棰樸€?

## 3. 璁惧鏁版嵁妯″瀷

璁惧鑰楁潗淇℃伅鏉ヨ嚜锛?

- `src/slic3r/GUI/print_manage/data/DataType.hpp`

鍏抽敭缁撴瀯锛?

### 3.1 `DM::Material`

瀛楁閲屽拰鏄犲皠鏈€鐩稿叧鐨勬槸锛?

- `material_id`
- `type`
- `name`
- `color`
- `percent`
- `state`
- `selected`

璇箟涓婏細

- `type` 鏇村儚鏉愭枡绫诲埆锛屼緥濡?`PLA / PETG`
- `name` 鏇村儚璁惧閲屾樉绀虹殑鑰楁潗鍚嶏紝渚嬪 `Hyper PLA`
- `color` 鏄?`#RRGGBB`
- `selected` 鐢ㄤ簬澶栫疆鏂欐灦妯″紡涓嬪喅瀹氣€滃綋鍓嶉€変腑鐨勫缃€楁潗鈥?

### 3.2 `DM::MaterialBox`

瀛楁閲屾渶鍏抽敭鐨勬槸锛?

- `box_id`
- `box_type`
- `materials`

褰撳墠浠ｇ爜閲屽父瑙佸惈涔夛細

- `box_type == 0`锛欳FS
- `box_type == 1`锛欵xternal Spool
- `box_type == 2`锛氶儴鍒嗛€昏緫閲屼綔涓?CFS-mini / EXT 鍏煎妲?

### 3.3 `DM::Device`

鏄犲皠閫昏緫涓昏璇诲彇锛?

- `valid`
- `materialBoxes`

ImGui 闈㈡澘涓嶄細鐩存帴渚濊禆 `boxColorInfos`锛岃€屾槸浠?`materialBoxes` 缁勮鍊欓€夋潗鏂欍€?

## 4. ImGui 闈㈡澘鐨勬暟鎹姸鎬?

瀹氫箟鍦細

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`

鍗曚釜妲戒綅鐘舵€佺粨鏋勬槸 `ImGuiFilamentItemState`锛?

- `index`锛氭尋鍑烘満绱㈠紩
- `color`锛氬満鏅綋鍓嶆尋鍑烘満棰滆壊
- `match_color`锛氭槧灏勫埌璁惧鍚庢樉绀虹殑鍖归厤棰滆壊
- `preset_display`锛氬綋鍓嶈€楁潗 preset 鐨勬樉绀哄悕
- `type_label`锛氬綋鍓嶆Ы浣嶇殑鏉愭枡鏍囩
- `sync_label`锛氱晫闈㈡樉绀虹殑妲戒綅鏍囩锛屽 `1A` 鎴?`EXT`
- `device_match_slot`锛氬疄闄呮槧灏勫埌鐨勮澶囨Ы浣嶆枃鏈?

浠庤亴璐ｇ湅锛?

- `color` 琛ㄧず鈥滃満鏅娇鐢ㄤ粈涔堥鑹测€?
- `match_color` 琛ㄧず鈥滆澶囧皢鐢ㄤ粈涔堥鑹叉墦鍗扳€?
- `device_match_slot` 涓虹┖鏃讹紝琛ㄧず璇ユЫ浣嶈繕娌″拰璁惧鑰楁潗寤虹珛鏄犲皠

## 5. UI 鍏ュ彛涓庢ā寮忓垏鎹?

鍏ュ彛锛?

- `src/slic3r/GUI/simple/filamentSimplePage.cpp`

涓绘祦绋嬶細

1. 浠?`DM::DataCenter::Ins().get_current_device_data()` 鍙栧綋鍓嶈澶?
2. 璋?`ImGuiFilamentPanel::mode_availability_from_device()`
3. 鏍规嵁璁惧鑳藉姏鍐冲畾鏄惁鏄剧ず锛?
   - `Enable CFS`
   - `Use External Spool`
4. 鑻ョ敤鎴峰垏鎹㈡ā寮忥紝绔嬪嵆璋冪敤 `on_auto_mapping_filament_ex()`
5. 姣忓抚璋冪敤 `check_device_filament_auto_mapping()`锛屽綋璁惧鑰楁潗绛惧悕鍙樺寲鏃惰嚜鍔ㄩ噸鏄犲皠

### 5.1 妯″紡鍒ゅ畾瑙勫垯

鍦?`ImGuiFilamentPanel::mode_availability_from_device()` 涓細

- 鍙湁褰?`box_type == 0` 涓斿瓨鍦ㄥ甫棰滆壊鐨勮€楁潗鏃讹紝鎵嶆樉绀?`CFS`
- `External`锛?
  - 鑻ヨ澶囨槑纭笂鎶ヤ簡 `box_type == 1`锛屽垯瑕佹眰鑷冲皯鏈変竴涓甫棰滆壊鑰楁潗
  - 鑻ヨ澶囨病鏈夊缃枡鏋朵俊鎭紝鍒欎粛鍏佽浣滀负 fallback 鏄剧ず

### 5.2 褰撳墠瀹為檯浣跨敤鐨勮嚜鍔ㄦ槧灏勫嚱鏁?

鐜板湪绠€鍗曟ā寮忛〉闈富瑕佽蛋锛?

- `on_auto_mapping_filament_ex()`

鑰屼笉鏄細

- `on_auto_mapping_filament()`

涓よ€呭樊鍒緢澶э細

- `on_auto_mapping_filament()` 浼氭妸鍦烘櫙鑰楁潗妲戒綅鏁扮洿鎺ユ敼鎴愨€滆澶囧彲鐢ㄨ€楁潗鏁扳€?
- `on_auto_mapping_filament_ex()` 涓嶆敼妲戒綅鏁帮紝鍙湪鈥滃綋鍓嶅満鏅Ы浣嶁€濆熀纭€涓婂仛鏄犲皠

鍥犳锛?

- `on_auto_mapping_filament_ex()` 鏇撮€傚悎浣滀负褰撳墠 UI 鐨勯粯璁よ涓?
- `on_auto_mapping_filament()` 鏇村儚鏃?sidebar 鑷姩鍚屾閫昏緫

## 6. ImGui 鑷姩鏄犲皠涓婚摼璺?

鏍稿績鍑芥暟锛?

- `ImGuiFilamentPanel::on_auto_mapping_filament_ex()`
- `ImGuiFilamentPanel::remap_item_with_match_color()`

### 6.1 `on_auto_mapping_filament_ex()` 鍋氫簡浠€涔?

瀹冨厛浠庤澶囦腑杩囨护鍑哄綋鍓嶆ā寮忎笅鍙鐨勮€楁潗鍊欓€夛細

- `CFS` 妯″紡鍙嬁 `box_type == 0`
- `External` 妯″紡鍙嬁 `box_type == 1`
- 鍊欓€夊繀椤?`color` 闈炵┖

鐒跺悗瀹冧笉浼氱洿鎺ユ敼鍦烘櫙妲戒綅鏁伴噺锛岃€屾槸锛?

1. 璇诲彇褰撳墠 `preset_bundle->filament_presets`
2. 鎸夊満鏅凡鏈夋Ы浣嶆暟鍒濆鍖?`m_items`
3. 鐢?`project_config["filament_colour"]` 濉瘡涓Ы浣嶇殑褰撳墠棰滆壊
4. 浠庡綋鍓?filament preset 鍚嶅瓧鎺ㄤ竴涓?`type_label`
5. 鏈€鍚庯細
   - `External` 妯″紡璧?`remap_external_item()`
   - `CFS` 妯″紡璧?`remap_item_with_match_color(-1)`

### 6.2 `remap_item_with_match_color()` 杈撳叆

瀹冩妸褰撳墠璁惧涓庡綋鍓嶅満鏅浆鎹㈡垚 `match_color` 妯″潡鐨勬牸寮忥細

璁惧渚ц浆鎴?`ColorMatch::DeviceBoxColorInfo`锛?

- `boxType = box.box_type`
- `color = material.color`
- `filamentType = material.name`
- `materialId = material.material_id`
- `boxId = box.box_id`

鍦烘櫙渚ц浆鎴?`ColorMatch::ModelColor`锛?

- `extruderId = 妲戒綅绱㈠紩`
- `extruderColor = 褰撳墠 filament_colour`
- `filamentType = item.type_label`
- 鑻?`type_label` 涓虹┖锛屽垯 fallback 涓?`preset_display`

### 6.3 鍖归厤鎴愬姛鍚庣殑鍐欏洖

褰?`matchStatusCode == 0` 鏃讹紝浼氭洿鏂帮細

- `match_color`
- `device_match_slot`
- `sync_label`
- 鑻ヨ繑鍥炰簡 `matchFilamentType`锛岃繕浼氳鐩?`type_label`

妲戒綅鏍囩鐢熸垚瑙勫垯锛?

- `boxId > 0` 鏃讹紝鏍囩涓?`boxId + 瀛楁瘝`
- 瀛楁瘝鏉ヨ嚜 `materialId`
- 澶栫疆鏂欐灦鍏滃簳涓?`EXT`

## 7. `match_color` 绠楁硶璇存槑

瀹炵幇鏂囦欢锛?

- `src/slic3r/GUI/simple/filamentMapping/match_color.cpp`

### 7.1 棰滆壊璺濈

棰滆壊鍖归厤涓嶆槸绠€鍗?RGB 娆ф皬璺濈锛岃€屾槸锛?

1. sRGB -> XYZ
2. XYZ -> Lab
3. 鐢?`deltaE2000`

骞朵笖浣跨敤浜嗚嚜瀹氫箟鏉冮噸锛?

- `KL = 0.8`
- `KC = 0.6`
- `KH = 0.8`

杩欐剰鍛崇潃褰撳墠瀹炵幇鏇存帴杩戔€滄劅鐭ヨ壊宸€濊€屼笉鏄€滄暟鍊艰壊宸€濄€?

### 7.2 绫诲瀷鍖归厤浼樺厛绾?

`getMatchColor()` 鐨勭涓€閬撶瓫閫夐潪甯镐弗鏍硷細

- 鍙湁 `box.filamentType == filamentType` 鎵嶄細杩涘叆鑹插樊姣旇緝

涔熷氨鏄褰撳墠绠楁硶鏄細

- 鍏堝畬鍏ㄦ寜鏉愭枡绫诲瀷/鍚嶇О鍋氱簿纭尮閰?
- 鍐嶅湪鍚岀被鍨嬪€欓€変腑鎵鹃鑹茶窛绂绘渶灏忕殑

### 7.3 鍏ㄥ眬鍖归厤绛栫暐

`getColorMatchInfo()` 鐨勬祦绋嬫槸璐績鍖归厤锛?

1. 灏嗘ā鍨嬩晶 `plateColors` 鎸?`filamentType` 鍊掑簭鎺掑簭
2. 姣忚疆鎵弿鎵€鏈夊墿浣欐尋鍑烘満锛屾壘褰撳墠鏈€浼樺尮閰?
3. 涓€鏃﹀尮閰嶆垚鍔燂細
   - 浠庡緟鍖归厤鎸ゅ嚭鏈洪泦鍚堜腑鍒犳帀璇ユ尋鍑烘満
   - 浠庤澶囧€欓€変腑鍒犳帀璇ヨ澶囪€楁潗
4. 缁х画涓嬩竴杞?

澶栫疆鏂欐灦鏈変竴涓壒娈婂垎鏀細

- 鑻ヨ澶囦晶鍙湁涓€涓€欓€夛紝涓?`boxType` 鏄?`1` 鎴?`2`
- 鍒欒繖涓€欓€夊彲浠ヨ閲嶅澶嶇敤锛屼笉浼氫粠璁惧闆嗗悎鍒犻櫎

杩欐濂藉搴斺€滃缃枡鏋跺崟鏂欙紝澶氫釜鎸ゅ嚭鏈哄叡浜悓涓€澶栫疆鑰楁潗棰滆壊鈥濈殑鍦烘櫙銆?

## 8. 鎵嬪姩鏄犲皠浜や簰

鎵嬪姩鏄犲皠閫昏緫鍦細

- `ImGuiFilamentPanel::render_item()`

UI 琛屼负锛?

1. 姣忎釜妲戒綅涓婃柟鏄剧ず褰撳墠鍦烘櫙鑰楁潗鍗＄墖
2. 涓嬫柟鏄剧ず鏄犲皠鍗＄墖
3. 鐐逛笅鏂规槧灏勫崱鐗囷紝浼氬脊鍑?CFS 妲戒綅鍊欓€夊垪琛?
4. 姣忎釜鍊欓€夐」鏄剧ず锛?
   - 椤堕儴妲戒綅鍙凤紝濡?`1A`
   - 搴曢儴鏉愭枡绫诲瀷鎴栧崰浣嶇 `/`銆乣?`
   - 鑳屾櫙鑹蹭负璁惧鑰楁潗棰滆壊

### 8.1 鍏佽鏄剧ず鍝簺鍊欓€?

寮瑰眰閲屽彧鍙栵細

- `box_type == 0`
- `box_type == 2`

骞跺仛浜嗗幓閲嶏細

- `(box_id, material_id)` 鍞竴

### 8.2 鎵嬪姩鐐瑰嚮鍚庣湡姝ｅ啓鍥炰簡浠€涔?

鐐瑰嚮鍊欓€夐」鍚庯紝褰撳墠浠ｇ爜鍙洿鏂帮細

- `sync_label`
- `device_match_slot`
- `match_color`

娌℃湁鏇存柊锛?

- `color`
- filament preset
- `m_material_options` 涓褰曠殑璁惧妲戒綅鍏冩暟鎹?

杩欐剰鍛崇潃褰撳墠鎵嬪姩鏄犲皠鏇村儚锛?

- 鈥滅晫闈㈡樉绀哄眰鐨勬墜鍔ㄩ噸缁戝畾鈥?

鑰屼笉鏄細

- 鈥滃畬鏁寸殑鎸佷箙鍖栬澶囨Ы浣嶇粦瀹氣€?

## 9. External 妯″紡

External 妯″紡涓撻棬璧帮細

- `render_external_item()`
- `remap_external_item()`

瑙勫垯寰堢畝鍗曪細

- 鍙湅 `box_type == 1`
- 浼樺厛鍙?`selected == true` 鐨勫缃€楁潗
- 鑻ユ病鏈?selected锛屽垯鍙栫涓€涓甫棰滆壊鐨勮€楁潗
- 鎵€鏈夊満鏅Ы浣嶇粺涓€鏄犲皠鍒拌繖涓€涓缃€楁潗棰滆壊
- `device_match_slot` 鍜?`sync_label` 閮藉啓鎴?`EXT`

鎵€浠?External 妯″紡鏈川涓婁笉鏄€滀竴瀵逛竴鏄犲皠鈥濓紝鑰屾槸鈥滃叏灞€鍏变韩涓€涓缃枡鏋堕鑹测€濄€?

## 10. 澧炲垹鑰楁潗妲戒綅

鐩稿叧鍑芥暟锛?

- `render_add_button_and_palette()`
- `on_pick_and_add_filament_colour()`
- `remove_last_filament()`

### 10.1 鏂板

鏂板娴佺▼锛?

1. 閫氳繃璋冭壊鏉挎垨鑷畾涔夐鑹叉柊澧炰竴涓満鏅Ы浣?
2. `preset_bundle->set_num_filaments(filament_count)`
3. `plater()->on_filaments_change(filament_count)`
4. 棰滆壊鍐欏叆 `filament_colour`
5. 鐢ㄥ綋鍓嶉€夋嫨鐨勬潗鏂欏悕璧?`on_update_filament_type()`
6. 瀵规柊妲戒綅鎵ц涓€娆?`remap_item_with_match_color(last_item_idx)`

### 10.2 鍒犻櫎

鍒犻櫎娴佺▼锛?

1. `preset_bundle->set_num_filaments(filament_count - 1)`
2. `plater()->on_filaments_change()`
3. 鏇存柊 flush volumes
4. 浠?`m_items` 寮瑰嚭鏈€鍚庝竴涓?
5. 鍐?`refresh_items_from_config()`

## 11. 鏄犲皠缁撴灉濡備綍涓嬪彂鍒版墦鍗伴摼璺?

鍏抽敭浠ｇ爜锛?

- `Plater::priv::build_color_match_info()`
- `Plater::priv::build_print_data()`

### 11.1 瀵煎嚭鏍煎紡

`ImGuiFilamentPanel::export_color_match_info()` 瀵煎嚭鐨勬瘡椤瑰寘鍚細

- `boxId`
- `extruderId`
- `extruderFilamentType`
- `matchColor`
- `materialId`

鐒跺悗 `Plater::priv::build_print_data()` 浼氭妸瀹冨杩涳細

- `color_match_info`
- `open_cfs`
- `print_calibration`

杩欎唤鏁版嵁浼氱户缁敤浜庯細

- 鍙戦€佸埌鎵撳嵃鏈?
- 涓婁紶 G-code / 3MF
- 绠€鍗曟ā寮?`EasyPrintSender`

### 11.2 缂╃暐鍥鹃瑙?

`render_thumbnail_preview()` 浼氫紭鍏堢敤 `match_color` 閲嶇潃鑹茬缉鐣ュ浘銆?

鎵€浠ョ晫闈㈠彸渚ч瑙堝睍绀虹殑鏄細

- 濡傛灉宸插尮閰嶏細璁惧鍖归厤棰滆壊
- 濡傛灉鏈尮閰嶏細鍦烘櫙鍘熷棰滆壊

## 12. AI Chat / CxAgent 鐨勫叧绯?

### 12.1 Tool 鏆撮湶

鍦細

- `src/slic3r/GUI/simple/bridge/CxAgentClientBridge.cpp`

鏆撮湶浜嗭細

- `open_filament_mapping`

鎻忚堪鏄細

- 鎵撳紑鏈湴鑰楁潗鏄犲皠瀵硅瘽锛屽苟绛夊緟鍚庣画 context update

### 12.2 瀹為檯鎵ц

鍦細

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

鏀跺埌 `tool == "open_filament_mapping"` 鍚庯紝褰撳墠鍙槸锛?

1. `wxPostEvent(plater, EVT_ON_MAPPING_DEVICE_FILAMENT)`
2. 杩斿洖 `await_context_update = true`

涔熷氨鏄細

- 瀹冩病鏈夌洿鎺ラ┍鍔?`ImGuiFilamentPanel`
- 瀹冭蛋鐨勬槸鏃х殑 sidebar 鏄犲皠鍏ュ彛

## 13. 鏃х増 Sidebar / RemotePrint 瀹炵幇

杩欓儴鍒嗕富瑕佸湪锛?

- `Plater.cpp`
- `PrinterBoxFilamentPanel.cpp`
- `MaterialMapPanel.cpp`

鍏稿瀷鐗圭偣锛?

- 鏈?`PrinterBoxFilamentPanel`
- 鏈?`MaterialMapPanel`
- 鏈?`BoxColorSelectPopupData`
- 閫氳繃 wx 浜嬩欢鎵撳紑鏄犲皠寮瑰眰

杩欏閫昏緫浠嶇劧鏈嶅姟浜庯細

- 鏃т晶杈规爮鏉愭枡闈㈡澘
- 鍙戦€佹墦鍗?/ 涓婁紶鍒颁簯绔瓑 RemotePrint 鐩稿叧璺緞

鎵€浠ュ綋鍓嶄唬鐮佷笉鏄€滃畬鍏ㄨ縼绉诲埌 ImGui鈥濈姸鎬侊紝鑰屾槸鈥滃弻杞ㄥ埗鈥濄€?

## 14. 褰撳墠瀹炵幇涓殑鍏抽敭椋庨櫓

### 14.1 鎵嬪姩鏄犲皠娌℃湁瀹屾暣鎸佷箙鍖栤€滈€変腑鐨勮澶囨Ы浣嶁€?

鎵嬪姩鐐瑰嚮寮瑰眰鍊欓€夋椂锛孖mGui 闈㈡澘鍙敼浜嗭細

- `device_match_slot`
- `sync_label`
- `match_color`

浣?`export_color_match_info()` 杈撳嚭 `boxId/materialId` 鏃讹紝渚濊禆鐨勬槸 `m_material_options[i]`銆?

`m_material_options[i]` 鏉ヨ嚜鏈€杩戜竴娆¤澶囨壂鎻忛『搴忥紝涓嶆槸鎵嬪姩鐐瑰嚮鍚庣殑鐪熷疄閫夋嫨缁撴灉銆?

杩欐剰鍛崇潃锛?

- 鐣岄潰涓婄湅璧锋潵宸茬粡鏀规垚 `2B`
- 鏈€缁堝鍑虹殑 `boxId/materialId` 浠嶅彲鑳芥槸鑷姩鏄犲皠鏃剁殑鏃у€?

杩欐槸褰撳墠瀹炵幇閲屾渶鍊煎緱浼樺厛淇殑鐐广€?

### 14.2 鈥滅鐢ㄦ€佲€濆€欓€変粛鍙兘琚偣鍑婚€変腑

鍦ㄦ墜鍔ㄥ脊灞傞噷锛岀被鍨嬩笉鍖归厤鐨勫€欓€変細琚敾鎴愮鐢ㄦ牱寮忋€?

浣嗙偣鍑诲垽瀹氬疄闄呭彧妫€鏌ワ細

- `clk`
- `is_owner`
- `!no_color`

娌℃湁鎶?`type_mismatch` 绾冲叆鏈€缁堢偣鍑讳繚鎶ゃ€?

鎵€浠ュ綋鍓嶈涓哄彲鑳芥槸锛?

- UI 涓婄湅璧锋潵涓嶅彲閫?
- 瀹為檯鐐瑰嚮浠嶄細琚帴鍙?

### 14.3 鍖归厤绫诲瀷浣跨敤鐨勬槸 `material.name`锛屼笉鏄?`material.type`

鑷姩鍖归厤鏃惰澶囦晶鍐欏叆鐨勬槸锛?

- `info.filamentType = m.name`

鑰屼笉鏄細

- `m.type`

鍐嶅姞涓?`match_color.cpp` 浣跨敤鐨勬槸鈥滃瓧绗︿覆瀹屽叏鐩哥瓑鈥濆尮閰嶃€?

杩欎細瀵艰嚧鑷姩鍖归厤楂樺害渚濊禆鍛藉悕涓€鑷存€э紝渚嬪锛?

- 鍦烘櫙閲屾槸 `PLA`
- 璁惧閲屾槸 `Hyper PLA`

杩欑鎯呭喌涓嬪彲鑳芥牴鏈笉浼氳繘鍏ラ鑹叉瘮杈冮樁娈点€?

### 14.4 `open_filament_mapping` 鍏ュ彛浠嶆寚鍚戞棫閫昏緫

褰撳墠 AI 宸ュ叿璋冪敤鎵撳紑鐨勬槸锛?

- 鏃?sidebar 鏄犲皠鍏ュ彛

涓嶆槸锛?

- 褰撳墠绠€鍗曟ā寮忕敤鎴峰彲瑙佺殑 ImGui 鏄犲皠鐣岄潰

杩欎細閫犳垚锛?

- 鐢ㄦ埛鐪嬪埌鐨勭晫闈?
- AI 瑙﹀彂鐨勭晫闈?

涓嶆槸鍚屼竴濂楅€昏緫銆?

### 14.5 `on_auto_mapping_filament()` 涓?`on_auto_mapping_filament_ex()` 璇箟涓嶄竴鑷?

鍓嶈€呬細锛?

- 鐩存帴鎸夎澶囪€楁潗鏁伴噺閲嶅缓鍦烘櫙 filament 鏁伴噺

鍚庤€呬細锛?

- 淇濈暀褰撳墠鍦烘櫙 filament 鏁伴噺锛屼粎鍋氭槧灏?

濡傛灉涓ゅ鍏ュ彛娣风敤锛屽鏄撻€犳垚锛?

- 鍦烘櫙妲戒綅鏁拌閲嶇疆
- 鏄犲皠缁撴灉鍜岀敤鎴烽鏈熶笉涓€鑷?

## 15. 寤鸿鐨勫悗缁敹鏁涙柟鍚?

### 15.1 缁熶竴鍏ュ彛

寤鸿鏄庣‘鍙繚鐣欎竴涓€滆€楁潗鏄犲皠涓诲叆鍙ｂ€濓細

- 绠€鍗曟ā寮忎紭鍏堜互 `ImGuiFilamentPanel` 涓轰富
- AI Chat 鐨?`open_filament_mapping` 涔熷簲鏀瑰埌鍚屼竴鍏ュ彛

### 15.2 琛ュ叏鎵嬪姩鏄犲皠鐨勭湡瀹炲厓鏁版嵁

寤鸿鍦?`ImGuiFilamentItemState` 涓鍔犵湡瀹炵粦瀹氬瓧娈碉紝渚嬪锛?

- `matched_box_id`
- `matched_material_id`
- `matched_box_type`

杩欐牱 `export_color_match_info()` 灏变笉蹇呬緷璧?`m_material_options[i]` 鐨勯『搴忓亣璁俱€?

### 15.3 灏嗙被鍨嬪尮閰嶄粠鈥滅簿纭瓧绗︿覆鐩哥瓑鈥濇敼鎴愨€滄爣鍑嗗寲鍖归厤鈥?

寤鸿鍦ㄥ尮閰嶅墠缁熶竴褰掍竴鍖栵細

- 浼樺厛鐢?`material.type`
- 鍐?fallback 鍒?`material.name`
- 鍋?alias / 鍒嗙被鏄犲皠

### 15.4 鏄庣‘鑷姩鏄犲皠绛栫暐

寤鸿鏄庣‘锛?

- 褰撳墠 UI 姝ｅ紡浣跨敤 `on_auto_mapping_filament_ex()`
- `on_auto_mapping_filament()` 浠呬繚鐣欑粰鏃?sidebar锛屾垨閫愭鍒犻櫎

## 16. 涓€鍙ヨ瘽鎬荤粨

褰撳墠鑰楁潗鏄犲皠閾捐矾宸茬粡鍏峰瀹屾暣鐨勶細

- 璁惧鑰楁潗璇诲彇
- 鑷姩鍖归厤
- 鎵嬪姩鏀规槧灏?
- 缂╃暐鍥鹃瑙?
- 鎵撳嵃涓嬪彂

浣嗛」鐩噷浠嶅浜庘€淚mGui 鏂板疄鐜?+ wx 鏃у疄鐜板苟瀛樷€濈殑杩囨浮闃舵銆? 
鐜伴樁娈垫渶鏍稿績鐨勯棶棰樹笉鏄畻娉曟湰韬紝鑰屾槸锛?

- 鍏ュ彛涓嶇粺涓€
- 鎵嬪姩鏄犲皠娌℃湁鎶婄湡瀹炶澶囨Ы浣嶅厓鏁版嵁瀹屾暣钀藉埌瀵煎嚭缁撴灉閲?

## 17. 褰撳墠 resident mapping 浠ｇ爜鍒嗗眰鏇存柊

闄ゅ師鏈?`ImGuiFilamentPanel` 涔嬪锛屽綋鍓?simple mode 鐨?resident mapping 鐩稿叧瀹炵幇宸茬粡寮€濮嬫媶鍒嗕负鍑犲眰鐙珛鏂囦欢锛?

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

### 17.1 褰撳墠鑱岃矗鍒掑垎

`ResidentFilamentMappingLogic`

1. 璐熻矗 `RuntimeSignals -> UiMode -> UiModel`
2. 璐熻矗鎶婅繍琛屾椂璁惧鐘舵€佹槧灏勪负锛?
   - `MultiColorOnline`
   - `SingleColorDevice`
   - `MultiColorOffline`

`ResidentFilamentMappingAdapter`

1. 璐熻矗锛?
   - `collect_unified_output_input(...)`
   - `collect_row_inputs_from_items(...)`
   - `build_popup_option_catalog(...)`
   - `build_panel_view_data(...)`
   - `apply_popup_selection(...)`
2. 鏈川涓婃壙鎷呪€滆澶囨暟鎹?/ item states / view data鈥濅箣闂寸殑閫傞厤

`ResidentFilamentMappingView`

1. 璐熻矗 panel 鐨勭函缁樺埗
2. 涓嶇洿鎺ヨ鍙栬澶囧師濮嬫暟鎹?
3. 涓嶅湪缁樺埗鍑芥暟閲屼复鏃舵嫾瑁?popup 鍊欓€夎鍒?

`ResidentFilamentMappingPopupPolicy`

1. 璐熻矗 popup 鍊欓€夌殑绛涢€夈€佸垎缁勩€佺鐢ㄨ鍒?
2. 璐熻矗 popup 鐨勫尮閰嶇瓥鐣ラ厤缃?
3. 璐熻矗鏍规嵁杩愯鏃剁幆澧冮€夋嫨鍖归厤绛栫暐

`ImGuiSpoolWidget`

1. 璐熻矗鍏叡鍗风洏鎺т欢缁樺埗
2. 渚?resident mapping 涓婚潰鏉夸笌 popup 鍊欓€夊叡鐢?

### 17.2 popup 鍖归厤绛栫暐鏇存柊

褰撳墠 popup 鍖归厤琛屼负宸茬粡鏀舵暃涓烘樉寮忛厤缃細

```cpp
enum class PopupMatchMode
{
    StrictType = 0,
    LooseType,
    AvailabilityOnly
};
```

璇箟濡備笅锛?

1. `StrictType`
   - 鎸夎€楁潗绫诲瀷寮哄尮閰?
2. `LooseType`
   - 鍏佽澶у皬鍐欏綊涓€銆佸寘鍚叧绯诲拰鍩虹鏉愭枡瀹舵棌褰掑苟
3. `AvailabilityOnly`
   - 鍙寜鍊欓€夋槸鍚﹀彲鐢ㄨ繃婊?

### 17.3 runtime 鑷姩绛栫暐閫夋嫨鏇存柊

褰撳墠 controller 鍦ㄦ瀯寤?resident panel view data 涔嬪墠锛屼細鍏堥€夋嫨涓€浠?popup policy config銆?

褰撳墠鑷姩瑙勫垯涓猴細

1. 璁惧绂荤嚎锛屾垨鏈鍙栧埌鍙敤璁惧鑰楁潗
   - 浣跨敤 `AvailabilityOnly`
2. 澶栨寕鏂欐灦妯″紡锛屾垨褰撳墠璁惧涓嶆槸澶氳壊璁惧
   - 浣跨敤 `LooseType`
3. 澶氳壊鍦ㄧ嚎妯″紡涓嬶細
   - 鑻ュ瓨鍦ㄥ閮ㄦ潵婧愭枡鐩掞紙`box_type == 1 / 2`锛夛紝浣跨敤 `LooseType`
   - 鍚﹀垯浣跨敤 `StrictType`

杩欐剰鍛崇潃锛?

1. popup 鍊欓€夌殑 enable / disable 閫昏緫锛屽凡涓嶅啀鍥哄畾鍐欐
2. 鍚庣画鑻ヤ骇鍝佽姹傝繘涓€姝ヨ皟鑺傗€滀弗鏍煎害鈥濓紝浼樺厛淇敼 `PopupPolicy` 灞傦紝鑰屼笉鏄洖鍒?`ImGuiFilamentPanel` 涓慨鏀圭粯鍒跺垎鏀?
### 17.4 褰撳墠 resident popup 缁撴瀯涓庡嵎鐩樿祫婧愬疄鐜板悓姝?

鎴嚦褰撳墠锛宺esident mapping 鐨?popup 涓庡嵎鐩樿瑙夊張鏈変袱椤瑰疄鐜版敹鏁涳紝琛ュ厖濡備笅锛?

1. popup 宸叉敼涓衡€滄潵婧愮瓫閫?chip + 鍗曟潵婧愬€欓€夌煩闃?+ 鍥哄畾楂樺害鍐呴儴婊氬姩鈥?
   - 涓嶅啀鎶婃墍鏈?`CFS N` 鍒嗙粍鍚屾椂绾靛悜灞曞紑
   - 榛樿钀藉埌褰撳墠宸叉槧灏勭洰鏍囨墍鍦ㄦ潵婧愮粍
2. 鍏变韩鍗风洏缁勪欢宸叉敼涓哄垎灞傝祫婧愮粯鍒?
   - `simple_mode_spool_back.svg`
   - `simple_mode_spool_filament.svg`
   - `simple_mode_spool_front.svg`
3. selector 涓?popup 澶嶇敤鍚屼竴濂楀嵎鐩樼粍浠讹紝涓嶅啀鍒嗗埆缁存姢涓ゅ椋庢牸
4. selector 鍙充晶鏂囨湰鍖哄凡鏀逛负绔栫洿瀵归綈锛岀澶寸嫭绔嬪湪鏈€鍙充晶锛岄伩鍏嶄笌 `slot / material type` 閲嶅彔

杩欐剰鍛崇潃鍚庣画瀹炵幇浼樺厛绾у簲涓猴細

1. popup 缁撴瀯璋冩暣鍏堢湅 `ResidentFilamentMappingView`
2. 鍊欓€夌瓫閫変笌鏉ユ簮鍒嗙粍鍏堢湅 `ResidentFilamentMappingPopupPolicy`
3. 鍗风洏瑙嗚缁嗚皟鍏堢湅 `ImGuiSpoolWidget` 涓庡搴?SVG 璧勬簮

### 17.5 褰撳墠 resident 鍏ュ彛涓?hover preview 鍚屾

琛ュ厖涓ら」褰撳墠宸茶惤鍦板疄鐜帮細

1. simple mode 椤堕儴涓诲伐鍏锋爮涓殑 `鑰楁潗` 鍙鎸夐挳宸茬Щ闄?
   - 宸︿笂 resident mapping 鍖哄煙鎴愪负褰撳墠涓诲叆鍙?
2. popup 鍊欓€夐」宸叉敮鎸?hover preview
   - 鎮诞鏌愪釜 enabled 鍊欓€夋椂锛岄瑙堝浘涓存椂鍒囨崲鍒拌鍊欓€夐鑹?
   - 绂诲紑 hover 鎴栧叧闂?popup 鏃惰嚜鍔ㄦ仮澶?
   - 鐐瑰嚮鏃舵墠鐪熸鎻愪氦鏄犲皠

瀵瑰簲瀹炵幇灞傦細

1. hover 浜嬩欢鐢?`ResidentFilamentMappingView` 鍙戝嚭
2. 鍊欓€?token 瑙ｆ瀽鐢?`ResidentFilamentMappingAdapter` 璐熻矗
3. 涓存椂棰勮瑕嗙洊鐢?`ImGuiFilamentPanel` 鍦ㄧ缉鐣ュ浘缁樺埗闃舵娑堣垂

### 17.6 褰撳墠 resident 澧炲垹棰滆壊鑼冨洿鏀舵暃

琛ュ厖褰撳墠浜у搧缁撹锛?

1. simple mode resident mapping 浠呬繚鐣欙細
   - `娣诲姞棰滆壊`
   - `鍒犻櫎璇ラ鑹瞏
2. 涓嶅湪绠€鏄撴ā寮忓唴鏆撮湶 `鍚堝苟鍒?..`

鍘熷洜锛?

1. `鍚堝苟` 瀵规柊鎵嬩笉澶熺洿瑙?
2. 鍏跺簳灞傚疄闄呭奖鍝嶅埌 filament 绱㈠紩杩佺Щ銆佹ā鍨嬩綋绉尋鍑烘満缂栧彿銆乻upport filament 涓庨儴鍒?toolchange 淇
3. 涓嶉€傚悎浣滀负 resident mapping 榛樿鑳藉姏鏆撮湶

瀹炵幇鎻愰啋锛?

1. 褰撳墠 `ImGuiFilamentPanel::remove_last_filament()` 鍙鐩栤€滃垹闄ゆ渶鍚庝竴涓鑹测€濈殑鏃ц矾寰?
2. 鍚庣画 resident 琛岀骇鍒犻櫎鑻ヨ钀藉湴锛屽簲浼樺厛鎺ュ埌宸叉湁鎴愮啛搴曞眰鍒犻櫎閾捐矾锛岃€屼笉鏄户缁墿灞?`pop_back` 寮忓疄鐜?

