#ifndef __BYTE_CIRCULAR_BUFFER_H__
#define __BYTE_CIRCULAR_BUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include <string>

#define DEFAULT_CBC_SIZE 10240

class ByteCircularBuffer
{
public:
    ByteCircularBuffer(uint16_t size = DEFAULT_CBC_SIZE);

    inline uint16_t GetCurrentSize() const {return current_size;}

    uint16_t Write(const char* data, uint16_t data_size);
    uint16_t Read(char* buffer, uint16_t data_size, bool remove_read_bytes);
    bool IsFull() const;
    uint16_t AvailableSize() const;
    void Reset();

private:
    char* BCB(uint16_t n);

private:
    std::string buffer;
    uint16_t buffer_size;
    uint16_t front;
    uint16_t rear;
    uint16_t current_size;
};

#endif