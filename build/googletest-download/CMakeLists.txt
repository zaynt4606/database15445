cmake_minimum_required(VERSION 3.8)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
        GIT_REPOSITORY https://gitee.com/zaynt/googletest.git
        GIT_TAG main
        SOURCE_DIR "/opt/project/cmu15-445/bustub-private/build/googletest-src"
        BINARY_DIR "/opt/project/cmu15-445/bustub-private/build/googletest-build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        )
