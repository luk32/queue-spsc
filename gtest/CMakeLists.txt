include(GoogleTest)
add_executable(correctness ./queue-correctness.cpp)
target_link_libraries(correctness GTest::gtest GTest::gmock GTest::gtest_main)
gtest_discover_tests(correctness
    # set a working directory to your project root so that you can find test data via paths relative to the project root
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
set_target_properties(correctness PROPERTIES FOLDER gtest)