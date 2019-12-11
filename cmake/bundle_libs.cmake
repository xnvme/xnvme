#[[
	This function relies on the following variables in global scope:

	* BUNDLED_LIBS, and array of libraries to bundle with 'tgt_name'
]]
function(bundle_libs tgt_name bundled_tgt_name)
	message( STATUS "Bundling libraries")

	# For the bundled library
	list(REMOVE_DUPLICATES BUNDLED_LIBS)
	foreach(dep_fpath IN LISTS BUNDLED_LIBS)
		message( STATUS "adding to bundle: ${dep_fpath}" )
	endforeach()

	set(bundled_tgt_full_name
		${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

	set(ar_script ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar)
	set(ar_script_in ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

	# Generate archive script
	file(WRITE ${ar_script_in} "CREATE ${bundled_tgt_full_name}\n" )
	foreach(tgt_path IN LISTS BUNDLED_LIBS)
		file(APPEND ${ar_script_in} "ADDLIB ${tgt_path}\n")
	endforeach()
	file(APPEND ${ar_script_in} "SAVE\n")
	file(APPEND ${ar_script_in} "END\n")
	file(GENERATE OUTPUT ${ar_script} INPUT ${ar_script_in})

	set(ar_tool ${CMAKE_AR})
	#if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
	if (HAS_IPO)
		set(ar_tool ${CMAKE_C_COMPILER_AR})
	endif()

	add_custom_command(
		COMMAND ${ar_tool} -M < ${ar_script}
		OUTPUT ${bundled_tgt_full_name}
		COMMENT "Bundling ${bundled_tgt_name}"
		VERBATIM)

	# Create CMAKE target
	add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
	add_dependencies(bundling_target ${tgt_name})

	add_library(${bundled_tgt_name} STATIC IMPORTED)

	set_target_properties(${bundled_tgt_name}
		PROPERTIES
		IMPORTED_LOCATION ${bundled_tgt_full_name}
		INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)

	add_dependencies(${bundled_tgt_name} bundling_target)

	install(FILES ${bundled_tgt_full_name} DESTINATION lib COMPONENT dev)

	set(BUNDLE_LIBS "SUCCESS" PARENT_SCOPE)
endfunction()
