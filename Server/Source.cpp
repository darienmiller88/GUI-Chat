#include <iostream>
#include <SFML/Graphics.hpp>
#include "Server.h"

using namespace sf;

int main() {
	constexpr uint16_t PORT = 2000;
	Server server(PORT);

	server.runServer();
}