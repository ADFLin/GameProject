---
type: implementation_plan
title: Decouple MainMenuStage Dependencies
---

## Problem
`WorkspaceRenderer` and `SidebarNavigator` currently depend on `MainMenuStage.h` to access shared constants (`UI_SCROLLBAR`, `ViewMode`, etc.). This creates a circular dependency or tight coupling, where helper classes depend on the main controller class.

## Solution
Extract shared constants and enums into a new header file `MainMenuConstants.h`. Update all classes to reference this common header instead of linking back to `MainMenuStage`.

## Steps

### 1. Create `Stage/MainMenuConstants.h`
This file will contain:
- `MainMenuViewMode` enum (formerly `MainMenuStage::ViewMode`).
- `MainMenuUIID` namespace with all UI IDs (formerly `MainMenuStage` anonymous enum).
- It will include `GameMenuStage.h` to ensure UI ID continuity.

### 2. Refactor `MainMenuStage.h`
- Include `MainMenuConstants.h`.
- Remove `ViewMode` definition.
- Remove UI ID enum definitions.
- Update member variables to use `MainMenuViewMode`.

### 3. Refactor `WorkspaceRenderer.cpp` & `SidebarNavigator.cpp`
- Replace `#include "MainMenuStage.h"` with `#include "MainMenuConstants.h"`.
- Update usage of `MainMenuStage::ViewMode` to `MainMenuViewMode`.
- Update usage of `MainMenuStage::UI_` constants to `MainMenuUIID::UI_`.

## Benefits
- Breaks circular reference (WorkspaceRenderer -> MainMenuStage -> WorkspaceRenderer).
- Improves compile times by reducing header dependencies.
- Clearer separation of concerns (Interface Definitions vs Implementation).
