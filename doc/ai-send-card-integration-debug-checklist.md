# AI Send Card 鑱旇皟娓呭崟涓庡缓璁墦鐐逛綅缃?
## 1. 鐩殑

杩欎唤鏂囨。鐢ㄤ簬鎸囧涓嬮潰杩欐潯鏂伴摼璺殑鑱旇皟锛?
- `C3DSlicer` 渚э細`send_to_printer` 涓嶅啀鐩存帴杩涘叆鏃у彂閫佸脊绐楋紝鑰屾槸鎵撳紑 AI 鍙戦€佸崱鐗?- `AIChatPage` 渚э細鎺ユ敹 `ai_send_card_snapshot / progress / result / error`
- `AISendWorkflowService` 渚э細璐熻矗鍗曠洏鍙戦€併€佺洏鍒囨崲銆佽€楁潗鏄犲皠銆佷笂浼犺繘搴︿笌缁撴灉鍥炰紶

鐩爣涓嶆槸閲嶆柊瑙ｉ噴鏋舵瀯锛岃€屾槸缁欎竴浠藉彲浠ヨ竟璺戣竟瀵圭収鐨勮仈璋冩竻鍗曪紝骞舵槑纭細

- 姣忎竴姝ュ簲璇ュ厛鐪嬪摢涓簨浠?- 浜嬩欢鐨勫墠鍚庝緷璧栨槸浠€涔?- 鐜版湁鏃ュ織鍦ㄥ摢浜涗綅缃凡缁忔湁
- 鍝簺浣嶇疆寤鸿缁х画琛ユ棩蹇?
## 2. 鑱旇皟鑼冨洿

鏈疆涓昏鐪嬩笁鍧楋細

- `C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `C3DSlicer/src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
- `AIChatPage/src/controller/chatWorkspaceController.js`

鍓嶇娓叉煋鍜屼簨浠堕€忎紶鐩稿叧锛?
- `AIChatPage/src/widgets/AISendCard.vue`
- `AIChatPage/src/widgets/MessageBubble.vue`
- `AIChatPage/src/widgets/MessageList.vue`
- `AIChatPage/src/layout/ChatDockShell.vue`
- `AIChatPage/src/host/c3dSlicerHostAdapter.js`

## 3. 鍏堝喅鏉′欢妫€鏌?
姝ｅ紡鑱旇皟鍓嶏紝鍏堢‘璁や互涓嬫潯浠舵垚绔嬶細

1. `AIChatPage` 宸叉瀯寤洪€氳繃銆?   - 鏈疆宸查獙璇?`npm run build` 閫氳繃銆?
2. `C3DSlicer` 鐨?AI 鍙戦€佸崱鐗?handler 宸叉敞鍐屻€?   - `MCPChatPanel.cpp:948-955`

3. `send_to_printer` 宸ュ叿璋冪敤宸茶鏀逛负鈥滃紑鍗＄墖骞舵寕璧封€濄€?   - `MCPChatPanel.cpp:2155-2178`

4. `chat_ready` 浼氬洖鏀炬椿鍔ㄥ崱鐗囥€?   - `MCPChatPanel.cpp:915-929`

5. `AISendWorkflowService` 宸茶兘鍙戝嚭鍥涚被浜嬩欢銆?   - snapshot: `AISendWorkflowService.cpp:956-960`
   - progress: `AISendWorkflowService.cpp:962-966`
   - result: `AISendWorkflowService.cpp:968-972`
   - error: `AISendWorkflowService.cpp:974-978`

6. `AIChatPage` 宸茶兘鎺ヤ綇鍥涚被 host bridge 浜嬩欢銆?   - `chatWorkspaceController.js:2072-2087`

## 4. 鎬讳綋鑱旇皟椤哄簭

