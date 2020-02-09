#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <future>
#include <vector>
#include "Textbox.h"

constexpr float OFFSET_BETWEEN_TEXTS = 2.f;
void sendMessage(sf::TcpSocket &serverSocket, const Textbox &textbox, std::vector<sf::Text> &chat,
	const sf::RenderWindow &window, const sf::Font &textFont, const std::string &name);
void connectToServer(sf::TcpSocket &serverSocket);
void sendNameToServer(sf::TcpSocket& serverSocket, std::string &name);
void drawChat(const std::vector<sf::Text> &chat, sf::RenderWindow &window);
void updateChatPosition(std::vector<sf::Text> &chat);

int main() {	
	sf::TcpSocket serverSocket;
	sf::Font textFont;
	std::vector<sf::Text> chat;
	std::string text, name;

	textFont.loadFromFile("fonts/times-new-roman.ttf");
	connectToServer(serverSocket);
	sendNameToServer(serverSocket, name);
	serverSocket.setBlocking(false);
	
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML works!");
	sf::Vector2f textboxSize(window.getSize().x *  0.75f, 60.f), position(0.f, (float)window.getSize().y - textboxSize.y);
	Textbox textbox(textboxSize, position, textFont, "Enter your message");

	textbox.setOnEnterEvent([&]() {
		sendMessage(serverSocket, textbox, chat, window, textFont, name);
	});
	while (window.isOpen()) {
		sf::Event e;

		while (window.pollEvent(e)) {
			if (e.type == sf::Event::Closed) {
				sf::Packet messagePacket;
				std::string messageToServer = name + " has left the chat!";

				messagePacket << messageToServer;
				auto status = serverSocket.send(messagePacket);
				serverSocket.disconnect();
				window.close();
			}
				
			textbox.eventHandler(window, e);
		}

		sf::Packet packetFromServer;	
		std::string messageFromServer;

		//Due to using non blocking mode, we need check to see if the message received by the server was completed before
		//attempting to read from the packet.
		if (serverSocket.receive(packetFromServer) == sf::Socket::Done and packetFromServer >> messageFromServer) {
			sf::Text message(messageFromServer, textFont, 30);

			message.setFillColor(sf::Color::Blue);
			message.setPosition(0.f, window.getSize().y - message.getGlobalBounds().height - textboxSize.y *
				OFFSET_BETWEEN_TEXTS);
			chat.push_back(message);
			std::cout << "x: " << chat.back().getPosition().x << " and y: " << chat.back().getPosition().y << "\n";
			updateChatPosition(chat);
		}
			
		window.clear();
		drawChat(chat, window);
		textbox.drawTextBox(window);
		window.display();
	}
}


void updateChatPosition(std::vector<sf::Text> &chat) {
	//Move all but the last message up by its own height. The last message will always be placed at the bottom
	for (size_t i = 0; i < chat.size() - 1; i++){
		chat[i].move(0.f, -(chat[i].getGlobalBounds().height * OFFSET_BETWEEN_TEXTS));
	}
}

void drawChat(const std::vector<sf::Text> &chat, sf::RenderWindow &window) {
	for (auto &message : chat) {
		window.draw(message);
	}
}

void sendMessage(sf::TcpSocket &serverSocket, const Textbox &textbox, std::vector<sf::Text> &chat,
	const sf::RenderWindow &window, const sf::Font &textFont, const std::string &name) {
	sf::Text message(name + ": " + textbox.getTypedWord(), textFont, 30);
	sf::Packet messageToServer;
	
	message.setFillColor(sf::Color::Red);
	message.setPosition(0.f, window.getSize().y - message.getGlobalBounds().height - textbox.getTextboxSize().y *
		OFFSET_BETWEEN_TEXTS);
	chat.push_back(message);
	updateChatPosition(chat);

	//Assign the text the user typed in into a packet to be sent to the server.
	messageToServer << name + ": " + textbox.getTypedWord();
	serverSocket.send(messageToServer);
}

void connectToServer(sf::TcpSocket &serverSocket) {
	if (serverSocket.connect(sf::IpAddress::LocalHost, 2000) != sf::Socket::Done) {
		std::cout << "Could not connect to server :(\n";
		system("pause");
		exit(1);
	}
	else
		std::cout << "connected to server!\n";
}

void sendNameToServer(sf::TcpSocket &serverSocket, std::string &name){
	sf::Packet packet;

	std::cout << "Enter your id: ";
	std::getline(std::cin, name);

	packet << name;
	serverSocket.send(packet);
}