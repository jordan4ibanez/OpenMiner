/*
 * =====================================================================================
 *
 *       Filename:  Client.hpp
 *
 *    Description:
 *
 *        Created:  25/01/2018 12:26:01
 *
 *         Author:  Quentin Bazin, <quent42340@gmail.com>
 *
 * =====================================================================================
 */
#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <functional>
#include <unordered_map>

#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>

#include <gk/core/ApplicationStateStack.hpp>
#include <gk/core/Timer.hpp>

#include "Network.hpp"

class Client {
	using CommandCallback = std::function<void()>;

	public:
		void connect(sf::IpAddress serverAddress, u16 serverPort);
		void disconnect();

		void sendReady();
		void sendKeyState();

		void update(bool &hasGameStarted);

		void addCommand(Network::Command command, const CommandCallback &callback);

		bool isConnected() const { return m_isConnected; }

		u16 id() const { return m_id; }

	private:
		bool m_isConnected = false;

		u16 m_id;

		sf::IpAddress m_serverAddress;
		u16 m_serverPort;

		std::unique_ptr<sf::TcpSocket> m_tcpSocket;

		sf::UdpSocket m_socket;

		gk::Timer m_keyUpdateTimer;

		std::unordered_map<Network::Command, CommandCallback> m_commands;
};

#endif // CLIENT_HPP_
