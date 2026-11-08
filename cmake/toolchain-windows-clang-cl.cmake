# Cross-compile the Windows DLL from Linux / macOS with clang-cl + lld-link.
# No Windows SDK is needed: the code declares the few Win32 / UCRT functions it uses itself and links against
# import libraries that CMakeLists.txt generates from cmake/imports/*.def (llvm-dlltool).
#
#   cmake -S . -B build/cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-clang-cl.cmake
#   cmake --build build/cmake            ->  build/cmake/slinky.dll
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
# the compiler check must not need the SDK's libraries
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# the flags the engine code was verified with are set explicitly in CMakeLists.txt: drop CMake's MSVC defaults
# (/EHsc, /GR, /MD, /O2 /Ob2 /DNDEBUG ...)
set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_CXX_FLAGS_INIT "")
set(CMAKE_C_FLAGS_RELEASE_INIT "")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "")
set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
