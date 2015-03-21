IF (NOT THIRDPARTY_PATH)
    SET(THIRDPARTY_PATH "${CMAKE_CURRENT_LIST_DIR}/../3rdParty")
ENDIF (NOT THIRDPARTY_PATH)

SET (OPENSSL_SOURCES
    ${THIRDPARTY_PATH}/openssl/crypto/modes/cbc128.c
    ${THIRDPARTY_PATH}/openssl/crypto/aes/aes_core.c
    ${THIRDPARTY_PATH}/openssl/crypto/aes/aes_misc.c
    ${THIRDPARTY_PATH}/openssl/crypto/aes/aes_cbc.c
    ${THIRDPARTY_PATH}/openssl/crypto/des/set_key.c
    ${THIRDPARTY_PATH}/openssl/crypto/des/cbc_enc.c
    ${THIRDPARTY_PATH}/openssl/crypto/des/des_enc.c
)
