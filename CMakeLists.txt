cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(XNVME_VERSION_MAJOR 0 CACHE INTERNAL "xnvme_ver: XNVME_VERSION_MAJOR")
set(XNVME_VERSION_MINOR 0 CACHE INTERNAL "xnvme_ver: XNVME_VERSION_MINOR")
set(XNVME_VERSION_PATCH 10 CACHE INTERNAL "xnvme_ver: XNVME_VERSION_PATH")
set(XNVME_VERSION "${XNVME_VERSION_MAJOR}.${XNVME_VERSION_MINOR}.${XNVME_VERSION_PATCH}")

project(xnvme C)
set(CMAKE_C_STANDARD 11)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(bundle_libs)

set(LNAME "xnvme")

# Add versioning
add_definitions(-DXNVME_VERSION_MAJOR=${XNVME_VERSION_MAJOR})
add_definitions(-DXNVME_VERSION_MINOR=${XNVME_VERSION_MINOR})
add_definitions(-DXNVME_VERSION_PATCH=${XNVME_VERSION_PATCH})
add_definitions(-DXNVME_VERSION=${XNVME_VERSION})

if(WIN32)
	add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()

function(enable_c_flag flag)
	string(FIND "${CMAKE_C_FLAGS}" "${flag}" flag_already_set)
	if(flag_already_set EQUAL -1)
		check_c_compiler_flag("${flag}" flag_supported)
		if(flag_supported)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
		endif()
	endif()
endfunction()

enable_c_flag("-std=gnu11")
enable_c_flag("-Wall")
enable_c_flag("-Wextra")
enable_c_flag("-Werror")

# Can't be pedantic due to SPDK and liburing using non-ISO features
#enable_c_flag("-pedantic")

find_package(bash-completion QUIET)
message( STATUS "BASH_COMPLETION_FOUND: '${BASH_COMPLETION_FOUND}', PATH('${BASH_COMPLETION_COMPLETIONSDIR}'")

# Find pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

message( STATUS "CMAKE_C_FLAGS(${CMAKE_C_FLAGS})" )
message( STATUS "CMAKE_SYSTEM(${CMAKE_SYSTEM})" )
message( STATUS "CMAKE_SYSTEM_NAME(${CMAKE_SYSTEM_NAME})" )
message( STATUS "CMAKE_HOST_SYSTEM_NAME(${CMAKE_HOST_SYSTEM_NAME})" )

if(CMAKE_SYSTEM MATCHES "Linux")
	set(FREEBSD 0)
	set(LINUX 1)
elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
	set(FREEBSD 1)
	set(LINUX 0)
else()
	set(FREEBSD 0)
	set(LINUX 0)
endif()
message( STATUS "UNIX(${UNIX}), FREEBSD(${FREEBSD}), LINUX(${LINUX}), WIN32(${WIN32})" )

#
# Check for struct timespec ts and clock_gettime
#
check_library_exists(c clock_gettime "" LIBC_HAS_CLOCK_GETTIME)
check_library_exists(rt clock_gettime "time.h" LIBRT_HAS_CLOCK_GETTIME)

# On Windows we assume it is available via the TDM-GCC compiler suite
if(WIN32)
	message( WARNING "Skipping check for clock_gettime..." )
else ()
	if((NOT LIBC_HAS_BLOCK_GETTIME) AND (NOT LIBRT_HAS_CLOCK_GETTIME))
		message( FATAL_ERROR "Cannot find clock_gettime" )
	endif()
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DXNVME_DEBUG_ENABLED")

set(XNVME_TRACE_ENABLED FALSE CACHE BOOL "Enable NVMe command tracing")
if(XNVME_TRACE_ENABLED)
	add_definitions(-DXNVME_TRACE_ENABLED)
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU)$")
	message( "" )
	message( WARNING "You are using an untested toolchain" )
	message( "  It is recommened that use GNU toolchain, e.g.:" )
	message( "  CC=gcc ./configure --enable-<options>" )
	message( "    or" )
	message( "  make CC=gcc" )
	message( "" )
	message( "" )
endif()

