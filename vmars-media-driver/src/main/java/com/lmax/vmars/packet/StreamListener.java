package com.lmax.vmars.packet;

import org.agrona.DirectBuffer;

public interface StreamListener
{
    void onPacket(DirectBuffer buffer, int offset, int length, int flags);
}
