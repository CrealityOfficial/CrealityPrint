# AI Chat Visual Parameter Recommendation Design

## Overview

This document describes how to upgrade the current CxAgent parameter recommendation flow from a text-only recommendation pipeline to a visual-enhanced recommendation pipeline.

The proposed direction is:

- Move `3MF parsing` and `multi-view rendering` to the client side
- Keep `parameter analysis` on the server side
- Reuse the current `RecommendationModule -> recommendation_card -> apply_param_patch / apply_and_slice` main flow
- Treat the visual model as an upstream recommendation engine, not as a replacement for the full recommendation orchestration layer

## Background

Current CxAgent recommendation flow is centered around:

- `IntentRouter`
- `RecommendationModule`
- `LLMGateway.generate_recommendation()`
- `ConstraintEngine.validate_param_patch(...)`
- `recommendation_card`
- downstream execution actions such as:
  - `apply_param_patch`
  - `apply_preset`
  - `apply_and_slice`
  - `apply_slice_and_send_to_printer`

This flow is already well integrated with the chat product experience, but it mainly relies on textual context such as:

- current slice parameters
- presets
- printer model
- filament type
- project context
- scene error text

The prototype in [`support_recommendation.py`](/c:/work/CxAgent/server/scripts/support_recommendation.py) demonstrates a different capability:

- inspect the model visually
- infer structure and category
- detect overhang / support need / likely orientation
- generate more geometry-aware print parameter suggestions

Its weakness is that it is currently an offline standalone script, not a service-ready module.

## Goal

Integrate the useful part of the prototype into CxAgent:

- keep client-side model preprocessing
- keep server-side recommendation generation
- keep existing orchestration and action pipeline
- improve recommendation quality using model images and geometry metadata

Non-goal:

- replacing `RecommendationModule`
- replacing `Orchestrator`
- replacing `ExecutionModule`
- introducing a new standalone recommendation product flow

## Core Decision

Do not replace the current recommendation orchestration layer.

Replace only the recommendation generation engine behind it.

Recommended target architecture:

1. Client prepares visual inputs
2. Client sends model views + geometry metadata in chat context
3. Server runs visual recommendation analysis
4. Server converts visual analysis into the existing recommendation schema
5. Existing `RecommendationModule` continues:
   - validation
   - recommendation card generation
   - follow-up execution actions

## Current State vs Target State

Current state:

- Recommendation is generated mainly from textual and structured slicer context
- Visual information is optional and only weakly used by provider prompts
- No dedicated server-side visual recommendation module exists

Target state:

- Client sends normalized visual context for the current model
- Server has a dedicated `VisualRecommendationModule`
- `RecommendationModule` prefers visual recommendation when model views are available
- Output remains compatible with the current recommendation schema

## Proposed Architecture

### Client Responsibilities

Client side should own:

- 3MF parsing
- geometry extraction
- multi-view rendering
- image encoding
- caching of rendered results if possible

Client should not own:

- final parameter recommendation logic
- recommendation schema generation
- constraint validation
- next-action planning

### Server Responsibilities

Server side should own:

- visual recommendation prompt orchestration
- visual model invocation
- mapping analysis output into recommendation schema
- parameter constraint validation
- recommendation card generation
- follow-up action orchestration

## Input Contract

Client should provide a normalized visual recommendation context.

Recommended schema:

```json
{
  "visual_recommendation_input": {
    "images": {
      "01_front": "base64...",
      "02_back": "base64...",
      "03_left": "base64...",
      "04_right": "base64...",
      "05_top": "base64...",
      "06_bottom": "base64...",
      "07_iso_front_left": "base64...",
      "08_iso_front_right": "base64...",
      "09_iso_back_left": "base64...",
      "10_iso_back_right": "base64..."
    },
    "geometry": {
      "dimensions": [60.0, 31.0, 48.0],
      "volume": 15550.5,
      "face_count": 225154,
      "vertex_count": 112577,
      "is_watertight": true
    },
    "printer_model": "K2 Plus",
    "material": "PLA"
  }
}
```

Minimum required fields:

- `images`
- `geometry.dimensions`
- `geometry.volume`
- `geometry.face_count`

Optional fields:

- `geometry.vertex_count`
- `geometry.is_watertight`
- `printer_model`
- `material`

## Server-Side Module Design

Recommended new module:

- `server/app/domain/modules/visual_recommendation_module.py`

Suggested responsibilities:

1. Accept `visual_recommendation_input`
2. Validate the input shape
3. Call the visual LLM
4. Produce a normalized visual analysis result
5. Convert that result into the current recommendation schema

Suggested public API:

```python
class VisualRecommendationModule:
    async def recommend(
        self,
        message: str,
        context: dict[str, Any],
    ) -> dict[str, Any]:
        ...
```

## Two-Layer Output Design

The prototype currently returns an analysis-style payload.

That is useful, but not directly compatible with CxAgent main flow.

Recommended design:

### Layer 1: Visual Analysis

Internal structured result:

```json
{
  "object_category": "figurine",
  "geometry_traits": ["overhang", "thin_wall"],
  "support_analysis": {
    "enable_support": true,
    "support_type": "tree",
    "support_density": "medium",
    "critical_areas": ["arms", "chin"]
  },
  "orientation_recommendation": "rotate to place the broadest face on the bed",
  "suggested_params": {
    "layer_height": 0.2,
    "wall_loops": 3,
    "sparse_infill_density": 15,
    "sparse_infill_pattern": "Grid",
    "brim_type": "Outer brim",
    "brim_width": 5.0,
    "recommended_filament": "PLA"
  },
  "confidence": 0.84,
  "reasoning": "..."
}
```