寤鸿涓ユ牸鎸変笅闈㈤『搴忚仈锛?
1. 鍗＄墖鎵撳紑閾捐矾
2. snapshot 棣栧睆娓叉煋
3. 鐩樺垏鎹?4. auto match
5. update mapping
6. send only
7. start print
8. cancel
9. 鍒囩墖鍚庤嚜鍔ㄥ紑鍗＄墖
10. chat reload 鍚庢椿鍔ㄥ崱鐗囧洖鏀?
杩欐牱鍋氱殑鍘熷洜鏄細鍓?1-5 姝ュ彧楠岃瘉鈥滅姸鎬侀噰闆嗕笌浜や簰鍒锋柊鈥濓紝涓嶄細娣峰叆涓婁紶寮傛闂锛?-8 鍐嶉獙璇佺湡姝ｇ殑鍙戦€侀棴鐜€?
## 5. 鍒嗛樁娈佃仈璋冩竻鍗?
## 5.1 鍗＄墖鎵撳紑閾捐矾

### 瑙﹀彂鏂瑰紡

- 鍦?AI 鑱婂ぉ涓 agent 浜у嚭 `send_to_printer`
- 鎴栬蛋 recommendation action 涓殑 `send_to_printer`
- 鎴栬蛋 `slice_completed -> openAISendCard`

### 鍏抽敭閾捐矾

1. Vue 璋冪敤 `openAISendCard`
   - `chatWorkspaceController.js:1186-1218`

2. 閫氳繃 fire-and-forget 妗ユ帴鍙戠粰 C++
   - `chatWorkspaceController.js:1172-1183`
   - `c3dSlicerHostAdapter.js:252-254`

3. C++ handler 鏀跺埌 `ai_send_card_open`
   - `MCPChatPanel.cpp:2511-2533`

4. `AISendWorkflowService::OpenCard` 鐢熸垚 snapshot
   - `AISendWorkflowService.cpp:69-86`

5. C++ 鍙戝嚭 `ai_send_card_snapshot`
   - `MCPChatPanel.cpp:2633-2639`

6. Vue 鏀跺埌 snapshot锛岃惤鍒?message.aiSendCard
   - `chatWorkspaceController.js:1132-1169`

### 棰勬湡鐜拌薄

- 鑱婂ぉ鍖哄厛鍑虹幇 opening 鍗＄墖
- 寰堝揩琚涓?snapshot 鍒锋柊
- 濡傛灉 snapshot 鎴愬姛锛屽崱鐗囦腑搴旇嚦灏戠湅鍒帮細
  - 璁惧鍚?  - 鐩樺垪琛?  - 鏄犲皠鎽樿
  - 鎿嶄綔鎸夐挳鍚敤鐘舵€?
### 寤鸿鍏堢湅鍝簺鏃ュ織

- Vue:
  - `[AIChatPage] ai_send_card event from host bridge`
  - `[AIChatPage] handleRunAction forwarding action`

- C++:
  - `[MCPChatPanel] ai_send_card_open payload=...`
  - `send_to_printer` 宸ュ叿鎸傝捣鐩稿叧鏃ュ織

### 甯歌澶辫触鐐?
- opening 鍗＄墖鍑虹幇锛屼絾娌℃湁 snapshot
  - 浼樺厛鎺掓煡 `HandleAISendCardOpen`
  - 鍐嶇湅 `AISendWorkflowService::OpenCard`
  - 鍐嶇湅 `refresh_state_locked`

- C++ 鏀跺埌 open锛屼絾鍓嶇娌℃湁鏇存柊
  - 鐪?`SendAgentEvent("ai_send_card_snapshot", ...)`
  - 鐪?`handleHostAgentMessage`
  - 鐪?`applyAISendCardEnvelope`

## 5.2 snapshot 棣栧睆娓叉煋

### 鏍稿績鍑芥暟

- `AISendWorkflowService.cpp:642-749`

### 閲嶇偣妫€鏌ュ瓧娈?
- `request_id`
- `card_id`
- `data.status`
- `data.status_text`
- `data.device`
- `data.plate_selector.available`
- `data.plate`
- `data.mapping.items`
- `data.mapping.option_groups`
- `data.actions`

### 棰勬湡鐜拌薄

