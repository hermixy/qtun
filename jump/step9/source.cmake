SET(QTUN_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

SET(SOURCE
    ${QTUN_SOURCE_DIR}/main.h
    ${QTUN_SOURCE_DIR}/main.c

    ${QTUN_SOURCE_DIR}/network/msg.h
    ${QTUN_SOURCE_DIR}/network/msg.c
    ${QTUN_SOURCE_DIR}/network/network.h
    ${QTUN_SOURCE_DIR}/network/network.c
    ${QTUN_SOURCE_DIR}/network/server.c
    ${QTUN_SOURCE_DIR}/network/client.c

    ${QTUN_SOURCE_DIR}/struct/active_vector.h
    ${QTUN_SOURCE_DIR}/struct/active_vector.c
    ${QTUN_SOURCE_DIR}/struct/hash.h
    ${QTUN_SOURCE_DIR}/struct/hash.c
    ${QTUN_SOURCE_DIR}/struct/link.h
    ${QTUN_SOURCE_DIR}/struct/link.c
    ${QTUN_SOURCE_DIR}/struct/pool.h
    ${QTUN_SOURCE_DIR}/struct/pool.c
    ${QTUN_SOURCE_DIR}/struct/typedef.h
    ${QTUN_SOURCE_DIR}/struct/vector.h
    ${QTUN_SOURCE_DIR}/struct/vector.c

    ${QTUN_SOURCE_DIR}/common.h
    ${QTUN_SOURCE_DIR}/common.c
    ${QTUN_SOURCE_DIR}/library.h
    ${QTUN_SOURCE_DIR}/library.c
)
