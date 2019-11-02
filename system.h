#pragma once

#define O_RDONLY    00
#define O_WRONLY    01
#define O_RDWR      02

#define O_CREAT   0100
#define O_EXCL    0200
#define O_RUNC   01000

int dputc(unsigned ch);

int open(const char* path, unsigned flags, unsigned mode);
int close(int fd);
int read(int fd, void* ptr, int len);
int write(int fd, void* ptr, int len);

