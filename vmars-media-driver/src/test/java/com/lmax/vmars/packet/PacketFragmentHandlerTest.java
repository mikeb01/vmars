package com.lmax.vmars.packet;

import org.agrona.concurrent.UnsafeBuffer;
import org.junit.Test;

import static com.lmax.vmars.packet.PacketHandler.NEW_STREAM;
import static com.lmax.vmars.packet.PacketHandler.PACKET_LOSS;
import static com.lmax.vmars.packet.PacketHandler.RESEND;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

public class PacketFragmentHandlerTest
{
    private final PacketFlyweight flyweight = mock(PacketFlyweight.class);
    private final PacketHandler packetHandler = mock(PacketHandler.class);
    private final PacketFragmentHandler handler = new PacketFragmentHandler(packetHandler, flyweight);

    @Test
    public void shouldStartAndContinueStream() throws Exception
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

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(NEW_STREAM));

        when(flyweight.tcpSequence()).thenReturn(sequence + payloadLength);
        when(flyweight.payloadLength()).thenReturn(payloadLength + 2);

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(0));
    }

    @Test
    public void shouldStartAndFlagResend() throws Exception
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

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(NEW_STREAM));

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(RESEND));
    }

    @Test
    public void shouldStartAndSignalPacketLoss() throws Exception
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

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(NEW_STREAM));

        when(flyweight.tcpSequence()).thenReturn(sequence + payloadLength + 1);
        when(flyweight.payloadLength()).thenReturn(payloadLength + 2);

        handler.onFragment(new UnsafeBuffer(new byte[0]), 0, 0, null);

        verify(packetHandler).onPacket(any(StreamContext.class), any(PacketFlyweight.class), eq(NEW_STREAM | PACKET_LOSS));
    }
}