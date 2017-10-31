package com.lmax.vmars;

import org.agrona.DirectBuffer;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

public class PacketFlyweight
{
    private static final int TIMESTAMP_SEC_OFFSET = 0;
    private static final int TIMESTAMP_NSEC_OFFSET = TIMESTAMP_SEC_OFFSET + 8;
    private static final int ETH_DEST_ADDR_OFFSET = TIMESTAMP_NSEC_OFFSET + 8;
    private static final int ETH_SOURCE_ADDR_OFFSET = ETH_DEST_ADDR_OFFSET + 6;
    private static final int ETH_PROTO_OFFSET = ETH_SOURCE_ADDR_OFFSET + 6;

    private static final int IP_HEADER_BASE_OFFSET = ETH_PROTO_OFFSET + 2;
    private static final int IP_VERSION_IHL_OFFSET = IP_HEADER_BASE_OFFSET;
    private static final int IP_TOS_OFFSET = IP_VERSION_IHL_OFFSET + 1;
    private static final int IP_TOTAL_LENGTH_OFFSET = IP_TOS_OFFSET + 1;
    private static final int IP_IDENTIFICATION_OFFSET = IP_TOTAL_LENGTH_OFFSET + 2;
    private static final int IP_FRAGMENT_OFFSET = IP_IDENTIFICATION_OFFSET + 2;
    private static final int IP_TTL_OFFSET = IP_FRAGMENT_OFFSET + 2;
    private static final int IP_PROTOCOL_OFFSET = IP_TTL_OFFSET + 1;
    private static final int IP_HEADER_CHECKSUM_OFFSET = IP_PROTOCOL_OFFSET + 1;
    private static final int IP_SOURCE_ADDRESS_OFFSET = IP_HEADER_CHECKSUM_OFFSET + 2;
    private static final int IP_DEST_ADDRESS_OFFSET = IP_SOURCE_ADDRESS_OFFSET + 4;

    private static final int TCP_SOURCE_PORT_OFFSET = 0;
    private static final int TCP_DEST_PORT_OFFSET = TCP_SOURCE_PORT_OFFSET + 2;
    private static final int TCP_SEQ_OFFSET = TCP_DEST_PORT_OFFSET + 2;
    private static final int TCP_ACK_SEQ_OFFSET = TCP_SEQ_OFFSET + 4;
    private static final int TCP_HEADER_FLAGS_OFFSET = TCP_ACK_SEQ_OFFSET + 4;
    private static final int TCP_WINDOW_SIZE_OFFSET = TCP_HEADER_FLAGS_OFFSET + 2;
    private static final int TCP_CHECKSUM_OFFSET = TCP_WINDOW_SIZE_OFFSET + 2;
    private static final int TCP_URGENT_POINTER_OFFSET = TCP_CHECKSUM_OFFSET + 2;

    private DirectBuffer buffer;
    private int offset;
    private int length;

    public void wrap(DirectBuffer buffer, int offset, int length)
    {
        this.buffer = buffer;
        this.offset = offset;
        this.length = length;
    }

    public long timestampSeconds()
    {
        return buffer.getLong(offset + TIMESTAMP_SEC_OFFSET, ByteOrder.nativeOrder());
    }

    public long timestampNanos()
    {
        return buffer.getLong(offset + TIMESTAMP_NSEC_OFFSET, ByteOrder.nativeOrder());
    }

    public int ethProtocol()
    {
        return 0xFFFF & buffer.getShort(offset + ETH_PROTO_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public int totalLength()
    {
        return 0xFFFF & buffer.getShort(offset + IP_TOTAL_LENGTH_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public int ipHeaderLength()
    {
        return 4 * (0xF & buffer.getByte(offset + IP_VERSION_IHL_OFFSET));
    }

    public int ipProtocol()
    {
        return 0xFF & buffer.getByte(offset + IP_PROTOCOL_OFFSET);
    }

    public int ipSourceAddr()
    {
        return buffer.getInt(offset + IP_SOURCE_ADDRESS_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public InetAddress ipSourceInetAddress() throws UnknownHostException
    {
        final byte[] ipBytes = new byte[4];
        buffer.getBytes(offset + IP_SOURCE_ADDRESS_OFFSET, ipBytes);
        return InetAddress.getByAddress(ipBytes);
    }

    public int ipDestAddr()
    {
        return buffer.getInt(offset + IP_DEST_ADDRESS_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public InetAddress ipDestInetAddress() throws UnknownHostException
    {
        final byte[] ipBytes = new byte[4];
        buffer.getBytes(offset + IP_DEST_ADDRESS_OFFSET, ipBytes);
        return InetAddress.getByAddress(ipBytes);
    }

    public int tcpHeaderOffset()
    {
        return IP_HEADER_BASE_OFFSET + ipHeaderLength();
    }

    public int tcpSourcePort()
    {
        return 0xFFFF & buffer.getShort(offset + tcpHeaderOffset() + TCP_SOURCE_PORT_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public int tcpDestPort()
    {
        return 0xFFFF & buffer.getShort(offset + tcpHeaderOffset() + TCP_DEST_PORT_OFFSET, ByteOrder.BIG_ENDIAN);
    }

    public int tcpHeaderLength()
    {
        return 4 * ((0xF0 & buffer.getByte(offset + tcpHeaderOffset() + TCP_HEADER_FLAGS_OFFSET)) >>> 4);
    }

    public int payloadOffset()
    {
        return offset + tcpHeaderOffset() + tcpHeaderLength();
    }

    public int payloadLength()
    {
        return totalLength() - (ipHeaderLength() + tcpHeaderLength());
    }

    public DirectBuffer buffer()
    {
        return buffer;
    }

    public int tcpSequence()
    {
        return buffer.getInt(tcpHeaderOffset() + TCP_SEQ_OFFSET, ByteOrder.BIG_ENDIAN);
    }
}
