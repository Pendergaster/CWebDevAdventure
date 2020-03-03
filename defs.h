/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */

#ifndef DEFS_H
#define DEFS_H

typedef uint64_t    u64;
typedef int64_t     i64;
typedef uint32_t    u32;
typedef int32_t     i32;
typedef uint16_t    u16;
typedef int16_t     i16;
typedef uint8_t     u8;
typedef int8_t      i8;

#define ARRAY_SIZE(ARR) (sizeof((ARR)) / sizeof(*(ARR)))

static const char* SERVER_PORT = "12913";
static const char* SSL_CERT = "server.crt";
static const char* SSL_KEY = "server.key";

#endif /* DEFS_H */
