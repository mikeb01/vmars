package com.lmax.vmars.packet;

import org.agrona.concurrent.UnsafeBuffer;
import org.junit.Test;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockingDetails;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

public class PacketFragmentHandlerTest
{
    private final PacketFlyweight flyweight = mock(PacketFlyweight.class);
    private final PacketHandler packetHandler = mock(PacketHandler.class);
    private final PacketFragmentHandler handler = new PacketFragmentHandler(packetHandler, flyweight);

    @Test
    public void shouldStartNewStream() throws Exception
    {
        final StreamId streamId = new StreamId(1, 2, 3, 4);
        final int sequence = 3245;
        final int payloadLength = 40;

        when(flyweight.ipSourceAddr()).thenReturn(streamId.sourceIp);
        when(flyweight.tcpSourcePort()).thenReturn(streamId.sourcePort);
        when(flyweight.ipDestAddr()).thenReturn(streamId.destIp);
        when(flyweight.tcpDestPort()).thenReturn(streamId.destPort);
        when(flyweight.tcpSequence()).thenReturn(sequence);
        when(flyweight.payloadLength()).thenReturn(payloadLength);

        handler.onFragment(new UnsafeBuffer(), 0, 0, null);

        verify(packetHandler).onPacket(any(PacketFlyweight.class), PacketHandler.NEW_STREAM);
    }
}