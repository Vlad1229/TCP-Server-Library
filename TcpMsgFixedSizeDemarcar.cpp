#include "TcpMsgFixedSizeDemarcar.h"
#include "TcpClient.h"
#include "TcpServerController.h"
#include "ByteCircularBuffer.h"

TcpMsgFixedSizeDemarcar::TcpMsgFixedSizeDemarcar(uint16_t fixed_size) : TcpMsgDemarcar()
{
    this->msg_fixed_size = fixed_size;
}

bool TcpMsgFixedSizeDemarcar::IsBufferReadyToFlush(const std::shared_ptr<ByteCircularBuffer> &bcb) const
{
    return bcb->GetCurrentSize() >= msg_fixed_size;
}

bool TcpMsgFixedSizeDemarcar::ReadMsg(const std::shared_ptr<ByteCircularBuffer> &bcb)
{
    uint16_t bytes_read;

    while (bytes_read = bcb->Read(buffer, msg_fixed_size, true))
    {
        if (on_msg_demarked)
        {
            on_msg_demarked(buffer, bytes_read);
        }
    }
    return true;
}