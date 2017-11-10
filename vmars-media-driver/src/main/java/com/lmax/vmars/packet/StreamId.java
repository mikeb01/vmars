package com.lmax.vmars.packet;

public class StreamId
{
    final int sourceIp;
    final int sourcePort;
    final int destIp;
    final int destPort;

    public StreamId(int sourceIp, int sourcePort, int destIp, int destPort)
    {
        this.sourceIp = sourceIp;
        this.sourcePort = sourcePort;
        this.destIp = destIp;
        this.destPort = destPort;
    }

    @Override
    public boolean equals(Object o)
    {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        StreamId streamId = (StreamId) o;

        if (sourceIp != streamId.sourceIp) return false;
        if (sourcePort != streamId.sourcePort) return false;
        if (destIp != streamId.destIp) return false;
        return destPort == streamId.destPort;
    }

    @Override
    public int hashCode()
    {
        int result = sourceIp;
        result = 31 * result + sourcePort;
        result = 31 * result + destIp;
        result = 31 * result + destPort;
        return result;
    }
}
