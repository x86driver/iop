#include "uart_io.h"
#include "mt3329_types.h"

#define IOP_MAX_DATA_SIZE     255
#define IOP_MAX_PACKET_SIZE   (IOP_MAX_DATA_SIZE + 6)

struct iop_content
{
    u8   inst_id;
    u8   size;
    u8   data[IOP_MAX_DATA_SIZE];
    int  chksm;
};

int packing_iop_packet(struct iop_content *pIOP_Content, char *pPacket);
int Send_IOP_Packet( int id, void *data, int size );

