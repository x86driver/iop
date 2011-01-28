#ifndef __UART_IO_H__
#define __UART_IO_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/times.h>

#if 0
typedef unsigned char	boolean;
typedef unsigned char	u8;
typedef signed   char  	s8;
typedef unsigned short	u16;
typedef signed   short	s16;
typedef unsigned int	u32;
typedef signed   int	s32;
typedef double   u64;
#endif

#define DLE_CHAR    0x10
#define ETX_CHAR    0x03

#define DFLT_WAIT_TIME 3
#define DFLT_WAIT_BYTES 0

#endif
