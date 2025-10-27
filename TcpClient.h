#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <stdint.h>
#include <functional>
#include <ctime>
#include <memory>

class TcpServerController;
class ByteCircularBuffer;
class TcpMsgDemarcar;

#define CLIENT_LIFETIME 60

class TcpClient : public std::enable_shared_from_this<TcpClient>
{
public:
	std::function<void(const std::shared_ptr<TcpClient>& client, std::string)> msg_rcvd;

	TcpClient(uint32_t ip_addr, uint16_t port_no, int comm_fd);
	~TcpClient();

	uint32_t GetIpAddr() const;
	uint16_t GetPortNo() const;
	int GetFD() const;
	std::string ToString();

	void SetDemarcar(const std::shared_ptr<TcpMsgDemarcar>& demarcar);
	bool ProcessMessage(const char* msg, uint16_t msg_size);
	void SetMessageReceivedCallback(std::function<void(const std::shared_ptr<TcpClient>& client, const std::string&)> msg_rcvd);

private:
    void OnMsgDemarced(const char* msg, uint16_t msg_size);

private:
	uint32_t ip_addr;
	uint16_t port_no;
	int comm_fd;

	std::weak_ptr<TcpMsgDemarcar> demarcar;

	std::shared_ptr<ByteCircularBuffer> circular_buffer;
};

#endif