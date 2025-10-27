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

	circular_buffer = std::make_unique<ByteCircularBuffer>();
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

void TcpClient::SetDemarcar(const std::shared_ptr<TcpMsgDemarcar> &demarcar)
{
	this->demarcar = demarcar;
}

bool TcpClient::ProcessMessage(const char* msg, uint16_t msg_size)
{
	if (auto demarcar_locked = demarcar.lock())
	{
		demarcar_locked->msg_demarked = std::bind(&TcpClient::OnMsgDemarced, shared_from_this(), std::placeholders::_1, std::placeholders::_2);
		bool result = demarcar_locked->ProcessMsg(circular_buffer, msg, msg_size);
		demarcar_locked->msg_demarked = nullptr;

		return result;
	}
	else if (msg_rcvd)
	{
		msg_rcvd(shared_from_this(), std::string(msg, msg_size));
	}
	return true;
}

void TcpClient::SetMessageReceivedCallback(std::function<void(const std::shared_ptr<TcpClient>& client, const std::string&)> msg_rcvd)
{
	this->msg_rcvd = msg_rcvd;
}

void TcpClient::OnMsgDemarced(const char* msg, uint16_t msg_size)
{
	if (msg_rcvd)
	{
		msg_rcvd(shared_from_this(), std::string(msg, msg_size));
	}
}
