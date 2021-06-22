cmake_minimum_required(VERSION 3.1.3)

project(test C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (MSVC)
	add_compile_options(/O2 /J /Wall /std:c11 /GS)	
else()
	add_compile_options(-g -O2 -pipe 
						-D_FORTIFY_SOURCE=2 
						-Werror=implicit-function-declaration -Wall
						-funsigned-char -std=c11 -fms-extensions)
endif()

set(INCLUDE_FOLDER include)
set(ITERABLE_FOLDER include/Iterable)
set(INTERFACES_FOLDER include/Interfaces)

set(DIRS 
	${INCLUDE_FOLDER}
	${ITERABLE_FOLDER}
	${INTERFACES_FOLDER}
)

add_library(base STATIC
	${INCLUDE_FOLDER}/Object.c
	${INCLUDE_FOLDER}/Interface.c
	${INCLUDE_FOLDER}/Utils.c
	${INCLUDE_FOLDER}/Messages.c
	${ITERABLE_FOLDER}/Array.c
	${ITERABLE_FOLDER}/SList.c
	${ITERABLE_FOLDER}/DList.c
	${INTERFACES_FOLDER}/StringerInterface.c
)
target_include_directories(base PUBLIC ${DIRS})

set(EXECUTABLES
	main
	aud4
	aud8)

foreach(EXEC IN LISTS EXECUTABLES)
	add_executable(${EXEC} ${EXEC}.c)
	target_link_libraries(${EXEC} base m)
endforeach()
