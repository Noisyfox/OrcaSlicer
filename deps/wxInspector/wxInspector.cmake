orcaslicer_add_cmake_project(
    wxInspector
    GIT_REPOSITORY https://github.com/Noisyfox/wxInspector.git
    GIT_TAG        e4428bf5fadc505e79b2142060652d95413736a2 
    DEPENDS dep_wxWidgets
    CMAKE_ARGS
        -DCMAKE_CXX_FLAGS="-DwxDEBUG_LEVEL=0"
)

if (MSVC)
    add_debug_dep(dep_wxInspector)
endif ()
