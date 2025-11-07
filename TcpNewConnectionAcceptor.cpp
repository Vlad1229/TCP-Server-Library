#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include "TcpNewConnectionAcceptor.h"
#include "TcpServerController.h"
#include "TcpClient.h"
#include "network_utils.h"

#define ACCEPT_POLL_TIMEOUT 3

TcpNewConnectionAcceptor::TcpNewConnectionAcceptor()
{
	pipe(command_pipe_fds);
	if (pipe(command_pipe_fds) < 0)
	{
		printf("Error: Could not create Command Pipe FDs\n");
		exit(0);
	}
}

TcpNewConnectionAcceptor::~TcpNewConnectionAcceptor()
{
	StopTcpNewConnectionAcceptorThread();
	close(command_pipe_fds[0]);
	close(command_pipe_fds[1]);
}

void TcpNewConnectionAcceptor::StartTcpNewConnectionAcceptorThread(uint32_t ip_addr, uint16_t port_no)
{
	accept_new_conn_thread = std::thread(&TcpNewConnectionAcceptor::TcpListenForNewConnection, this, ip_addr, port_no);
}

void TcpNewConnectionAcceptor::StopTcpNewConnectionAcceptorThread()
{
	//stop command
	write(command_pipe_fds[1], "s", 1);
	accept_new_conn_thread.join();
}

void TcpNewConnectionAcceptor::TcpListenForNewConnection(uint32_t ip_addr, uint16_t port_no)
{
	accept_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (accept_fd < 0)
	{
		printf("Error: Could not create Accept FD\n");
		exit(0);
	}

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_no);
	server_addr.sin_addr.s_addr = htonl(ip_addr);

	int opt = 1;
	if (setsockopt(accept_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
	{
		printf("Setsockopt SO_REUSEADDR failed, error =  %d\n", errno);
		exit(0);
	}

	if (setsockopt(accept_fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt)) < 0)
	{
		printf("Setsockopt SO_REUSEPORT failed, error =  %d\n", errno);
		exit(0);
	}
	
	if (bind(accept_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		printf("Error: Acceptor socket bind failed [%s(0x%x), %d], error = %d\n",
			network_convert_ip_n_to_p(ip_addr),
			ip_addr,
			port_no,
			errno);
		exit(0);
	}

	if (listen(accept_fd, 5) < 0)
	{
		printf("listen Failed, error =  %d\n", errno);
		exit(0);
	}

	sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int comm_sock_fd;

	pollfd poll_fds[2];
	poll_fds[0].fd = accept_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = command_pipe_fds[0];
	poll_fds[1].events = POLLIN;

	while (true)
	{
		int ret = poll(poll_fds, 2, ACCEPT_POLL_TIMEOUT * 1000);

		if (ret > 0)
		{
			if (poll_fds[0].revents & POLLIN)
			{
				poll_fds[0].revents = 0;

				comm_sock_fd = accept(accept_fd, (struct sockaddr*)&client_addr, &addr_len);

				if (comm_sock_fd < 0)
				{
					printf("Error in Accepting New Connections, error =  %d\n", errno);
					continue;
				}

				if (on_client_connected)
				{
					on_client_connected(client_addr.sin_addr.s_addr, 
									 	client_addr.sin_port,
									 	comm_sock_fd);
				}
			}

			if (poll_fds[1].revents & POLLIN)
			{
				poll_fds[1].revents = 0;

				char buff[1] = "";
				read(poll_fds[1].fd, buff, 1);

				if (buff[0] == 's') //stop command
				{
					close(accept_fd);
					return;
				}
			}
		}
	}
}
