#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "TcpMsgDemarcar.h"
#include "ByteCircularBuffer.h"
#include "TcpClient.h"

TcpMsgDemarcar::TcpMsgDemarcar()
{
    buffer = new char[MAX_BUFFER_SIZE];
}

bool TcpMsgDemarcar::ProcessMsg(std::shared_ptr<ByteCircularBuffer> &bcb, const char* msg, uint16_t msg_size)
{
    assert(bcb->Write(msg, msg_size));

    if (!IsBufferReadyToFlush(bcb))
        return true;

    return ReadMsg(bcb);
}