- `plate_selector.available` 涓庡綋鍓嶅伐绋嬬洏鏁颁竴鑷?- `plate.plate_index` 涓庡綋鍓嶇洏涓€鑷?- `mapping.items` 涓?simple 妯″紡鑰楁潗鏄犲皠绠楁硶杈撳嚭涓€鑷?- `actions.can_send_only / can_start_print` 浠呭湪鍙彂閫佹椂鍚敤

### 甯歌澶辫触鐐?
- 鐩樺垪琛ㄤ负绌猴細鐪?`state.plates`
- preview 绌猴細褰撳墠瀹炵幇閲?`preview_image` 浠嶅彲鑳戒负绌猴紝杩欎竴椤瑰彲浠ユ殏鏃舵帴鍙?- mapping 绌猴細鍏堢‘璁?filament panel 鏄惁鍙敤

## 5.3 鐩樺垏鎹㈣仈璋?
### 瑙﹀彂鏂瑰紡

- 鐐瑰嚮 AI 鍙戦€佸崱鐗囩殑 plate tab

### 鍏抽敭閾捐矾

1. `AISendCard.vue` 鍙戝嚭 `select_plate`
2. `ChatDockShell.vue` 杞粰 controller
3. `executeAISendCardAction` 鏄犲皠鍒?`ai_send_card_select_plate`
   - `chatWorkspaceController.js:2327-2350`

4. C++ handler 鏀跺埌
   - `MCPChatPanel.cpp:2535-2552`

5. `AISendWorkflowService::SelectPlate`
   - `AISendWorkflowService.cpp:99-165`

6. 閲嶆柊鍙?snapshot

### 棰勬湡鐜拌薄

- 褰撳墠閫変腑鐨?tab 鍒囨崲
- `plate.plate_index` 鏇存柊
- 鏄犲皠鍒楄〃璺熺潃鏂扮洏鍒锋柊
- 鍙彂閫佺姸鎬佽窡鐫€鏂扮洏鍙樺寲

### 浼樺厛瑙傚療瀛楁

- `plate_selector.selected_plate_index`
- `plate.plate_index`
- `mapping.items`
- `actions.can_send_only`
- `actions.can_start_print`

### 寤鸿鎵撶偣

- Vue 绔缓璁ˉ锛?  - `AISendCard.vue` 鐨?`handleSelectPlate`
  - 鎵撳嵃 `card_id`, `request_id`, `plate_index`

- C++ 绔缓璁ˉ锛?  - `AISendWorkflowService::SelectPlate`
  - 鎵撳嵃 `card_id`, `old_plate_index`, `new_plate_index`, `revision`

## 5.4 Auto Match 鑱旇皟

### 鍏抽敭閾捐矾

- handler:
  - `MCPChatPanel.cpp:2554-2563`
- service:
  - `AISendWorkflowService.cpp:167-230`

### 棰勬湡鐜拌薄

- 鐐瑰嚮鍚庤嫢鎴愬姛锛屽簲鍥炰竴甯ф柊鐨?snapshot
- `mapping.summary_text` 鏇存柊
- `mapping.items` 涓凡鍖归厤椤瑰鍔?
### 閲嶇偣鐪?
- `current_device.valid`
- `panel->is_current_device_valid()`
- `panel->on_auto_mapping_filament_ex(current_device)`

### 甯歌澶辫触鐐?
- 鎶?`FILAMENT_PANEL_NOT_AVAILABLE`
- 鎶?`DEVICE_NOT_AVAILABLE`
- error 浜嬩欢鍒颁簡鍓嶇锛屼絾鍗＄墖涓嶅簲璇ョ粨鏉熷伐鍏疯皟鐢?  - 鐪?`finish_tool_call = false` 鏄惁姝ｇ‘
  - `MCPChatPanel.cpp:2558-2561`
  - `AISendWorkflowService.cpp:215-219`

## 5.5 Update Mapping 鑱旇皟

### 鍏抽敭閾捐矾

- handler:
  - `MCPChatPanel.cpp:2565-2574`
- service:
  - `AISendWorkflowService.cpp:233-342`

### 棰勬湡鐜拌薄

