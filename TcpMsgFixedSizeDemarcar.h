#ifndef __TCP_MSG_DEMARCAR_FIXED_SIZE_H__
#define __TCP_MSG_DEMARCAR_FIXED_SIZE_H__

#include "TcpMsgDemarcar.h"

class TcpClient;

class TcpMsgFixedSizeDemarcar : public TcpMsgDemarcar
{
public:
    TcpMsgFixedSizeDemarcar(uint16_t fixed_size);

protected:
    bool IsBufferReadyToFlush(const std::shared_ptr<ByteCircularBuffer>& bcb) const override;
    bool ReadMsg(const std::shared_ptr<ByteCircularBuffer>& bcb) override;

private:
    uint16_t msg_fixed_size;

};

#endif