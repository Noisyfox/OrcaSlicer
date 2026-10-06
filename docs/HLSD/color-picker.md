# Color picker

The color picker is a local HTML dialog hosted by `ColorPickerDialog`, a
`WebViewHostDialog` subclass. Its colors use the existing `ColorRGBA` utility.
A solid selection contains one color; a gradient contains exactly two ordered
endpoints. The bridge represents these as `{"type":"solid|gradient",
"colors":["#RRGGBBAA", ...]}`. RGB input is also accepted and becomes opaque.
Native validation rejects malformed hex and incorrect cardinality.

The project filament color action and the official filament picker's More Colors
action enable two-endpoint gradients and disable alpha. They read project colors
in their stored vector order and apply changes through `sync_colour_config`,
retaining the existing RGB profile/project format and dirty marking. Ordered
custom results bypass the official palette's `FilamentColor` set. Choosing an
official swatch retains its existing palette behavior.

Striped colors and gradients with more than two endpoints cannot be represented
by this editor. A notice explains that the current selection is retained; the
editor starts from the first color, and unchanged confirmation is a no-op.
Cancellation is also a no-op. The system color picker is used only when the
WebView backend is unavailable.

Native gettext strings are installed at document start. Static page labels and
palette names use stable English keys; RAL codes, channel symbols, and numeric
labels remain data. The palette color values do not depend on the UI language.

Gradient and alpha capabilities are independent and default off. Unsupported
gradients are rejected; disabled alpha normalizes the selected result to opaque.
Favorites retain both capabilities independently of the current caller. The page
stores alpha as a byte and only converts to percentage when displaying or editing
opacity, so switching endpoints does not change alpha precision.

The page starts disabled. Each document supplies a page identifier in its ready
message, receives native initialization, and acknowledges it before native code
accepts confirmation or favorite updates. Repeated initialization does not reset
edits, and messages from an obsolete document are ignored. Native WebView message
handling is deferred and guarded by a lifetime token. Only a validated confirmation
sets the optional result. Cancel, Escape and native window close leave it empty.

Favorites belong to application configuration, separately from the selected result,
in the editor configuration returned by `AppConfig::config_path()` under `data_dir()`.
The `color_picker` section has string `version` equal to `1` and a `favorites`
string containing a JSON array of typed selections. Native helpers normalize and
deduplicate this array in order and limit it to 24 entries. Updates replace the
entire section and immediately call `AppConfig::save()`, so accepted favorite
changes survive cancellation. Empty arrays are saved explicitly. Core `AppConfig`
only stores the raw section; color validation belongs to the GUI boundary.

When the section is absent, valid solid colors from `custom_color_list` are imported
once, without changing that legacy section. The legacy native picker saves decimal
`r,g,b,a` byte strings; hexadecimal RGB and RGBA strings are also supported. Migration
strictly validates all four decimal channels and preserves alpha. Gradient strings
and malformed values are ignored. The new version and an empty array are
saved even when there is nothing to import. A present section with an unknown or
missing version is left untouched and disables favorite writes. Malformed version
1 data is displayed as empty and remains on disk until the user explicitly saves
a valid replacement. Other configuration sections and project/profile color
formats are independent of this storage.

Interface colors consume the host's `data-orca-theme` and `--orca-*` contract through
shared styles. The picker opts into scoped global control roles. Application theme
changes update UI surfaces without reloading the page or changing color data.
