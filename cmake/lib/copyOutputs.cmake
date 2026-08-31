add_custom_target(copy_public_files ALL
    DEPENDS ${PUBLIC_FILES}
    COMMENT "Copying public files..."
)

add_custom_target(copy_ui_to_game ALL
    DEPENDS build_ui
    COMMENT "Copying UI..."
)

add_dependencies(${PROJECT_NAME} copy_public_files)

function(copyOutputs TARGET_FOLDER)
    set(DLL_FOLDER "${TARGET_FOLDER}/SKSE/Plugins")
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_PDB_FILE:${PROJECT_NAME}>"
                "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()

    file(GLOB_RECURSE PUBLIC_FILES "${CMAKE_SOURCE_DIR}/public/*")
    add_custom_command(
        TARGET copy_public_files
        PRE_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/public"
            "${TARGET_FOLDER}"
        VERBATIM
    )

    if(NOT DEV_SERVER)
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${TARGET_FOLDER}/PrismaUI/views"
            COMMAND "${CMAKE_COMMAND}" -E rm -rf "${TARGET_FOLDER}/PrismaUI/views/${PRODUCT_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_BINARY_DIR}/PrismaUI/views/${PRODUCT_NAME}"
                "${TARGET_FOLDER}/PrismaUI/views/${PRODUCT_NAME}"
            VERBATIM
        )
    endif()
endfunction()
