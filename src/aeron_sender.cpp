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

    int64_t offer(aeron::AtomicBuffer& message)
    {
        return m_publication->offer(message);
    }
};

vmars_aeron_ctx vmars_aeron_setup()
{
    auto* sender = new VmarsAeronCtx{};
    return reinterpret_cast<vmars_aeron_ctx>(sender);
}

int64_t vmars_aeron_send(vmars_aeron_ctx sender, uint8_t* buffer, int32_t length)
{
    auto* ctx = reinterpret_cast<VmarsAeronCtx*>(sender);
    aeron::AtomicBuffer data{buffer, length};

    while (0 > ctx->offer(data))
    {
        std::this_thread::yield();
    }
}