### Layer 2: Recommendation Candidate

Mapped into the current CxAgent recommendation schema:

```json
{
  "recommendation_name": "Complex Figurine Stable Print Plan",
  "goal": "Reduce overhang failure risk while keeping good surface quality.",
  "changes": [
    {"key": "enable_support", "from": false, "to": true},
    {"key": "wall_loops", "from": 2, "to": 3},
    {"key": "sparse_infill_density", "from": 10, "to": 15}
  ],
  "reasons": [
    "The model shows clear overhang structures in the image set.",
    "The side and isometric views suggest support is needed under protruding regions."
  ],
  "risks": [
    "More support increases cleanup cost."
  ],
  "requires_confirmation": true
}
```

This second layer is what should be fed into the current `RecommendationModule` flow.

## Integration Strategy

### Recommended Integration Point

Do not bypass `RecommendationModule`.

Instead:

1. Extend `RecommendationModule.recommend()`
2. If `visual_recommendation_input` exists in context:
   - call `VisualRecommendationModule`
   - get a candidate recommendation
3. Run the existing:
   - `_filter_disallowed_changes`
   - `ConstraintEngine.validate_param_patch(...)`
   - `_build_summary(...)`
   - `_build_recommendation_card(...)`

### Fallback Logic

Recommended fallback order:

1. Visual recommendation available and valid
2. Current text-only `generate_recommendation(...)`
3. Existing fallback provider behavior

This ensures the system still works when:

- client has not uploaded model views
- rendering failed
- visual LLM failed
- image input is incomplete

## Why This Is Better Than Direct Replacement

Benefits:

- preserves current product flow
- preserves current card rendering and action semantics
- preserves validation and safety rails
- improves quality for geometry-sensitive recommendations
- allows gradual rollout

Avoided risks:

- no full rewrite of recommendation UX
- no orchestration redesign
- no breakage of downstream execution actions
- no requirement to always run expensive visual analysis

## Mapping Rules

Suggested mapping from prototype fields to current recommendation candidate:

- `layer_height` -> `changes[].key = "layer_height"` only if still allowed by business rules
- `wall_loops` -> `changes[].key = "wall_loops"`
- `sparse_infill_density` -> `changes[].key = "sparse_infill_density"`
- `sparse_infill_pattern` -> `changes[].key = "sparse_infill_pattern"`
- `brim_type` -> `changes[].key = "brim_type"`
- `brim_width` -> `changes[].key = "brim_width"`
- `recommended_filament` -> prefer preset recommendation or a textual suggestion instead of direct patch unless supported safely
- `enable_support` -> `changes[].key = "enable_support"`
- `support_type` -> `changes[].key = "support_type"`
- `support_style` -> `changes[].key = "support_style"`

Recommended handling:

- If a field has safe direct mapping to an existing parameter key, emit a change
- If a field does not have a safe direct mapping, keep it in `reasons` or `risks`
- If a field implies preset choice rather than param patch, convert it into `preset_recommendation`

## Prompt Strategy

The visual recommendation prompt should not directly return a UI card.

It should return analysis first, then normalized recommendation fields.

Recommended prompt responsibilities:

- identify object category
- identify structural traits
- analyze support need and orientation
- suggest only actionable print parameters
- provide confidence
- provide concise reasoning
- avoid unsupported keys

Recommended restriction:

- only recommend parameters that exist in current config schema or approved mapping list

## Validation Strategy

Visual recommendation output must still go through:

- `ConstraintEngine.validate_param_patch(...)`

This is required because visual reasoning may suggest:

- unsupported parameter keys
- invalid parameter values
- printer/material-incompatible values

Validation remains owned by the existing main flow.

## Suggested File-Level Changes

### Server

Files to add:

- `../CxAgent/server/app/domain/modules/visual_recommendation_module.py`

Files to modify:

- `../CxAgent/server/app/domain/modules/recommendation_module.py`
- `../CxAgent/server/app/domain/llm/providers/qwen_provider.py`
- `../CxAgent/server/app/dependencies.py`

### Client

Files likely to modify:

- chat context upload path
- current model view capture flow
- geometry extraction payload assembly

Potential files:

- `resources/web/chat/chat.js`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

## Suggested Rollout Plan

### Phase 1

- Client sends rendered views and geometry metadata
- Server adds `VisualRecommendationModule`
- Recommendation flow prefers visual recommendation when input is present
- No UI redesign

### Phase 2

- Add caching by model hash / plate hash
- Add visual recommendation confidence threshold
- Fall back automatically when confidence is too low

### Phase 3

- Add specialized prompts for object categories:
  - figurine
  - functional part
  - bracket
  - enclosure
  - decorative model

## Recommendation

Recommended final direction:

- client owns preprocessing
- server owns visual parameter analysis
- current `RecommendationModule` remains the orchestration layer
- current `recommendation_card` remains the product output layer

In short:

Do not replace the current CxAgent recommendation workflow.

Replace only the recommendation generation engine with a visual-enhanced engine when model views are available.
