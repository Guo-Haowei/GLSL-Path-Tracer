# lua
add_library(lua
    ${PROJECT_SOURCE_DIR}/third_party/lua/lapi.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lauxlib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lbaselib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lcode.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lcorolib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lctype.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ldblib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ldebug.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ldo.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ldump.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lfunc.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lgc.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/linit.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/liolib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/llex.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lmathlib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lmem.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/loadlib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lobject.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lopcodes.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/loslib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lparser.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lstate.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lstring.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lstrlib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ltablib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ltable.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/ltm.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lundump.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lutf8lib.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lvm.c
    ${PROJECT_SOURCE_DIR}/third_party/lua/lzio.c
)

add_subdirectory(universal)

add_executable(glsl-path-tracer
    application.cpp
    camera.cpp
    com_file.cpp
    com_misc.cpp
    glutil.cpp
    image.cpp
    imgui_impl_glfw.cpp
    imgui_impl_opengl3.cpp
    main.cpp
    renderer.cpp
    scene_loader.cpp
    scene.cpp
    viewer.cpp
    utility/clock.cpp
    utility/string_util.cpp
    geomath/bvh.cpp
    geomath/geometry.cpp
    ${PROJECT_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/third_party/imgui/imgui.cpp
)

target_include_directories(glsl-path-tracer PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/glad/include/
)

target_link_libraries(glsl-path-tracer PRIVATE
    lua
    universal
    glfw
    glad
)

target_compile_definitions(glsl-path-tracer PRIVATE
    -DDATA_DIR="${PROJECT_SOURCE_DIR}/data/"
)
