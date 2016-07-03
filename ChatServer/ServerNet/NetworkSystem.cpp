#include "NetworkSystem.h"
#include <memory.h>
#include <Winsock2.h>
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

		CreateSessionPool(m_config.maxClientCount + m_config.extraClientCount);

		return NETWORK_ERROR_CODE::NONE;
	}

	ChatServer::NETWORK_ERROR_CODE NetworkSystem::BindListenSocket(short port, int backLogCount)
	{
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);
		
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
			session.pRecvBuffer = new char[m_config.maxClientRecvBufferSize];
			session.pSendBuffer = new char[m_config.maxClientSendBufferSize];

			m_clientSessionPool.push_back(session);
			m_ClientSessionPoolIndex.push_back(session.Index);

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
		auto read_set = m_fdSet;
		auto write_set = m_fdSet;
		auto exc_set = m_fdSet;

		timeval timeout{ 0, 1000 };
		auto selectResult = select(0, &read_set, &write_set, &exc_set, &timeout);

		auto isFDSetChanged = CheckSelectResult(selectResult);

		if (!isFDSetChanged)
		{
			return;
		}

		if (FD_ISSET(m_listeningSocket, &m_fdSet))
		{
			NewSession();
		}
		else
		{
			CheckChangedSockets(exc_set, read_set, write_set);
		}

	}

	ChatServer::NETWORK_ERROR_CODE NetworkSystem::NewSession()
{
		SOCKADDR_IN client_addr;
		auto client_len = static_cast<int>(sizeof(client_addr));
		auto client_sock = accept(m_listeningSocket, (SOCKADDR*)&client_addr, &client_len);

		if (client_sock < 0)
		{
			m_logger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, client_sock);
			return NETWORK_ERROR_CODE::ACCEPT_API_ERROR;
		}


		auto newSessionIndex = AllocClientSessionIndex();
		if (newSessionIndex < 0)
		{
			m_logger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, client_sock);

			// 더 이상 수용할 수 없으므로 바로 짜른다.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sock, -1);
			return NETWORK_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, MAX_IP_LEN - 1);

		SetSockOption(client_sock);

		FD_SET(client_sock, &m_fdSet);

		ConnectedSession(newSessionIndex, (int)client_sock, clientIP);

		return NETWORK_ERROR_CODE::NONE;
	}


	void NetworkSystem::SetSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_config.maxClientSockOptRecvBufferSize;
		int size2 = m_config.maxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}


	void NetworkSystem::CheckChangedSockets(fd_set& exc_set, fd_set& read_set, fd_set& write_set)
	{
		for (int i = 0; i < m_clientSessionPool.size(); i++)
		{
			auto& session = m_clientSessionPool[i];

			if (session.IsConnected() == false)
			{
				continue;
			}

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			if (FD_ISSET(fd, &exc_set))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, fd, sessionIndex);
				continue;
			}
		}
	}

	bool NetworkSystem::CheckSelectResult(const int result)
	{
		if (result == 0)
		{
			return false;
		}
		else if (result == -1)
		{
			// TODO:로그 남기기
			return false;
		}

		return true;
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