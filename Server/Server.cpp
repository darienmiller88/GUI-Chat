#include "Server.h"
#include <iostream>
#include <future>

Server::Server(uint16_t portToListen) : clientJoined(false) {
	// bind the listener to a port	
	if (listener.listen(portToListen) != sf::Socket::Done) {
		std::cerr << "Could not listen to port: " << listener.getLocalPort() << " for whatever reason :/";
		system("pause");
		exit(1);
	}

	selector.add(listener);
	std::cout << "SERVER IS RUNNING YOO!! WE LISTENING ON PORT " << listener.getLocalPort() << "\n";
}

void Server::runServer(){
	
	while (true){
		if (selector.wait()) {

			//Test to see if the server is receiving a connection from a client. If so, add them to our map of clients
			if (selector.isReady(listener))
				listenForClients();

			//Otherwise, iterate over each client to send and receive messages from them.
			else
				processClients();
		}
	}

}

void Server::listenForClients(){
	//std::cout << "waiting for a connection...\n";

	auto client = std::make_unique<sf::TcpSocket>();
	std::string clientID;
	sf::Packet packet;

	//If a client has successfully connected to our server...
	if (listener.accept(*client) == sf::Socket::Done){
		clientJoined = true;

		auto status = client->receive(packet);
		switch (status){
			//Read in the id sent by the client. Error handle to make sure it was received and extracted successfully.
			case sf::Socket::Done:
				if(packet >> clientID)
					std::cout << clientID << " has connected to the chat!\n";
				break;
			case sf::Socket::NotReady:
				std::cout << "not ready\n";
				break;
			case sf::Socket::Partial:
				std::cout << "partial\n";
				break;
			case sf::Socket::Disconnected:
				std::cout << "disconnected\n";
				break;
			case sf::Socket::Error:
				std::cout << "error\n";
				break;
		}
		
		// Add the new client to the clients list
		clients[clientID] = std::move(client);

		// Add the new client to the selector so that we will be notified when he sends something
		selector.add(*clients[clientID]);

		//After adding in the new client, notify the other one's that this new client has been added.
		notifyClientsOfNewClient(clientID);
	}
	else
		std::cout << "Client: " << client->getRemoteAddress() << " failed to connect :/\n";
}

void Server::processClients(){
	std::vector<std::string> clientsToRemove;

	for (auto &client : clients) {
		
		//Check each client to see if they are ready to receive data
		if (selector.isReady(*client.second)) {
			sf::Packet receivePacket, sendPacket;
			std::string message;

			//Check to see if any of the clients has sent a message to the server. If so, send that message to every
			//other client except for the client that sent it.
			if (client.second->receive(receivePacket) == sf::Socket::Done) {
				receivePacket >> message;
				sendPacket << message;

				for (auto &clientCopy : clients) {
					//Iterate through all of the clients. If the client in this for loop is not the same one has the
					//client we are currently processing in the outer loop, send the message.
					if (clientCopy.first != client.first)
						clientCopy.second->send(sendPacket);
				}
			}
			else if (client.second->receive(receivePacket) == sf::Socket::Disconnected) {
				std::cout << "client: " << client.first << " has disconnected\n";
				clientsToRemove.push_back(client.first);
			}
		}
	}

	//Once we have processsed all the clients requests, remove any clients that may have disconnected.
	for (auto& clientName : clientsToRemove) {
		selector.remove(*clients[clientName]);
		clients.erase(clientName);
	}
}

void Server::notifyClientsOfNewClient(const std::string &nameOfNewClient) {
	for (auto &client : clients) {
		sf::Packet clientPacket;

		//Insert the message that a new client has joined the server into the packet, and send it to the clients.
		if (client.first != nameOfNewClient) {
			clientPacket << nameOfNewClient + " has joined the chat!\n";
			client.second->send(clientPacket);
		}
	}
}