#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "TcpServerController.h"
#include "TcpNewConnectionAcceptor.h"
#include "TcpClientDBManager.h"
#include "TcpClientServiceManager.h"
#include "TcpClient.h"
#include "network_utils.h"

TcpServerController::~TcpServerController() = default;

TcpServerController::TcpServerController(std::string ip_addr, uint16_t port_no, std::string name)
{
	this->ip_addr = network_convert_ip_p_to_n(ip_addr.c_str());
	this->port_no = port_no;
	this->name = name;

	tcp_new_conn_acc = std::make_unique<TcpNewConnectionAcceptor>();
	tcp_client_db_mgr = std::make_unique<TcpClientDBManager>();
	tcp_client_svc_mgr = std::make_unique<TcpClientServiceManager>();

	SetBit(TCP_SERVER_INITIALIZED);
}

void TcpServerController::SetServerNotifCallbacks(
	void(*on_client_connected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&),
	void(*on_client_disconnected)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&),
	void(*on_client_msg_recvd)(const std::shared_ptr<TcpServerController>&, const std::shared_ptr<TcpClient>&, const std::string&))
{
	this->on_client_connected = on_client_connected;
	this->on_client_disconnected = on_client_disconnected;
	this->on_client_msg_recvd = on_client_msg_recvd;

	tcp_new_conn_acc->on_client_connected = std::bind(&TcpServerController::OnClientConnected, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	tcp_client_svc_mgr->on_client_disconnected = std::bind(&TcpServerController::OnClientDisconnected, this, std::placeholders::_1);
	tcp_client_svc_mgr->on_msg_rcvd = std::bind(&TcpServerController::OnClientMessageReceived, this, std::placeholders::_1, std::placeholders::_2);
}

void TcpServerController::Start()
{
	if (!IsBitSet(TCP_SERVER_RUNNING))
	{
		tcp_new_conn_acc->StartTcpNewConnectionAcceptorThread(ip_addr, port_no);
		UnSetBit(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS);

		tcp_client_svc_mgr->Start();
		UnSetBit(TCP_SERVER_NOT_LISTENING_CLIENTS);

		printf("Tcp Server is Up and Running [%s, %d]\nOk.\n",
			network_convert_ip_n_to_p(this->ip_addr), this->port_no);

		SetBit(TCP_SERVER_RUNNING);
	}
}

void TcpServerController::Stop()
{
	if (IsBitSet(TCP_SERVER_RUNNING))
	{
		StopConnectionAcceptingSvc();
		StopClientSvcMgr();
		tcp_client_db_mgr->Purge();

		printf("Tcp Server Stoped");

		UnSetBit(TCP_SERVER_RUNNING);
	}
}

void TcpServerController::Display()
{
	printf("Server Name: %s\n", this->name.c_str());

	if (IsBitSet(TCP_SERVER_RUNNING))
	{
		printf("Listening on: [%s, %d]\n", network_convert_ip_n_to_p(this->ip_addr), this->port_no);
	}
	else 
	{
		printf("Tcp Server Not Running\n");
		return;
	}

	tcp_client_svc_mgr->Dispaly();
	
	printf("Flags: %s %s %s %s\n",
		   IsBitSet(TCP_SERVER_INITIALIZED) ? "I" : "Not-I",
		   IsBitSet(TCP_SERVER_RUNNING) ? "R" : "Not-R",
		   IsBitSet(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS) ? "Not-Acc" : "Acc",
		   IsBitSet(TCP_SERVER_NOT_LISTENING_CLIENTS) ? "Not-L" : "L");

	tcp_client_db_mgr->DisplayClientDB();
}

void TcpServerController::StartConnectionAcceptingSvc()
{
	if (IsBitSet(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS))
	{
		tcp_new_conn_acc->StartTcpNewConnectionAcceptorThread(ip_addr, port_no);
		UnSetBit(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS);
	}
}

void TcpServerController::StopConnectionAcceptingSvc()
{
	if (!IsBitSet(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS))
	{
		tcp_new_conn_acc->StopTcpNewConnectionAcceptorThread();
		SetBit(TCP_SERVER_NOT_ACCEPTING_NEW_CONNECTIONS);
	}
}

void TcpServerController::StartClientSvcMgr()
{
	if (IsBitSet(TCP_SERVER_NOT_LISTENING_CLIENTS))
	{
		tcp_client_svc_mgr->Start(tcp_client_db_mgr->GetClients());
		UnSetBit(TCP_SERVER_NOT_LISTENING_CLIENTS);
	}
}

void TcpServerController::StopClientSvcMgr()
{
	if (!IsBitSet(TCP_SERVER_NOT_LISTENING_CLIENTS))
	{
		tcp_client_svc_mgr->Stop();
		SetBit(TCP_SERVER_NOT_LISTENING_CLIENTS);
	}
}

void TcpServerController::SetBit(uint8_t bit_value)
{
	state_flags |= bit_value;
}

void TcpServerController::UnSetBit(uint8_t bit_value)
{
	state_flags &= ~bit_value;
}

bool TcpServerController::IsBitSet(uint8_t bit_value)
{
    return state_flags & bit_value;
}

void TcpServerController::OnClientConnected(uint32_t ip_addr, uint16_t port_no, int comm_fd)
{
	auto tcp_client = tcp_client_db_mgr->AddClient(ip_addr, port_no, comm_fd);
	tcp_client_svc_mgr->StartListenTcpClient(tcp_client);
	this->on_client_connected(shared_from_this(), tcp_client);
}

void TcpServerController::OnClientDisconnected(std::shared_ptr<TcpClient>& tcp_client)
{
	this->on_client_disconnected(shared_from_this(), tcp_client);
	tcp_client_db_mgr->RemoveClient(tcp_client);
}

void TcpServerController::OnClientMessageReceived(const std::shared_ptr<TcpClient>& tcp_client, const std::string& msg)
{
	this->on_client_msg_recvd(shared_from_this(), tcp_client, msg);
}
