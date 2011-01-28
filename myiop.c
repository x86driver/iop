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
#include "UTL_func.h"
#include "IOP_packet.h"

int Uart_fd;

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

void show_time(unsigned char *buf)
{
        GPS_packed_remote_state_type *remote_packet = (GPS_packed_remote_state_type*)buf;
        time_t utcsec = (time_t) UTL_get_time(remote_packet->tow,
                                          remote_packet->wn_lng,
                                          remote_packet->leap_scnds);
        struct tm *tm = localtime(&utcsec);
        printf("time %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void send_packet()
{
}

int main()
{
	int packet = 0;
	int count = 0;
	unsigned char buffer[255];
	u8 data = 1;
	int read_count = 0;
	memset(&buffer[0], 0, sizeof(buffer));

	Uart_fd = open("/dev/tcc-uart5", O_RDWR);
	uart_setup(B57600);

	Send_IOP_Packet(10, &data, 2);

read_again:
	while (1) {
		read(Uart_fd, &buffer[count], 1);
		if (buffer[count] == 0x03 && buffer[count-1] == 0x10) {
			++count;
			break;
		}
		++count;
	}

	printf("packet: %d, count: %d\t", packet++, count);
	int i;
	for (i = 0; i < ((count < 16) ? count : 16); ++i)
		printf("0x%02x ", buffer[i]);
	printf("\n");

//	if (buffer[1] != 0x99) {
	if (read_count < 80) {
//	while (1) {
		count = 0;
		++read_count;
		goto read_again;
	}

	close(Uart_fd);

	show_time(buffer+3);

	GPS_packed_remote_state_type *remote_packet = (GPS_packed_remote_state_type*)(buffer+3);

	double latitude  = (double)remote_packet->posn_lat*(180.0/3.1415926535898); // Done
	double longitude = (double)remote_packet->posn_lon*(180.0/3.1415926535898); // Done

	printf("%f, %f\n", latitude, longitude);

	printf("size: %d, size: %d\n", sizeof(GPS_packed_sat_sts_type), sizeof(GPS_packed_remote_state_type));

	return 0;
}

