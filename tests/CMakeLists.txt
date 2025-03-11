# Google Test
find_package(GTest REQUIRED)

# Physics tests
add_executable(physics_tests
    unit/physics/MaterialTests.cpp
    unit/physics/CellTests.cpp
)

target_link_libraries(physics_tests
    PRIVATE
    astral_physics
    GTest::GTest
    GTest::Main
)

add_test(NAME physics_tests COMMAND physics_tests)

# Core tests
add_executable(core_tests
    unit/core/TimerTests.cpp
    unit/core/ConfigTests.cpp
)

target_link_libraries(core_tests
    PRIVATE
    astral_core
    GTest::GTest
    GTest::Main
)

add_test(NAME core_tests COMMAND core_tests)