- 淇敼鏌愪竴椤规槧灏勫悗锛屾柊鐨?snapshot 鍥炴潵
- 瀵瑰簲 `mapping.items[n]` 鐘舵€佹洿鏂?- `mapping.summary_text` 鏇存柊
- 濡傛灉鎵€鏈夋槧灏勫畬鎴愶紝鍙戦€佹寜閽簲鍙敤

### 閲嶇偣妫€鏌?payload

- `card_id`
- `request_id`
- `mapping.item_index`
- `mapping.selection_token`
- `mapping.extruderId`

### 寤鸿鎵撶偣

- Vue:
  - `AISendCard.vue` 鐨?`handleMappingChange`
  - 鎵撳嵃 `item_index`, `selection_token`

- C++:
  - `AISendWorkflowService::UpdateMapping`
  - 鎵撳嵃锛?    - 褰撳墠 item 绱㈠紩
    - 杈撳叆 token
    - 鏇存柊鍚?mapped_count / total_count

## 5.6 Send Only 鑱旇皟

### 鍏抽敭閾捐矾

1. Vue 鍙?`send_only`
2. `executeAISendCardAction` -> `ai_send_card_send_only`
3. `MCPChatPanel::HandleAISendCardSendOnly`
   - `MCPChatPanel.cpp:2576-2594`

4. `AISendWorkflowService::StartSendOnly`
   - `AISendWorkflowService.cpp:89-92`

5. `start_send_internal(false)`
   - `AISendWorkflowService.cpp:406-484`

6. 涓婁紶杩囩▼锛?   - progress: `AISendWorkflowService.cpp:873-918`
   - complete: `AISendWorkflowService.cpp:920-953`

7. C++锛?   - `OnAISendProgress`: `MCPChatPanel.cpp:2641-2667`
   - `OnAISendResult`: `MCPChatPanel.cpp:2669-2683`

### 棰勬湡鐜拌薄

- 鍗＄墖鎸夐挳杩涘叆 busy
- 鐪嬪埌 progress 鏉℃洿鏂?- 鏈€缁堟敹鍒?`result_type = send_only_done`
- 鎸傝捣鐨?`send_to_printer` tool call 琚垚鍔熺粨鏉?
### 閲嶇偣瑙傚療

- `build_print_data` 缁撴灉
  - `AISendWorkflowService.cpp:563-592`
- `resolve_gcode_file`
  - `AISendWorkflowService.cpp:543-561`
- `color_match_info`
- `open_cfs`
- `print_calibration`

### 甯歌澶辫触鐐?
- UI 鐐逛簡娌″弽搴旓細鐪?`executeAISendCardAction`
- C++ 鏀跺埌鍛戒护浣嗕笉涓婁紶锛氱湅 `can_send_locked`
- 鏈?progress 鏃?result锛氱湅涓婁紶瀹屾垚鍥炶皟鏄惁杩?`on_upload_complete`
- result 鍥炴潵浜嗕絾 tool call 娌＄粨鏉燂細鐪?`FinishAISendToolCallSuccess`

## 5.7 Start Print 鑱旇皟

### 涓?Send Only 鐨勫尯鍒?
- 鍏ュ彛涓?`StartSendAndPrint`
  - `AISendWorkflowService.cpp:94-97`
- 鏈€缁堢粨鏋滃簲涓猴細
  - `result_type = print_started`

### 棰濆閲嶇偣

- 璁惧鍦ㄧ嚎鐘舵€?- 褰撳墠璁惧鏄惁 idle
- 鍙戦€佸悗鏄惁甯︿簡鈥滃惎鍔ㄦ墦鍗扳€濈殑涓氬姟瀛楁

### 寤鸿鎵撶偣

- `start_send_internal(card_id, true)` 杩涘叆鏃?- 涓婁紶瀹屾垚鍚庡疄闄?result_type

## 5.8 Cancel 鑱旇皟

### 鍏抽敭閾捐矾

- `MCPChatPanel.cpp:2616-2625`
- `AISendWorkflowService.cpp:344-370`

