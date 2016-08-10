# CMake entry point
cmake_minimum_required (VERSION 3.6)
project (breakout)

if( CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR )
    message( FATAL_ERROR "Please select another Build Directory ! (and give it a clever name, like bin_Visual2012_64bits/)" )
endif()
if( CMAKE_SOURCE_DIR MATCHES " " )
	message( "Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()
if( CMAKE_BINARY_DIR MATCHES " " )
	message( "Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
	add_subdirectory(distrib)
endif(INCLUDE_DISTRIB)

include_directories(
	include/
	include/freetype/
	include/irrKlang/
	include/SOIL/
	.
)

find_package(OpenGL REQUIRED)

set(ALL_LIBS
	${OPENGL_LIBRARIES}
	${CMAKE_CURRENT_SOURCE_DIR}/lib/GLFW/glfw3.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/GL/glew32s.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/soil/SOIL.lib	
	${CMAKE_CURRENT_SOURCE_DIR}/lib/irrKlang/irrKlang.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype/freetype262.lib
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

file(GLOB SHADERS "shaders/*")
file(GLOB SOURCE_CODE "src/*")

add_executable(breakout
	${SHADERS}
	${SOURCE_CODE}
)

target_link_libraries(breakout
	${ALL_LIBS}
)

create_target_launcher(breakout WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")
create_default_target_launcher(breakout WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

file(GLOB DLL_FILES
  "bin/*.dll"
)

add_custom_command(
   TARGET breakout POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy ${DLL_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/"
   COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/breakout${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/bin/"
)
