
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(PROJECT_NAME step7)

PROJECT(${PROJECT_NAME})

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../../warning2error.cmake)

FIND_PACKAGE(ZLIB REQUIRED)
MESSAGE("zlib include dir: ${ZLIB_INCLUDE_DIR}")
MESSAGE("zlib libraries: ${ZLIB_LIBRARIES}")

FIND_PACKAGE(OpenSSL REQUIRED)
MESSAGE("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
MESSAGE("OpenSSL libraries: ${OPENSSL_LIBRARIES}")

ADD_EXECUTABLE(${PROJECT_NAME}
    main.h
    main.c
    common.h
    common.c
    network.h
    network.c
    typedef.h
    hash.h
    hash.c
    link.h
    link.c
    msg.h
    msg.c
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    "m"
)
