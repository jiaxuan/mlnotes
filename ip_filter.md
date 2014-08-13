External connections are managed by the gateway through ProxyConnectionPool

	
	bool enableExternalProxyConnections =  // check key "enable_external"

	// new_gateway::main.cpp
	if (enableExternalProxyConnections) {
		string extern_dmz_host = rootKey[ExtHostRegAttributes].c_str();
		int extern_dmz_port = atoi(rootkey[ExtPortRegAttribute].c_str());
		proxyConnections.reset(new ProxyConnectionPool(manager, 5, extern_dmz_host,extern_dmz_port));
	}


The connection pool:

	class ProxyConnectionPool : private Thread

	SessionManager& manager;
	unsigned int m_poolSize;
	InetAddress m_dmz_addr;
	std::list< TcpSocket::Ptr > m_sockPool;

	bool onProcess() 
		create m_poolSize TcpSockets on m_dmz_addr and put them in m_sockPool
		for each connection sockIter:
			read an http header and parse it to get certCN, certSlno and certIssuer
			pass the socket to the session manager by:
			manager.manage(*sockIter, true, certCN, certSlno, certIssuer)


The session manager:

	manage(sock, isExternal, certCN, certSlno, certIssuer)
		shared_ptr< Session > session(new Session(*this, sessionId, ..., isExternal, certCN, certSlno, certIssuer));
		sessions.insert(session);


The session:

	void processMessage(Message& message, uint32 messageSize)
		Header& header = message.getHeader();
		Nullable< RoutingKey > routingKey = header.getRoutingKey();

		if (header.getTopic() == "LoginContract")
			if (isExternal)
				routingKey->keyValue = strng("EXTERNAL") + "|" + sessionId + "|" + manager.getGatewayId() + "|" + 
					certSlno + "|" + certIssuer + "|" + certCN + "|" + Application::instance().identifier();
			else
				routingKey->keyValue = strng("INTERNAL") + "|" + sessionId + "|" + manager.getGatewayId() + "|" + 
					"NONE|NONE|NONE|" + Application::instance().identifier();