### 棰勬湡鐜拌薄

- 鑻ヨ繕鍦ㄧ瓑寰呯‘璁わ細
  - 鍗＄墖搴旂粨鏉熶负 canceled

- 鑻ヤ笂浼犱腑锛?  - `sender->cancelUpload()` 琚皟鐢?  - 鏈€缁?result 涓?`canceled`

### 閲嶇偣鐪?
- `session.sender.reset()`
- `session.in_progress = false`
- `result_type = canceled`

## 5.9 鍒囩墖鍚庤嚜鍔ㄥ紑鍗＄墖鑱旇皟

### 鍏抽敭閾捐矾

- recommendation action 璁板綍寰呮墽琛屼笂涓嬫枃
  - `chatWorkspaceController.js:2395-2409`

- 鏀跺埌 `slice_completed` 鍚庤嚜鍔?`openAISendCard`
  - `chatWorkspaceController.js:1994-2013`

### 棰勬湡鐜拌薄

- `start_slice_then_send_to_printer`
- `apply_config_then_slice_then_send_to_printer`

涓婅堪涓ょ被 action锛屼笉搴旂洿鎺ュ彂鏃у彂閫佹祦绋嬶紝鑰屽簲鍦ㄥ垏鐗囧畬鎴愬悗鎵撳紑 AI 鍙戦€佸崱鐗囥€?
### 甯歌澶辫触鐐?
- 鍒囩墖瀹屾垚浜嗕絾娌″紑鍗＄墖
  - 鐪?`pendingRecommendationPostSliceAction`
  - 鐪?`pendingRecommendationPostSliceContext`
  - 鐪?`host action result` 鏄惁姝ｇ‘钀藉埌浜?`slice_completed`

## 5.10 chat reload / reopen 鍚庢椿鍔ㄥ崱鐗囧洖鏀?
### 鍏抽敭閾捐矾

- `chat_ready`
  - `MCPChatPanel.cpp:915-929`

- `ReplayAISendCardsToJS`
  - 褰撳墠鍦?`chat_ready` 鍜岄噸澶嶈姹傚満鏅笅閮戒細璧?
### 棰勬湡鐜拌薄

- WebView 閲嶈浇鍚庯紝浠嶈兘鐪嬪埌宸叉湁娲诲姩鍗＄墖
- 濡傛灉涔嬪墠杩樺湪绛夊緟纭锛屽崱鐗囧簲鎭㈠鍒?snapshot 鎬?
### 閲嶇偣妫€鏌?
- `request_id` 鏄惁杩樿兘瀵瑰簲鍥炲師娑堟伅
- `card_id` 鍒?message 鐨勬槧灏勬槸鍚﹂噸寤烘垚鍔?- Vue 渚?opening placeholder 鏄惁浼氳 snapshot 姝ｇ‘瑕嗙洊

## 6. 寤鸿閲嶇偣瑙傚療鐨勪簨浠跺簭鍒?
涓嬮潰鏄渶鍏抽敭鐨勫嚑涓爣鍑嗗簭鍒椼€?
## 6.1 鐩存帴寮€鍗＄墖

1. Vue `openAISendCard`
2. C++ `ai_send_card_open`
3. Service `OpenCard`
4. C++ `ai_send_card_snapshot`
5. Vue `applyAISendCardEnvelope(snapshot)`

## 6.2 鐩樺垏鎹?
1. Vue `select_plate`
2. C++ `ai_send_card_select_plate`
3. Service `SelectPlate`
4. C++ `ai_send_card_snapshot`
5. Vue 鍗＄墖鍒锋柊

## 6.3 鍙戦€?
1. Vue `send_only` 鎴?`start_print`
2. C++ handler
3. Service `start_send_internal`
4. progress x N
5. result 鎴?error
6. C++ 瀹屾垚鎸傝捣 tool call
7. Vue 鍗＄墖杩涘叆 terminal

## 7. 鐜版湁鏃ュ織浣嶇疆

## 7.1 C++ 宸叉湁鏃ュ織

