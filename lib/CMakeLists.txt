
install (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/commsdsl
    DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}
)

add_subdirectory(src)
add_subdirectory(test)
