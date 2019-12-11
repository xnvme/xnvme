cmake_minimum_required(VERSION 3.1)
set(TOOLS true CACHE BOOL "tools: Command-Line Interface (CLI) tools")
if(NOT TOOLS)
	return()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DXNVME_DEBUG_ENABLED")

include_directories("${PROJECT_SOURCE_DIR}/include")
message(STATUS "TOOLS-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/lblk.c
	${CMAKE_CURRENT_SOURCE_DIR}/nvmec.c
	${CMAKE_CURRENT_SOURCE_DIR}/xnvme.c
	${CMAKE_CURRENT_SOURCE_DIR}/zoned.c
)

foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)

	set(EXE_NAME "${SRC_FN_WE}-TOOLS")
	set(EXE_FN "${SRC_FN_WE}")

	add_executable(${EXE_NAME} ${SRC_FN})
	target_link_libraries(${EXE_NAME} ${LNAME})
	target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	install(TARGETS ${EXE_NAME} DESTINATION bin COMPONENT tools)
	set_target_properties(${EXE_NAME} PROPERTIES OUTPUT_NAME ${EXE_FN})

	set(COMPLETION_SCRIPT
		${PROJECT_SOURCE_DIR}/scripts/bash_completion.d/${EXE_FN}-completions)

	# TODO: improve determination of DESTINATION
	if(DEFINED BASH_COMPLETION_COMPLETIONSDIR AND EXISTS ${COMPLETION_SCRIPT})
		install(FILES ${COMPLETION_SCRIPT}
			DESTINATION share/bash-completion/completions/
			RENAME ${EXE_FN}
			COMPONENT tools)
	endif()

	file(GLOB MAN_PAGES ${PROJECT_SOURCE_DIR}/man/${EXE_FN}-*.1)
	list(APPEND MAN_PAGES ${PROJECT_SOURCE_DIR}/man/${EXE_FN}.1)
	foreach(MAN_PAGE ${MAN_PAGES})
		if(EXISTS ${MAN_PAGE})
			install(FILES ${MAN_PAGE}
				DESTINATION share/man/man1
				COMPONENT tools)
		endif()
	endforeach()
endforeach()

# Install driver-script as part of the xNVMe-tools package
set(DRIVER_SCRIPT
	${PROJECT_SOURCE_DIR}/scripts/xnvme-driver.sh)

install(PROGRAMS ${DRIVER_SCRIPT}
	DESTINATION bin
	RENAME xnvme-driver
	COMPONENT tools)
