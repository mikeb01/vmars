//
// Created by barkerm on 26/10/17.
//

#include <cstdint>
#include <Aeron.h>

extern "C"
{
    #include "aeron_sender.h"
}

const int VMARS_AERON_STREAM_ID = 42;

class VmarsAeronCtx
{
private:
    aeron::Context m_context;
    std::shared_ptr<aeron::Aeron> m_aeron;
    std::shared_ptr<aeron::Publication> m_publication;
    
public:
    VmarsAeronCtx() :
        m_context{}
    {
        m_aeron = aeron::Aeron::connect(m_context);
        const int64_t registrationId = m_aeron->addPublication("aeron:ipc", VMARS_AERON_STREAM_ID);
        m_publication = m_aeron->findPublication(registrationId);
        while (!m_publication)
        {
            std::this_thread::yield();
            m_publication = m_aeron->findPublication(registrationId);
        }
    }

    int64_t offer(std::array<aeron::AtomicBuffer, 2>& message)
    {
        if (m_publication)
        {
            return m_publication->offer(message);
        }
        else
        {
            fprintf(stderr, "publication is null\n");
            return -1;
        }
    }
};

vmars_aeron_ctx vmars_aeron_setup()
{
    auto* sender = new VmarsAeronCtx{};
    return reinterpret_cast<vmars_aeron_ctx>(sender);
}

typedef std::array<uint8_t, 16> buffer_t;

int64_t vmars_aeron_send(vmars_aeron_ctx sender, uint32_t tp_sec, uint32_t tp_nsec, uint8_t* buffer, int32_t length)
{
    auto* ctx = reinterpret_cast<VmarsAeronCtx*>(sender);
    AERON_DECL_ALIGNED(buffer_t timestamp, 16);

    std::array<aeron::AtomicBuffer, 2> dataItems{
        {{timestamp}, {buffer, length}}
    };

    dataItems[0].putInt64(0, tp_sec);
    dataItems[0].putInt64(8, tp_nsec);

    while (0 > ctx->offer(dataItems))
    {
        std::this_thread::yield();
    }
}