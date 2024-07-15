#include "avpacket.h"
#define MEM_ITEM_SIZE (20*1024*102)
#define AVPACKET_LOOP_COUNT 1000
void av_packet_test1(){
    AVPacket *pkt = NULL;
    int ret = 0;
    pkt = av_packet_alloc();
    ret = av_new_packet(pkt,MEM_ITEM_SIZE);
    memccpy(pkt->data,(void*)&av_packet_test1,1,MEM_ITEM_SIZE);
    av_packet_unref(pkt);
    av_packet_free(&pkt);
}

void av_packet_test2(){

}

void av_packet_test3(){

}

void av_packet_test4(){

}


void av_packet_test(){

}
