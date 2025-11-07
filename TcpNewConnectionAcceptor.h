#ifndef __TCP_NEW_CONNECTION_ACCEPTOR_H__
#define __TCP_NEW_CONNECTION_ACCEPTOR_H__

#include <thread>
#include <functional>

class TcpServerController;
class TcpClient;

class TcpNewConnectionAcceptor
{
public:
	TcpNewConnectionAcceptor();
	~TcpNewConnectionAcceptor();

	void StartTcpNewConnectionAcceptorThread(uint32_t ip_addr, uint16_t port_no);
	void StopTcpNewConnectionAcceptorThread();

private:
	void TcpListenForNewConnection(uint32_t ip_addr, uint16_t port_no);

public:
    std::function<void(uint32_t, uint16_t, int)> on_client_connected;

private:
	int accept_fd;
	int command_pipe_fds[2];
	std::thread accept_new_conn_thread;
};

#endif
