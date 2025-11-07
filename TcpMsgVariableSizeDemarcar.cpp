#include <cstring>
#include "TcpMsgVariableSizeDemarcar.h"
#include "TcpClient.h"
#include "TcpServerController.h"
#include "ByteCircularBuffer.h"

TcpMsgVariableSizeDemarcar::TcpMsgVariableSizeDemarcar(uint16_t header_size) : TcpMsgDemarcar()
{
    this->msg_header_size = header_size;
}

bool TcpMsgVariableSizeDemarcar::IsBufferReadyToFlush(const std::shared_ptr<ByteCircularBuffer> &bcb) const
{
    if (IsReadingMsg)
    {
        return bcb->GetCurrentSize() >= msg_size;
    }
    else
    {
        return bcb->GetCurrentSize() >= msg_header_size;
    }
}

bool TcpMsgVariableSizeDemarcar::ReadMsg(const std::shared_ptr<ByteCircularBuffer> &bcb)
{
    uint16_t bytes_read;

    while (true)
    {
        memset(buffer, 0, MAX_BUFFER_SIZE);
        if (bytes_read = bcb->Read(buffer, IsReadingMsg ? msg_size : msg_header_size, true))
        {
            if (IsReadingMsg)
            {
                IsReadingMsg = false;
                if (on_msg_demarked)
                {
                    on_msg_demarked(buffer, bytes_read);
                }
            }
            else
            {
                char header[bytes_read];
                for (int i = 0; i < bytes_read; i++)
                {
                    header[i] = buffer[i];
                }

                msg_size = std::atoi(header);

                if (msg_size == 0)
                {
                    return false;
                }

                IsReadingMsg = true;
            }
        }
        else
        {
            return true;
        }
    }
}