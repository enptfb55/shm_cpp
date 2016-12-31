cmake_minimum_required(VERSION 3.5.0)
project(shm_cpp)

set(WARNING_FLAGS "-Wall -Wextra -Wfatal-errors -Werror")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} -std=c++14 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_EXE_LINKER_FLAGS "-g")

find_package(Threads REQUIRED)
enable_testing()
find_package(GTest REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(TARGETS segment_example
            spsc_queue_example)

foreach(TARGET ${TARGETS})
    add_executable(${TARGET} ${CMAKE_SOURCE_DIR}/src/${TARGET}.cpp)

    if(THREADS_HAVE_PTHREAD_ARG)
        set_property(TARGET ${TARGET} PROPERTY COMPILE_OPTIONS "-pthread")
        set_property(TARGET ${TARGET} PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
    endif()
    if(CMAKE_THREAD_LIBS_INIT)
        target_link_libraries(${TARGET} "${CMAKE_THREAD_LIBS_INIT}")
    endif()

    include_directories(${GTEST_INCLUDE_DIRS})

    target_link_libraries(${TARGET} ${GTEST_BOTH_LIBRARIES})
    target_link_libraries(${TARGET} "rt")
endforeach()

