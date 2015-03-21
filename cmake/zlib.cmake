IF (NOT THIRDPARTY_PATH)
    SET(THIRDPARTY_PATH "${CMAKE_CURRENT_LIST_DIR}/../3rdParty")
ENDIF (NOT THIRDPARTY_PATH)

SET(ZLIB_INCLUDE_DIR ${THIRDPARTY_PATH}/zlib)

SET(ZLIB_SOURCES
    ${THIRDPARTY_PATH}/zlib/adler32.c
    ${THIRDPARTY_PATH}/zlib/compress.c
    ${THIRDPARTY_PATH}/zlib/crc32.c
    ${THIRDPARTY_PATH}/zlib/deflate.c
    ${THIRDPARTY_PATH}/zlib/inflate.c
    ${THIRDPARTY_PATH}/zlib/inftrees.c
    ${THIRDPARTY_PATH}/zlib/inffast.c
    ${THIRDPARTY_PATH}/zlib/trees.c
    ${THIRDPARTY_PATH}/zlib/zutil.c
)
