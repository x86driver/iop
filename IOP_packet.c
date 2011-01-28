#include "IOP_packet.h"
#include "mt3329_types.h"

extern int Uart_fd;

int packing_iop_packet(struct iop_content *pIOP_Content, char *pPacket)
{
    int  i=0, j=0;

    pPacket[j++] = DLE_CHAR;
    pPacket[j++] = pIOP_Content->inst_id;

    if( pIOP_Content->size == DLE_CHAR )
        pPacket[j++] = DLE_CHAR;

    pPacket[j++] = pIOP_Content->size;

    for ( i=0; i<pIOP_Content->size; i++ )
    {
        if(pIOP_Content->data[i] == DLE_CHAR)
            pPacket[j++] = DLE_CHAR;
        pPacket[j++] = pIOP_Content->data[i];
    }

    if(pIOP_Content->chksm == DLE_CHAR)
        pPacket[j++] = DLE_CHAR;

    pPacket[j++] = (char)pIOP_Content->chksm;

    pPacket[j++] = DLE_CHAR;
    pPacket[j++] = ETX_CHAR;

    return j;
}

int Send_IOP_Packet( int id, void *data, int size )
{
    struct   iop_content IOP_Content;
    int      i = 0;
    u8      *ptr = NULL;
    char     packet_raw[IOP_MAX_PACKET_SIZE];
    int      packet_size;

    memset( &IOP_Content, 0x0, sizeof(struct iop_content));
    memset( packet_raw, 0x0, sizeof(packet_raw));

    IOP_Content.inst_id = id;
    IOP_Content.size    = size;

    memcpy( &IOP_Content.data[0], data, size);

    IOP_Content.chksm   = IOP_Content.inst_id + IOP_Content.size;
    ptr = (u8*)data;
    for(i=0; i<size; i++)
    {
        IOP_Content.chksm   =  IOP_Content.chksm + *ptr;
        IOP_Content.data[i] = *ptr;
        ptr++;
    }
    IOP_Content.chksm = (u8)((IOP_Content.chksm ^ 0xFF) + 1);

    packet_size = packing_iop_packet(&IOP_Content,packet_raw);

    printf("send iop:\n");
    for (i = 0; i < packet_size; ++i) {
	if ((i % 8) == 0)
		printf("\n");
	printf("0x%x  ", packet_raw[i]);
    }
    printf("\n\n");
    ssize_t ret = write(Uart_fd, (u8*)packet_raw, packet_size);
    printf("send IOP ret: %lu\n", ret);

    return 0;
}


