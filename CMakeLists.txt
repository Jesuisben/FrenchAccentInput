cmake_minimum_required(VERSION 3.24)
project(FrenchAccentInput VERSION 1.0.0 LANGUAGES CXX RC)

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

# WIN32 옵션은 console 창이 없는 GUI subsystem 실행 파일을 만든다.
add_executable(FrenchAccentInput WIN32 src/main.cpp resources/version.rc)
target_compile_features(FrenchAccentInput PRIVATE cxx_std_20)
target_compile_definitions(FrenchAccentInput PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(FrenchAccentInput PRIVATE fai_core user32 shell32)

if(MSVC)
  target_compile_options(FrenchAccentInput PRIVATE /W4 /WX /permissive- /utf-8)
endif()
