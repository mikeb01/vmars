package com.lmax.vmars.packet;

public class StreamContext
{
    public final StreamId streamId;
    private int tcpSequence;
    private int payloadLength;

    public StreamContext(StreamId streamId)
    {
        this.streamId = streamId;
    }

    public void setSequence(int tcpSequence, int payloadLength)
    {
        this.tcpSequence = tcpSequence;
        this.payloadLength = payloadLength;
    }

    public int nextExpectedSequence()
    {
        return tcpSequence + payloadLength;
    }
}
