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
unsigned char RxBuff[256];

# define likely(x)  __builtin_expect(!!(x), 1)
# define unlikely(x)    __builtin_expect(!!(x), 0)

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

void show_length_iop()
{
    unsigned char *ptr = &RxBuff[0];
    while (1) {
        printf("0x%02x ", *ptr);
        if (*ptr == 0x03 && *(ptr-1) == 0x10)
            break;
        ++ptr;
    }
    printf("\n\n");
}

void show_iop()
{
    int i = 0;
    for (; i < (unsigned char)RxBuff[2]+3; ++i) {
        printf("0x%02x ", RxBuff[i]);
    }
#if 0
    do {
        printf("0x%02x ", *ptr++);
        if (*(ptr-1) == 0x03 && *(ptr-2) == 0x10)
            break;
    } while (1);
#endif
    printf("\n\n");
}

static inline int check_length(int count, int index)
{
    if ((count-5) != RxBuff[2]) {   /* Length error */
        printf("No. #%d length error, buffer = %d, receive = %d\n", index, RxBuff[2], count-5);
        return -1;
    }
    return 0;
}

static inline int remain()
{
    int count = 0;
    int flag = 0;
    unsigned char *ptr = &RxBuff[1];

    while (1) {
        if (read(Uart_fd, ptr, 1) == 0)
            return 0;
        if (flag == 1) {
            if (unlikely(*ptr == 0x03)) {
                ++count;
                break;
            }
            /* It should check *ptr == 0x10, but if not detect,
             * it seems packet lose.
             */
            flag = 0;
            continue;
        }
        if (*ptr == 0x10)
            flag = 1;

        ++ptr;
        ++count;
    }

    return count;
}

int do_checksum(int index)
{
    unsigned char checksum = 0;
    int j;
    int len = (unsigned char)RxBuff[2]+4;
    for (j = 1; j < len; ++j)
        checksum += (unsigned char)RxBuff[j];
    if (checksum != 0) {
        printf("\n\nNo. #%d checksum error, correct = 0x%02x, RxBuff = 0x%02x\n", index, checksum, RxBuff[RxBuff[2]+3]);
        return 0;
    }
    return 1;
}

int parse_iop(int index)
{
    int count;
    memset(&RxBuff[0], 0, sizeof(RxBuff));

    do {
        if (read(Uart_fd, &RxBuff[0], 1) == 0)
            return 0;
    } while (RxBuff[0] != 0x10);

    if ((count = remain()) == 0)
        return 0;

    if (check_length(count, index) == -1) {
        show_length_iop();
        return 2;
    }

    if (do_checksum(index) == 0) {
        show_iop();
        return 2;
    }
    return 1;
}

int main(int argc, char **argv)
{
    char *file;
    printf("Build on %s\n", __TIMESTAMP__);

    if (argc == 2) {
        file = argv[1];
        Uart_fd = open(file, O_RDONLY);
    } else {
        file = "/dev/tcc-uart5";
        system("echo 1 > /sys/class/misc/mt3329/mt3329_power");
        Uart_fd = open(file, O_RDONLY);
        uart_setup(B57600);
    }
    printf("Open %s OK\n", file);

    int i = 0, ret;
    int good = 0, bad = 0;
    while (1) {
        ret = parse_iop(i);
        if (ret == 0)
            break;
        if (ret == 1) {
            ++good;
            printf("Check %d packets OK\r", i);
            fflush(NULL);
        } else if (ret == 2)
            ++bad;
        ++i;
    }

    printf("\n\n");
    printf("Good: %d, Bad: %d, Ratio: %f %%\n", good, bad, (double)bad/(good+bad)*100.0);
    close(Uart_fd);
    return 0;
}
