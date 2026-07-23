# Move `wxInspectable` into `DPIAware` — Design Spec

Date: 2026-07-23
Branch: `dev/layout-inspector`

## Overview

Move the `wxInspector::wxInspectable` base class from individual leaf classes (`DPIDialog`, `MainFrame`) into the common `DPIAware<P>` template. This makes every DPIAware widget automatically visible in the inspector tree without requiring each subclass to opt in.

## Motivation

Currently, only `DPIDialog` and `MainFrame` explicitly inherit `wxInspectable`. `DPIFrame` (which `MainFrame` inherits from) does not — `MainFrame` adds it manually. This means:

- Any `DPIAware<T>` widget that isn't `DPIDialog` or `MainFrame` is invisible in the inspector tree
- `DPIFrame` subclasses (`BaseTransparentDPIFrame`, `ImageDPIFrame`, `ModelMallDialog`, `MediaFileFrame`, `SecondaryCheckDialog`, `PrintErrorDialog`, etc.) don't appear
- Adding a new DPIAware widget type requires remembering to also inherit `wxInspectable`

Moving `wxInspectable` to `DPIAware` fixes this for all current and future DPIAware widgets at once.

## Design

### Change 1: `GUI_Utils.hpp` — `DPIAware<P>`

Add `wxInspector::wxInspectable` as a second base class:

```cpp
// Before:
template<class P> class DPIAware : public P

// After:
template<class P> class DPIAware : public P, public wxInspector::wxInspectable
```

This gives every `DPIAware<T>` widget inspectability automatically. `#include <wx/inspector/inspector.h>` is already present in the file.

### Change 2: `GUI_Utils.hpp` — `DPIDialog`

Remove the now-redundant `wxInspector::wxInspectable`:

```cpp
// Before:
class DPIDialog : public DPIAware<wxDialog>, public wxInspector::wxInspectable

// After:
class DPIDialog : public DPIAware<wxDialog>
```

`DPIDialog` gets `wxInspectable` through `DPIAware<wxDialog>` now. `SetupInspectorAccelerator(this)` stays in the constructor — the accelerator shortcut is a top-level-window concern, not something every DPIAware widget should register.

### Change 3: `MainFrame.hpp` — `MainFrame`

Remove the now-redundant `wxInspector::wxInspectable`:

```cpp
// Before:
class MainFrame : public DPIFrame, public wxInspector::wxInspectable

// After:
class MainFrame : public DPIFrame
```

`MainFrame` gets `wxInspectable` through `DPIFrame` → `DPIAware<wxFrame>`.

## Impact

| Widget | Before | After |
|--------|--------|-------|
| `DPIDialog` subclasses (~80) | ✓ inspectable | ✓ inspectable (transitive) |
| `MainFrame` | ✓ inspectable | ✓ inspectable (transitive) |
| `DPIFrame` subclasses (8 others) | ✗ invisible | ✓ inspectable |
| Future `DPIAware<T>` | ✗ invisible | ✓ inspectable |

## Files Modified

| File | Change |
|------|--------|
| `src/slic3r/GUI/GUI_Utils.hpp` | `DPIAware<P>` gains `wxInspector::wxInspectable`; `DPIDialog` drops redundant `wxInspector::wxInspectable` |
| `src/slic3r/GUI/MainFrame.hpp` | `MainFrame` drops redundant `wxInspector::wxInspectable` |

## Non-Goals

- The `DPIAwarePlugin` detection logic (`dynamic_cast<DPIFrame*>` / `dynamic_cast<DPIDialog*>`) is unchanged
- No new DPI properties — this is purely about tree visibility
- No changes to `SetupInspectorAccelerator` placement — it stays on top-level windows only

## Risk Assessment

- **Multiple inheritance**: `DPIAware<P>` already has a vtable (virtual destructor). Adding `wxInspectable` adds a second base but no additional data members. The `wxInspector::wxInspectable` class is expected to be a lightweight marker interface.
- **Build**: No new includes needed; `<wx/inspector/inspector.h>` is already included in `GUI_Utils.hpp`.
- **Cross-platform**: The change is standard C++ multiple inheritance — no platform-specific concerns.
