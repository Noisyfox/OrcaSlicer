#pragma once

namespace wxInspector {
class wxInspectorPlugin;
void RegisterPlugin(wxInspectorPlugin* plugin);
}

// Forward declare our plugins
class DPIAwarePlugin;
class CustomWidgetsPlugin;

inline void RegisterOrcaInspectorPlugins()
{
    static DPIAwarePlugin          dpiaware;
    static CustomWidgetsPlugin     customWidgets;
    wxInspector::RegisterPlugin(&dpiaware);
    wxInspector::RegisterPlugin(&customWidgets);
}
