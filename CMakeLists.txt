cmake_minimum_required(VERSION 3.24)
project(FrenchAccentInput VERSION 1.0.0 LANGUAGES CXX RC)

# Ship an EXE without a separate MSVC runtime; rebuild releases for CRT updates.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

enable_testing()

# OS와 무관한 상태 머신을 별도 library로 두면 실제 keyboard hook 없이도 규칙을 검증할 수 있다.
add_library(fai_core STATIC src/input_core.cpp)
target_compile_features(fai_core PUBLIC cxx_std_20)
target_include_directories(fai_core PUBLIC include)

add_executable(fai_tests tests/input_core_tests.cpp)
target_compile_features(fai_tests PRIVATE cxx_std_20)
target_link_libraries(fai_tests PRIVATE fai_core)

if(MSVC)
  target_compile_options(fai_core PRIVATE /W4 /WX /permissive- /utf-8)
  target_compile_options(fai_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()

add_test(NAME input_core COMMAND fai_tests)

# Compile production output code with a recording SendInput replacement. This test
# never injects input or changes the active desktop.
add_executable(fai_output_tests tests/win32_output_tests.cpp)
target_compile_features(fai_output_tests PRIVATE cxx_std_20)
target_compile_definitions(fai_output_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(fai_output_tests PRIVATE fai_core user32 shell32)
if(MSVC)
  target_compile_options(fai_output_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()
add_test(NAME win32_output_contract COMMAND fai_output_tests)

# WIN32 옵션은 console 창이 없는 GUI subsystem 실행 파일을 만든다.
add_executable(FrenchAccentInput WIN32 src/main.cpp resources/version.rc)
target_compile_features(FrenchAccentInput PRIVATE cxx_std_20)
target_compile_definitions(FrenchAccentInput PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(FrenchAccentInput PRIVATE fai_core user32 shell32 imm32)

if(MSVC)
  target_compile_options(FrenchAccentInput PRIVATE /W4 /WX /permissive- /utf-8)
endif()

# dist에는 복사하지 않는 test-only host다. injected virtual keyboard event만 router에 통과시킨다.
add_executable(FrenchAccentInputVirtualKeyboardHost WIN32 src/main.cpp resources/version.rc)
target_compile_features(FrenchAccentInputVirtualKeyboardHost PRIVATE cxx_std_20)
target_compile_definitions(FrenchAccentInputVirtualKeyboardHost PRIVATE FAI_TEST_ALLOW_INJECTED UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(FrenchAccentInputVirtualKeyboardHost PRIVATE fai_core user32 shell32 imm32)

if(MSVC)
  target_compile_options(FrenchAccentInputVirtualKeyboardHost PRIVATE /W4 /WX /permissive- /utf-8)
endif()

# Explicitly invoked only: this driver sends fixed synthetic integration input.
# It is not a CTest and is never copied to dist.
add_executable(fai_virtual_driver tests/virtual_keyboard_driver.cpp)
target_compile_features(fai_virtual_driver PRIVATE cxx_std_20)
target_compile_definitions(fai_virtual_driver PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(fai_virtual_driver PRIVATE user32)
if(MSVC)
  target_compile_options(fai_virtual_driver PRIVATE /W4 /WX /permissive- /utf-8)
endif()

add_executable(fai_masked_restore_host WIN32 tests/masked_restore_host.cpp)
target_compile_features(fai_masked_restore_host PRIVATE cxx_std_20)
target_compile_definitions(fai_masked_restore_host PRIVATE FAI_TEST_ALLOW_INJECTED UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(fai_masked_restore_host PRIVATE fai_core user32 shell32 imm32)
if(MSVC)
  target_compile_options(fai_masked_restore_host PRIVATE /W4 /WX /permissive- /utf-8)
endif()
