#include <list>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "TcpServerController.h"
#include "TcpClient.h"
#include "network_utils.h"
#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"

#define TCP_SERVER_CREATE 1
#define TCP_SERVER_START 2
#define TCP_SERVER_STOP 3
#define TCP_SERVER_SHOW 4
#define TCP_SERVER_STOP_CONN_ACCEPT 5
#define TCP_SERVER_STOP_CLIENT_LISTEN 6

std::list<std::shared_ptr<TcpServerController>> tcp_servers_lst;
uint16_t default_port_no = 8080;

static void appln_client_connected(const std::shared_ptr<TcpServerController>& tcp_server, const std::shared_ptr<TcpClient>& tcp_client)
{
    printf("Server %s: client %s conneccted\n", tcp_server->name.c_str(), tcp_client->ToString().c_str());
}

static void appln_client_disconnected(const std::shared_ptr<TcpServerController>& tcp_server, const std::shared_ptr<TcpClient>& tcp_client)
{
    printf("Server %s: client %s disconneccted\n", tcp_server->name.c_str(), tcp_client->ToString().c_str());
}

static void appln_client_msg_recvd(const std::shared_ptr<TcpServerController>& tcp_server, const std::shared_ptr<TcpClient>& tcp_client, const std::string& msg)
{
    printf("Bytes recvd from %p: %zu msg: %s\n", (void*)tcp_client.get(), msg.size(), msg.c_str());
}

static std::shared_ptr<TcpServerController> TcpServerLookup(std::string tcp_server_name)
{
    std::shared_ptr<TcpServerController> ctrl;

    std::list<std::shared_ptr<TcpServerController>>::iterator it;
    for (it = tcp_servers_lst.begin(); it != tcp_servers_lst.end(); ++it)
    {
        ctrl = *it;
        if (ctrl->name == tcp_server_name)
        {
            return ctrl;
        }
    }

    return NULL;
}

