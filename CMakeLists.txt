cmake_minimum_required(VERSION 3.9)

cmake_policy(SET CMP0060 NEW)

if (DEFINED ENV{TRAVIS_BUILD_NUMBER})
    project(otus-netbulk VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})
else()
    project(otus-netbulk VERSION 0.0.0)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

option(WITH_SANITIZE_ADDRESS "Enable address sanitizer" OFF)
option(WITH_SANITIZE_THREAD "Enable thread sanitizer" OFF)

add_executable(bulk_server
        src/server-main.cpp
        src/async_impl.cpp)
add_executable(tests src/tests.cpp)

# Google Test
find_package(GTest REQUIRED)
target_include_directories(tests PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${GTEST_LIBRARIES} pthread)

# Poco
find_package(Poco REQUIRED COMPONENTS Foundation Net)

message("Poco_LIBRARIES: ${Poco_LIBRARIES}")

target_link_libraries(bulk_server PRIVATE ${Poco_LIBRARIES})
target_include_directories(bulk_server PRIVATE ${Poco_INCLUDE_DIRS})

target_link_libraries(bulk_server PRIVATE pthread)

#
set_target_properties(bulk_server PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        COMPILE_OPTIONS -Wpedantic -Wall -Wextra)

set_target_properties(tests PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        COMPILE_OPTIONS -Wpedantic -Wall -Wextra)

if (WITH_SANITIZE_ADDRESS)
    set_target_properties(bulk_server PROPERTIES COMPILE_OPTIONS "-fsanitize=address")
    set_target_properties(tests PROPERTIES COMPILE_OPTIONS "-fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if (WITH_SANITIZE_THREAD)
    set_target_properties(bulk_server PROPERTIES COMPILE_OPTIONS "-fsanitize=thread")
    set_target_properties(tests PROPERTIES COMPILE_OPTIONS "-fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

install(TARGETS bulk_server RUNTIME DESTINATION bin)
install(TARGETS tests RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT yukigaru@gmail.com)

include(CPack)

enable_testing()
add_test(test tests)
