cmake_minimum_required(VERSION 3.20)
project(SmartCarpool LANGUAGES CXX)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_SSL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.1
)
FetchContent_MakeAvailable(Crow)

add_executable(smartcarpool
    backend/src/main.cpp
    backend/src/Graph.cpp
    backend/src/Matching.cpp
    backend/src/CarpoolService.cpp
)
target_include_directories(smartcarpool PRIVATE backend/include)
target_link_libraries(smartcarpool PRIVATE Crow::Crow)
if (WIN32)
    target_link_libraries(smartcarpool PRIVATE ws2_32 mswsock)
endif()

if (BUILD_TESTING)
    add_executable(graph_tests
        backend/tests/GraphTests.cpp
        backend/src/Graph.cpp
        backend/src/Matching.cpp
        backend/src/CarpoolService.cpp
    )
    target_include_directories(graph_tests PRIVATE backend/include)
    add_test(NAME graph_tests COMMAND graph_tests)
endif()
