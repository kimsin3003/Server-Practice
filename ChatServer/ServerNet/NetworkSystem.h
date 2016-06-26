#pragma once

namespace ChatServer 
{
	struct ServerConfig
	{
		unsigned short port;
		int backLogCount;
		int maxClientCount;
		int extraClientCount;
	};
	struct PacketInfo 
	{

	};
	enum class NETWORK_ERROR_CODE : short
	{
		NONE = 0,

		SERVER_SOCKET_CREATE_FAIL = 11,
		SERVER_SOCKET_SO_REUSEADDR_FAIL = 12,
		SERVER_SOCKET_BIND_FAIL = 14,
		SERVER_SOCKET_LISTEN_FAIL = 15,

		SEND_CLOSE_SOCKET = 21,
		SEND_SIZE_ZERO = 22,
		CLIENT_SEND_BUFFER_FULL = 23,
		CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE = 24,
		ACCEPT_API_ERROR = 26,
		ACCEPT_MAX_SESSION_COUNT = 27,
		RECV_API_ERROR = 32,
		RECV_BUFFER_OVERFLOW = 32,
		RECV_REMOTE_CLOSE = 33,
		RECV_PROCESS_NOT_CONNECTED = 34,
		RECV_CLIENT_MAX_PACKET = 35,
	};


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
		NETWORK_ERROR_CODE InitLIsteningSocket();
		ChatServer::NETWORK_ERROR_CODE BindListenSocket(short port, int backLogCount);
		ServerConfig* m_config;
		SOCKET m_listeningSocket;
		fd_set m_fdSet;
		Logger* m_logger;
	};
}


