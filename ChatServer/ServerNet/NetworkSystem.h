#pragma once

namespace ChatServer 
{
	

	// # �� �������̽��� ���̳���?
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
		NETWORK_ERROR_CODE InitLIsteningSocket();
		ChatServer::NETWORK_ERROR_CODE BindListenSocket(short port, int backLogCount);
		ServerConfig* m_config;
		SOCKET m_listeningSocket;
		fd_set m_fdSet;
		Logger* m_logger;
	};
}


