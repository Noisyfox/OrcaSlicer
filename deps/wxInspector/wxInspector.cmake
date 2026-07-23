orcaslicer_add_cmake_project(
    wxInspector
    GIT_REPOSITORY https://github.com/Noisyfox/wxInspector.git
    GIT_TAG        e27ed8eca4e68cd1e64e20328a9706d97668eb65 
    DEPENDS dep_wxWidgets
    CMAKE_ARGS
        -DCMAKE_CXX_FLAGS="-DwxDEBUG_LEVEL=0"
)

if (MSVC)
    add_debug_dep(dep_wxInspector)
endif ()
