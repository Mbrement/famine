#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <vector>
#include <iostream>

#define MAX_EVENTS 10
#define PORT 3002
#define MAX_CLIENTS 100

int main() {
	// Create socket
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == 0)
	{
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	int option = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1)
	{
		perror("setsockopt");
		return (EXIT_FAILURE);
	}

	// Prepare sockaddr_in structure
	struct sockaddr_in socket_addr;
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = INADDR_ANY;
	socket_addr.sin_port = htons(PORT);

	// Bind socket to socket_addr and port
	if (bind(socket_fd, (struct sockaddr *)&socket_addr, sizeof(socket_addr)) < 0)
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(socket_fd, MAX_EVENTS) < 0)
	{
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d...\n", PORT);

	std::vector<pollfd>		poll_fds;

	// Add server socket to poll set
	poll_fds.push_back((pollfd){socket_fd, POLLIN, 0});

	while (true)
	{
		// We cannot remove elements from poll_fds while iterating over it
		std::vector<int>	to_remove;

		if (poll(poll_fds.data(), poll_fds.size(), -1) < 0)
		{
			perror("Poll failed");
			exit(EXIT_FAILURE);
		}

		for (size_t i = 0; i < poll_fds.size(); ++i)
		{
			if (poll_fds[i].revents & POLLIN)
			{
				if (poll_fds[i].fd == socket_fd)
				{
					// Accept new connection
					int new_socket_fd = accept(socket_fd, NULL, NULL);
					if (new_socket_fd < 0)
					{
						perror("Accept failed");
						exit(EXIT_FAILURE);
					}

					printf("New connection accepted\n");

					// Add new client to poll set
					poll_fds.push_back((pollfd){new_socket_fd, POLLIN, 0});
				}
				else
				{
					// Read data from client
					char buffer[1024] = {0};
					int valread = read(poll_fds[i].fd, buffer, sizeof(buffer));
					if (valread == 0)
					{
						// Client disconnected
						printf("Client disconnected\n");
						close(poll_fds[i].fd);
						to_remove.push_back(i);
					}
					else
					{
						printf("Received: %s\n", buffer);
					}
				}
			}
		}

		for (std::vector<int>::reverse_iterator it = to_remove.rbegin(); it != to_remove.rend(); it++)
		{
			poll_fds.erase(poll_fds.begin() + *it);
		}
	}

	return 0;
}