宸插瓨鍦ㄣ€佽仈璋冩椂浼樺厛鐪嬬殑浣嶇疆锛?
- `MCPChatPanel.cpp:920`
  - `chat_ready received, bootstrap JS state`

- `MCPChatPanel.cpp:2513`
  - `ai_send_card_open payload=...`

- `MCPChatPanel.cpp:2537`
  - `ai_send_card_select_plate payload=...`

- `MCPChatPanel.cpp:2556`
  - `ai_send_card_auto_match payload=...`

- `MCPChatPanel.cpp:2567`
  - `ai_send_card_update_mapping payload=...`

- `MCPChatPanel.cpp:2578`
  - `ai_send_card_send_only payload=...`

- `MCPChatPanel.cpp:2598`
  - `ai_send_card_start_print payload=...`

- `MCPChatPanel.cpp:2618`
  - `ai_send_card_cancel payload=...`

## 7.2 Vue 宸叉湁鏃ュ織

- `chatWorkspaceController.js:2081-2085`
  - host bridge 鏀跺埌 `ai_send_card_*`

- `chatWorkspaceController.js:2000-2012`
  - 鍒囩墖瀹屾垚鍚庤嚜鍔ㄥ紑鍗＄墖

- `ChatDockShell.vue:238-241`
  - AI 鍗＄墖鍔ㄤ綔澶辫触

- `MessageBubble.vue`
  - 鍘熸湁 action 鐐瑰嚮鏃ュ織浠嶅彲杈呭姪纭鏉ユ簮娑堟伅

## 8. 寤鸿鏂板鏃ュ織浣嶇疆

涓嬮潰杩欎簺鏃ュ織涓嶆槸蹇呴』鐜板湪鏀癸紝浣嗛潪甯稿€煎緱琛ャ€?
## 8.1 C++ 寤鸿琛?
### `AISendWorkflowService::OpenCard`

浣嶇疆锛?
- `AISendWorkflowService.cpp:69-86`

寤鸿鎵撳嵃锛?
- `request_id`
- `card_id`
- `selected_plate_index`
- `mapping_count`
- `can_send`

### `AISendWorkflowService::refresh_state_locked`

浣嶇疆锛?
- `AISendWorkflowService.cpp:486-499`

寤鸿鎵撳嵃锛?
- `has_model`
- `current_plate_index`
- `current_plate_can_print`
- `device.has_bound_device`
- `device.online`
- `device.is_idle`

### `AISendWorkflowService::SelectPlate`

浣嶇疆锛?
- `AISendWorkflowService.cpp:99-165`

寤鸿鎵撳嵃锛?
- `card_id`
- `from_plate`
- `to_plate`
- `revision`

### `AISendWorkflowService::UpdateMapping`

浣嶇疆锛?
- `AISendWorkflowService.cpp:233-342`

寤鸿鎵撳嵃锛?
- `card_id`
- `item_index`
- `selection_token`
- `mapped_count`
- `total_count`

### `AISendWorkflowService::start_send_internal`

浣嶇疆锛?
- `AISendWorkflowService.cpp:406-484`

寤鸿鎵撳嵃锛?
- `card_id`
- `start_print`
- `file_path`
- `upload_name`
- `color_match_info.size`
- `open_cfs`
- `print_calibration`

### `AISendWorkflowService::on_upload_complete`

浣嶇疆锛?
- `AISendWorkflowService.cpp:920-953`

寤鸿鎵撳嵃锛?
- `card_id`
- `start_print`
- `is_upload_successful`
- `result_type`
- `body` 鎽樿

## 8.2 Vue 寤鸿琛?
### `AISendCard.vue::emitCardAction`

寤鸿鎵撳嵃锛?
- `command`
- `card_id`
- `request_id`

### `AISendCard.vue::handleSelectPlate`

寤鸿鎵撳嵃锛?
- `plate_index`

### `AISendCard.vue::handleMappingChange`

寤鸿鎵撳嵃锛?
- `item_index`
- `extruderId`
- `selection_token`

### `chatWorkspaceController.js::applyAISendCardEnvelope`

