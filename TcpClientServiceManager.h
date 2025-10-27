#ifndef __TCP_CLIENT_SERVICE_MANAGER_H__
#define __TCP_CLIENT_SERVICE_MANAGER_H__

#include <poll.h>
#include <thread>
#include <list>
#include <mutex>
#include <functional>
#include <memory>

#define MAX_CLIENTS 200
#define MAX_CLIENT_BUFFER_SIZE 1024

class TcpServerController;
class TcpMsgDemarcar;
class TcpClient;

class TcpClientServiceManager
{
public:
	TcpClientServiceManager();
	~TcpClientServiceManager();

	void SetStartClients(const std::list<std::shared_ptr<TcpClient>>& clients);

	void StartTcpClientServiceManagerThread();
	void StopTcpClientServiceManagerThread();
	bool ClientFDStartListen(std::shared_ptr<TcpClient>& tcp_client);

private:
	std::shared_ptr<TcpClient> FindClientByFD(int client_fd);
	void RemoveClient(std::shared_ptr<TcpClient>& client);
	void RemoveFDFromPoll(int fd_idx);

	void ListenForClientsMessages();

public:
	std::function<void(std::shared_ptr<TcpClient>&)> client_diconnected;

private:
	pollfd poll_fds[MAX_CLIENTS + 1];
	int poll_fds_num;

	int command_pipe_fds[2];
	bool is_running;
	std::thread client_svc_mgr_tread;
	std::mutex thread_mtx;
	mutable std::mutex db_mtx;

	char* client_recv_buffer;

	std::shared_ptr<TcpMsgDemarcar> demarcar;
	std::list<std::shared_ptr<TcpClient>> tcp_clients_db;
};

#endif
