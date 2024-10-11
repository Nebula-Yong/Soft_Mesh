add_subdirectory_if_exist(hal)
add_subdirectory_if_exist(network)

set(SOURCES "${SOURCES}" PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}" PARENT_SCOPE)