package com.lmax.vmars;

import io.aeron.Aeron;
import io.aeron.Subscription;
import io.aeron.driver.MediaDriver;
import io.aeron.driver.ThreadingMode;
import org.agrona.concurrent.ShutdownSignalBarrier;
import org.agrona.concurrent.SleepingMillisIdleStrategy;

import java.time.ZonedDateTime;
import java.util.Base64;

import static java.lang.String.format;

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

            Thread t1 = new Thread(Main::pollSubscription);
            t1.setName("Thing1");
            t1.setDaemon(true);
            t1.start();

            new ShutdownSignalBarrier().await();

            t1.interrupt();
            t1.join();

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
            final Base64.Encoder encoder = Base64.getEncoder();

            while (!Thread.currentThread().isInterrupted())
            {
                final int numOfFragments = subscription.poll(
                    (buffer, offset, length, header) ->
                    {
                        byte[] bytes = new byte[length];
                        buffer.getBytes(offset, bytes);
                        System.out.printf("[%d]: %s%n", length, encoder.encodeToString(bytes));
                    },
                    10
                );

                SLEEPING_MILLIS_IDLE_STRATEGY.idle(numOfFragments);
            }
        }
    }
}
