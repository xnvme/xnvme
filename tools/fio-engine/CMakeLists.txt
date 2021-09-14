cmake_minimum_required(VERSION 3.9)
set(TOOLS_FIOE true CACHE BOOL "tools: xNVMe fio IO-engine")
if(NOT TOOLS_FIOE)
	return()
endif()

set(CMAKE_C_STANDARD 11)
set(FIOE_NAME "xnvme-fio-engine")

message(STATUS "TOOLS-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

set(FIOE_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/xnvme_fioe.c
)

#add_library(${FIOE_NAME} SHARED ${SOURCE_FILES} ${FIOE_SOURCE_FILES})
add_library(${FIOE_NAME} SHARED ${FIOE_SOURCE_FILES})
set_target_properties(${FIOE_NAME} PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION HAS_IPO
	ENABLE_EXPORTS 1)

add_dependencies(${FIOE_NAME} ${LIB_STATIC})
add_dependencies(${FIOE_NAME} ${LIB_BUNDLE})

target_compile_definitions(${FIOE_NAME} PUBLIC -D_GNU_SOURCE)

target_compile_options(${FIOE_NAME} PUBLIC -std=gnu11)
target_compile_options(${FIOE_NAME} PUBLIC -g)
target_compile_options(${FIOE_NAME} PUBLIC -O2)
target_compile_options(${FIOE_NAME} PUBLIC -fPIC)
target_compile_options(${FIOE_NAME} PUBLIC -Wall)
target_compile_options(${FIOE_NAME} PUBLIC -Wextra)
target_compile_options(${FIOE_NAME} PUBLIC -Werror)
target_compile_options(${FIOE_NAME} PUBLIC -Wno-missing-braces)

# skipping warning on unused-parameter as it triggers on fio headers
target_compile_options(${FIOE_NAME} PUBLIC -Wno-unused-parameter)
target_compile_options(${FIOE_NAME} PUBLIC -include
	"${PROJECT_SOURCE_DIR}/third-party/fio/repos/config-host.h")

target_include_directories(${FIOE_NAME} PRIVATE
       "${PROJECT_SOURCE_DIR}/third-party/fio/repos")

if (WINDOWS)
	target_include_directories(${FIOE_NAME} PRIVATE
		"${FIO_SRC_PATH}/os/windows/posix/include")
endif()

# LINKAGE
if (XNVME_LINK_CTORS)
	target_link_libraries(${FIOE_NAME} -Wl,--whole-archive)
	target_link_libraries(${FIOE_NAME} -Wl,--no-as-needed)
endif()
target_link_libraries(${FIOE_NAME} ${LIB_BUNDLE})
if (XNVME_LINK_CTORS)
	target_link_libraries(${FIOE_NAME} -Wl,--no-whole-archive)
	target_link_libraries(${FIOE_NAME} -Wl,--as-needed)
endif()
target_link_libraries(${FIOE_NAME} ${LIBS_SYSTEM})

install(TARGETS ${FIOE_NAME} DESTINATION lib COMPONENT tools)
install(FILES xnvme-verify.fio DESTINATION share/xnvme COMPONENT tools)
install(FILES xnvme-compare.fio DESTINATION share/xnvme COMPONENT tools)
install(FILES xnvme-zoned.fio DESTINATION share/xnvme COMPONENT tools)

# Define fio as a dependency, this ensures that arch-conf headers are available
if(WINDOWS)
	set(FIO_BINARY_NAME "fio.exe")
else()
	set(FIO_BINARY_NAME "fio")
endif()

if(NOT EXISTS "${FIO_SRC_PATH}/${FIO_BINARY_NAME}")
	message("Building fio")

	execute_process(
		COMMAND make third-party-fio
		RESULT_VARIABLE res
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
	if(res)
		message( FATAL_ERROR "Failed building fio" )
	endif()
endif()
