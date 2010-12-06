#pragma once

typedef struct {
  ULONG         i[2];
  ULONG         buf[4];
  unsigned char in[64];
  unsigned char digest[16];
} MD5_CTX;

typedef void (WINAPI *MD5INIT)(MD5_CTX*);
typedef void (WINAPI *MD5UPDATE)(MD5_CTX*, void* input, unsigned int inlen);
typedef void (WINAPI *MD5FINAL)(MD5_CTX*);

extern MD5INIT MD5Init;
extern MD5UPDATE MD5Update;
extern MD5FINAL MD5Final;
