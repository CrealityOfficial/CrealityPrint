# AI Chat Visual Parameter Recommendation Dev Checklist

## Overview

This document breaks the visual-enhanced parameter recommendation solution into implementation tasks by file and module.

Goal:

- Move `3MF parsing` and `multi-view rendering` to the client side
- Keep `parameter analysis` on the server side
- Reuse the current CxAgent recommendation flow
- Upgrade recommendation quality with model-view and geometry-aware analysis

Out of scope:

- replacing `RecommendationModule`
- replacing `Orchestrator`
- redesigning recommendation card UI
- always-on vision processing for every recommendation request

## Target Behavior

Expected flow:

1. User asks for parameter recommendation
2. Client prepares model view images and geometry metadata
3. Client sends visual recommendation context to CxAgent
4. Server runs visual recommendation analysis if visual input is present
5. Server converts the analysis into the current recommendation schema
6. Existing validation and recommendation card flow continues unchanged
7. User can still apply parameters, apply and slice, or apply and send to printer

## Task List

### 1. Client Context Assembly

Files:

- [chat.js](/c:/work/C3DSlicer/resources/web/chat/chat.js)
- [MCPChatPanel.cpp](/c:/work/C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp)

Tasks:

- Add a dedicated visual recommendation payload into the chat request context.
- Reuse or extend existing model-view capture flow so recommendation requests can include rendered model images.
- Add geometry metadata payload fields:
  - `dimensions`
  - `volume`
  - `face_count`
  - `vertex_count`
  - `is_watertight`
- Add visual image payload fields:
  - `01_front`
  - `02_back`
  - `03_left`
  - `04_right`
  - `05_top`
  - `06_bottom`
  - `07_iso_front_left`
  - `08_iso_front_right`
  - `09_iso_back_left`
  - `10_iso_back_right`
- Ensure recommendation requests send the visual context only when available.
- Keep fallback behavior intact when model views are absent.

Implementation notes:

- Prefer adding a top-level context field such as `visual_recommendation_input`.
- Avoid mixing raw render-generation logic into chat display logic.
- If image capture is expensive, gate it behind recommendation-intent requests only.

### 2. Client Geometry Extraction

Files:

- [MCPChatPanel.cpp](/c:/work/C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp)
- [GUI_App.cpp](/c:/work/C3DSlicer/src/slic3r/GUI/GUI_App.cpp)

Tasks:

- Provide a stable bridge path for current-model geometry metadata.
- Gather the normalized geometry fields needed by the server:
  - dimensions
  - volume
  - face count
  - vertex count
  - watertight flag if available
- Return geometry in a JSON structure that can be inserted into CxAgent context.
- Log when geometry extraction succeeds or fails.

Implementation notes:

- Geometry extraction should not block the whole chat flow if some optional values are unavailable.
- Missing optional geometry fields should not break recommendation fallback.

### 3. Visual Recommendation Input Contract

Files:

- [ai-chat-visual-parameter-recommendation-design.md](/c:/work/C3DSlicer/doc/ai-chat-visual-parameter-recommendation-design.md)
- [protocol.py](/c:/work/CxAgent/server/app/domain/models/protocol.py)

Tasks:

- Define the server-side payload schema for `visual_recommendation_input`.
- Document the expected client input shape in code-facing structures.
- Ensure the payload can coexist with existing context fields such as:
  - `project_context`
  - `current_slice_params`
  - `geometry_analysis`
  - `slice_result`

Recommended schema:

- `visual_recommendation_input.images`
- `visual_recommendation_input.geometry`
- `visual_recommendation_input.printer_model`
- `visual_recommendation_input.material`

### 4. New Server Visual Recommendation Module

Files:

- `../CxAgent/server/app/domain/modules/visual_recommendation_module.py`

Tasks:

- Create a new module responsible only for visual recommendation analysis.
- Accept:
  - user message
  - visual recommendation input
  - current print context
- Call the visual model with:
  - images
  - geometry metadata
  - printer model
  - material
  - current recommendation request text
- Return a normalized internal analysis structure.

Suggested responsibilities:

- input validation
- vision prompt construction
- vision model call
- output normalization
- confidence handling

Suggested output shape:

- `object_category`
- `geometry_traits`
- `support_analysis`
- `orientation_recommendation`
- `suggested_params`
- `confidence`
- `reasoning`

### 5. Provider-Level Vision Recommendation Call

Files:

- [qwen_provider.py](/c:/work/CxAgent/server/app/domain/llm/providers/qwen_provider.py)
- [gateway.py](/c:/work/CxAgent/server/app/domain/llm/gateway.py)

Tasks:

- Add a dedicated visual recommendation API path in the provider layer.
- Keep current `generate_recommendation()` intact for text-only fallback.
- Add a new method if needed, for example:
  - `generate_visual_recommendation(message, context)`
- Build multimodal messages from:
  - images
  - geometry
  - printer/material context
  - user request
- Ensure the provider returns structured JSON only.

Implementation notes:

- Do not return UI card payloads directly from the provider.
- Return analysis-oriented structured data first, then map it in module logic.
- Reuse Qwen multimodal capability already used elsewhere.

### 6. Recommendation Schema Mapping

Files:

- `../CxAgent/server/app/domain/modules/visual_recommendation_module.py`
- [recommendation_module.py](/c:/work/CxAgent/server/app/domain/modules/recommendation_module.py)

Tasks:

- Convert visual analysis output into the current recommendation schema:
  - `recommendation_name`
  - `goal`
  - `changes`
  - `reasons`
  - `risks`
  - `requires_confirmation`
