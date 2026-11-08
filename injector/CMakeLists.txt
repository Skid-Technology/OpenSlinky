cmake_minimum_required(VERSION 3.20)
project(slinky-inject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(slinky-inject
    main.cpp
    tui.cpp
)

if(WIN32)
    target_sources(slinky-inject PRIVATE inject_windows.cpp)
    target_compile_definitions(slinky-inject PRIVATE _WIN32_WINNT=0x0601)
    # A standalone .exe: no MinGW runtime DLLs to copy beside it.
    target_link_options(slinky-inject PRIVATE -static -static-libgcc -static-libstdc++)
else()
    target_sources(slinky-inject PRIVATE inject_linux.cpp)
    target_link_libraries(slinky-inject PRIVATE ${CMAKE_DL_LIBS} pthread)
endif()

# Copy the client library next to the injector when it is built in the same
# tree (the top-level CMakeLists.txt does this via add_subdirectory).
# A separate always-run target: a POST_BUILD step on slinky-inject only ran when
# the injector itself was relinked, so the sibling copy went stale whenever only
# the client changed.
if(TARGET slinky)
    add_custom_target(slinky-copy-beside-injector ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:slinky>
                $<TARGET_FILE_DIR:slinky-inject>/$<TARGET_FILE_NAME:slinky>
        DEPENDS slinky slinky-inject
        VERBATIM)
endif()
