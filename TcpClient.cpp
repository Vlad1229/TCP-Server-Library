#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/tcp.h> 
#include <cerrno>
#include <unistd.h>
#include <sstream>
#include "TcpClient.h"
#include "ByteCircularBuffer.h"
#include "TcpMsgDemarcar.h"
#include "network_utils.h"

TcpClient::TcpClient(uint32_t ip_addr, uint16_t port_no, int comm_fd)
{
	this->ip_addr = ip_addr;
	this->port_no = port_no;
	this->comm_fd = comm_fd;

	int enableKeepAlive = 1;
    if (setsockopt(comm_fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive)) < 0)
	{
		printf("Setsockopt SO_KEEPALIVE failed, error =  %d\n", errno);
		exit(0);
	}
	
	int keepIdle = 60; 
    if (setsockopt(comm_fd, SOL_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle)) < 0)
	{
		printf("Setsockopt TCP_KEEPIDLE failed, error =  %d\n", errno);
		exit(0);
	}

	int keepInterval = 5; 
    if (setsockopt(comm_fd, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval)) < 0)
	{
		printf("Setsockopt TCP_KEEPINTVL failed, error =  %d\n", errno);
		exit(0);
	}

	int keepCount = 3; 
    if (setsockopt(comm_fd, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount)) < 0)
	{
		printf("Setsockopt TCP_KEEPCNT failed, error =  %d\n", errno);
		exit(0);
	}
}

TcpClient::~TcpClient()
{
	close(comm_fd);
}

uint32_t TcpClient::GetIpAddr() const
{
    return ip_addr;
}

uint16_t TcpClient::GetPortNo() const
{
    return port_no;
}

int TcpClient::GetFD() const
{
    return comm_fd;
}

std::string TcpClient::ToString()
{
	std::stringstream strm;
	strm << reinterpret_cast<void*>(this) 
		 << " [" << network_convert_ip_n_to_p(htonl(ip_addr)) 
		 << ", " << htons(port_no) << "]";
	return strm.str();
}
