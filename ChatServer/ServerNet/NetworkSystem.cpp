#include "NetworkSystem.h"
#include <memory.h>
#include <Winsock2.h>
#include "define.h"
#include "Logger.h"

namespace ChatServer
{


	NetworkSystem::NetworkSystem()
	{
	}


	NetworkSystem::~NetworkSystem()
	{
	}

	ChatServer::NETWORK_ERROR_CODE NetworkSystem::Init(const ServerConfig* config)
	{
		memcpy(&m_config, config, sizeof(ServerConfig));

		auto initRet = InitLIsteningSocket();
		if (initRet != NETWORK_ERROR_CODE::NONE)
		{
			return initRet;
		}

		auto bindRet = BindListenSocket(m_config->port, m_config->backLogCount);

		if (bindRet != NETWORK_ERROR_CODE::NONE)
		{
			return bindRet;
		}

		FD_ZERO(&m_fdSet);
		FD_SET(m_listeningSocket, &m_fdSet);

		CreateSessionPool(m_config->maxClientCount + m_config->extraClientCount);
	}

	ChatServer::NETWORK_ERROR_CODE NetworkSystem::BindListenSocket(short port, int backLogCount)
	{
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);

		// #강제 형변환할거면 뭐하러 IN으로 만들어서 저 바인딩을 해준건가요...?
		if (bind(m_listeningSocket, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0)
		{
			return NETWORK_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		if (listen(m_listeningSocket, backLogCount) == SOCKET_ERROR)
		{
			return NETWORK_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		m_logger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%d)", __FUNCTION__, m_listeningSocket);

		return NETWORK_ERROR_CODE::NONE;
	}

	void NetworkSystem::CreateSessionPool(const int totalAcceptCount)
	{
		for (int i = 0; i < totalAcceptCount; i++)
		{
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[m_config->maxClientCount];
		}
	}

	ChatServer::NETWORK_ERROR_CODE NetworkSystem::InitLIsteningSocket()
	{
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);


		// # SOCK_STREAM이 아니면 어떻게 되는건가요?
		m_listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_listeningSocket < 0)
		{
			return NETWORK_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto n = 1;

		// # REUSEADDR이 뭘 재사용한다는 뜻인가요?
		if (setsockopt(m_listeningSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)))
		{
			return NETWORK_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NETWORK_ERROR_CODE::NONE;
	}

	void NetworkSystem::Run()
	{


	}



	NETWORK_ERROR_CODE NetworkSystem::SendData()
	{
		return NETWORK_ERROR_CODE();
	}

	PacketInfo NetworkSystem::GetPacketInfo()
	{
		return PacketInfo();
	}

}