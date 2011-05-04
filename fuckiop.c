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

int Uart_fd;
FILE *gps_fp;
unsigned char buffer[256];

int uart_setup(unsigned int baud_rate)
{
	int    ret = 0;
	struct termios termios_now;
	struct termios termios_new;
	struct termios termios_chk;

	if( Uart_fd > 0 )
	{
		memset( &termios_now, 0x0, sizeof(struct termios));
		memset( &termios_new, 0x0, sizeof(struct termios));
		memset( &termios_chk, 0x0, sizeof(struct termios));
	}

	tcflush(Uart_fd, TCIOFLUSH);
	tcgetattr(Uart_fd, &termios_new);
	termios_new.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios_new.c_cflag &= ~(CSIZE | PARENB);
	termios_new.c_cflag |= CS8;
	termios_new.c_cflag &= ~CRTSCTS;
	termios_new.c_cc[VTIME] = 0;  /* Over than VTIME*0.1 second(s), read operation will return from blocked */
	termios_new.c_cc[VMIN]  = 1; /* Over than VMIN bytes received, read operation will return from blocked */
	tcsetattr(Uart_fd, TCSANOW, &termios_new);
	tcflush(Uart_fd, TCIOFLUSH);
	tcsetattr(Uart_fd, TCSANOW, &termios_new);
	tcflush(Uart_fd, TCIOFLUSH);
	tcflush(Uart_fd, TCIOFLUSH);
	cfsetospeed(&termios_new, baud_rate);
	cfsetispeed(&termios_new, baud_rate);
	tcsetattr(Uart_fd, TCSANOW, &termios_new);

	if ( ret == 0 )
	{
		/* Write back finished, so read the settings again to check the content */
		ret = tcgetattr( Uart_fd, &termios_chk );
		if( ret == 0 )
		{
			if( termios_new.c_cflag != termios_chk.c_cflag )
			{
				printf("Failed to set the parameters of termios");
			} else
			{
				printf("Set the paremeters successfully\n");
				ret = 0;
			}
		}
	}
	return ret;
}

void show_iop()
{
    int i, size;
    size = (unsigned char)(buffer[2]) + 6;
    for (i = 0; i < size; ++i) {
        printf("0x%02x ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

static inline int remain()
{
    unsigned char *ptr = &buffer[3];
    do {
        if (read(Uart_fd, ptr++, 1) == 0) return 0;
        //fread(ptr++, 1, 1, gps_fp);
        if (*(ptr-1) == 0x10 && *(ptr-2) == 0x10)
            --ptr;
    } while (*(ptr-1) != 0x03 || *(ptr-2) != 0x10);

    return 1;
}

void reduce()
{
    unsigned char tempbuf[256];
    int size = buffer[2];
    unsigned char *ptr = &buffer[3];
    unsigned char *dst = &tempbuf[0];
    while (*ptr != 0x10 && *(ptr+1) != 0x03) {
        if (*ptr == 0x10)
            ++ptr;
        *dst++ = *ptr++;
    }
    memcpy(&buffer[3], &tempbuf[0], size+3);
}

int do_checksum()
{
    unsigned char checksum = 0;
    int j;
    for (j = 1; j < (unsigned char)buffer[2]+3; ++j)
        checksum += (unsigned char)buffer[j];
    checksum ^= 0xff;
    ++checksum;
    if (checksum != (unsigned char)buffer[buffer[2]+3]) {
        printf("Checksum error, correct = 0x%02x, buffer = 0x%02x\n", checksum, buffer[buffer[2]+3]);
        return 0;
    }
    return 1;
}

int parse_iop()
{
    memset(&buffer[0], 0, sizeof(buffer));

reread:
        //fread(&buffer[0], 2, 1, gps_fp);
        if (read(Uart_fd, &buffer[0], 2) == 0) return 0;
        if (buffer[0] == 0x10 && buffer[1] != 0x03) { /* beginning */
            //fread(&buffer[2], 1, 1, gps_fp);    /* read id, size */
            if (read(Uart_fd, &buffer[2], 1) == 0) return 0;
            if (remain() == 0) return 0;
        } else {
            printf("Can't locate beginning\n");
            goto reread;
        }

    if (do_checksum() == 0)
        show_iop();
    return 1;
}

int main()
{
    printf("Build on %s\n", __TIMESTAMP__);
//    Uart_fd = open("/dev/tcc-uart5", O_RDONLY);
    Uart_fd = open("gps.bin", O_RDONLY);
    uart_setup(B57600);

#if 1
    gps_fp = fdopen(Uart_fd, "rb");
    if (!gps_fp) {
        perror("fdopen");
    }
#endif

    int i = 0;
//    for (i = 0; i < 16; ++i) {
    while (1) {
//        printf("### Number %d of packets ###\n", i);
        if (parse_iop() == 0) break;
        printf("Check %d packets OK\r", ++i);
        fflush(NULL);
        if (feof(gps_fp)) {
            printf("FEOF detected!\n");
            break;
        }
    }

    printf("\n");
    fclose(gps_fp);
    close(Uart_fd);
    return 0;
}
