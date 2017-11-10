package com.lmax.vmars.packet;

public interface PacketHandler
{
    int NEW_STREAM = 1;
    int PACKET_LOSS = NEW_STREAM << 1;

    void onPacket(PacketFlyweight packetFlyweight, int flags);
}
