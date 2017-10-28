package com.lmax.vmars;

import io.aeron.Aeron;
import io.aeron.Subscription;
import io.aeron.driver.MediaDriver;
import io.aeron.driver.ThreadingMode;
import org.agrona.concurrent.ShutdownSignalBarrier;
import org.agrona.concurrent.SleepingMillisIdleStrategy;

import java.time.ZonedDateTime;

import static java.nio.charset.StandardCharsets.US_ASCII;

public class Main
{

    public static final SleepingMillisIdleStrategy SLEEPING_MILLIS_IDLE_STRATEGY = new SleepingMillisIdleStrategy(4);

    public static void main(String[] args) throws InterruptedException
    {
        MediaDriver.Context ctx = (new MediaDriver.Context())
            .termBufferSparseFile(false)
            .threadingMode(ThreadingMode.SHARED)
            .sharedIdleStrategy(SLEEPING_MILLIS_IDLE_STRATEGY)
            .dirDeleteOnStart(true);

        try (MediaDriver ignored = MediaDriver.launch(ctx))
        {
            System.out.println("Started media driver");

            Thread t = new Thread(Main::pollSubscription);
            t.setDaemon(true);
            t.start();

            new ShutdownSignalBarrier().await();

            t.interrupt();
            t.join();
            System.out.println("Shutdown Driver...");
        }
    }

    private static void pollSubscription()
    {
        Aeron.Context ctx = new Aeron.Context()
            .idleStrategy(SLEEPING_MILLIS_IDLE_STRATEGY);

        Aeron aeron = Aeron.connect(ctx);

        try (final Subscription subscription = aeron.addSubscription("aeron:ipc", 42))
        {
            System.out.printf("Created subscription: %d%n", subscription.registrationId());

            while (!Thread.currentThread().isInterrupted())
            {
                final int numOfFragments = subscription.poll(
                    (buffer, offset, length, header) ->
                    {
                        long sec = -1;
                        long nsec = -1;
                        if (length > 16)
                        {
                            sec = buffer.getLong(offset);
                            nsec = buffer.getLong(offset + 8);
                        }

                        System.out.printf(
                            "[%s] Message - timestamp: %d.%d, offset: %d, limit: %d%n",
                            ZonedDateTime.now(),
                            sec, nsec,
                            offset, length);
                    },
                    10
                );

                SLEEPING_MILLIS_IDLE_STRATEGY.idle(numOfFragments);
            }
        }
    }
}
