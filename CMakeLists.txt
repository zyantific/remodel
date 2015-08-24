cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0054 NEW)
project(remodel_test)

option(REMODEL_TESTING "Build all tests." OFF)
set(REMODEL_ZYCORE_ROOT "dependencies/zycore" CACHE STRING
	"ZyCore library root directory.")
set(REMODEL_ZYCORE_BIN_DIR CACHE STRING
	"ZyCore binary directory (only required for out-of-source builds).")

set(ZYCORE_HEADER_ONLY TRUE CACHE BOOL "")
add_subdirectory(${REMODEL_ZYCORE_ROOT} ${REMODEL_ZYCORE_BIN_DIR})

add_library(remodel INTERFACE)
target_include_directories(remodel INTERFACE include/)
target_link_libraries(remodel INTERFACE Zycore)

if (REMODEL_TESTING)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		foreach (flag_var
	    		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	     	string(REPLACE "/MD" "-MT" ${flag_var} "${${flag_var}}")
	     	string(REPLACE "/W3" "-W4" ${flag_var} "${${flag_var}}")
	 	endforeach ()
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14" CACHE STRING "" FORCE)
	endif ()

	enable_testing()

	add_subdirectory(testing/gtest-1.7.0)
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

	add_executable(remodel_run_unittests testing/test.cpp)
	target_link_libraries(remodel_run_unittests gtest gtest_main remodel)

	if (UNIX)
		target_link_libraries(remodel_run_unittests ${CMAKE_DL_LIBS})
	endif ()

	add_test(remodel-unittests remodel_run_unittests)
endif ()