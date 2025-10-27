#ifndef __TCP_CLIENT_DB_MANAGER_H__
#define __TCP_CLIENT_DB_MANAGER_H__

#include <list>
#include <mutex>
#include <memory>

class TcpClient;
class TcpServerController;

class TcpClientDBManager
{
public:
	~TcpClientDBManager();

	std::shared_ptr<TcpClient> AddClient(uint32_t ip_addr, uint16_t port_no, int comm_fd);
	void RemoveClient(std::shared_ptr<TcpClient> tcp_client);
	void Purge();

	const std::list<std::shared_ptr<TcpClient>>& GetClients();

	void DisplayClientDB() const;

private:
	std::list<std::shared_ptr<TcpClient>> tcp_clients_db;
	mutable std::mutex mtx;
};

#endif
