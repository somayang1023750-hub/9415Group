# Sources are collected by directory; CONFIGURE_DEPENDS re-runs the glob at
# build time, so adding a .cpp file needs no CMake edit.
#   core/, graphics/, world/ -> craft_core (library, also linked by tests)
#   app/, main.cpp   -> Craft (executable, needs ImGui)
file(GLOB_RECURSE CRAFT_CORE_SOURCES CONFIGURE_DEPENDS
    core/*.cpp
    graphics/*.cpp
    world/*.cpp
)
file(GLOB_RECURSE CRAFT_APP_SOURCES CONFIGURE_DEPENDS
    app/*.cpp
)

# SceneBenchmark has no ImGui dependency, so it lives in the library.
set(CRAFT_BENCHMARK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/app/SceneBenchmark.cpp)
list(REMOVE_ITEM CRAFT_APP_SOURCES ${CRAFT_BENCHMARK_SOURCE})

add_library(craft_core STATIC
    ${CRAFT_CORE_SOURCES}
    ${CRAFT_BENCHMARK_SOURCE}
)

target_include_directories(craft_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(craft_core
    PUBLIC
        glm::glm
    PRIVATE
        glfw
        FastNoise2
        stb::headers
)

target_compile_definitions(craft_core PRIVATE
    CRAFT_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets"
)

if(APPLE)
    target_link_libraries(craft_core PRIVATE "-framework OpenGL")
    target_compile_definitions(craft_core PRIVATE GL_SILENCE_DEPRECATION)
else()
    # PUBLIC because the starter's main.cpp includes graphics/OpenGL.h.
    target_link_libraries(craft_core PUBLIC craft_glad_gl_core_33)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(Craft
    main.cpp
    ${CRAFT_APP_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(Craft PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(Craft PRIVATE
    craft_core
    glfw
)

# Keep the existing Pixi command (`build/Craft`) stable after adding src/.
set_target_properties(Craft PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)

# Running from the build directory should work without source-tree paths.
add_custom_command(TARGET Craft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Craft>/assets"
    COMMENT "Copying runtime assets"
)

# Optional report capture tool. Production models/effects are shared, not copied.
option(CRAFT_BUILD_MEMBER_A_SHOWCASE "Build Member A report capture program" OFF)
if(CRAFT_BUILD_MEMBER_A_SHOWCASE)
  add_executable(MemberAShowcase
    ${PROJECT_SOURCE_DIR}/report/member-a/main.cpp
    ${CRAFT_APP_SOURCES} ${IMGUI_SOURCES})
  target_include_directories(MemberAShowcase PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)
  target_link_libraries(MemberAShowcase PRIVATE craft_core glfw stb::headers)
  set_target_properties(MemberAShowcase PROPERTIES
    OUTPUT_NAME MemberATrainingRange
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  # The launcher reads the actual generator/configuration-specific target path.
  file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/member-a-training-$<CONFIG>.txt"
    CONTENT "$<TARGET_FILE:MemberAShowcase>\n")
  add_custom_command(TARGET MemberAShowcase POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets"
      "$<TARGET_FILE_DIR:MemberAShowcase>/assets")
endif()