set(HEADER_FILES
	${PROJECT_SOURCE_DIR}/include/libznd.h
	${PROJECT_SOURCE_DIR}/include/liblblk.h
	${PROJECT_SOURCE_DIR}/include/libxnvme.h
	${PROJECT_SOURCE_DIR}/include/libxnvme_spec.h
	${PROJECT_SOURCE_DIR}/include/libxnvme_util.h
	${PROJECT_SOURCE_DIR}/include/libxnvmec.h
	${PROJECT_SOURCE_DIR}/include/xnvme_async.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_registry.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_nosys.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_fioc.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_lioc.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_liou.h
	${PROJECT_SOURCE_DIR}/include/xnvme_be_spdk.h
	${PROJECT_SOURCE_DIR}/include/xnvme_dev.h
	${PROJECT_SOURCE_DIR}/include/xnvme_sgl.h
)

set(SOURCE_FILES
	${PROJECT_SOURCE_DIR}/src/znd.c
	${PROJECT_SOURCE_DIR}/src/lblk.c
	${PROJECT_SOURCE_DIR}/src/xnvme_async.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be_nosys.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be_fioc.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be_lioc.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be_liou.c
	${PROJECT_SOURCE_DIR}/src/xnvme_be_spdk.c
	${PROJECT_SOURCE_DIR}/src/xnvme_buf.c
	${PROJECT_SOURCE_DIR}/src/xnvme_cmd.c
	${PROJECT_SOURCE_DIR}/src/xnvmec.c
	${PROJECT_SOURCE_DIR}/src/xnvme_dev.c
	${PROJECT_SOURCE_DIR}/src/xnvme_geo.c
	${PROJECT_SOURCE_DIR}/src/xnvme_ret.c
	${PROJECT_SOURCE_DIR}/src/xnvme_sgl.c
	${PROJECT_SOURCE_DIR}/src/xnvme_spec.c
	${PROJECT_SOURCE_DIR}/src/xnvme_ver.c
)

include_directories("${PROJECT_SOURCE_DIR}/include")

# Default 'clang' on FreeBSD does not look here, so we tell it to probably
# another symptom of 'base-system-compiler-we-do-not-care'
link_directories("/usr/local/lib")

