add_subdirectory(platforms)
add_subdirectory(modules)

# Used by src/modules and demos/ directories.
set(QPA_MIRSERVER_NAME "${QPA_MIRSERVER_NAME}" PARENT_SCOPE)