int config_tcp_server_handler(param_t* param, ser_buff_t* ser_buff, op_mode enable_or_disable)
{
    const char* server_name = NULL;
    tlv_struct_t* tlv = NULL;
    std::shared_ptr<TcpServerController> tcp_server;

	struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
    }

    char ip[INET_ADDRSTRLEN];

	bool local_skipped = true;
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (!local_skipped)
            {
                local_skipped = true;
                continue;
            }

            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, ip, sizeof(ip));
        }
    }
    char* ip_addr = ip;
    uint16_t port_no = default_port_no;

    int cmd_code = EXTRACT_CMD_CODE(ser_buff);

    TLV_LOOP_BEGIN(ser_buff, tlv)
    {
        if (strncmp(tlv->leaf_id, "tcp-server-name", strlen("tcp-server-name")) == 0)
        {
            server_name = tlv->value;
        }
        else if (strncmp(tlv->leaf_id, "tcp-server-addr", strlen("tcp-server-addr")) == 0)
        {
            ip_addr = tlv->value;
        }
        else if (strncmp(tlv->leaf_id, "tcp-server-port", strlen("tcp-server-port")) == 0)
        {
            port_no = atoi(tlv->value);
        }
    } TLV_LOOP_END;

    switch (cmd_code)
    {
    case TCP_SERVER_CREATE:
        /* config tcp-server <server-name> */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (tcp_server)
        {
            printf("Error: Tcp Server Already Exist\n");
            return -1;
        }
        tcp_server = std::make_shared<TcpServerController>(std::string(ip_addr), port_no, std::string(server_name));
        tcp_server->SetServerNotifCallbacks(appln_client_connected, appln_client_disconnected, appln_client_msg_recvd);
        tcp_servers_lst.push_back(tcp_server);
        break;
    case TCP_SERVER_START:
        /* config tcp-server <server-name> start */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (!tcp_server)
        {
            printf("Error: Tcp Server does not Exist\n");
            return -1;
        }
        tcp_server->Start();
        break;
    case TCP_SERVER_STOP:
        /* config tcp-server <server-name> stop */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (!tcp_server)
        {
            printf("Error: Tcp Server does not Exist\n");
            return -1;
        }
        tcp_server->Stop();
        break;
    case TCP_SERVER_STOP_CONN_ACCEPT:
        /* config tcp-server <server-name> [no] disable-conn-accept */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (!tcp_server)
        {
            printf("Error: Tcp Server does not Exist\n");
            return -1;
        }
        switch(enable_or_disable)
        {
        case CONFIG_ENABLE:
            tcp_server->StopConnectionAcceptingSvc();
            break;
        case CONFIG_DISABLE:
            tcp_server->StartConnectionAcceptingSvc();
            break;
        default:
            break;
        }
        break;
    case TCP_SERVER_STOP_CLIENT_LISTEN:
        /* config tcp-server <server-name> [no] disable-client-listen */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (!tcp_server)
        {
            printf("Error: Tcp Server does not Exist\n");
            return -1;
        }
        switch(enable_or_disable)
        {
        case CONFIG_ENABLE:
            tcp_server->StopClientSvcMgr();
            break;
        case CONFIG_DISABLE:
            tcp_server->StartClientSvcMgr();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}

int show_tcp_server_handler(param_t* param, ser_buff_t* ser_buff, op_mode enable_or_disable)
{
    const char* server_name = NULL;
    tlv_struct_t* tlv = NULL;
    std::shared_ptr<TcpServerController> tcp_server;

    int cmd_code = EXTRACT_CMD_CODE(ser_buff);

    TLV_LOOP_BEGIN(ser_buff, tlv)
    {
        if (strncmp(tlv->leaf_id, "tcp-server-name", strlen("tcp-server-name")) == 0)
        {
            server_name = tlv->value;
        }
    } TLV_LOOP_END;

    switch (cmd_code)
    {
    case TCP_SERVER_SHOW:
        /* show tcp-server <server-name> */
        tcp_server = TcpServerLookup(std::string(server_name));
        if (!tcp_server)
        {
            printf("Error: Tcp Server does not Exist\n");
            return -1;
        }
        tcp_server->Display();
        break;
    default:
    break;
    }
    return 0;
}

static void tcp_build_config_cli_tree()
{
    /* config tsp-server <name> */
    param_t* config_hook = libcli_get_config_hook();
    
    {
        /* config tsp-server ...*/
        static param_t tcp_server;
        init_param(&tcp_server, CMD, const_cast<char*>("tcp-server"), NULL, NULL, INVALID, NULL, const_cast<char*>("Cconfig tcp server"));
        libcli_register_param(config_hook, &tcp_server);

        {
            /* config tcp-server <name> */
            static param_t tcp_server_name;
            init_param(&tcp_server_name, LEAF, NULL, config_tcp_server_handler, NULL, STRING, const_cast<char*>("tcp-server-name"), const_cast<char*>("Tcp Server Name"));
            libcli_register_param(&tcp_server, &tcp_server_name);
            set_param_cmd_code(&tcp_server_name, TCP_SERVER_CREATE);

            {
                /* config tcp-server <name> start */
                static param_t start;
                init_param(&start, CMD, const_cast<char*>("start"), config_tcp_server_handler, NULL, INVALID, NULL, const_cast<char*>("Start"));
                libcli_register_param(&tcp_server_name, &start);
                set_param_cmd_code(&start, TCP_SERVER_START);
            }
            {
                /* config tcp-server <name> stop */
                static param_t stop;
                init_param(&stop, CMD, const_cast<char*>("stop"), config_tcp_server_handler, NULL, INVALID, NULL, const_cast<char*>("Stop"));
                libcli_register_param(&tcp_server_name, &stop);
                set_param_cmd_code(&stop, TCP_SERVER_STOP);
            }
            {
                /* config tcp-server <name> [no] disable-conn-accept */
                static param_t dis_conn_accept;
                init_param(&dis_conn_accept, CMD, const_cast<char*>("disable-conn-accept"), config_tcp_server_handler, NULL, INVALID, NULL, const_cast<char*>("Connection Acceptor Settings"));
                libcli_register_param(&tcp_server_name, &dis_conn_accept);
                set_param_cmd_code(&dis_conn_accept, TCP_SERVER_STOP_CONN_ACCEPT);
            }
            {
                /* config tcp-server <name> [no] disable-client-listen */
                static param_t dis_client_listen;
                init_param(&dis_client_listen, CMD, const_cast<char*>("disable-client-listen"), config_tcp_server_handler, NULL, INVALID, NULL, const_cast<char*>("Listening Settings"));
                libcli_register_param(&tcp_server_name, &dis_client_listen);
                set_param_cmd_code(&dis_client_listen, TCP_SERVER_STOP_CLIENT_LISTEN);
            }
            {
                /* config tcp-server <name> [<ip-addr>] ... */
                static param_t tcp_server_addr;
                init_param(&tcp_server_addr, LEAF, 0, NULL, NULL, IPV4, const_cast<char*>("tcp-server-addr"), const_cast<char*>("Tcp Server Address"));
                libcli_register_param(&tcp_server_name, &tcp_server_addr);

                {
                    /* config tcp-server <name> [<ip-addr>] [<port-no>] */
                    static param_t tcp_server_port;
                    init_param(&tcp_server_port, LEAF, 0, config_tcp_server_handler, NULL, INT, const_cast<char*>("tcp-server-port"), const_cast<char*>("Tcp Server Port Number"));
                    libcli_register_param(&tcp_server_addr, &tcp_server_port);
                    set_param_cmd_code(&tcp_server_port, TCP_SERVER_CREATE);
                }
            }
            support_cmd_negation(&tcp_server_name);
        }
    }
}

static void tcp_build_show_cli_tree()
{
    param_t* show_hook = libcli_get_show_hook();

    {
        /* config tsp-server ...*/
        static param_t tcp_server;
        init_param(&tcp_server, CMD, const_cast<char*>("tcp-server"), NULL, NULL, INVALID, NULL, const_cast<char*>("Show tcp server"));
        libcli_register_param(show_hook, &tcp_server);

        {
            /* config tcp-server <name> */
            static param_t tcp_server_name;
            init_param(&tcp_server_name, LEAF, NULL, show_tcp_server_handler, NULL, STRING, const_cast<char*>("tcp-server-name"), const_cast<char*>("Tcp Server Name"));
            libcli_register_param(&tcp_server, &tcp_server_name);
            set_param_cmd_code(&tcp_server_name, TCP_SERVER_SHOW);
        }
    }
}

static void tcp_build_cli()
{
    tcp_build_config_cli_tree();
    tcp_build_show_cli_tree();
}

int main(int argc, char* argv[])
{
    init_libcli();

    tcp_build_cli();

    start_shell();

    return 0;
}