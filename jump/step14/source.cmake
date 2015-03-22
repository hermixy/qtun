SET(QTUN_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

SET(SOURCE_NETWORK
    ${QTUN_SOURCE_DIR}/network/msg.h
    ${QTUN_SOURCE_DIR}/network/msg.c
    ${QTUN_SOURCE_DIR}/network/msg_group.h
    ${QTUN_SOURCE_DIR}/network/msg_group.c
    ${QTUN_SOURCE_DIR}/network/network.h
    ${QTUN_SOURCE_DIR}/network/network.c
    ${QTUN_SOURCE_DIR}/network/server.c
    ${QTUN_SOURCE_DIR}/network/client.c
)
SOURCE_GROUP(network FILES ${SOURCE_NETWORK})

SET(SOURCE_STRUCT
    ${QTUN_SOURCE_DIR}/struct/active_vector.h
    ${QTUN_SOURCE_DIR}/struct/active_vector.c
    ${QTUN_SOURCE_DIR}/struct/hash.h
    ${QTUN_SOURCE_DIR}/struct/hash.c
    ${QTUN_SOURCE_DIR}/struct/link.h
    ${QTUN_SOURCE_DIR}/struct/link.c
    ${QTUN_SOURCE_DIR}/struct/pool.h
    ${QTUN_SOURCE_DIR}/struct/pool.c
    ${QTUN_SOURCE_DIR}/struct/group_pool.h
    ${QTUN_SOURCE_DIR}/struct/group_pool.c
    ${QTUN_SOURCE_DIR}/struct/typedef.h
    ${QTUN_SOURCE_DIR}/struct/vector.h
    ${QTUN_SOURCE_DIR}/struct/vector.c
)
SOURCE_GROUP(struct FILES ${SOURCE_STRUCT})

SET(SOURCE_LIBRARY
    ${QTUN_SOURCE_DIR}/library/common.h
    ${QTUN_SOURCE_DIR}/library/common.c
    ${QTUN_SOURCE_DIR}/library/library.h
    ${QTUN_SOURCE_DIR}/library/library.c
    ${QTUN_SOURCE_DIR}/library/proto.h
    ${QTUN_SOURCE_DIR}/library/script.h
    ${QTUN_SOURCE_DIR}/library/script.c
    ${QTUN_SOURCE_DIR}/library/win.h
    ${QTUN_SOURCE_DIR}/library/win.c
)
SOURCE_GROUP(library FILES ${SOURCE_LIBRARY})

SET(SOURCE
    ${QTUN_SOURCE_DIR}/main.h
    ${QTUN_SOURCE_DIR}/main.c

    ${SOURCE_NETWORK}
    ${SOURCE_STRUCT}
    ${SOURCE_LIBRARY}
)

IF (WIN32)
    SET(SOURCE_3RDPARTY
        ${QTUN_SOURCE_DIR}/3rdParty/getopt.h
        ${QTUN_SOURCE_DIR}/3rdParty/getopt.c
        ${QTUN_SOURCE_DIR}/3rdParty/getopt_long.c
    )
    LIST(APPEND SOURCE ${SOURCE_3RDPARTY})
    SOURCE_GROUP(3rdParty FILES ${SOURCE_3RDPARTY})
ENDIF ()
