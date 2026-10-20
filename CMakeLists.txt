cmake_minimum_required(VERSION 3.16)
project(wave-effect VERSION 0.1.0 LANGUAGES CXX)

find_package(SDL2 REQUIRED CONFIG)

add_executable(wave-effect main.cpp)
target_compile_features(wave-effect PRIVATE cxx_std_17)
set_target_properties(wave-effect PROPERTIES CXX_EXTENSIONS OFF)
if(TARGET SDL2::SDL2main)
    target_link_libraries(wave-effect PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(wave-effect PRIVATE SDL2::SDL2)

if(MSVC)
    target_compile_options(wave-effect PRIVATE /W4)
else()
    target_compile_options(wave-effect PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME headless-smoke COMMAND wave-effect --frames 5)
    set_tests_properties(headless-smoke PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
        TIMEOUT 15)
endif()
