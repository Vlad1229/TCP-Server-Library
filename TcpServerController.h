#ifndef __TCP_SERVER_CONTROLLER_H__
#define __TCP_SERVER_CONTROLLER_H__

#include <stdint.h>
#include <string>
#include <memory>

#define TCP_SERVER_INITIALIZED (1)
#define TCP_SERVER_RUNNING (2)
#define TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS (4)
#define TCP_SERVER_NOT_LISTENING_CLIENTS (8)

class TcpNewConnectionAcceptor;
class TcpClientDBManager;
class TcpClientServiceManager;
class TcpClient;

class TcpServerController : public std::enable_shared_from_this<TcpServerController>
{
public:
	TcpServerController(std::string ip_addr, uint16_t port_no, std::string name);
	~TcpServerController();

	void SetServerNotifCallbacks(
		void (*client_connected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&),
		void (*client_disconnected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&),
		void (*client_msg_recvd)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&, const std::string&)
	);

	void Start();
	void Stop();
	void Display();

	void StartConnectionAcceptingSvc();
	void StopConnectionAcceptingSvc();

	void StartClientSvcMgr();
	void StopClientSvcMgr();

	void SetBit(uint8_t bit_value);
	void UnSetBit(uint8_t bit_value);
	bool IsBitSet(uint8_t bit_value);

private:
	void OnClientConnected(uint32_t ip_addr, uint16_t port_no, int comm_fd);
	void OnClientDisconnected(std::shared_ptr<TcpClient>& client);
	void OnClientMessageReceived(const std::shared_ptr<TcpClient>& client, const std::string& msg);

public:
	uint32_t ip_addr;
	uint16_t port_no;
	std::string name;
	uint8_t state_flags;

	void (*client_connected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&);
	void (*client_disconnected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&);
	void (*client_msg_recvd)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&, const std::string&);

private:
	std::unique_ptr<TcpNewConnectionAcceptor> tcp_new_conn_acc;
	std::unique_ptr<TcpClientDBManager> tcp_client_db_mgr;
	std::unique_ptr<TcpClientServiceManager> tcp_client_svc_mgr;

};

#endif
