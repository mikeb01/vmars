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
    private final PacketFlyweight packetFlyweight;
    private final Map<StreamId, StreamContext> streams = new HashMap<>();

    public PacketFragmentHandler(PacketHandler packetHandler)
    {
        this(packetHandler, new PacketFlyweight());
    }

    PacketFragmentHandler(PacketHandler packetHandler, PacketFlyweight packetFlyweight)
    {
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

        if (null == context)
        {
            context = new StreamContext(streamId);
            streams.put(streamId, context);
            context.setSequence(packetFlyweight.tcpSequence(), packetFlyweight.payloadLength());
            System.out.println("New Stream");
        }
        else
        {
            final int seqDiff = packetFlyweight.tcpSequence() - context.nextExpectedSequence();
            if (0 == seqDiff)
            {
                context.setSequence(packetFlyweight.tcpSequence(), packetFlyweight.payloadLength());
                System.out.printf("Sequence match%n");
            }
            else if (seqDiff < 0)
            {
                System.out.printf("Resend! Expected: %d, found: %d%n", packetFlyweight.tcpSequence(), context.nextExpectedSequence());
            }
            else
            {
                System.out.printf("Missed packets! Expected: %d, found: %d%n", packetFlyweight.tcpSequence(), context.nextExpectedSequence());
            }
        }

        debug(System.out, packetFlyweight);
    }

    private static void debug(PrintStream out, PacketFlyweight packetFlyweight)
    {
        out.format("Timestamp: %d.%09d%n", packetFlyweight.timestampSeconds(), packetFlyweight.timestampNanos());
        out.format("tcpSourcePort: %d, tcpDestPort: %d%n", packetFlyweight.tcpSourcePort(), packetFlyweight.tcpDestPort());
        out.format("tcpOffset: %d%n", packetFlyweight.tcpHeaderOffset());
        out.format("tcpSequence: %d%n", packetFlyweight.tcpSequence());
        out.format("payloadLength: %d%n", packetFlyweight.payloadLength());

        byte[] bytes = new byte[packetFlyweight.payloadLength()];
        packetFlyweight.buffer().getBytes(packetFlyweight.payloadOffset(), bytes);
        out.format("Data: %s%n", new String(bytes, US_ASCII));

        out.println("---");
    }

    public static void main(String[] args)
    {
        Aeron.Context ctx = new Aeron.Context()
            .idleStrategy(SLEEPING_MILLIS_IDLE_STRATEGY);

        try (Aeron aeron = Aeron.connect(ctx);
             final Subscription subscription = aeron.addSubscription("aeron:ipc", 42))
        {
            final PacketFragmentHandler handler = new PacketFragmentHandler();

            while (!Thread.currentThread().isInterrupted())
            {
                final int poll = subscription.poll(handler, 100);
                SLEEPING_MILLIS_IDLE_STRATEGY.idle(poll);
            }
        }
    }
}
