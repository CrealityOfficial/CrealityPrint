# Bug 修复记录：发送页关闭后的上传任务取消与重发恢复

## 1. 基本信息
- Bug ID：`15494`
- 标题：`F022-广域网-报错后无法关闭发送页`
- 禅道地址：https://zentao.creality.com/zentao/bug-view-15494.html
- 日期：`2026-07-25`
- 产品：`Creality Print`
- 模块：`设备管理`
- 计划：`CP 7.2.1`
- Bug 类型：`代码错误`
- 严重程度：`严重`
- 优先级：`高`
- 状态：`已解决`
- 创建人：`冷金辉`
- 当前指派：`冷金辉`
- 解决人：`王昭`
- 影响版本：`CrealityPrint_7.1.0.4381_Beta`
- 解决版本：`主干`
- 开发基线：`d08bfc2970`（当前工作区为 detached HEAD，来源为 `release-260731`）
- 关联历史修复：`35d64e2c21 fix:[15494] 发送页回调阻塞导致关不掉`
- 关联记录：
  - https://zentao.creality.com/zentao/bug-view-3283.html
  - https://zentao.creality.com/zentao/bug-view-15321.html

> 禅道历史备注明确记录：原问题与关闭窗口时阻塞等待消息回调有关，不等待又会触发崩溃；2026-07-25 的备注指出关联修改后可能出现新的 `17431`、`17432`。以下后台取消、队列阻塞和线程竞态分析由本次代码检查推断。

## 2. 问题现象
- F022 广域网发送发生错误后，发送页无法正常关闭，WebView 进程无法退出。
- 历史修复 `35d64e2c21` 将窗口关闭从上传回调中解耦后，窗口可以立即关闭，但后台上传不一定真正结束。
- 弱网下云端取消可能长时间等待，旧任务持续占用上传线程。
- 用户重新打开发送页并再次发送时，新任务只能排队等待旧任务，表现为“下次发送不了”。
- 旧任务的迟到进度、状态或完成回调还可能干扰新窗口、新任务或重复上报结果。

## 3. 影响范围
- 模块：`设备管理 / 发送到打印机`
- 关键文件：
  - `src/slic3r/GUI/print_manage/App/SendToPrinter.cpp`
  - `src/slic3r/GUI/print_manage/App/SendToPrinter.hpp`
  - `src/slic3r/GUI/print_manage/RemotePrinterManager.cpp`
  - `src/slic3r/GUI/print_manage/RemotePrinterManager.hpp`
  - `src/slic3r/GUI/print_manage/UploadCancellation.hpp`
  - `src/slic3r/GUI/UploadFile.cpp`
  - `src/slic3r/GUI/UploadFile.hpp`
  - `src/slic3r/GUI/print_manage/Device/KlipperCXInterface.cpp`
  - `src/slic3r/GUI/print_manage/Device/Klipper4408Interface.cpp`
  - `src/slic3r/GUI/print_manage/Device/KlipperInterface.cpp`
  - `deps/aliyun-oss-cpp-sdk/0001-fix-slicer-build.patch`
- 受影响流程：
  - WAN GCode / 3MF 云端发送
  - 发送页关闭和前端取消
  - 弱网、断网、云端请求阻塞后的再次发送
  - Klipper4408 局域网发送取消
  - Moonraker/Klipper 上传取消
  - 设备列表、多机发送和简易发送入口

## 4. 修复前复现步骤
1. 打开发送页并选择 F022 广域网设备。
2. 开始发送 GCode 或 3MF。
3. 使用弱网、断网或服务端异常，使上传停留在获取凭证、获取 OSS 信息、分片上传或云端登记阶段。
4. 点击关闭发送页。
5. 发送页可能已经关闭，但后台上传线程仍等待旧任务的 `future`。
6. 重新打开发送页并再次发送。
7. 结果：新任务无法及时开始；旧任务回调还可能迟到。

