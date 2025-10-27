#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "ByteCircularBuffer.h"

ByteCircularBuffer::ByteCircularBuffer(uint16_t size)
{
    buffer_size = size;
    buffer.resize(size);
    current_size = 0;
    front = 0;
    rear = 0;
}

inline char* ByteCircularBuffer::BCB(uint16_t n)
{
    return &buffer[n];
}

uint16_t ByteCircularBuffer::Write(const char* data, uint16_t data_size)
{
    if (IsFull()) return 0;

    if (AvailableSize() < data_size) return 0;
    
    if (front < rear) 
    {
        memcpy(BCB(front), data, data_size);
        front += data_size;

        if (front == buffer_size) front = 0;

        current_size += data_size;
        return data_size;
    }

    uint16_t leading_space = buffer_size - front;

    if (data_size <= leading_space) 
    {
        memcpy(BCB(front), data, data_size);
        front += data_size;

        if (front == buffer_size) front = 0;

        current_size += data_size;
        return data_size;
    }

    memcpy(BCB(front), data, leading_space);
    memcpy(BCB(0), data + leading_space, data_size - leading_space);

    front = data_size - leading_space;
    current_size += data_size;
    return data_size;
}
    
uint16_t ByteCircularBuffer::Read(char* buffer, uint16_t data_size, bool remove_read_bytes)
{
    if (current_size < data_size) return 0;

    if (rear < front) 
    {
        memcpy(buffer, BCB(rear), data_size);
        if (remove_read_bytes) 
        {
            rear += data_size;

            if (rear == buffer_size) rear = 0;

            current_size -= data_size;
        }
        return data_size;
    }

    uint16_t leading_space = buffer_size - rear;

    if (data_size <= leading_space)
    {
        memcpy(buffer, BCB(rear), data_size);

        if (remove_read_bytes) 
        {
            rear += data_size;

            if (rear == buffer_size) rear = 0;

            current_size -= data_size;
        }
        return data_size;
    }

    memcpy(buffer, BCB(rear), leading_space);
    memcpy(buffer + leading_space, BCB(0), data_size - leading_space);

    if (remove_read_bytes) 
    {
        rear = (data_size - leading_space);
        current_size -= data_size;
    }
    return data_size;
}
    
bool ByteCircularBuffer::IsFull() const
{
    return current_size == buffer_size;
}
    
inline uint16_t ByteCircularBuffer::AvailableSize() const
{
    return buffer_size - current_size;
}
    
void ByteCircularBuffer::Reset()
{
    current_size = 0;
    front = 0;
    rear = 0;
}