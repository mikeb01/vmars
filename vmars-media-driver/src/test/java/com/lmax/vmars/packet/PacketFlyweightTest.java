package com.lmax.vmars.packet;

import org.agrona.concurrent.UnsafeBuffer;
import org.junit.Test;

import java.util.Base64;

import static java.nio.charset.StandardCharsets.US_ASCII;
import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class PacketFlyweightTest
{
    private final String base64EncodedPacketWithTimestamp =
        "cwz4WQAAAAB/pJMsAAAAAAAAAAAAAAAAAAAAAAgARQAAXTYpQABABgZwfwAAAX8AAAHCRicP278TLHLVvDqAGAFW/lEAAAEBCAqrKihQqyn" +
        "WPkhlcmUgaXMgYSBtZXNzYWdlIHRoYXQgSSBzZW50IGFzIGEgdGVzdC4K";

    @Test
    public void parseMessage() throws Exception
    {
        final byte[] bytes = Base64.getDecoder().decode(base64EncodedPacketWithTimestamp);
        final UnsafeBuffer unsafeBuffer = new UnsafeBuffer(bytes);
        final PacketFlyweight packetFlyweight = new PacketFlyweight();
        packetFlyweight.wrap(unsafeBuffer, 0, bytes.length);

        final int payloadOffset = packetFlyweight.payloadOffset();

        byte[] message = new byte[unsafeBuffer.capacity() - payloadOffset];

        assertThat(packetFlyweight.tcpSourcePort(), is(49734));
        assertThat(packetFlyweight.tcpDestPort(), is(9999));
        assertThat(packetFlyweight.payloadLength(), is(message.length));
        assertThat(packetFlyweight.ipHeaderLength(), is(20));
        assertThat(packetFlyweight.tcpHeaderLength(), is(32));
        assertThat(packetFlyweight.tcpSequence(), is(-608234708));

        packetFlyweight.buffer().getBytes(payloadOffset, message);
        String payload = new String(message, US_ASCII);

        assertThat(payload, is("Here is a message that I sent as a test.\n"));
    }
}