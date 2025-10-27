#include "network_utils.h"

#include <stdio.h>

char* network_convert_ip_n_to_p(uint32_t ip_addr, char* output_buffer)
{
    static char buffer[16];

    uint32_t bytes[4];
    bytes[0] = ip_addr & 0xFF;
    bytes[1] = (ip_addr >> 8) & 0xFF;
    bytes[2] = (ip_addr >> 16) & 0xFF;
    bytes[3] = (ip_addr >> 24) & 0xFF;

    if (output_buffer)
    {
        snprintf(output_buffer, sizeof(output_buffer), "%u.%u.%u.%u", bytes[3], bytes[2], bytes[1], bytes[0]);
        return nullptr;
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", bytes[3], bytes[2], bytes[1], bytes[0]);
        return buffer;
    }
}

uint32_t network_convert_ip_p_to_n(const char* ip_addr)
{
    unsigned int byte3;
    unsigned int byte2;
    unsigned int byte1;
    unsigned int byte0;
    char dummyString[2];

    static char pattern[] = "%u.%u.%u.%u%1s";

    if (sscanf(ip_addr, pattern, &byte3, &byte2, &byte1, &byte0, dummyString, sizeof(pattern)) == 4)
    {
        if (byte3 < 256 && byte2 < 256 && byte1 < 256 && byte0 < 256)
        {
            return (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0;
        }
    }

    return -1;
}
