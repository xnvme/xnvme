include(FindPackageHandleStandardArgs)
include("$ENV{WORKSPACE_TOP}/common/cmake/lb.cmake")

lb_local_path(XNVME_BUILD xnvme xnvme)

find_path(Xnvme_INCLUDE_DIR
  "libxnvme.h"
  PATHS "$ENV{WORKSPACE_TOP}/xnvme"
  PATH_SUFFIXES "include"
)

find_library(Xnvme_LIBRARY NAMES libxnvme.a PATHS "${XNVME_BUILD}/lib")

message( STATUS "xnvme-config.cmake : Xnvme_LIBRARY = ${Xnvme_LIBRARY}" )

lb_version(Xnvme_VERSION xnvme xnvme)

find_package_handle_standard_args(Xnvme DEFAULT_MSG
  Xnvme_INCLUDE_DIR Xnvme_LIBRARY
)

# I'm not entirely sure if it's needed, but it sounds
# like a best practice. I'm not sure I understand the documentation:
# https://cmake.org/cmake/help/v3.12/manual/cmake-developer.7.html#modules
if(Xnvme_FOUND)
  set(Xnvme_LIBRARIES ${Xnvme_LIBRARY})
  set(Xnvme_INCLUDE_DIRS ${Xnvme_INCLUDE_DIR})
  message("GOT ${Xnvme_INCLUDE_DIR}")
endif()

if(Xnvme_FOUND AND NOT TARGET Xnvme::Xnvme)
  add_library(Xnvme::Xnvme UNKNOWN IMPORTED)
  set_target_properties(Xnvme::Xnvme PROPERTIES
    IMPORTED_LOCATION "${Xnvme_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Xnvme_INCLUDE_DIR}"
)
endif()
