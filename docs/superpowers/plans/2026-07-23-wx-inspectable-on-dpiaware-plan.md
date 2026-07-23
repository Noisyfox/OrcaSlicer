# Move `wxInspectable` into `DPIAware` — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `wxInspector::wxInspectable` from individual leaf classes into the common `DPIAware<P>` template so every DPIAware widget is automatically inspectable.

**Architecture:** Three line edits across two files. `DPIAware<P>` gains `wxInspector::wxInspectable` as a second base class; `DPIDialog` and `MainFrame` drop their now-redundant direct inheritance of it.

**Tech Stack:** C++17, wxWidgets, wxInspector

## Global Constraints

- Build with `D:\VisualStudio\2026\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
- Cross-platform: must compile on Windows, macOS, and Linux
- Match existing code style: PascalCase classes, `#pragma once`
- `SetupInspectorAccelerator` stays on top-level windows only (DPIDialog, MainFrame)

---

### Task 1: Move `wxInspectable` into `DPIAware<P>` and clean up leaf classes

**Files:**
- Modify: `src/slic3r/GUI/GUI_Utils.hpp:92` (DPIAware template declaration)
- Modify: `src/slic3r/GUI/GUI_Utils.hpp:276` (DPIDialog class declaration)
- Modify: `src/slic3r/GUI/MainFrame.hpp:96` (MainFrame class declaration)

**Interfaces:**
- Consumes: Nothing (standalone refactor)
- Produces: All DPIAware widgets automatically inherit `wxInspector::wxInspectable`

- [ ] **Step 1: Add `wxInspectable` to `DPIAware<P>`**

In `src/slic3r/GUI/GUI_Utils.hpp`, line 92, change:

```cpp
// Before:
template<class P> class DPIAware : public P
// After:
template<class P> class DPIAware : public P, public wxInspector::wxInspectable
```

(`<wx/inspector/inspector.h>` is already included at line 23.)

- [ ] **Step 2: Remove redundant `wxInspectable` from `DPIDialog`**

In `src/slic3r/GUI/GUI_Utils.hpp`, line 276, change:

```cpp
// Before:
class DPIDialog : public DPIAware<wxDialog>, public wxInspector::wxInspectable
// After:
class DPIDialog : public DPIAware<wxDialog>
```

`DPIDialog` now gets `wxInspectable` through `DPIAware<wxDialog>`. The constructor and `EndModal` override are unchanged.

- [ ] **Step 3: Remove redundant `wxInspectable` from `MainFrame`**

In `src/slic3r/GUI/MainFrame.hpp`, line 96, change:

```cpp
// Before:
class MainFrame : public DPIFrame, public wxInspector::wxInspectable
// After:
class MainFrame : public DPIFrame
```

`MainFrame` now gets `wxInspectable` through `DPIFrame` → `DPIAware<wxFrame>`.

- [ ] **Step 4: Build to verify compilation**

```powershell
$cmakePath = "D:\VisualStudio\2026\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmakePath --build . --config Debug --target ALL_BUILD -- -m
```

Expected: Build succeeds with zero new errors or warnings.

- [ ] **Step 5: Commit**

```bash
git add src/slic3r/GUI/GUI_Utils.hpp src/slic3r/GUI/MainFrame.hpp
git commit -m "refactor: move wxInspectable into DPIAware template

DPIAware<P> now inherits wxInspector::wxInspectable directly, making
all DPIAware widgets automatically appear in the inspector tree.
Remove redundant wxInspectable inheritance from DPIDialog and
MainFrame, which now get it transitively through DPIAware.

Co-Authored-By: Claude <noreply@anthropic.com>"
```
