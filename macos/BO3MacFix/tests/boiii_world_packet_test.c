#include "boiii_world_packet.h"
#include <assert.h>
int main(void) {
 uint8_t packet[1251] = {0};packet[1]=0xe0;
 assert(!boiii_world_packet(NULL,100));
 assert(!boiii_world_packet(packet,0));
 assert(!boiii_world_packet(packet,41));
 assert(boiii_world_packet(packet,693));
 assert(boiii_world_packet(packet,1251));
 packet[18]=0x40;assert(!boiii_world_packet(packet,1251));
 packet[18]=1;assert(!boiii_world_packet(packet,1251));
 packet[18]=0;packet[1]=0xff;assert(!boiii_world_packet(packet,1251));
 return 0;
}
