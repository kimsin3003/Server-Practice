#pragma once

#define FD_SETSIZE 1024 // http://blog.naver.com/znfgkro1/220175848048

//#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include "define.h"

namespace ChatServer 
{
	

	// # 왜 인터페이스를 쓰셨나요?
	class NetworkSystem
	{
	public:

		NetworkSystem();
		~NetworkSystem();
		NETWORK_ERROR_CODE Init(const ServerConfig* config);
		void Run();
		NETWORK_ERROR_CODE SendData();
		PacketInfo GetPacketInfo();

	private:
		NETWORK_ERROR_CODE NewSession();
		void SetSockOption(const SOCKET fd);
		void CheckChangedSockets(fd_set& exc_set, fd_set& read_set, fd_set& write_set);
		bool CheckSelectResult(const int result);
		NETWORK_ERROR_CODE InitLIsteningSocket();
		ChatServer::NETWORK_ERROR_CODE BindListenSocket(short port, int backLogCount);
		void CreateSessionPool(const int totalAcceptCount);
		ServerConfig m_config;
		fd_set m_fdSet;

		std::vector<ClientSession> m_clientSessionPool;
		std::deque<int> m_ClientSessionPoolIndex;
		SOCKET m_listeningSocket;
		fd_set m_fdSet;
		Logger* m_logger;
	};
}


