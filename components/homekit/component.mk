COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_PRIV_INCLUDEDIRS := src

COMPONENT_SRCDIRS := src 

WOLFSSL_SETTINGS =        \
    -DSIZEOF_LONG_LONG=8  \
    -DSMALL_SESSION_CACHE \
    -DWOLFSSL_SMALL_STACK \
	-DWOLFCRYPT_HAVE_SRP  \
	-DWOLFSSL_SHA512      \
    -DHAVE_CHACHA         \
	-DHAVE_HKDF			  \
    -DHAVE_ONE_TIME_AUTH  \
    -DHAVE_ED25519        \
	-DHAVE_ED25519_KEY_EXPORT\
	-DHAVE_ED25519_KEY_IMPORT\
    -DHAVE_OCSP           \
    -DHAVE_CURVE25519     \
	-DHAVE_POLY1305       \
    -DHAVE_SNI            \
    -DHAVE_TLS_EXTENSIONS \
    -DTIME_OVERRIDES      \
    -DNO_DES              \
    -DNO_DES3             \
    -DNO_DSA              \
    -DNO_ERROR_STRINGS    \
    -DNO_HC128            \
    -DNO_MD4              \
    -DNO_OLD_TLS          \
    -DNO_PSK              \
    -DNO_PWDBASED         \
    -DNO_RC4              \
    -DNO_RABBIT           \
    -DNO_STDIO_FILESYSTEM \
    -DNO_WOLFSSL_DIR      \
    -DNO_DH               \
    -DWOLFSSL_STATIC_RSA  \
    -DWOLFSSL_IAR_ARM     \
    -DNDEBUG              \
    -DHAVE_CERTIFICATE_STATUS_REQUEST \
    -DCUSTOM_RAND_GENERATE_SEED=os_get_random

CFLAGS = \
    -fstrict-volatile-bitfields \
    -ffunction-sections         \
    -fdata-sections             \
    -mlongcalls                 \
    -nostdlib                   \
    -ggdb                       \
    -Os                         \
    -DNDEBUG                    \
    -std=gnu99                  \
    -Wno-old-style-declaration  \
    $(WOLFSSL_SETTINGS)
