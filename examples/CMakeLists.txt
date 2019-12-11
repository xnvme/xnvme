cmake_minimum_required(VERSION 3.1)
set(EXAMPLES true CACHE BOOL "examples: include example programs in build")
if(NOT EXAMPLES)
	return()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DXNVME_DEBUG_ENABLED")

include_directories("${PROJECT_SOURCE_DIR}/include")
message(STATUS "EXAMPLES-CMAKE_C_FLAGS(${CMAKE_C_FLAGS})")

set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/xnvme_hello.c
	${CMAKE_CURRENT_SOURCE_DIR}/xnvme_io_async.c
	${CMAKE_CURRENT_SOURCE_DIR}/zoned_io_async.c
	${CMAKE_CURRENT_SOURCE_DIR}/zoned_io_sync.c
)

#
# static linking, against xnvme_a, to avoid runtime dependency on libxnvme
#
foreach(SRC_FN ${SOURCE_FILES})
	get_filename_component(SRC_FN_WE ${SRC_FN} NAME_WE)

	set(EXE_FN "${SRC_FN_WE}")
	add_executable(${EXE_FN} ${SRC_FN})

	target_link_libraries(${EXE_FN} ${LNAME})
	target_include_directories(${EXE_FN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	install(TARGETS ${EXE_FN} DESTINATION bin COMPONENT examples)

	set(COMPLETION_SCRIPT
		${PROJECT_SOURCE_DIR}/scripts/bash_completion.d/${EXE_FN}-completions)

	# TODO: improve determination of DESTINATION
	if(DEFINED BASH_COMPLETION_COMPLETIONSDIR AND EXISTS ${COMPLETION_SCRIPT})
		install(FILES ${COMPLETION_SCRIPT}
			DESTINATION share/bash-completion/completions/
			RENAME ${EXE_FN}
			COMPONENT examples)
	endif()

	file(GLOB MAN_PAGES ${PROJECT_SOURCE_DIR}/man/${EXE_FN}-*.1)
	list(APPEND MAN_PAGES ${PROJECT_SOURCE_DIR}/man/${EXE_FN}.1)
	foreach(MAN_PAGE ${MAN_PAGES})
		if(EXISTS ${MAN_PAGE})
			install(FILES ${MAN_PAGE}
				DESTINATION share/man/man1
				COMPONENT examples)
		endif()
	endforeach()

endforeach()

