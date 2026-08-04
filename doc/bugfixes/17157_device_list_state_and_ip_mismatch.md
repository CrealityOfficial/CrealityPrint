# 17157 【AI版】设备列表页显示的机器状态对不上，且IP地址也是错误的

## 1. 基本信息
- Bug ID：17157
- 标题：【AI版】设备列表页显示的机器状态对不上，且IP地址也是错误的
- 反馈人：测试反馈
- 处理人：
- 影响模块/影响文件：
  - `src/slic3r/GUI/simple/DeviceListSimple.cpp`（AI/简约版设备列表弹窗 key 唯一性、去重）
  - `src/slic3r/GUI/print_manage/data/DataCenter.cpp`（当前设备择取）

## 2. 现象与复现
- 前置：**未登录创想云**，全部为局域网（本地）设备；界面为 AI / 简约版。
- 复现场景：同一台设备（同一 MAC）先后以两个不同 IP 被加入两次（如 `172.23.198.88` → `172.23.1.119`），期间不刷新界面；随后切换机器端 wifi，使两条一条在线、一条离线。
- 实际结果（准备页）：
  - 顶部设备列表弹窗卡片显示的仍是离线的那条（明显是第一条/旧条目），而下方设备管理列表已是在线新 IP。
  - 顶部"当前设备"汇总卡与实际在线设备对不上。
- 期望结果：同一 MAC 设备在列表去重后优先显示在线的那条；顶部当前设备汇总卡与之对应；若均离线，取最后加入的那条。

## 3. 根因分析（纯本地 / 简约版）
准备页顶部设备列表弹窗由 `simple/DeviceListSimple.cpp` 的 `render_device_list_popup` 渲染（**不是** `GUI_ObjectList.cpp` 的表格版）；顶部当前设备卡与设备择取由 `DataCenter::_get_acive_device()` 决定。两处都因"同一 MAC 存在两条 deviceType==0 本地条目"而出错：

1. `SimpleDeviceMgr` 组织列表数据时的 key 生成：
   `key_name = device.name.empty() ? (modelName+mac+address) : device.name`
   - 有名字的本地设备（如 `SPARKX i7-005E`）只用 name 作 key，不含 mac/address，也不追加唯一后缀（唯一后缀仅对 deviceType==1 云端设备追加）。
   - 同名同 MAC 的两条本地条目 key 完全相同，导致：
     - `datas`（map）后一条覆盖前一条；
     - `online_device_list` / `offline_device_list`（set）残留同一个 key；
     - `mac_2_key_map[mac].second` 塞入两个相同 key，`manager_duplicate_deivce` 遍历到的都是被覆盖后的同一条，去重失效，最终保留了错误（离线）条目或重复显示。

2. `DataCenter::_get_acive_device()`：按 MAC 收集本地条目时直接 `device_local = printer` 覆盖（online 判断被注释），且提前 `break` 条件宽松，容易选中先遍历到的离线旧条目，与列表显示不一致。

## 4. 修复方案
统一"同一 MAC：在线优先；均离线取最后一条"，并修正列表 key 唯一性。

- 修改点 1（`DeviceListSimple.cpp`，核心）：本地设备 key_name 统一追加 `mac + address` 保证唯一：
  `key_name = (name.empty()? modelName : name) + "_" + mac + "_" + address`。
  两条本地条目在 `datas`/set 中不再互相覆盖，`mac_2_key_map` 去重按 mac 归并正常工作（去重键是 mac，不受 key_name 变化影响）。
- 修改点 2（`DeviceListSimple.cpp` 的 `manager_duplicate_deivce`）：均离线时由 `second[0]` 改为 `second.back()`，并对空数组做保护，与当前设备择取规则一致。
- 修改点 3（`DataCenter::_get_acive_device`）：收集 deviceType==0 条目时，`device_local` 为空、或新条目在线、或当前保留的是离线条目时均以新条目覆盖，实现"在线优先、均离线取最后一条"；提前 `break` 仅在已拿到在线本地条目且存在云端条目时触发。

## 5. 影响范围与风险
- 正向影响：同一 MAC 设备在弹窗列表去重且优先显示在线条目；顶部当前设备与列表一致；均离线时统一取最新条目。
- 可能风险：低。key_name 改为含 mac/address 仅影响弹窗内部索引，不影响设备数据与其它界面；择取规则调整仅作用于同 MAC 多条本地条目。
- 是否改变旧行为：单条设备、云端设备、Fluidd 设备显示逻辑不变；均离线场景当前显示条目由"第一条"改为"最后一条"，属预期调整。
- 备注：本次为表现层修复。数据层同 MAC 存两条本地条目属脏数据，根因宜在 Web 端 IP 变更时按 MAC 更新而非新增来杜绝；本次改动保证在存在重复条目时显示正确且一致。

## 6. 回归建议
- 必测：未登录云端，同一 MAC 设备以两个 IP 加入两次、不刷新界面，切换 wifi 使一条在线一条离线，确认弹窗列表只显示在线一条、顶部当前设备与之一致。
- 必测：两条均离线时，列表与当前设备卡都显示最后加入的那条。
- 必测：单条本地设备（无重复），在线/离线显示正常。
- 必测：登录云端后，同一设备本地在线 + 云端在线，优先显示本地在线条目；本地离线时回退云端。
- 边界：Fluidd（deviceType 1001）设备作为当前设备显示正常。
