cmake_minimum_required(VERSION 4.0)
project(neko_basic
        VERSION 1.1.1
        DESCRIPTION "A simple BASIC REPL"
        LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(NEKO_BASIC_GRAPHICS "Enable Graphics Instructions" ON)

add_executable(${PROJECT_NAME}
               src/main.c
               src/lexer.c
               src/pool.c
               src/program.c
               src/vars.c
               src/interpreter.c
               src/repl.c
               src/signals.c
)

find_package(raylib QUIET)

if(raylib_FOUND AND NEKO_BASIC_GRAPHICS)
	message(STATUS "Raylib found, enabling graphics support")
	target_link_libraries(neko_basic PRIVATE raylib m)
	target_compile_definitions(neko_basic PRIVATE NEKO_GRAPHICS)
else()
	message(STATUS "Raylib not found, graphics support disabled")
	target_link_libraries(neko_basic PRIVATE m)
endif()

if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CPACK_GENERATOR "NSIS")
endif ()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# --- CPack config ---
set(CPACK_PACKAGE_NAME "Neko Basic")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "thea@nekothecatgirl.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Thea Nicoline Flink")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# .deb specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thea Nicoline Flink <thea@nekothecatgirl.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "interpreters")

# .rpm specific
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")

# NSIS specific
set(CPACK_NSIS_DISPLAY_NAME "Neko Basic")
set(CPACK_NSIS_PACKAGE_NAME "neko_basic")
set(CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files")
set(CPACK_NSIS_MODIFY_PATH ON)

include(CPack)