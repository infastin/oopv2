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

set(INCLUDE_DIR include)
set(SRC_DIR src)

set(DIRS 
	${INCLUDE_DIR}
)

add_library(base STATIC
	${SRC_DIR}/Base/Object.c
	${SRC_DIR}/Base/Interface.c
	${SRC_DIR}/Base/Messages.c
)

add_library(ds STATIC
	${SRC_DIR}/DataStructs/Array.c
	${SRC_DIR}/DataStructs/SList.c
	${SRC_DIR}/DataStructs/DList.c
	${SRC_DIR}/DataStructs/BigInt.c
)

add_library(utils STATIC
	${SRC_DIR}/Utils/Stuff.c
	${SRC_DIR}/Utils/Sort.c
	${SRC_DIR}/Utils/Search.c
)

add_library(interfaces STATIC
	${SRC_DIR}/Interfaces/StringerInterface.c
)

target_link_libraries(base utils)
target_link_libraries(ds base interfaces)
target_link_libraries(interfaces base)

set(EXECUTABLES
	main
)

set(LIBRARIES
	base
	ds
	utils
	interfaces
)

foreach(LIB IN LISTS LIBRARIES)
	target_include_directories(${LIB} PUBLIC ${DIRS})
endforeach()

foreach(EXEC IN LISTS EXECUTABLES)
	add_executable(${EXEC} ${EXEC}.c)
	target_link_libraries(${EXEC} ${LIBRARIES} m)
endforeach()
