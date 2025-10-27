#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "TcpClientServiceManager.h"
#include "TcpClient.h"
#include "TcpMsgVariableSizeDemarcar.h"

#define POLL_TIMEOUT 3

TcpClientServiceManager::TcpClientServiceManager()
{
	poll_fds_num = 1;

	pipe(command_pipe_fds);

	poll_fds[0].fd = command_pipe_fds[0];
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;

	for (int i = 1; i < MAX_CLIENTS; i++)
	{
		poll_fds[i].revents = 0;
	}

	client_recv_buffer = new char[MAX_CLIENT_BUFFER_SIZE];

	demarcar = std::make_shared<TcpMsgVariableSizeDemarcar>(4);
}

TcpClientServiceManager::~TcpClientServiceManager()
{
	StopTcpClientServiceManagerThread();

	close(command_pipe_fds[0]);
	close(command_pipe_fds[1]);
}

void TcpClientServiceManager::SetStartClients(const std::list<std::shared_ptr<TcpClient>>& clients)
{
	std::lock_guard<std::mutex> thread_lock(thread_mtx);
	if (!is_running)
	{
		std::lock_guard<std::mutex> db_lock(db_mtx);
		tcp_clients_db = clients;
	}
}

void TcpClientServiceManager::StartTcpClientServiceManagerThread()
{
	std::lock_guard<std::mutex> lock(thread_mtx);
	if (!is_running)
	{
		client_svc_mgr_tread = std::thread(&TcpClientServiceManager::ListenForClientsMessages, this);
		is_running = true;
	}
	
	printf("Service started: TcpClientServiceManagerThread\n");
}

void TcpClientServiceManager::StopTcpClientServiceManagerThread()
{
	{
		std::lock_guard<std::mutex> thread_lock(thread_mtx);
		if (is_running)
		{
			//stop command
			write(command_pipe_fds[1], "s", 1);
			client_svc_mgr_tread.join();
			is_running = false;
		}
	}
}

bool TcpClientServiceManager::ClientFDStartListen(std::shared_ptr<TcpClient>& tcp_client)
{
	std::lock_guard<std::mutex> thread_lock(thread_mtx);

	if (is_running)
	{
		{
			std::lock_guard<std::mutex> db_lock(db_mtx);
			if (tcp_clients_db.size() == MAX_CLIENTS)
			{
				return false;
			}

			tcp_client->SetDemarcar(demarcar);
			tcp_clients_db.push_back(tcp_client);
		}

		std::string add_client_command = "a ";
		char client_socket_str[11];
		sprintf(client_socket_str, "%010d", tcp_client->GetFD());
		add_client_command.append(client_socket_str);

		write(command_pipe_fds[1], add_client_command.c_str(), 12);
	}

	return true;
}

std::shared_ptr<TcpClient> TcpClientServiceManager::FindClientByFD(int client_fd)
{
	std::lock_guard<std::mutex> db_lock(db_mtx);
	auto it = std::find_if(tcp_clients_db.begin(), tcp_clients_db.end(), 
						[client_fd](const std::shared_ptr<TcpClient>& tcp_client)
						{
							return tcp_client->GetFD() == client_fd;
						});
		
	return *it;
}

void TcpClientServiceManager::RemoveClient(std::shared_ptr<TcpClient>& client)
{
	{
		std::lock_guard<std::mutex> db_lock(db_mtx);
		tcp_clients_db.remove(client);
	}

	client_diconnected(client);
}

void TcpClientServiceManager::RemoveFDFromPoll(int fd_idx)
{
	poll_fds[fd_idx].fd = 0;
	poll_fds[fd_idx].events = 0;
	poll_fds[fd_idx].revents = 0;

	while(poll_fds[--poll_fds_num].fd == 0){}
	++poll_fds_num;
}

void TcpClientServiceManager::ListenForClientsMessages()
{
	int rcv_bytes;
	sockaddr_in client_addr;

	socklen_t addr_len = sizeof(client_addr);

	poll_fds_num = 1;
	{
		std::lock_guard<std::mutex> db_lock(db_mtx);
		for(auto [i, it] = std::make_tuple(1, tcp_clients_db.begin()); i < (MAX_CLIENTS + 1) && it != tcp_clients_db.cend(); ++i, ++it)
		{
			poll_fds[i].fd = (*it)->GetFD();
			poll_fds[i].events = POLLIN;
			poll_fds_num++;

			(*it)->SetDemarcar(demarcar);
		}
	}

	while (true)
	{
		poll(poll_fds, poll_fds_num, POLL_TIMEOUT * 1000);

		//check if there is a command message
		if (poll_fds[0].revents & POLLIN)
		{
			poll_fds[0].revents = 0;

			char buff[12] = "";
			read(poll_fds[0].fd, buff, 12);

			if (buff[0] == 's') //stop command
			{
				for (int i = 1; i < poll_fds_num; i++)
				{
					poll_fds[i].fd = 0;
					poll_fds[i].events = 0;
					poll_fds[i].revents = 0;
				}

				tcp_clients_db.clear();

				return;
			}
			else if (buff[0] == 'a') //add client command
			{
				for (int i = 1; i < MAX_CLIENTS + 1; i++)
				{
					if (poll_fds[i].fd == 0)
					{
						std::string fd_str = buff;
						fd_str = fd_str.substr(3, 10);

						poll_fds[i].fd = std::atoi(fd_str.c_str());
						poll_fds[i].events = POLLIN;
						//poll_fds[i].revents = 0;

						if (i == poll_fds_num)
							poll_fds_num = i + 1;

						break;
					}
				}
			}
		}

		//check client messages
		for (int i = 1; i < poll_fds_num; i++)
		{
			if (poll_fds[i].revents & (POLLERR | POLLHUP))
			{
				auto client = FindClientByFD(poll_fds[i].fd);

				RemoveFDFromPoll(i);
				RemoveClient(client);
			}
			else if (poll_fds[i].revents & POLLIN)
			{
				poll_fds[i].revents = 0;

				rcv_bytes = recv(poll_fds[i].fd, 
								 client_recv_buffer,
								 MAX_CLIENT_BUFFER_SIZE,
								 0);

				std::shared_ptr<TcpClient> client = FindClientByFD(poll_fds[i].fd);

				if (rcv_bytes <= 0 || !client->ProcessMessage(client_recv_buffer, rcv_bytes))
				{
					RemoveFDFromPoll(i);
					RemoveClient(client);
				}

				memset(&client_recv_buffer[0], 0, rcv_bytes);
			}
		}
	}
}
