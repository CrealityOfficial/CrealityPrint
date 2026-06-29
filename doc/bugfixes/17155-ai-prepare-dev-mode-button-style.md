# 17155 AI版准备页测试模式按钮字体修复

## 1. 基本信息
- Bug ID: 17155
- 标题: 【AI版】狂点准备页进入测试模式后，准备页字体错误
- 反馈人: 未提供
- 处理人: Codex
- 影响模块/影响文件: 顶部页签按钮，`src/slic3r/GUI/BBLTopbar.cpp`

## 2. 现象与复现
- 复现场景: 在 AI 版准备页连续点击“准备”页签，触发隐藏 CxAgent dev/test 模式切换。
- 实际结果: 准备页页签按钮在进入测试模式后文字颜色被特殊切换，导致按钮字体显示异常。
- 期望结果: 隐藏点击仍可切换 CxAgent dev 环境，但顶部页签按钮样式保持原有选中/未选中显示效果。

## 3. 责任提交追溯
- commit hash: 34106c396d2721beaf7d7235cd207b127ec003d9
- Author: hemiao
- AuthorDate: 2026-05-23 11:32:16 +0800
- Subject 原文: Add hidden CxAgent dev switch
- Change-Id: I80fed778d53128abac9329996f735c897463b258

## 4. 根因分析
- 触发条件: 非 Release 版本下连续点击 `Prepare` 页签达到隐藏开关阈值，`ButtonsCtrl::SetDevMode()` 将 `m_dev_mode` 置为 true。
- 代码链路: 隐藏开关切换 dev 环境后调用 `SetDevMode()`，随后 `ApplyButtonStyle()` 对 `MainFrame::tp3DEditor` 单独设置一套 dev 文字颜色。
- 为什么会出现该现象: dev 模式状态同时承担了“切换 CxAgent API 环境”和“改变 Prepare 按钮视觉样式”两个职责。测试模式只需要切换后端地址，不应改变页签按钮样式；特殊文字颜色覆盖了原有选中态/普通态颜色，导致字体表现异常。

## 5. 修复方案
- 修复思路: 保留隐藏 dev/test 模式开关及 `cxagent-dev` 地址切换逻辑，移除 dev 模式对 Prepare 按钮文字颜色的特殊覆盖。
- 修改点: `src/slic3r/GUI/BBLTopbar.cpp` 中 `ButtonsCtrl::ApplyButtonStyle()` 始终使用 `DefaultTextColor(selected)`。
- 为什么这样改: 按钮外观应只由页签选中状态和主题决定，dev 模式只作为 CxAgent 环境状态，不再参与 UI 样式切换。

## 6. 影响范围与风险
- 正向影响: 狂点准备页进入测试模式后，准备页页签字体保持正常显示。
- 可能风险: 原先 dev 模式没有额外视觉提示，测试人员需要通过日志或配置确认当前 CxAgent API 地址。
- 是否改变旧行为: 仅取消隐藏 dev 模式下 Prepare 按钮的特殊字体颜色；连续点击切换 dev/prod API 的行为不变。

## 7. 回归建议
- 必测场景: 非 Release 版本连续点击“准备”页签进入 dev/test 模式，确认按钮字体颜色与普通准备页一致，且 CxAgent API 切换到 `https://cxagent-dev.crealitycloud.cn`。
- 边界场景: 在深色/浅色主题下分别进入和退出 dev/test 模式，确认 Prepare、Preview、Device 等页签选中/未选中样式正常。
- 反向场景: Release 版本不应启用隐藏 dev 开关；普通点击页签切换不应改变 CxAgent API 地址。