## 5. 根因分析
- `RemotePrinterManager` 原来按 IP 管理取消，没有独立的上传任务 ID 和明确的任务终态。
- 单上传线程取出任务后会持续等待协议层 `future` 完成；窗口关闭只发出取消请求，无法保证旧任务及时退出。
- 任务管理器无法区分“云 API 准备阶段”和“源文件已打开阶段”，因此即使请求尚未占用文件，也会等待网络请求物理退出后才恢复 UI。
- 原有 `m_cancelUploadMap` 检查位于被禁用的 `#if 0` 分支中，当前运行路径没有通过该状态推动任务退出和回收。
- CX 云端上传复用 `KlipperCXInterface::m_upload_file`：
  - 取消状态是跨任务共享的普通 `bool`，存在数据竞争。
  - 新任务开始时重置取消状态，可能影响仍在清理的旧任务。
  - 获取云端凭证、获取 OSS 信息和最终登记请求没有统一接入取消令牌。
  - OSS 分片只在单个请求返回后检查取消，弱网时响应较慢。
  - `OssClient::DisableRequest()` 只设置禁用标志，仍依赖 libcurl 后续进度回调；网络完全阻塞时无法及时唤醒同步请求。
  - `shutdown` 无法处理尚未创建 socket 的 DNS 阶段；弱网或 DNS 异常下，等待传输层完整退出会直接恶化窗口关闭体验。
- Klipper4408 将异步任务栈上的 `Http` 裸指针放入共享 map，再由其他线程调用取消，存在并发访问和悬垂指针风险。
- `35d64e2c21` 中的 UI gate 能保护窗口生命周期，但不能代表后台上传已经取消，也不能释放上传调度资源。

## 6. 修复方案
- 引入任务级上传模型：
  - `pushUploadTasks(...)` 返回唯一 `taskId`。
  - 任务维护 `Queued / Running / CancelRequested / Cancelled / Succeeded / Failed` 状态。
  - 发送页保存当前 `taskId`，关闭或取消时精确调用 `cancelUploadTask(taskId)`。
- 扩展线程安全取消令牌：
  - 新增 `UploadCancellation.hpp`。
  - 每个任务独立保存取消状态和 `Preparing / FileInUse / FileReleased` 文件资源阶段。
  - 文件读取、压缩、HTTP multipart、FTP 和 OSS 上传使用 RAII 标记文件占用，退出作用域后再标记释放。
  - 进度、状态和完成回调只对所属任务生效，终态与状态回调最多投递一次。
- 按文件占用阶段执行取消：
  - 排队、获取 Aliyun 凭证、获取 OSS 信息和云端登记等未占用源文件的阶段，立即发布逻辑取消结果并恢复 UI。
  - 后台网络请求继续尽力取消，其迟到进度、成功、失败回调由任务终态和 UI generation 双重过滤。
  - 逻辑取消后的旧请求若仍处于网络超时，后续发送临时转入现有有界工作池，不再被单上传线程阻塞。
  - MD5、gzip、HTTP multipart、FTP 或 OSS 已打开文件时，窗口保持 `Cancelling`；协议 `future` 结束且 RAII 确认文件释放后才投递 `601` 和关闭窗口。
  - 重复取消保持幂等，不重复投递终态。
- 重构 CX 云端上传：
  - 每个任务创建独立 `UploadFile`，不再共享取消状态和进度回调。
  - 获取 Aliyun 凭证、OSS 信息和云端登记接口增加取消检查、`5` 秒连接超时和 `15` 秒总请求超时。
  - 上述云 API 请求继续尽力执行 socket 取消；DNS 等无文件阶段不再要求传输层先退出才能关闭窗口。
  - GCode MD5 和 gzip 压缩改为分块处理，每个数据块检查取消令牌。
  - 压缩结束后立即关闭原始 GCode，不再让文件句柄持续到云端上传结束。
  - 使用 OSS SDK `DisableRequest()` 中断正在进行的同步分片请求。
  - 扩展 OSS SDK 补丁，使 `DisableRequest()` 接管并关闭当前请求 socket，覆盖完全断流和建连阻塞。
  - libcurl 关闭回调通过所有权标记跳过取消线程已关闭的句柄，避免重复关闭和句柄复用。
  - 取消后不再同步等待 `AbortMultipartUpload`，避免弱网清理请求阻塞本地取消完成。
  - 压缩临时文件使用随机唯一名称并自动清理，避免旧任务后台回收时覆盖新任务文件。
  - OSS SDK 初始化/释放增加并发引用计数保护。
- 简化通用 HTTP 层：
  - 撤销 cURL `CURLOPT_QUICK_EXIT` 回移和 `curl_multi_wakeup()` 执行路径。
  - 普通请求恢复 `curl_easy_perform()`；文件上传仍保留进度回调和活动 socket 关闭能力。
  - 避免为无文件的 DNS 等待修改第三方解析线程生命周期。
