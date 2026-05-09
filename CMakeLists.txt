cmake_minimum_required(VERSION 3.29)

project(W3Editor CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(outcome CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(unofficial-qscintilla CONFIG REQUIRED)
find_package(tracy CONFIG REQUIRED)

qt_standard_project_setup()

function(w3editor_copy_qt_plugins target_name)
	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/platforms"
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/styles"
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/imageformats"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/platforms"
		"$<TARGET_FILE_DIR:${target_name}>/platforms"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/styles"
		"$<TARGET_FILE_DIR:${target_name}>/styles"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/imageformats"
		"$<TARGET_FILE_DIR:${target_name}>/imageformats"
	)
endfunction()

add_executable(W3EditorHive
	${QT_RESOURCES}
)

add_subdirectory(src)

target_include_directories(W3EditorHive PRIVATE
	"src"
	"src/base"
	"src/brush"
	"src/custom_widgets"
	"src/file_formats"
	"src/main_window"
	"src/menus"
	"src/models"
	"src/model_editor"
	"src/object_editor"
	"src/trigger_editor"
	"src/resources"
	"src/resources/skinned_mesh"
	"src/utilities"
	${BULLET_INCLUDE_DIR}
)

target_link_libraries(W3EditorHive PUBLIC
	mimalloc
	Qt6::Gui
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	glm::glm
	glad::glad
	soil2::soil2
	StormLib::storm
	BZip2::BZip2
	libjpeg-turbo::jpeg
	absl::base
	libjpeg-turbo::turbojpeg
	ads::qtadvanceddocking-qt6
	TBB::tbb
	imgui::imgui
	outcome::hl
	unordered_dense::unordered_dense
	nlohmann_json::nlohmann_json
	unofficial::qscintilla::qscintilla
	${BULLET_LIBRARIES}
	Tracy::TracyClient
)

target_compile_options(W3EditorHive PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /MP /sdl /diagnostics:caret /permissive->
	$<$<CXX_COMPILER_ID:Clang>:-Wextra -Wpedantic -Werror -Wno-multichar>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror -Wno-multichar>
)

target_compile_definitions(W3EditorHive PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:UNICODE _UNICODE>
	$<$<CXX_COMPILER_ID:Clang>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
	$<$<CXX_COMPILER_ID:GNU>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
)

if(TRACY_ENABLE)
	target_compile_definitions(W3EditorHive PRIVATE TRACY_ENABLE)
endif()

if(TRACY_NO_EXIT)
	target_compile_definitions(W3EditorHive PRIVATE TRACY_NO_EXIT)
endif()

if(WIN32)
	target_link_libraries(W3EditorHive PUBLIC CascLib::casc)
else()
	target_link_libraries(W3EditorHive PUBLIC CascLib::casc_static)
endif()

target_compile_features(W3EditorHive PUBLIC cxx_std_23)

add_custom_command(TARGET W3EditorHive POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${PROJECT_SOURCE_DIR}/data"
	"$<TARGET_FILE_DIR:W3EditorHive>/data"
)

w3editor_copy_qt_plugins(W3EditorHive)
