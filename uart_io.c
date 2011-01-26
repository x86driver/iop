#include "uart_io.h"

static int Uart_fd = 0;

int UartGetFD(void)
{
    return Uart_fd;
}

int Uart_Open(char *pDevPath)
{
    Uart_fd = open( pDevPath, O_RDWR);
    if ( Uart_fd < 0 )
    {
        uart_dbg("Failed(%d:%s) to open uart(%s).", errno, strerror(errno), pDevPath);
        return -1;
    }
    return Uart_fd;
}

int Uart_Setup(u32 baud_rate)
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
                uart_dbg("Failed to set the parameters of termios");
            } else
            {
                uart_dbg("Set the paremeters successfully");
                ret = 0;
            }
        }
    }
    return ret;
}

int Uart_Close()
{
    if(Uart_fd > 0)
    {
        /* Clear the data in FIFO */
        tcflush(Uart_fd, TCIFLUSH);
        close(Uart_fd);
        Uart_fd = 0;
    }

    return Uart_fd;
}

int Uart_Read(u8 *pData, int max_size)
{
    int ret = 0;

    if(Uart_fd > 0)
    {
        ret = read(Uart_fd, pData, max_size);

        if (ret < 0)
        {
            uart_dbg("Failed(%d:%s) to read from uart", errno, strerror(errno));
        }
    }
    return ret;
}

int Uart_Write(u8 *pData, int max_size)
{
    int ret = 0;

    if(Uart_fd > 0)
    {
        ret = write(Uart_fd, pData, max_size);

        if (ret < 0)
        {
            uart_dbg("Failed(%d:%s) to write from uart", errno, strerror(errno));
        }
    }
    return ret;
}
