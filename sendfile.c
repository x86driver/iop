#include <stdio.h>
#include <sys/sendfile.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <termios.h>
#include <string.h>

int uart_setup(int Uart_fd, unsigned int baud_rate)
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

void save_data(int fdin, int fdout, long size)
{
    long count = 0;
    ssize_t ret;
    printf("Saving %ld bytes data\n", size);
    do {
        ret = sendfile(fdout, fdin, NULL, size);
        printf("ret: %d\n", ret);
    } while (count < size);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s [size]\n", argv[0]);
        exit(1);
    }

    int fdin, fdout;
    fdin = open("/dev/tcc-uart5", O_RDONLY);
    if (fdin == -1)
        perror("open uart5");

    uart_setup(fdin, B57600);

    fdout = open("/system/gps.bin", O_WRONLY);
    if (fdout == -1)
        perror("open gps.bin");

    save_data(fdin, fdout, atol(argv[1]));
    return 0;
}