- 加固局域网上传：
  - Klipper4408 和 Moonraker/Klipper 改用任务级取消令牌。
  - 移除跨线程共享的栈对象 `Http*` 和普通取消布尔值。
  - 保留原 HTTP multipart、进度上报和 `601` 取消状态码语义。
  - 修正 Klipper4408 网络错误 HTTP 状态为 0 时被误判为成功的问题。
  - FTP 局域网上传接入任务取消令牌和 cURL 传输进度回调，取消时立即退出 `curl_easy_perform()` 并关闭源文件。
  - Klipper4408 和 Moonraker 上传启用主动 socket 取消；即使限速后没有进度回调，也能通过 `shutdown` 唤醒阻塞传输。
- 保留兼容接口：
  - 旧的 `cancelUpload(ip)` 继续保留，内部映射到该地址最新任务。
  - 设备列表、多机发送和简易发送入口不需要同步修改调用方式。
- 增加 `[UploadCancel]` 与 `[CloudUpload]` 阶段日志，区分任务创建、取消命中和各云端阻塞阶段。

## 7. 代码改动摘要
- `SendToPrinter.cpp/.hpp`
  - 保存当前上传 `taskId`。
  - 窗口关闭和前端取消优先按 `taskId` 精确取消。
  - 上传期间拦截窗口关闭，物理上传线程结束后再清理任务并关闭模态窗口。
  - 完成业务回调不再提前清空上传标记，消除文件句柄析构前放行窗口的竞态。
  - 收到 `601` 后递增当前 UI generation，丢弃已经排队的旧进度和完成回调。
- `RemotePrinterManager.cpp/.hpp`
  - 引入上传任务状态、任务注册表、地址到最新任务的兼容映射。
  - 队列和无文件准备任务立即逻辑取消；文件占用任务等待协议 `future` 真正退出。
  - 旧准备请求后台清理时，后续发送使用有界工作池绕过被占用的单上传线程。
  - 统一过滤取消后的进度、状态、完成回调。
  - 协议结果延迟到本地资源释放后统一投递。
  - 修复打印机元数据 map 的并发访问。
- `UploadCancellation.hpp`
  - 提供取消状态、文件资源阶段和 RAII 文件占用保护。
- `UploadFile.cpp/.hpp`
  - 普通 `bool` 取消状态替换为任务级原子取消令牌。
  - 云端各请求阶段接入取消和超时。
  - GCode MD5、gzip 压缩和 OSS 分片上传支持快速取消。
  - 压缩结束立即关闭原始 GCode；云 API 和 OSS 同步请求均可主动中断 socket。
  - 修复分片缓冲区泄漏和固定临时文件名冲突。
- `deps/aliyun-oss-cpp-sdk/0001-fix-slicer-build.patch`
  - 跟踪 OSS SDK 当前活动 socket，并在 `DisableRequest()` 时执行 `shutdown + close`。
  - 已由取消线程关闭的 socket 从活动集合移除，libcurl 关闭回调不再重复释放。
  - 修正新版 Windows SDK 环境下 `XMLDocument` 符号歧义，保证依赖可重建。
- `deps/CURL/CURL.cmake`
  - 移除私有 quick-exit 补丁，恢复标准 cURL 7.75 构建。
- `KlipperCXInterface.cpp/.hpp`
  - 每个任务独立创建 `UploadFile`。
  - 各阶段保留真实错误码，并统一返回 `601` 取消结果。
- `Klipper4408Interface.cpp/.hpp`
  - 移除共享 `mapHttp` 和栈对象裸指针。
  - 使用任务令牌在 HTTP 进度回调中安全取消。
- `KlipperInterface.cpp/.hpp`
  - 移除共享 `m_pHttp`、`m_bCancelSend`，使用任务级取消。

