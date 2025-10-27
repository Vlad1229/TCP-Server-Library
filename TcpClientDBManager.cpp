#include "TcpClientDBManager.h"
#include "TcpClient.h"
#include <cstdio>

TcpClientDBManager::~TcpClientDBManager()
{
	Purge();
}

std::shared_ptr<TcpClient> TcpClientDBManager::AddClient(uint32_t ip_addr, uint16_t port_no, int comm_fd)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::shared_ptr<TcpClient> tcp_client = std::make_shared<TcpClient>(ip_addr, port_no, comm_fd);
	tcp_clients_db.push_back(tcp_client);
	return tcp_client;

}

void TcpClientDBManager::RemoveClient(std::shared_ptr<TcpClient> tcp_client)
{
	std::lock_guard<std::mutex> lock(mtx);
	tcp_clients_db.remove(tcp_client);
}

void TcpClientDBManager::Purge()
{
	std::lock_guard<std::mutex> lock(mtx);
	tcp_clients_db.clear();
}

const std::list<std::shared_ptr<TcpClient>> &TcpClientDBManager::GetClients()
{
    return tcp_clients_db;
}

void TcpClientDBManager::DisplayClientDB() const
{
	std::list<std::shared_ptr<TcpClient>>::const_iterator it;

	std::lock_guard<std::mutex> lock(mtx);
	for (it = tcp_clients_db.cbegin(); it != tcp_clients_db.cend(); ++it)
	{
		printf("Tcp Client: %s\n", (*it)->ToString().c_str());
	}
}
