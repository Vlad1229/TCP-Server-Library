#ifndef __TCP_MSG_DEMARCAR_H__
#define __TCP_MSG_DEMARCAR_H__

#include <stdint.h>
#include <functional>
#include <memory>
#include <string>

#define DEFAULT_CBC_SIZE 10240
#define MAX_BUFFER_SIZE 1024

class ByteCircularBuffer;

enum class TcpMsgDemarcarType 
{
    TCP_DEMARCAR_NONE,
    TCP_DEMARCAR_FIXED_SIZE,
    TCP_DEMARCAR_VARIABLE_SIZE,
    TCP_DEMARCAR_PATTERN
};

class TcpClient;

class TcpMsgDemarcar
{
public:
    TcpMsgDemarcar();

    bool ProcessMsg(std::shared_ptr<ByteCircularBuffer> bcb, const char* msg, uint16_t msg_size);

protected:
    virtual bool IsBufferReadyToFlush(std::shared_ptr<ByteCircularBuffer> bcb) const = 0;
    virtual bool ReadMsg(std::shared_ptr<ByteCircularBuffer> bcb) = 0;

public:
    std::function<void(const char*, uint16_t)> msg_demarked;

protected:
    char* buffer;
};

#endif