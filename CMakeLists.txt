cmake_minimum_required(VERSION 3.29)

# Hack because vcpkg doesn't allow you to just declare that you want to be static on all platforms
# Linux and MacOS are static by default
# if(WIN32)
#    SET(VCPKG_TARGET_TRIPLET "x64-windows-static")
# endif()

project (W3Editor CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(BZip2 REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

qt_standard_project_setup()

add_executable(W3Editor
	${QT_RESOURCES}
)

add_subdirectory(src)

target_include_directories(W3Editor PRIVATE 
	"src"
	"src/main_window"
)

target_link_libraries(W3Editor PUBLIC
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	ZLIB::ZLIB
	BZip2::BZip2
	CascLib::casc
	StormLib::storm
)

target_compile_options(W3Editor PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /MP /sdl /diagnostics:caret /permissive->
	$<$<CXX_COMPILER_ID:Clang>:-Wextra -Wpedantic -Werror -Wno-multichar>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror -Wno-multichar>
)

target_compile_definitions(W3Editor PRIVATE
	__STORMLIB_NO_STATIC_LINK__
	$<$<CXX_COMPILER_ID:MSVC>:UNICODE _UNICODE>
	$<$<CXX_COMPILER_ID:Clang>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
	$<$<CXX_COMPILER_ID:GNU>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
)

if(TRACY_ENABLE)
	target_compile_definitions(W3Editor PRIVATE TRACY_ENABLE)
endif()

if(TRACY_NO_EXIT)
	target_compile_definitions(W3Editor PRIVATE TRACY_NO_EXIT)
endif()

target_compile_features(W3Editor PUBLIC cxx_std_23)

if (EXISTS "${PROJECT_SOURCE_DIR}/data")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		-E copy_directory
		"${PROJECT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:W3Editor>/data"
	)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:W3Editor>/platforms"
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:W3Editor>/styles"
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:W3Editor>/imageformats"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/platforms"
	"$<TARGET_FILE_DIR:W3Editor>/platforms"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/styles"
	"$<TARGET_FILE_DIR:W3Editor>/styles"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/imageformats"
	"$<TARGET_FILE_DIR:W3Editor>/imageformats"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/platforms"
	"$<TARGET_FILE_DIR:W3Editor>/platforms"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/styles"
	"$<TARGET_FILE_DIR:W3Editor>/styles"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/imageformats"
	"$<TARGET_FILE_DIR:W3Editor>/imageformats"
)
