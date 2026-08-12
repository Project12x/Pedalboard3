# Bootstrap the project-pinned CPM.cmake release into the build tree.
# Keeping the downloaded implementation out of the source tree avoids
# duplicating generated dependency-manager code while retaining a stable,
# hash-verified configure step.

set(CPM_DOWNLOAD_VERSION 0.42.1)
set(CPM_DOWNLOAD_SHA256
    f3a6dcc6a04ce9e7f51a127307fa4f699fb2bade357a8eb4c5b45df76e1dc6a5)
set(CPM_DOWNLOAD_LOCATION
    "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        EXPECTED_HASH "SHA256=${CPM_DOWNLOAD_SHA256}"
        TLS_VERIFY ON
        STATUS CPM_DOWNLOAD_STATUS
    )

    list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_ERROR_CODE)
    if(NOT CPM_DOWNLOAD_ERROR_CODE EQUAL 0)
        list(GET CPM_DOWNLOAD_STATUS 1 CPM_DOWNLOAD_ERROR_MESSAGE)
        message(FATAL_ERROR
            "Unable to download CPM.cmake ${CPM_DOWNLOAD_VERSION}: "
            "${CPM_DOWNLOAD_ERROR_MESSAGE}")
    endif()
endif()

include("${CPM_DOWNLOAD_LOCATION}")
