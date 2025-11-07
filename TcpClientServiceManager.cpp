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
#include "ByteCircularBuffer.h"

#define MAX_CLIENTS 2
#define MAX_BUFFER_SIZE 1024
#define POLL_TIMEOUT 3

TcpClientServiceManager::ListeningThread::ListeningThread(const std::list<std::shared_ptr<TcpClient>> &start_tcp_clients)
{
	poll_fds.resize(MAX_CLIENTS + 1);
	poll_fds_num = 1;

	pipe(command_pipe_fds);

	poll_fds[0].fd = command_pipe_fds[0];
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;

	recv_buffer = new char[MAX_BUFFER_SIZE];
	demarcar = std::make_shared<TcpMsgVariableSizeDemarcar>(4);

	for (auto& tcp_client : start_tcp_clients)
	{
		TcpClientBuffer tcp_client_buffer;
		tcp_client_buffer.tcp_client = tcp_client;
		tcp_client_buffer.circular_buffer = std::make_shared<ByteCircularBuffer>();
		tcp_clients_buffers.emplace_back(tcp_client_buffer);
	}

	thread = std::thread(&ListeningThread::Listen, this);
}

TcpClientServiceManager::ListeningThread::~ListeningThread()
{
	//stop command
	write(command_pipe_fds[1], "s", 1);
	thread.join();
	
	delete[] recv_buffer;

	close(command_pipe_fds[0]);
	close(command_pipe_fds[1]);
}

bool TcpClientServiceManager::ListeningThread::AddTcpClient(const std::shared_ptr<TcpClient> &tcp_client)
{
	{
		std::lock_guard<std::mutex> buffers_mtx_lock(buffers_mtx);
		if (tcp_clients_buffers.size() == MAX_CLIENTS)
		{
			return false;
		}

		TcpClientBuffer tcp_client_buffer;
		tcp_client_buffer.tcp_client = tcp_client;
		tcp_client_buffer.circular_buffer = std::make_shared<ByteCircularBuffer>();
		tcp_clients_buffers.push_back(tcp_client_buffer);
	}

	std::string add_client_command = "a ";
	char client_socket_str[11];
	sprintf(client_socket_str, "%010d", tcp_client->GetFD());
	add_client_command.append(client_socket_str);

	write(command_pipe_fds[1], add_client_command.c_str(), 12);
	return true;
}

void TcpClientServiceManager::ListeningThread::Listen()
{
	int rcv_bytes;
	sockaddr_in client_addr;

	socklen_t addr_len = sizeof(client_addr);

	poll_fds_num = 1;
	{
		std::lock_guard<std::mutex> buffers_mtx_lock(buffers_mtx);
		for(auto [i, it] = std::make_tuple(1, tcp_clients_buffers.begin()); i < (MAX_CLIENTS + 1) && it != tcp_clients_buffers.cend(); ++i, ++it)
		{
			poll_fds[i].fd = it->tcp_client->GetFD();
			poll_fds[i].events = POLLIN;
			poll_fds_num++;
		}
	}

	while (true)
	{
		poll(poll_fds.data(), poll_fds_num, POLL_TIMEOUT * 1000);

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

				tcp_clients_buffers.clear();

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
				auto& tcp_client_buffer = FindTcpClientAndBufferByFD(poll_fds[i].fd);

				on_client_disconnected(tcp_client_buffer.tcp_client);

				RemoveFDFromPoll(i);
				RemoveTcpClientBuffer(tcp_client_buffer);
			}
			else if (poll_fds[i].revents & POLLIN)
			{
				poll_fds[i].revents = 0;

				rcv_bytes = recv(poll_fds[i].fd, 
								 recv_buffer,
								 MAX_BUFFER_SIZE,
								 0);

				auto& tcp_client_buffer = FindTcpClientAndBufferByFD(poll_fds[i].fd);

				if (rcv_bytes <= 0)
				{
					on_client_disconnected(tcp_client_buffer.tcp_client);

					RemoveFDFromPoll(i);
					RemoveTcpClientBuffer(tcp_client_buffer);
				}

				if (demarcar)
				{
					demarcar->on_msg_demarked = std::bind(on_msg_rcvd, tcp_client_buffer.tcp_client, std::placeholders::_1, std::placeholders::_2);
					bool success = demarcar->ProcessMsg(tcp_client_buffer.circular_buffer, recv_buffer, rcv_bytes);
					demarcar->on_msg_demarked = nullptr;

					if (!success)
					{
						on_client_disconnected(tcp_client_buffer.tcp_client);

						RemoveFDFromPoll(i);
						RemoveTcpClientBuffer(tcp_client_buffer);
					}
				}
				else
				{
					on_msg_rcvd(tcp_client_buffer.tcp_client, recv_buffer, rcv_bytes);
				}

				memset(recv_buffer, 0, rcv_bytes);
			}
		}
	}
}

TcpClientServiceManager::TcpClientBuffer& TcpClientServiceManager::ListeningThread::FindTcpClientAndBufferByFD(int tcp_client_fd)
{
    std::lock_guard<std::mutex> buffers_mtx_lock(buffers_mtx);
	auto it = std::find_if(tcp_clients_buffers.begin(), tcp_clients_buffers.end(), 
						[tcp_client_fd](const auto& tcp_client_buffer)
						{
							return tcp_client_buffer.tcp_client->GetFD() == tcp_client_fd;
						});
		
	return *it;
}

