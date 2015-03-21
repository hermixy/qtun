IF (NOT THIRDPARTY_PATH)
    SET(THIRDPARTY_PATH "${CMAKE_CURRENT_LIST_DIR}/../3rdParty")
ENDIF (NOT THIRDPARTY_PATH)

SET(LUA_INCLUDE_DIR ${THIRDPARTY_PATH}/lua/src)

SET(LUA_SOURCES
    ${THIRDPARTY_PATH}/lua/src/lapi.c
    ${THIRDPARTY_PATH}/lua/src/lauxlib.c
    ${THIRDPARTY_PATH}/lua/src/lcode.c
    ${THIRDPARTY_PATH}/lua/src/lctype.c
    ${THIRDPARTY_PATH}/lua/src/ldebug.c
    ${THIRDPARTY_PATH}/lua/src/ldo.c
    ${THIRDPARTY_PATH}/lua/src/ldump.c
    ${THIRDPARTY_PATH}/lua/src/lfunc.c
    ${THIRDPARTY_PATH}/lua/src/lgc.c
    ${THIRDPARTY_PATH}/lua/src/llex.c
    ${THIRDPARTY_PATH}/lua/src/lmem.c
    ${THIRDPARTY_PATH}/lua/src/lobject.c
    ${THIRDPARTY_PATH}/lua/src/lopcodes.c
    ${THIRDPARTY_PATH}/lua/src/lparser.c
    ${THIRDPARTY_PATH}/lua/src/lstate.c
    ${THIRDPARTY_PATH}/lua/src/lstring.c
    ${THIRDPARTY_PATH}/lua/src/ltable.c
    ${THIRDPARTY_PATH}/lua/src/ltm.c
    ${THIRDPARTY_PATH}/lua/src/lundump.c
    ${THIRDPARTY_PATH}/lua/src/lvm.c
    ${THIRDPARTY_PATH}/lua/src/lzio.c
)
