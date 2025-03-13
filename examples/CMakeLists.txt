# Examples
message(STATUS "Configuring examples")

# Simple console-based test example that doesn't require additional libs
add_executable(test_physics test_physics.cpp)
target_link_libraries(test_physics PRIVATE astral_core astral_physics)
set_target_properties(test_physics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Large world test for chunk-based physics
add_executable(large_world_test large_world_test.cpp)
target_link_libraries(large_world_test PRIVATE astral_core astral_physics)
set_target_properties(large_world_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Material interactions test
add_executable(material_interactions_test material_interactions_test.cpp)
target_link_libraries(material_interactions_test PRIVATE astral_core astral_physics)
set_target_properties(material_interactions_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Water simulation test
add_executable(water_simulation_test water_simulation_test.cpp)
target_link_libraries(water_simulation_test PRIVATE astral_core astral_physics)
set_target_properties(water_simulation_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Cellular fluid dynamics test (with water and gas physics)
# Now with OpenGL rendering option
add_executable(cellular_fluid_test cellular_fluid_test.cpp)
target_link_libraries(cellular_fluid_test PRIVATE 
    astral_core 
    astral_physics
)
set_target_properties(cellular_fluid_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Lava interactions test
add_executable(lava_interactions_test lava_interactions_test.cpp)
target_link_libraries(lava_interactions_test PRIVATE astral_core astral_physics)
set_target_properties(lava_interactions_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# OpenGL-based physics sandbox example
# Make sure OpenGL and GLFW are available
find_package(OpenGL)
find_package(glfw3)

if(OpenGL_FOUND AND glfw3_FOUND)
    message(STATUS "OpenGL and GLFW found - building graphics examples")
    
    # Build GLAD as a separate library
    add_library(glad STATIC ${CMAKE_SOURCE_DIR}/src/glad/glad.c)
    set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
    target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
    
    # Physics sandbox example
    add_executable(physics_sandbox_example physics_sandbox_example.cpp)
    target_include_directories(physics_sandbox_example PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(physics_sandbox_example PRIVATE 
        astral_core 
        astral_physics
        glad
        OpenGL::GL
        glfw
    )
    set_target_properties(physics_sandbox_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        
    # Also update cellular_fluid_test to use OpenGL
    target_compile_definitions(cellular_fluid_test PRIVATE USE_OPENGL)
    target_include_directories(cellular_fluid_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(cellular_fluid_test PRIVATE 
        astral_core 
        astral_physics
        glad
        OpenGL::GL
        glfw
    )
else()
    message(STATUS "Skipping OpenGL examples - OpenGL or GLFW not found")
endif()

# Create a Visual Studio filter for examples
if(MSVC)
    set_property(TARGET test_physics PROPERTY FOLDER "Examples")
    set_property(TARGET large_world_test PROPERTY FOLDER "Examples")
    set_property(TARGET material_interactions_test PROPERTY FOLDER "Examples")
    set_property(TARGET water_simulation_test PROPERTY FOLDER "Examples")
    set_property(TARGET cellular_fluid_test PROPERTY FOLDER "Examples")
    set_property(TARGET lava_interactions_test PROPERTY FOLDER "Examples")
endif()