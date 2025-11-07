#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <stdint.h>
#include <functional>
#include <ctime>
#include <memory>

class TcpServerController;
class TcpMsgDemarcar;

#define CLIENT_LIFETIME 60

class TcpClient : public std::enable_shared_from_this<TcpClient>
{
public:
	TcpClient(uint32_t ip_addr, uint16_t port_no, int comm_fd);
	~TcpClient();

	uint32_t GetIpAddr() const;
	uint16_t GetPortNo() const;
	int GetFD() const;
	std::string ToString();

private:
	uint32_t ip_addr;
	uint16_t port_no;
	int comm_fd;
};

#endif