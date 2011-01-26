#include <stdio.h>
#include <stdlib.h>

#define STATE_DATA      0
#define STATE_START     1
#define STATE_ID        2
#define STATE_SIZE      3
#define STATE_END       4

struct IOP_packet
{
        unsigned char   id;
        unsigned char   size;
        unsigned char   start;
        unsigned char   end;
} gIOP;

unsigned char RxBuff[255];
unsigned char buffer[255];
unsigned char *buffptr = &buffer[0];

int iop_read(unsigned char *ptr, int size)
{
	*ptr = *buffptr++;
}

void get_gps_data()
{
	int count = 0;
	int ret   = 0;
	unsigned int status = STATE_DATA;

	/* Receive packet by indexing */
	if ( count > 254 )
	  count = 0;

    while (1) {
	if ( iop_read( &RxBuff[count], 1) ) /* Read 1 byte each time */
	{
		if ( status == STATE_START )
		{
			/* Received the ID field which is following the start byte(0x10) */
			gIOP.id = RxBuff[count];
			status  = STATE_ID;

		} else if ( status == STATE_ID )
		{
			/* Received the Size field which is following the ID field */
			gIOP.size = RxBuff[count];
			status    = STATE_SIZE;
		}
		if ( RxBuff[count] == 0x10 )
		{
			if ( count!=0 && RxBuff[(count-1)%255] == 0x10 )
			{
				/* Got redundent data of 0x10 */
				count--;
				status = STATE_DATA;

			} else if ( status == STATE_END )
			{
				/* Received the start byte which is following the CRC and end bytes(0x10 0x03) */
				gIOP.start = count + 3;
				status     = STATE_START;
			}

		} else if ( RxBuff[count]==0x03 )
		{
			if ( (count != 0) && (RxBuff[(count-1)%255] == 0x10) )
			{
				/* Received the End bytes which is combined by 0x10 and 0x03 */
				gIOP.end = count;
				status   = STATE_END;

			} else if ( status == STATE_ID || status == STATE_SIZE )
			{
				status = STATE_DATA;
			}

		}else if ( status!= STATE_ID && status!= STATE_SIZE )
		{
			status = STATE_DATA;
		}

		count++;

		/* Receving the end of packet */
		if( status == STATE_END )
		{
#if 0
			count=0;

			/* Check and parse the packet */
			ret = check_packet(&gIOP, RxBuff);
			switch (ret)
			{
			case 0x99:
			  pthread_mutex_unlock(&mutex_gps_data);
			  sched_yield();
			  adjust_time(&RxBuff[gIOP.start]);
			  pthread_mutex_lock(&mutex_gps_data);
			  break;
			case FAIL:
			default:
			  break;
			}
#endif
		}
	}
    }
}

int main()
{
	FILE *fp = fopen("c.bin", "rb");
	if (!fp) {
		perror("c.bin");
		exit(1);
	}

	fread(buffer, 152, 1, fp);
	fclose(fp);
	get_gps_data();
	return 0;
}

