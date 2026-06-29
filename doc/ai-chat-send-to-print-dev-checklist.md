# AI Chat Send To Print Dev Checklist

## Overview

This document breaks the "send to printer" optimization into implementation tasks by module and file.

Goal:

- Treat user input like `发送打印` as explicit authorization to start the send-to-print flow.
- Let the system automatically judge digital preconditions.
- Keep only one meaningful physical safety confirmation.
- Support automatic recovery chains such as `auto_arrange -> run_slice -> send_to_printer`.

Out of scope:

- Vision-based foreign object detection
- Session-level "do not remind again"
- Multi-printer scheduling
- Multi-plate intelligent selection

## Target Behavior

Expected flow:

1. User sends `发送打印`
2. Agent checks scene, slice, and printer facts
3. If recoverable, system auto-fixes and continues
4. If user action is required, system waits and guides
5. If only physical safety is unresolved, show one confirmation
6. After confirmation, commit the send-to-printer action

## Task List

### 1. Agent Contract Alignment

Files:

- `../CxAgent/server/app/domain/modules/tool_contract_registry.py`
- `../CxAgent/server/app/domain/modules/execution_module.py`
- `../CxAgent/server/app/domain/services/orchestrator.py`

Changes:

- Unify the meaning of `send_to_printer.requires_confirmation`
- Redefine confirmation as `physical safety confirmation only`
- Keep digital prerequisites under facts, not user confirmation
- Make the orchestration distinguish:
  - `WAIT_USER_ACTION`
  - `WAIT_SAFETY_CONFIRM`
  - `WAIT_TOOL_RESULT`
- Prevent retry loops when the same blocking issue repeats

Acceptance:

- `send_to_printer` no longer means "ask again whether user wants to print"
- Missing slice result automatically remediates through `run_slice`
- Scene blocking issues can branch into `auto_arrange` or `open_filament_mapping`
- When all digital facts are satisfied, the task enters `WAIT_SAFETY_CONFIRM` instead of generic confirmation

Suggested reason codes:

- `PHYSICAL_SAFETY_CONFIRM_REQUIRED`
- `NO_MODEL_LOADED`
- `NO_PRINTABLE_INSTANCES`
- `SLICE_REQUIRED`
- `GCODE_REQUIRED`
- `SCENE_BLOCKED`
- `PRINTER_OFFLINE`
- `PRINTER_BUSY`

### 2. Chat Frontend Flow

Files:

- `resources/web/chat/chat.js`
- `resources/web/chat/index.html`

Changes:

- Add send-to-print execution states in chat UI
- Render a dedicated safety confirmation card
- Ensure the post-slice continuation path still resumes `send_to_printer`
- Avoid duplicate confirmation cards for the same task
- Remove any logic that treats "plate has content" as a user confirmation problem

Suggested frontend state additions:

- `pendingSafetyConfirmTaskId`
- `pendingSafetyConfirmPayload`
- `sendPrintExecutionState`

Suggested UI states:

- `CHECKING`
- `AUTO_FIXING`
- `WAIT_USER_ACTION`
- `WAIT_SAFETY_CONFIRM`
- `SENDING`
- `SUCCESS`
- `FAILED`

Acceptance:

- User sees clear progress messages during arrange, slice, and send
- Safety confirmation appears exactly once
- Confirming from chat continues the same task
- Canceling from chat ends the task cleanly

Suggested confirmation copy:

`发送前请确认打印平台无异物，设备现场安全。确认后将立即开始打印。`

### 3. C++ Chat Bridge State Exposure

Files:

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

Changes:

- Keep send-to-print facts complete and stable
- Ensure the tool result shape is consistent for send success and send failure
- Support passing confirmed safety intent into the downstream send action if needed

Facts that must remain available:

- `project.has_model`
- `plate.current.has_printable_instances`
- `plate.current.slice_ready_for_print`
- `plate.current.gcode_available`
- `scene.no_blocking_errors`
- `device.current.valid`
- `device.current.online`
- `device.current.idle`

Acceptance:

- Agent always receives the necessary facts for send-to-print decisions
- Failures return structured `code`, `message`, and `details`

### 4. Bridge Tool Schema Alignment

Files:

- `src/slic3r/GUI/simple/bridge/CxAgentClientBridge.cpp`
- `src/slic3r/GUI/simple/bridge/SlicerBridgeActionRegistry.cpp`

Changes:

- Align `send_to_printer` schema with the server contract
- Remove the current semantic mismatch where client-side metadata says no confirmation while server-side contract requires confirmation
- If needed, add a parameter that indicates chat-level safety confirmation has already happened

Suggested optional params:

- `safety_confirmed`
- `skip_local_confirmation`

Acceptance:

- Server and client expose the same confirmation meaning
- Tool metadata no longer conflicts across layers

### 5. Send Execution Refactor

Files:

- `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp`
- `src/slic3r/GUI/Plater.cpp`

Changes:

- Refactor `DoSendToPrinter` to keep only digital fallback validation
- Do not use bottom-layer modal confirmation for digital state questions
- Split physical safety confirmation from actual send commit
- Allow the actual send action to proceed directly when safety has already been confirmed in chat

Recommended implementation direction:

- In `Plater.cpp`, separate:
  - safety confirmation
  - actual send commit
- Keep non-chat entry behavior unchanged where possible
- Add a path for chat-triggered send that skips repeated local confirmation

Acceptance:

- Chat-confirmed send does not trigger another identical native confirmation
- Non-chat send flow still works
- Physical safety protection is preserved

### 6. Test Coverage

Files:

- `../CxAgent/server/tests/*`
- Frontend test files if available

Add tests for:

- direct send when all facts are ready
- `run_slice -> send_to_printer`
- `auto_arrange -> run_slice -> send_to_printer`
- wait for filament mapping
- printer offline
- printer busy
- no model
- no printable instances
- one-time safety confirmation
- no duplicate confirmation across layers

Acceptance:

- The happy path is covered end to end
- Blocking branches have explicit tests
- Contract consistency is asserted in tests

## Delivery Order

Recommended implementation order:

1. Align contract semantics in Agent and bridge metadata
2. Add `WAIT_SAFETY_CONFIRM` state and orchestration handling
3. Build chat safety confirmation UI
4. Refactor bottom-layer send execution to avoid duplicate confirmation
5. Add regression tests

## Joint Debug Checklist

- Input `发送打印` with no model
- Input `发送打印` with model but no slice result
- Input `发送打印` with out-of-bounds layout
- Input `发送打印` with missing filament mapping
- Input `发送打印` with offline printer
- Input `发送打印` with all prerequisites ready
- Verify only one safety confirmation appears
- Verify chat confirmation does not trigger a second native confirmation

## Definition of Done

- The system no longer asks the user whether the plate has printable content
- Digital prerequisites are handled automatically by facts and remediation
- Only one physical safety confirmation remains
- Confirmation semantics are consistent across server, frontend, bridge, and send execution