- Map direct parameter fields into `changes[]` when safe.
- Preserve non-directly-applicable findings in:
  - `reasons`
  - `risks`
- Avoid introducing unsupported parameter keys.

Recommended mappings:

- `wall_loops` -> patch change
- `sparse_infill_density` -> patch change
- `sparse_infill_pattern` -> patch change
- `brim_type` -> patch change
- `brim_width` -> patch change
- `enable_support` -> patch change if supported safely
- `support_type` -> patch change if supported safely
- `recommended_filament` -> reason or preset recommendation unless directly supported

### 7. RecommendationModule Integration

Files:

- [recommendation_module.py](/c:/work/CxAgent/server/app/domain/modules/recommendation_module.py)

Tasks:

- Update `RecommendationModule.recommend()` to prefer visual recommendation when `visual_recommendation_input` exists.
- Keep the existing scene-error recommendation path unchanged.
- Keep text-only fallback unchanged.
- Run visual recommendation candidate through existing:
  - `_filter_disallowed_changes`
  - `ConstraintEngine.validate_param_patch(...)`
  - `_build_summary(...)`
  - `_build_recommendation_card(...)`

Target decision flow:

1. If scene errors exist -> use current scene error recommendation path
2. Else if visual recommendation input exists -> use visual recommendation path
3. Else -> use current text-only recommendation path

### 8. Constraint Validation

Files:

- [recommendation_module.py](/c:/work/CxAgent/server/app/domain/modules/recommendation_module.py)
- Constraint engine integration path

Tasks:

- Ensure all visual recommendation changes are validated through existing constraint rules.
- Reject or downgrade invalid suggested parameters.
- Keep disallowed keys filtering intact.
- If needed, extend allowlist/denylist handling for support-related fields.

Important:

- Visual confidence does not replace parameter validation.
- Even a high-confidence vision answer must still pass patch validation.

### 9. Dependency Wiring

Files:

- `../CxAgent/server/app/dependencies.py`

Tasks:

- Register and construct `VisualRecommendationModule`.
- Inject it into `RecommendationModule` or the appropriate service layer.
- Ensure provider dependencies are reused consistently with the current LLM gateway.

### 10. Response Payload Compatibility

Files:

- [orchestrator.py](/c:/work/CxAgent/server/app/domain/services/orchestrator.py)
- [protocol.py](/c:/work/CxAgent/server/app/domain/models/protocol.py)

Tasks:

- Ensure the final recommendation response format remains unchanged for the frontend.
- Keep `recommendation_card` payload compatible with existing chat rendering.
- Optionally include non-breaking debug fields such as:
  - `visual_analysis_available`
  - `visual_recommendation_confidence`

Do not:

- change the current card schema in a breaking way
- force frontend rework for the first version

### 11. Logging and Observability

Files:

- `../CxAgent/server/app/domain/modules/visual_recommendation_module.py`
- [recommendation_module.py](/c:/work/CxAgent/server/app/domain/modules/recommendation_module.py)
- [qwen_provider.py](/c:/work/CxAgent/server/app/domain/llm/providers/qwen_provider.py)

Tasks:

- Add structured logs for:
  - visual input detected
  - visual recommendation started
  - visual recommendation fallback triggered
  - confidence threshold result
  - mapping result count
  - validation failures
- Make it easy to distinguish:
  - text-only recommendation
  - visual-enhanced recommendation

Suggested log tags:

- `[recommendation.visual]`
- `[recommendation.visual.mapping]`
- `[recommendation.visual.fallback]`

### 12. Tests

Files:

- `../CxAgent/server/tests/test_recommendation_module.py`
- `../CxAgent/server/tests/test_llm.py`
- new tests for visual recommendation module

Tasks:

- Add tests for visual recommendation input path.
- Add tests that visual input overrides text-only recommendation path when valid.
- Add tests for fallback when visual input is missing or invalid.
- Add tests for schema mapping:
  - visual output -> recommendation candidate
- Add tests for validation:
  - invalid patch values are rejected or flagged
- Add tests for compatibility:
  - recommendation card shape remains unchanged

Suggested core cases:

1. Visual input present and valid -> visual path used
2. Visual input absent -> text path used
3. Visual output contains unsupported key -> filtered out
4. Visual output contains valid param changes -> mapped into `changes`
5. Visual output confidence too low -> fallback to text recommendation

### 13. Optional Caching

Files:

- client-side model hash / render cache path
- server-side optional cache path

Tasks:

- Add optional model-hash-based cache for rendered images on client side.
- Add optional recommendation cache for repeated requests on unchanged models.
- Ensure cache invalidation is tied to:
  - model change
  - plate change
  - transform change if relevant

This is optional for V1 and can be deferred.

## Recommended Implementation Order

1. Define input contract
2. Add client context upload
3. Add `VisualRecommendationModule`
4. Add provider-level visual recommendation call
5. Add schema mapping
6. Integrate into `RecommendationModule`
7. Add tests
8. Add logging
9. Add optional caching later

## Acceptance Criteria

The implementation is complete when:

- recommendation requests can include visual model context
- server can generate visual-enhanced recommendations from model views
- output is converted into the existing recommendation schema
- existing `recommendation_card` and action buttons still work
- current text-only fallback still works
- no breaking change is introduced to the chat response protocol

## Suggested Ticket Split

1. Client: visual recommendation context assembly
2. Client: geometry metadata extraction
3. Server: visual recommendation module
4. Server: provider multimodal recommendation call
5. Server: recommendation schema mapping
6. Server: RecommendationModule integration
7. QA: visual recommendation regression and fallback coverage
