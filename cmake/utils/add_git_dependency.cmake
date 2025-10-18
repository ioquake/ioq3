include_guard(GLOBAL)

function(add_git_dependency SOURCE_FILE)
    set(GIT_DIR ${CMAKE_SOURCE_DIR}/.git)
    if(NOT EXISTS ${GIT_DIR})
        return()
    endif()

    # If we're a submodule, .git won't be a directory
    if(NOT IS_DIRECTORY ${GIT_DIR})
        file(READ ${GIT_DIR} GIT_FILE_CONTENT)
        string(REGEX MATCH "gitdir: (.+)" MATCHED_PATH ${GIT_FILE_CONTENT})
        if(NOT MATCHED_PATH)
            return()
        endif()

        string(STRIP ${CMAKE_MATCH_1} GIT_DIR)
    endif()

    set(GIT_FILES)
    list(APPEND GIT_FILES ${GIT_DIR}/HEAD)
    list(APPEND GIT_FILES ${GIT_DIR}/packed-refs)

    file(READ ${GIT_DIR}/HEAD GIT_HEAD)
    string(REGEX MATCH "^ref: (.+)$" HAVE_REF ${GIT_HEAD})
    if(HAVE_REF)
        set(GIT_REF_PATH ${CMAKE_MATCH_1})
        string(STRIP ${GIT_REF_PATH} GIT_REF_PATH)
        list(APPEND GIT_FILES ${GIT_DIR}/${GIT_REF_PATH})
    endif()

    foreach(GIT_FILE IN LISTS GIT_FILES)
        if(EXISTS ${GIT_FILE})
            set_source_files_properties(${SOURCE_FILE}
                PROPERTIES OBJECT_DEPENDS ${GIT_FILE})
        endif()
    endforeach()
endfunction()
