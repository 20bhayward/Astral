# Core library
add_library(astral_core
    core/Engine.cpp
    core/Timer.cpp
    core/Config.cpp
    core/Logger.cpp
    core/Profiler.cpp
)

target_include_directories(astral_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(astral_core
    PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Physics library
add_library(astral_physics
    physics/Material.cpp
    physics/Cell.cpp
    physics/ChunkManager.cpp
    physics/CellularPhysics.cpp
    physics/CellularAutomaton.cpp
)

target_include_directories(astral_physics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(astral_physics
    PUBLIC
    astral_core
    glm::glm
)

# Disable rendering and tools for now to simplify build
set(BUILD_RENDERING FALSE)
set(BUILD_TOOLS FALSE)

# Check if we can build the tools library
if(NOT TARGET glfw)
    message(WARNING "GLFW not found - tools library will not be built")
    set(BUILD_TOOLS FALSE)
endif()

if(BUILD_TOOLS)
    # Tools library
    add_library(astral_tools
        tools/DebugUI.cpp
    )

    target_include_directories(astral_tools PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    # Check if ImGui was found, otherwise provide a warning
    if(TARGET imgui::imgui)
        target_link_libraries(astral_tools
            PUBLIC
            astral_core
            imgui::imgui
            glfw
        )
    else()
        message(WARNING "ImGui not found, debug UI will be disabled")
        target_compile_definitions(astral_tools PRIVATE ASTRAL_NO_IMGUI)
        target_link_libraries(astral_tools
            PUBLIC
            astral_core
            glfw
        )
    endif()
endif()

# Main application
add_executable(astral_app
    main.cpp
)

# Link with core and physics (always available)
target_link_libraries(astral_app
    PRIVATE
    astral_core
    astral_physics
)

# Link with rendering if available
if(BUILD_RENDERING)
    target_link_libraries(astral_app
        PRIVATE
        astral_rendering
    )
    target_compile_definitions(astral_app PRIVATE ASTRAL_HAS_RENDERING)
endif()

# Link with tools if available
if(BUILD_TOOLS)
    target_link_libraries(astral_app
        PRIVATE
        astral_tools
    )
    target_compile_definitions(astral_app PRIVATE ASTRAL_HAS_TOOLS)
endif()

# Define the targets to install
set(INSTALL_TARGETS astral_core astral_physics)

# Add optional components if built
if(BUILD_RENDERING)
    list(APPEND INSTALL_TARGETS astral_rendering)
endif()

if(BUILD_TOOLS)
    list(APPEND INSTALL_TARGETS astral_tools)
endif()

# Install the libraries
install(TARGETS ${INSTALL_TARGETS}
    EXPORT AstralTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install the main application
install(TARGETS astral_app
    RUNTIME DESTINATION bin)