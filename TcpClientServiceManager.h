#ifndef __TCP_CLIENT_SERVICE_MANAGER_H__
#define __TCP_CLIENT_SERVICE_MANAGER_H__

#include <poll.h>
#include <thread>
#include <list>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class TcpServerController;
class TcpMsgDemarcar;
class ByteCircularBuffer;
class TcpClient;

class TcpClientServiceManager
{
private:
	struct TcpClientBuffer
	{
		std::shared_ptr<TcpClient> tcp_client;
		std::shared_ptr<ByteCircularBuffer> circular_buffer;

		bool operator==(const TcpClientBuffer& other) const 
		{
        	return tcp_client == other.tcp_client;
    	}
	};

	struct TcpClientMsg
	{
		std::shared_ptr<TcpClient> tcp_client;
		std::string msg;
	};

	class ListeningThread
	{
	public:
		ListeningThread(const std::list<std::shared_ptr<TcpClient>>& start_tcp_clients = {});
		~ListeningThread();

		bool AddTcpClient(const std::shared_ptr<TcpClient>& tcp_client);

		std::thread::id GetId() const;

		void Display() const;

	private:
		void Listen();

		TcpClientBuffer& FindTcpClientAndBufferByFD(int tcp_client_fd);
		void RemoveTcpClientBuffer(TcpClientBuffer& tcp_client_buffer);
		void RemoveFDFromPoll(int fd_idx);

	public:
		std::function<void(std::shared_ptr<TcpClient>&)> on_client_disconnected;
		std::function<void(const std::shared_ptr<TcpClient>&, const char*, uint16_t)> on_msg_rcvd;

	private:
		std::thread thread;

		std::vector<pollfd> poll_fds;
		int poll_fds_num;

		int command_pipe_fds[2];
	
		std::shared_ptr<TcpMsgDemarcar> demarcar;
		char* recv_buffer;

		mutable std::mutex buffers_mtx;
		std::list<TcpClientBuffer> tcp_clients_buffers;
	};

public:
	TcpClientServiceManager();
	~TcpClientServiceManager();

	bool StartListenTcpClient(std::shared_ptr<TcpClient>& tcp_client);

	void Start(const std::list<std::shared_ptr<TcpClient>>& start_tcp_clients = {});
	void Stop();

	void Dispaly() const;

private:
	const std::shared_ptr<ListeningThread>& StartNewThread(const std::list<std::shared_ptr<TcpClient>>& start_tcp_clients = {});
	void StopThread(const std::thread::id& id);

	void OnMsgReceived(const std::shared_ptr<TcpClient>& tcp_client, const char* msg, uint16_t msg_size);
	void ReadMsgsQueue();

public:
	std::function<void(std::shared_ptr<TcpClient>&)> on_client_disconnected;
	std::function<void(const std::shared_ptr<TcpClient>&, std::string)> on_msg_rcvd;

private:
	bool is_running;

	std::thread msgs_thread;

	std::mutex threads_mtx;
	std::vector<std::shared_ptr<ListeningThread>> threads;

	std::mutex msgs_mtx;
	std::condition_variable msgs_cv;
	std::queue<TcpClientMsg> msgs_queue;
};

#endif
