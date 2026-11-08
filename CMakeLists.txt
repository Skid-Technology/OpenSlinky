cmake_minimum_required(VERSION 3.20)
project(slinky LANGUAGES C CXX)

# clang-cl + lld-link without a Windows SDK: CMake still wants a manifest tool for the DLL link step. LLVM ships
# llvm-mt, which takes the same arguments as mt.exe; without it the build fails after all objects are compiled.
if(WIN32 AND NOT CMAKE_MT)
    find_program(SLINKY_MT NAMES llvm-mt)
    if(SLINKY_MT)
        set(CMAKE_MT "${SLINKY_MT}")
    endif()
endif()

# The client as one shared library that is loaded into the Minecraft JVM.
#   engine    src/engine     the client's own engine (JNI, JVMTI, GUI, module manager, ...), start-up in protected/
#   logic     src/logic      the module logic (one function per original routine)
#             src/logic/*/*.h  module descriptors' callbacks (GUI, setting registration, status); each one is compiled
#                              on its own (they declare the settings as opaque blobs, the logic .cpp as typed values)
#   data      src/data       .rdata/.data of the original image (strings, tables, module descriptors), generated
#   platform  src/platform   everything that depends on the operating system, NanoVG, GL loader, C runtime bindings
if(NOT WIN32)
    message(FATAL_ERROR "Only the Windows build exists so far. Cross-compile with\n"
        "  cmake -S . -B build/cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-clang-cl.cmake\n"
        "(a Linux platform layer is planned: JVMTI agent, GLFW/X11 input, dlopen)")
endif()

set(CMAKE_CXX_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "" CACHE STRING "" FORCE)

# ---- sources ----
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/engine/*.cpp)
file(GLOB_RECURSE LOGIC_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/logic/*.cpp)
file(GLOB_RECURSE MODULE_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/logic/*.h)
set_source_files_properties(${MODULE_SRC} PROPERTIES LANGUAGE CXX COMPILE_OPTIONS /TP)
file(GLOB PLATFORM_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/platform/*.cpp ${CMAKE_SOURCE_DIR}/src/platform/win/*.cpp)
# stack probe for large frames (clang-cl would take it from the C runtime library)
set(CHKSTK_OBJ ${CMAKE_BINARY_DIR}/chkstk.obj)
# NanoVG core (+ fontstash, stb_truetype, stb_image) and its GL3 backend
set(NANOVG_SRC ${CMAKE_SOURCE_DIR}/third_party/nanovg/src/nanovg.c ${CMAKE_SOURCE_DIR}/src/platform/nanovg_impl.c)

# ---- the original's data, assembled (GNU syntax) ----
find_program(SLINKY_CLANG clang REQUIRED)
set(DATA_OBJ)
foreach(part rdata data iat)
    set(obj ${CMAKE_BINARY_DIR}/data_${part}.obj)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${SLINKY_CLANG} --target=x86_64-pc-windows-msvc -c ${CMAKE_SOURCE_DIR}/src/data/${part}.s -o ${obj}
        DEPENDS ${CMAKE_SOURCE_DIR}/src/data/${part}.s VERBATIM)
    list(APPEND DATA_OBJ ${obj})
endforeach()

add_custom_command(OUTPUT ${CHKSTK_OBJ}
    COMMAND ${SLINKY_CLANG} --target=x86_64-pc-windows-msvc -c ${CMAKE_SOURCE_DIR}/src/platform/win/chkstk.s -o ${CHKSTK_OBJ}
    DEPENDS ${CMAKE_SOURCE_DIR}/src/platform/win/chkstk.s VERBATIM)
list(APPEND DATA_OBJ ${CHKSTK_OBJ})

# ---- import libraries from the .def files (no Windows SDK needed) ----
find_program(SLINKY_DLLTOOL NAMES llvm-dlltool REQUIRED)
set(IMPORT_LIBS)
foreach(dll kernel32 user32 shell32 ucrtbase)
    set(lib ${CMAKE_BINARY_DIR}/imports/${dll}.lib)
    add_custom_command(OUTPUT ${lib}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/imports
        COMMAND ${SLINKY_DLLTOOL} -m i386:x86-64 -d ${CMAKE_SOURCE_DIR}/cmake/imports/${dll}.def -l ${lib}
        DEPENDS ${CMAKE_SOURCE_DIR}/cmake/imports/${dll}.def VERBATIM)
    list(APPEND IMPORT_LIBS ${lib})
endforeach()

add_custom_target(slinky_imports DEPENDS ${IMPORT_LIBS})
add_library(slinky SHARED ${ENGINE_SRC} ${LOGIC_SRC} ${MODULE_SRC} ${PLATFORM_SRC} ${NANOVG_SRC} ${DATA_OBJ})
add_dependencies(slinky slinky_imports)
target_link_libraries(slinky PRIVATE ${IMPORT_LIBS})
set_target_properties(slinky PROPERTIES PREFIX "" LINKER_LANGUAGE CXX)
set_source_files_properties(${DATA_OBJ} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

# The code generation is the one the sources were verified against (tools/asmdiff.py CFLAGS): same optimiser
# settings, frame pointers, no stack cookies, no COMDAT per function, the old compiler's inc/dec and nop choices.
target_compile_options(slinky PRIVATE
    -O2 -GS- /Gy-
    "SHELL:-Xclang -mframe-pointer=all"
    "SHELL:-Xclang -target-feature -Xclang +slow-incdec"
    "SHELL:-Xclang -target-feature -Xclang -fast-11bytenop"
    "SHELL:-Xclang -target-feature -Xclang -fast-15bytenop"
    -w)
target_include_directories(slinky PRIVATE include third_party/nanovg/src third_party/gl)
# the bundled C libraries (NanoVG, stb) get a minimal libc declaration set instead of the Windows SDK's headers
set_source_files_properties(${NANOVG_SRC} PROPERTIES COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/third_party/minicrt/include")
# glcorearb.h would include <windows.h> (not available in the cross build) just to define APIENTRY
target_compile_definitions(slinky PRIVATE APIENTRY=)

# some files need extra flags (`// cflags: ...` in their first lines): the compiler version that built the
# original lowered a few constructs differently
foreach(f ${ENGINE_SRC} ${LOGIC_SRC} ${MODULE_SRC})
    file(STRINGS ${f} line LIMIT_COUNT 1 REGEX "^// cflags:")
    if(line)
        string(REGEX REPLACE "^// cflags:[ ]*" "" extra "${line}")
        separate_arguments(extra_list NATIVE_COMMAND "${extra}")
        set_source_files_properties(${f} PROPERTIES COMPILE_OPTIONS "${extra_list}")
    endif()
endforeach()

# Reached from the DLL's own entry point; no C runtime start-up code is linked (the UCRT in the process is used)
target_link_options(slinky PRIVATE /NODEFAULTLIB /ENTRY:DllMain /SUBSYSTEM:WINDOWS /OPT:NOREF /OPT:NOICF /ERRORLIMIT:0 /MAP)
