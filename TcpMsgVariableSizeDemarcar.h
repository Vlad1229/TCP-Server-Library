#ifndef __TCP_MSG_DEMARCAR_VARIABLE_SIZE_H__
#define __TCP_MSG_DEMARCAR_VARIABLE_SIZE_H__

#include "TcpMsgDemarcar.h"

class TcpClient;

class TcpMsgVariableSizeDemarcar : public TcpMsgDemarcar
{
public:
    TcpMsgVariableSizeDemarcar(uint16_t header_size);

protected:
    bool IsBufferReadyToFlush(const std::shared_ptr<ByteCircularBuffer>& bcb) const override;
    bool ReadMsg(const std::shared_ptr<ByteCircularBuffer>& bcb) override;

private:
    bool IsReadingMsg = false;
    uint16_t msg_header_size;
    uint16_t msg_size;
};

#endif