寤鸿鎵撳嵃锛?
- `eventName`
- `request_id`
- `card_id`
- `target_message_id`
- `phase`
- `terminal`

### `chatWorkspaceController.js::openAISendCard`

寤鸿鎵撳嵃锛?
- `sourceMessageId`
- `request_id`
- `payload keys`

## 9. 鏈€灏忚仈璋冩墽琛岃剼鏈?
濡傛灉鍙兂蹇€熼獙璇佷富閾捐矾锛屽缓璁寜杩欎釜椤哄簭鎿嶄綔锛?
1. 鎵撳紑 AI 鑱婂ぉ椤碉紝纭鏀跺埌 `chat_ready`
2. 璁?agent 瑙﹀彂涓€娆?`send_to_printer`
3. 鐪?opening 鍗＄墖鏄惁鍑虹幇
4. 鐪?`ai_send_card_snapshot` 鏄惁鍒拌揪鍓嶇
5. 鐐瑰嚮鍒囨崲鍒板彟涓€涓洏
6. 鐐瑰嚮 `Auto Match`
7. 淇敼涓€椤规槧灏?8. 鐐瑰嚮 `Send Only`
9. 鐪?progress 鏄惁鎸佺画鏇存柊
10. 鐪?result 鏄惁鎴愬姛鏀跺熬

濡傛灉杩欎釜 10 姝ヨ兘璧伴€氾紝璇存槑 AI 鍗＄墖閾捐矾宸茬粡鍩烘湰鎵撻€氥€?
## 10. 甯歌闂瀹氫綅绱㈠紩

### 闂锛氬崱鐗囨墦涓嶅紑

浼樺厛妫€鏌ワ細

- `chatWorkspaceController.js:1186-1218`
- `MCPChatPanel.cpp:2511-2533`
- `AISendWorkflowService.cpp:69-86`

### 闂锛氬崱鐗囨墦寮€浣嗗唴瀹逛负绌?
浼樺厛妫€鏌ワ細

- `AISendWorkflowService.cpp:642-749`
- `chatWorkspaceController.js:1132-1169`

### 闂锛氱洏鍒囨崲鏃犳晥

浼樺厛妫€鏌ワ細

- `AISendCard.vue`
- `chatWorkspaceController.js:2327-2350`
- `MCPChatPanel.cpp:2535-2552`
- `AISendWorkflowService.cpp:99-165`

### 闂锛氭槧灏勬洿鏂版棤鏁?
浼樺厛妫€鏌ワ細

- `MCPChatPanel.cpp:2565-2574`
- `AISendWorkflowService.cpp:233-342`

### 闂锛氬彂閫佹寜閽笉鍙偣

浼樺厛妫€鏌ワ細

- `AISendWorkflowService.cpp:501-526`
- `AISendWorkflowService.cpp:696-738`

### 闂锛氭湁杩涘害鏃犵粨鏋?
浼樺厛妫€鏌ワ細

- `AISendWorkflowService.cpp:873-953`
- `MCPChatPanel.cpp:2641-2683`

## 11. 缁撹

杩欐潯閾捐矾褰撳墠鏈€鍏抽敭鐨勪笉鏄户缁墿鍔熻兘锛岃€屾槸鍏堟妸涓嬮潰涓変欢浜嬭仈绋筹細

1. `snapshot` 鏄惁濮嬬粓鑳芥纭埛鏂板崱鐗?2. `select_plate / auto_match / update_mapping` 鏄惁閮借兘鍥炲埌鏂扮殑 snapshot
3. `send_only / start_print` 鏄惁鑳藉畬鏁磋蛋鍒?progress 鍜?result

鍙杩欎笁浠朵簨绋冲畾锛屽悗闈㈠啀缁х画琛ワ細

- preview 鍥剧墖鐪熷疄鎺ュ叆
- retry 璇箟
- 鏇寸粏鐨勯敊璇€佸睍绀?- 鏇村畬鏁寸殑鏃ュ織閲囨牱

灏变細椤哄緢澶氥€?
