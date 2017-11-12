package com.lmax.vmars.packet;

import io.aeron.Aeron;
import io.aeron.Subscription;
import io.aeron.logbuffer.FragmentHandler;
import io.aeron.logbuffer.Header;
import org.agrona.DirectBuffer;

import java.io.PrintStream;
import java.util.HashMap;
import java.util.Map;

import static com.lmax.vmars.driver.Main.SLEEPING_MILLIS_IDLE_STRATEGY;
import static java.nio.charset.StandardCharsets.US_ASCII;

public class PacketFragmentHandler implements FragmentHandler
{
    private final PacketHandler packetHandler;
    private final PacketFlyweight packetFlyweight;
    private final Map<StreamId, StreamContext> streams = new HashMap<>();

    public PacketFragmentHandler(PacketHandler packetHandler)
    {
        this(packetHandler, new PacketFlyweight());
    }

    PacketFragmentHandler(PacketHandler packetHandler, PacketFlyweight packetFlyweight)
    {
        this.packetHandler = packetHandler;
        this.packetFlyweight = packetFlyweight;
    }

    @Override
    public void onFragment(DirectBuffer buffer, int offset, int length, Header header)
    {
        packetFlyweight.wrap(buffer, offset, length);

        final StreamId streamId = new StreamId(
            packetFlyweight.ipSourceAddr(), packetFlyweight.tcpSourcePort(),
            packetFlyweight.ipDestAddr(), packetFlyweight.tcpDestPort());

        StreamContext context = streams.get(streamId);

        int flags;
        if (null == context)
        {
            context = new StreamContext(streamId);
            streams.put(streamId, context);
            context.setSequence(packetFlyweight.tcpSequence(), packetFlyweight.payloadLength());

            flags = PacketHandler.NEW_STREAM;
        }
        else
        {
            final int seqDiff = packetFlyweight.tcpSequence() - context.nextExpectedSequence();
            if (0 == seqDiff)
            {
                context.setSequence(packetFlyweight.tcpSequence(), packetFlyweight.payloadLength());
                flags = 0;
            }
            else if (seqDiff < 0)
            {
                flags = PacketHandler.RESEND;
            }
            else
            {
                flags = PacketHandler.NEW_STREAM | PacketHandler.PACKET_LOSS;
            }
        }

        packetHandler.onPacket(context, packetFlyweight, flags);
    }

    public static void main(String[] args)
    {
        Aeron.Context ctx = new Aeron.Context()
            .idleStrategy(SLEEPING_MILLIS_IDLE_STRATEGY);

        try (Aeron aeron = Aeron.connect(ctx);
             final Subscription subscription = aeron.addSubscription("aeron:ipc", 42))
        {
            final PacketFragmentHandler handler = new PacketFragmentHandler(new DuplicateFilterPacketHandler(
                (stream, packet, flags) -> PacketFlyweight.debug(System.out, packet)
            ));

            while (!Thread.currentThread().isInterrupted())
            {
                final int poll = subscription.poll(handler, 100);
                SLEEPING_MILLIS_IDLE_STRATEGY.idle(poll);
            }
        }
    }
}
