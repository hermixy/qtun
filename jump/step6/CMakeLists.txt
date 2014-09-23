
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(step6)

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../../warning2error.cmake)

ADD_EXECUTABLE(step6
    main.h
    main.c
    common.h
    common.c
    network.h
    network.c
    typedef.h
    hash.h
    hash.c
)
