package com.lmax.vmars.packet;

import org.junit.Test;

import static com.lmax.vmars.packet.PacketHandler.NEW_STREAM;
import static com.lmax.vmars.packet.PacketHandler.RESEND;
import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class DuplicateFilterPacketHandlerTest
{
    @Test
    public void discardDuplicates() throws Exception
    {
        final PacketHandler delegate = mock(PacketHandler.class);
        final DuplicateFilterPacketHandler handler = new DuplicateFilterPacketHandler(delegate);

        handler.onPacket(null, null, NEW_STREAM);
        handler.onPacket(null, null, 0);
        handler.onPacket(null, null, RESEND);

        verify(delegate, times(1)).onPacket(null, null, NEW_STREAM);
        verify(delegate, times(1)).onPacket(null, null, 0);
        verify(delegate, never()).onPacket(null, null, RESEND);
    }
}