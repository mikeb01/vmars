package com.lmax.vmars.driver;

import io.aeron.driver.MediaDriver;
import io.aeron.driver.ThreadingMode;
import org.agrona.concurrent.ShutdownSignalBarrier;
import org.agrona.concurrent.SleepingMillisIdleStrategy;

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

            new ShutdownSignalBarrier().await();

            System.out.println("Shutdown Driver...");
        }
    }
}
