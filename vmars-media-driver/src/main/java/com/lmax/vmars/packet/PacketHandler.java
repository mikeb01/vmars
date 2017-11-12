package com.lmax.vmars.packet;

public interface PacketHandler
{
    int NEW_STREAM = 1;
    int PACKET_LOSS = NEW_STREAM << 1;
    int RESEND = PACKET_LOSS << 1;

    void onPacket(StreamContext context, PacketFlyweight packetFlyweight, int flags);

    static boolean isResend(int flags)
    {
        return (flags & RESEND) != 0;
    }
}