## 8. 验证清单
- [x] 直接修改的上传、协议和发送页源文件增量编译通过。
- [x] `libslic3r_gui` 完整静态库目标编译、链接通过。
- [x] OSS SDK Release 依赖重新编译、安装通过，补丁在 1.9.2 原始源码上使用项目 `${PATCH_CMD}` 参数检查并应用通过。
- [x] `CrealityPrint_Slicer.dll` 在常规构建目录和运行测试目录完整链接通过。
- [x] 局域网限速卡在 `1%` 后可取消并关闭窗口（用户实测）。
- [x] 云端阶段日志确认取消命中 `upload-1`，阻塞点为 `get_aliyun_info`，取消到 `601` 仍等待约 `20` 秒。
- [x] 二次日志确认同一阶段取消后仍等待约 `15.3` 秒，与 cURL 线程式 DNS 清理同步 `join` 的路径一致。
- [x] 后续日志显示 `curl_multi_wakeup()` 命中后仍等待约 `18` 秒，确认继续修改 DNS 传输层不适合作为 UI 取消完成条件。
- [x] 撤销 cURL quick-exit/multi 私有路径，`libslic3r_gui` 重新编译通过。
- [x] `PrinterMgrView`、`Upload3mfToCloud`、`UploadGcodeToCloud`、`EasyPrintSender` 调用入口编译通过。
- [x] `git diff --check` 通过。
- [ ] WAN GCode 正常发送：进度、完成通知、后续打印正常。
- [ ] WAN 3MF 正常发送：进度、完成通知和临时文件清理正常。
- [ ] 获取凭证、获取 OSS 信息和云端登记阶段取消：窗口立即恢复，后台迟到回调不影响新任务。
- [ ] MD5、gzip 和 OSS 分片阶段取消：等待文件释放后窗口自动关闭。
- [ ] 弱网取消：窗口保持“取消中”，本地上传线程退出并释放文件后自动关闭。
- [ ] 断网或 DNS 异常取消：cURL/OSS 请求可中断，不等待云端 multipart 清理。
- [ ] 取消后立即向同一设备重发：旧任务回调不污染新任务。
- [ ] 上传中请求关闭后重复点击关闭：窗口不会提前退出。
- [ ] 窗口关闭后移动模型并重新切片：不再出现源 GCode 被占用错误（关联 Bug `17431`）。
- [ ] 上传完成与取消竞态：只产生一个终态和一次结果上报。
- [ ] Klipper4408 正常局域网发送行为与修复前一致。
- [ ] Klipper4408 上传中关闭：返回一次 `601`，随后可以重新发送。
- [ ] Klipper4408 限速卡在 `1%`：取消不再等待约 `75` 秒的底层超时。
- [ ] 两台局域网设备并发发送：取消其中一台不影响另一台。
- [ ] Moonraker/Klipper 正常发送、取消及取消后重发。
- [ ] FTP 局域网上传中取消：返回一次 `601`，文件句柄及时释放。
- [ ] 应用退出时存在运行任务：无崩溃、无悬垂访问。

## 9. 关联提交与调查记录
- `35d64e2c21`：历史修复，解决发送页回调阻塞和窗口无法关闭问题。
- `d08bfc2970`：本次修改工作区基线。
- 禅道历史备注：
  - `webview进程关不掉。`
  - `之前的lambda表达式回调方式有问题。一起重构下。`
  - 关闭窗口阻塞等待回调；不等待会崩溃。
  - 关联修改后可能出现新的 Bug `17431`、`17432`。

## 10. 回滚与风险
- 回滚方式：
  - 回退 `RemotePrinterManager` 任务状态、taskId 和取消 deadline 改造。
  - 回退 `UploadFile` 原子取消令牌及云端请求取消检查。
  - 恢复各协议接口原签名和发送页按 IP 取消逻辑。
- 风险等级：`中高`，涉及发送任务调度、WAN/LAN 上传和应用退出回收路径。
- 重点风险：
  - 无文件阶段允许底层请求短暂后台清理，必须依赖任务终态和 generation 持续过滤所有迟到回调。
  - 文件占用阶段若第三方网络库未响应中断，窗口会继续保持“取消中”，不会在文件仍被占用时伪装为取消成功。
  - 正常请求仍由 libcurl 关闭 socket；只有取消请求由取消线程关闭并转移句柄所有权。
  - DNS 阶段取消时请求可能短暂存活至系统解析或请求超时，但不持有上传文件；后续发送通过有界工作池避免排队在旧请求之后。
  - 云端请求新增总超时后，极慢网络会更早返回失败，需要关注错误码和重试体验。
  - 取消时不再同步调用 OSS multipart 中止请求，需要依赖云端生命周期策略清理未完成分片。
  - 多机发送通过兼容接口接入新任务模型，需要重点回归并发与单设备取消。
  - 本次 `libslic3r_gui` 完整目标已构建通过；仍需在真实设备和弱网环境验证各协议的实际取消延迟。

## 11. 后续建议
- 为 `RemotePrinterManager` 增加可注入协议适配器，补充任务状态机自动化单元测试。
- 增加 `cancel_requested -> local_transfer_stopped` 耗时监控，分别统计压缩、HTTP、OSS 和 FTP 阶段。
- 根据线上弱网数据调整连接超时、请求超时和 cURL 低速超时。
- 将设备列表、发送页和简易发送流程逐步统一为显式保存并取消 `taskId`，最终弱化按 IP 取消兼容接口。