add_library(${LNAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${LNAME} PROPERTIES OUTPUT_NAME "${LNAME}_slim")
target_link_libraries(xnvme Threads::Threads)

#
# Library bundling
#
list(APPEND BUNDLED_LIBS "lib${LNAME}_slim.a")

#
# BACKENDS -- begin
#
set(BUNDLE_SYSTEM_DEPS FALSE CACHE BOOL
	"Whether or not to bundle system dependencies")
message( STATUS "BUNDLE_SYSTEM_DEPS(${BUNDLE_SYSTEM_DEPS})" )

#
# XNVME_BE_SPDK
#
set(XNVME_BE_SPDK_ENABLED ${UNIX} CACHE BOOL "be_spdk: SPDK backend")
set(SPDK_INCLUDE_PATH
	"${PROJECT_SOURCE_DIR}/third-party/spdk/include"
	CACHE PATH "SPDK include path")
set(SPDK_LIBRARY_PATH
	"${PROJECT_SOURCE_DIR}/third-party/spdk/build/lib"
	CACHE PATH "SPDK library path")
set(DPDK_INCLUDE_PATH
	"${PROJECT_SOURCE_DIR}/third-party/spdk/dpdk/build/include"
	CACHE PATH "DPDK include path")
set(DPDK_LIBRARY_PATH
	"${PROJECT_SOURCE_DIR}/third-party/spdk/dpdk/build/lib"
	CACHE PATH "DPDK library path")

if(XNVME_BE_SPDK_ENABLED)
	message( STATUS "SPDK_LIBRARY_PATH(${SPDK_LIBRARY_PATH})" )
	message( STATUS "SPDK_INCLUDE_PATH(${SPDK_INCLUDE_PATH})" )
	message( STATUS "DPDK_LIBRARY_PATH(${DPDK_LIBRARY_PATH})" )
	message( STATUS "DPDK_INCLUDE_PATH(${DPDK_INCLUDE_PATH})" )

	if(NOT EXISTS ${SPDK_LIBRARY_PATH})
		message( FATAL_ERROR "SPDK_LIBRARY_PATH does not exist" )
	endif()
	if(NOT EXISTS ${SPDK_INCLUDE_PATH})
		message( FATAL_ERROR "SPDK_INCLUDE_PATH does not exist" )
	endif()
	if(NOT EXISTS ${DPDK_LIBRARY_PATH})
		message( FATAL_ERROR "DPDK_LIBRARY_PATH does not exist" )
	endif()
	if(NOT EXISTS ${DPDK_INCLUDE_PATH})
		message( FATAL_ERROR "DPDK_INCLUDE_PATH does not exist" )
	endif()

	add_definitions(-DXNVME_BE_SPDK_ENABLED)

	message( STATUS "ENABLING ASYNC SUPPORT" )
	add_definitions(-DXNVME_ASYNC_ENABLED)

	# These are expected to be available on the system
	list(APPEND system_deps rt)
	if (LINUX)
		list(APPEND system_deps numa)
	elseif (FREEBSD)
		list(APPEND system_deps execinfo)
	endif()
	list(APPEND system_deps uuid)

	message( STATUS "system_deps(${system_deps})")
	
	if(BUNDLE_SYSTEM_DEPS)
		foreach(dep IN LISTS system_deps)
			set(dep_fname "lib${dep}.a")
			unset(dep_path CACHE)
			find_library(dep_path NAMES ${dep_fname})
			if ("${dep_path}" STREQUAL "dep_path-NOTFOUND")
				message( STATUS "FAILED: find_library(${dep_fname})")
				set(BUNDLE_LIBS "FAILED" PARENT_SCOPE)
				return()
			endif()

			list(APPEND BUNDLED_LIBS "${dep_path}")
		endforeach()
	endif()

	# Should be built and available in DPDK_LIBRARY_PATH
	set(dpdk_deps
		rte_eal
		rte_bus_pci
		rte_pci
		rte_ring
		rte_mempool
		rte_kvargs
	)

	message( STATUS "dpdk_deps(${dpdk_deps})")

	foreach(dep IN LISTS dpdk_deps)
		set(dep_fname "lib${dep}.a")
		set(dep_fpath "${DPDK_LIBRARY_PATH}/${dep_fname}")

		if (NOT EXISTS "${dep_fpath}")
			message( STATUS "FAILED: !exists(${dep_fpath})")
			set(BUNDLE_LIBS "FAILED" PARENT_SCOPE)
			return()
		endif()

		list(APPEND BUNDLED_LIBS "${dep_fpath}")
	endforeach()

	# Should be built and available in SPDK_LIBRARY_PATH
	set(spdk_deps
		spdk_nvme
		spdk_env_dpdk
		spdk_sock
		spdk_util
		spdk_log
	)

	message( STATUS "spdk_deps(${spdk_deps})")

	foreach(dep IN LISTS spdk_deps)
		set(dep_fname "lib${dep}.a")
		set(dep_fpath "${SPDK_LIBRARY_PATH}/${dep_fname}")

		if (NOT EXISTS "${dep_fpath}")
			message( STATUS "FAILED: !exists(${dep_fpath})")
			set(BUNDLE_LIBS "FAILED" PARENT_SCOPE)
			return()
		endif()

		list(APPEND BUNDLED_LIBS "${dep_fpath}")
	endforeach()

	# For the slim library
	include_directories("${SPDK_INCLUDE_PATH}")
	link_directories("${SPDK_LIBRARY_PATH}")

	include_directories("${DPDK_INCLUDE_PATH}")
	link_directories("${DPDK_LIBRARY_PATH}")

	target_link_libraries(${LNAME} ${spdk_deps} ${dpdk_deps} ${system_deps})
endif()
message( STATUS "BE:SPDK ENABLED(${XNVME_BE_SPDK_ENABLED})" )

#
# XNVME_BE_FIOC
#
set(XNVME_BE_FIOC_ENABLED ${FREEBSD} CACHE BOOL "be_fioc: FreeBSD/IOCTL backend")
if(XNVME_BE_FIOC_ENABLED)
	add_definitions(-DXNVME_BE_FIOC_ENABLED)
endif()
message( STATUS "BE:FIOC ENABLED(${XNVME_BE_FIOC_ENABLED})" )

#
# XNVME_BE_LIOC
#
set(XNVME_BE_LIOC_ENABLED ${LINUX} CACHE BOOL "be_lioc: Linux/IOCTL backend")
if(XNVME_BE_LIOC_ENABLED)
	add_definitions(-DXNVME_BE_LIOC_ENABLED)
endif()
message( STATUS "BE:LIOC ENABLED(${XNVME_BE_LIOC_ENABLED})" )

#
# XNVME_BE_LIOU
#
set(XNVME_BE_LIOU_ENABLED ${LINUX} CACHE BOOL "be_liou: Linux/io_uring backend")
set(LIBURING_INCLUDE_PATH
	"${PROJECT_SOURCE_DIR}/third-party/liburing/src/include"
	CACHE PATH "LIBURING include path")
set(LIBURING_LIBRARY_PATH
	"${PROJECT_SOURCE_DIR}/third-party/liburing/src"
	CACHE PATH "LIBURING library path")

if(XNVME_BE_LIOU_ENABLED)
	message( STATUS "LIBURING_LIBRARY_PATH(${LIBURING_LIBRARY_PATH})" )
	message( STATUS "LIBURING_INCLUDE_PATH(${LIBURING_INCLUDE_PATH})" )

	if(NOT EXISTS ${LIBURING_LIBRARY_PATH})
		message( FATAL_ERROR "LIBURING_LIBRARY_PATH does not exist" )
	endif()
	if(NOT EXISTS ${LIBURING_INCLUDE_PATH})
		message( FATAL_ERROR "LIBURING_INCLUDE_PATH does not exist" )
	endif()

	add_definitions(-DXNVME_BE_LIOU_ENABLED)

	message( STATUS "ENABLING ASYNC SUPPORT" )
	add_definitions(-DXNVME_ASYNC_ENABLED)

	set(be_liou_deps
		uring
	)

	# Add to be_liou_deps to BUNDLED_LIBS
	foreach(dep IN LISTS be_liou_deps)
		set(dep_fpath "${LIBURING_LIBRARY_PATH}/lib${dep}.a")

		if (NOT EXISTS "${dep_fpath}")
			message( STATUS "FAILED: !exists(${dep_fpath})")
			set(BUNDLE_LIBS "FAILED" PARENT_SCOPE)
			return()
		endif()

		list(APPEND BUNDLED_LIBS "${dep_fpath}")
	endforeach()

	# Link libxnvme with be_liou_deps (for the slim library)
	link_directories("${LIBURING_LIBRARY_PATH}")
	include_directories("${LIBURING_INCLUDE_PATH}")
	target_link_libraries(${LNAME} ${be_liou_deps})
endif()
message( STATUS "BE:LIOU ENABLED(${XNVME_BE_LIOU_ENABLED})" )

#
# BACKENDS -- end
#

bundle_libs("${LNAME}" "${LNAME}_bundle")
message(STATUS "bundle_libs(${BUNDLE_LIBS})")
if("${dep_path}" STREQUAL "dep_path-NOTFOUND")
	message( FATAL_ERROR "bundling libs failed")
endif()
if("${BUNDLE_LIBS}" STREQUAL "FAILED")
	message( FATAL_ERROR "bundling libs failed")
endif()

install(TARGETS ${LNAME} DESTINATION lib COMPONENT dev)

install(FILES "${PROJECT_SOURCE_DIR}/include/libxnvme.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/libxnvme_spec.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/libxnvme_util.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/libxnvmec.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/libznd.h"
	DESTINATION include COMPONENT dev)
install(FILES "${PROJECT_SOURCE_DIR}/include/liblblk.h"
	DESTINATION include COMPONENT dev)

add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(tools)

#
# Packages
#
if(WIN32)
	set(CPACK_GENERATOR "NSIS" "ZIP")
else()
	set(CPACK_GENERATOR "TGZ" "DEB")
endif()
#set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_VERSION "${XNVME_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${XNVME_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${XNVME_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${XNVME_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${XNVME_VERSION}.bin")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Simon A. F. Lund")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_COMPONENT_DEV_DESCRIPTION "xnvme-dev: Public header and static library for xNVMe")
if(TOOLS)
	set(CPACK_COMPONENT_TOOLS_DESCRIPTION "xnvme-tools: suite of xNVMe command-line tools")
endif()
if(EXAMPLES)
	set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "xnvme-examples: xNVMe examples provided as binaries")
endif()
if(TESTS)
	set(CPACK_COMPONENT_TESTS_DESCRIPTION "xnvme-tests: xNVMe test programs")
endif()

include(CPack)
