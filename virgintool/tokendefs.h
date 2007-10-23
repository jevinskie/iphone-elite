#if !defined(_TOKENDEFS_H_)

#define false   0;
#define true    1;

typedef unsigned int uint16;
typedef unsigned long uint32;
typedef unsigned short bool;

typedef struct _ltoken{
    unsigned char imei[0x10];
    unsigned char ltoken_str[0x10];
    unsigned char hwid[0x200];
    unsigned char cfi_data[0x600];
    unsigned char seczone[0x2000];
    unsigned char hash[0x14];
} ltoken;

typedef struct _utoken{
    unsigned char utoken_str[0x10];
    unsigned char seczone[0x2000];
    unsigned char hash[0x80];
} utoken;

#define LITTLE_ENDIAN
#if defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN)

#define htons(A)  (A)
#define htonl(A)  (A)
#define ntohs(A)  (A)
#define ntohl(A)  (A)

#elif defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)

#define htons(A)  ((((uint16)(A) & 0xff00) >> 8) | \
                   (((uint16)(A) & 0x00ff) << 8))
#define htonl(A)  ((((uint32)(A) & 0xff000000) >> 24) | \
                   (((uint32)(A) & 0x00ff0000) >> 8)  | \
                   (((uint32)(A) & 0x0000ff00) << 8)  | \
                   (((uint32)(A) & 0x000000ff) << 24))
#define ntohs     htons
#define ntohl     htohl

#else
#error "Either BIG_ENDIAN or LITTLE_ENDIAN must be #defined, but not both."
#endif	/* defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) */

#endif	/* !defined(_TOKENDEFS_H_) */