void TcpClientServiceManager::ListeningThread::RemoveTcpClientBuffer(TcpClientBuffer &tcp_client_buffer)
{
    std::lock_guard<std::mutex> buffers_mtx_lock(buffers_mtx);
	tcp_clients_buffers.remove(tcp_client_buffer);
}

void TcpClientServiceManager::ListeningThread::RemoveFDFromPoll(int fd_idx)
{
	poll_fds[fd_idx].fd = 0;
	poll_fds[fd_idx].events = 0;
	poll_fds[fd_idx].revents = 0;

	while(poll_fds[--poll_fds_num].fd == 0){}
	++poll_fds_num;
}

std::thread::id TcpClientServiceManager::ListeningThread::GetId() const
{
    return thread.get_id();
}

void TcpClientServiceManager::ListeningThread::Display() const
{
	printf("Thread: %d\n", static_cast<int>(std::hash<std::thread::id>{}(thread.get_id())));
		
	for (const auto& tcp_client_buffer : tcp_clients_buffers)
	{
		printf("Client: %p\n", static_cast<void*>(tcp_client_buffer.tcp_client.get()));
	}
	printf("\n");
}
TcpClientServiceManager::TcpClientServiceManager()
{
}

TcpClientServiceManager::~TcpClientServiceManager()
{
	Stop();
}

bool TcpClientServiceManager::StartListenTcpClient(std::shared_ptr<TcpClient>& tcp_client)
{
	std::lock_guard<std::mutex> threads_mtx_lock(threads_mtx);

	for (auto& thread : threads)
	{
		if (thread->AddTcpClient(tcp_client))
		{
			return true;
		};
	}

	//executes if all threads are full
	std::list<std::shared_ptr<TcpClient>> start_tcp_clients;
	start_tcp_clients.emplace_back(tcp_client);
	StartNewThread(start_tcp_clients);
	
	return true;
}

void TcpClientServiceManager::Start(const std::list<std::shared_ptr<TcpClient>>& start_tcp_clients)
{	
	if (!is_running)
	{
		{
			std::lock_guard<std::mutex> threads_mtx_lock(threads_mtx);
			
			int threads_num = start_tcp_clients.size() / MAX_CLIENTS + 1;
			auto last_client_it = start_tcp_clients.begin();
			auto first_client_it = last_client_it;
			for  (int i = 0; i < threads_num; i++)
			{
				first_client_it = last_client_it;
				std::advance(last_client_it, std::min(static_cast<int>(start_tcp_clients.size()) - i * MAX_CLIENTS, MAX_CLIENTS));

				std::list<std::shared_ptr<TcpClient>> thread_tcp_clients(first_client_it, last_client_it);
				StartNewThread(thread_tcp_clients);
			}
		}
		is_running = true;

		msgs_thread = std::thread(&TcpClientServiceManager::ReadMsgsQueue, this);

		printf("Service started: TcpClientServiceManagerThread\n");
	}
}

void TcpClientServiceManager::Stop()
{
	if (is_running)
	{
		std::lock_guard<std::mutex> threads_mtx_lock(threads_mtx);
		threads.clear();

		is_running = false;
		msgs_cv.notify_one();
		msgs_thread.join();
	}
}

void TcpClientServiceManager::Dispaly() const
{
	printf("Running threads: %d\n", static_cast<int>(threads.size()));

	for (const auto& thread : threads)
	{
		thread->Display();
	}
}

const std::shared_ptr<TcpClientServiceManager::ListeningThread>& TcpClientServiceManager::StartNewThread(const std::list<std::shared_ptr<TcpClient>>& start_tcp_clients)
{
	std::shared_ptr<ListeningThread> new_listening_thread = std::make_shared<ListeningThread>(start_tcp_clients);
	new_listening_thread->on_msg_rcvd = std::bind(&TcpClientServiceManager::OnMsgReceived, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	new_listening_thread->on_client_disconnected = on_client_disconnected;
	return threads.emplace_back(new_listening_thread);
}

void TcpClientServiceManager::StopThread(const std::thread::id& id)
{
	std::lock_guard<std::mutex> threads_mtx_lock(threads_mtx);
	auto thread_it = std::find_if(threads.begin(), threads.end(), 
   		[&id](const auto& thread) { 
      		return thread->GetId() == id;
   		});

	if (thread_it != threads.end())
	{
		threads.erase(thread_it);
	}
}

void TcpClientServiceManager::OnMsgReceived(const std::shared_ptr<TcpClient> &tcp_client, const char *msg, uint16_t msg_size)
{
	TcpClientMsg tcp_client_msg;
	tcp_client_msg.tcp_client = tcp_client;
	tcp_client_msg.msg = std::string(msg, msg_size);

	{
		std::lock_guard<std::mutex> msgs_mtx_lock(msgs_mtx);
		msgs_queue.push(tcp_client_msg);
	}
	msgs_cv.notify_one();
}

void TcpClientServiceManager::ReadMsgsQueue()
{
	while (true)
	{
		std::unique_lock<std::mutex> msgs_mtx_ulock(msgs_mtx);
        msgs_cv.wait(msgs_mtx_ulock, [this]{ return !msgs_queue.empty() || !is_running; });
		if (!is_running) return;
        const TcpClientMsg& tcp_client_msg = msgs_queue.front();
		on_msg_rcvd(tcp_client_msg.tcp_client, tcp_client_msg.msg);
        msgs_queue.pop();
	}
}
