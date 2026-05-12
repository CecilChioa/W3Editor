cmake_minimum_required(VERSION 3.24)

project(W3Editor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(W3EDITOR_BUILD_TESTS "Build W3Editor tests" ON)

# 源码和注释统一按 UTF-8 编译，保证中文注释、中文路径和日志文案不会被 MSVC 当前代码页破坏。
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# engine 保持 UI 无关；格式、地图、地形和资源能力都先沉到这里，Qt 只允许在 apps/editor 层接入。
add_library(w3editor_engine
    engine/formats/archive/DirectoryArchive.cpp
    engine/formats/archive/MpqArchive.cpp
)

target_include_directories(w3editor_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
)

target_compile_features(w3editor_engine PUBLIC cxx_std_20)

add_executable(w3editor_cli
    tools/w3editor_cli/main.cpp
)

target_link_libraries(w3editor_cli PRIVATE w3editor_engine)

if(W3EDITOR_BUILD_TESTS)
    enable_testing()

    # Archive 测试先验证目录夹具路径，保证 IArchive 上层调用方式稳定；MPQ 用例等 StormLib 接入后补齐。
    add_executable(w3editor_archive_tests
        tests/archive_tests/ArchiveTests.cpp
    )
    target_link_libraries(w3editor_archive_tests PRIVATE w3editor_engine)

    add_test(NAME archive_tests COMMAND w3editor_archive_tests)
    add_test(NAME cli_close COMMAND w3editor_cli --close)
endif()