enable_testing()
add_subdirectory(gtest-1.7.0)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(remodel_run_unittests test.cpp)
target_link_libraries(remodel_run_unittests gtest gtest_main remodel)

add_test(remodel-unittests remodel_run_unittests)