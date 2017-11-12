package com.lmax.vmars.packet;

public class DuplicateFilterPacketHandler implements PacketHandler
{
    private final PacketHandler delegate;

    public DuplicateFilterPacketHandler(PacketHandler delegate)
    {
        this.delegate = delegate;
    }

    @Override
    public void onPacket(StreamContext context, PacketFlyweight packetFlyweight, int flags)
    {
        if (!PacketHandler.isResend(flags))
        {
            delegate.onPacket(context, packetFlyweight, flags);
        }
    }
}
