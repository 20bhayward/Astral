# Core library
add_library(astral_core
    core/Engine.cpp
    core/Timer.cpp
    core/Config.cpp
    core/Logger.cpp
    core/Profiler.cpp
)

target_include_directories(astral_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
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
)

target_include_directories(astral_physics PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(astral_physics
    PUBLIC
    astral_core
    glm::glm
)

# Rendering library
add_library(astral_rendering
    rendering/Renderer.cpp
    rendering/ShaderProgram.cpp
    rendering/Camera.cpp
    rendering/GridRenderer.cpp
)

target_include_directories(astral_rendering PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(astral_rendering
    PUBLIC
    astral_core
    astral_physics
    ${OPENGL_LIBRARIES}
    glfw
    glm::glm
)

# Tools library
add_library(astral_tools
    tools/DebugUI.cpp
)

target_include_directories(astral_tools PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Check if ImGui was found, otherwise provide a warning
if (TARGET imgui::imgui)
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

# Main application
add_executable(astral_app
    main.cpp
)

target_link_libraries(astral_app
    PRIVATE
    astral_core
    astral_physics
    astral_rendering
    astral_tools
)

# Install targets
install(TARGETS 
    astral_core 
    astral_physics 
    astral_rendering 
    astral_tools
    EXPORT AstralTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(TARGETS astral_app
    RUNTIME DESTINATION bin
)