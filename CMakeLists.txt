add_subdirectory_if_exist(hal)
add_subdirectory_if_exist(network)
add_subdirectory_if_exist(routing_transport)
add_subdirectory_if_exist(mesh_api)

set(SOURCES "${SOURCES}" PARENT_SCOPE)
set(PUBLIC_HEADER "${PUBLIC_HEADER}" PARENT_SCOPE)