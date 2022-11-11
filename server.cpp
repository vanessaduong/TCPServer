#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <ctime>

bool running{true};
std::mutex m;
int uniqueId{0};
const int MAX_ClIENT_ALLOWED{6};

void getId(int& fileDescriptor)
{
	time_t seconds;
	//int increment = seconds + fileDescriptor*100000;
	char buffer[200] = { 0 };
	while (running)
	{
		//m.lock();
		seconds = time (NULL);
		uniqueId = seconds + fileDescriptor*100000 +1;
		//m.unlock();
		int len = snprintf(buffer, sizeof(buffer), "%d\n", uniqueId);
		int ret = send(fileDescriptor, buffer, len+1, 0);
		if (ret != len+1)
		{
			fileDescriptor = 0;
			return;
		}
		sleep(1);
	}
}


void signalHandler(int signalNum)
{
	if (signalNum == SIGINT)
	{
		//std::cout << "Thank you." << std::endl;
		running = false;
	}
}

int main()
{
    std::vector<std::thread> threads;

    std::cout << "Creating server socket..." << std::endl;
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        std::cerr << "Can't create a socket!";
        return -1;
    }

    struct sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    std::cout << "Binding socket to sockaddr..." << std::endl;
    if (bind(listening, (struct sockaddr *)&hint, sizeof(hint)) == -1) 
    {
        std::cerr << "Can't bind to IP/port";
        return -2;
    }

    std::cout << "Mark the socket for listening..." << std::endl;
    if (listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Can't listen !";
        return -3;
    }

    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    std::cout << "Accept client call..." << std::endl;
    signal(SIGINT, signalHandler);

	int fileDescriptor[6] = {};
	while (running)
	{
		int clientSocket = accept(listening, (struct sockaddr *)&client, &clientSize);
		int i;
		for (i = 0; i < MAX_ClIENT_ALLOWED; i++)
		{
			if (fileDescriptor[i] == 0)
			{
				fileDescriptor[i] = clientSocket;
				std::thread th (getId, std::ref(fileDescriptor[i]));
				th.detach();
				break;
			}
			if (i >= MAX_ClIENT_ALLOWED)
			{
				send(clientSocket, "No more connexion allowed.", 12, 0);
				close(clientSocket);
			}
		}
	}
	
	close(listening);
	//close(clientSocket);

/*
    std::cout << "Received call..." << std::endl;
    if (clientSocket == -1)
    {
        std::cerr << "Problem with client connecting!";
        return -4;
    }

    std::cout << "Client address: " << inet_ntoa(client.sin_addr) << " and port: " << client.sin_port << std::endl;

    close(listening);

    char buf[4096];
    while (true) {
        // clear buffer
        memset(buf, 0, 4096);

        // wait for a message
        int bytesRecv = recv(clientSocket, buf, 4096, 0);
        if (bytesRecv == -1)
        {
            std::cerr << "There was a connection issue." << std::endl;
        }
        if (bytesRecv == 0)
        {
            std::cout << "The client disconnected" << std::endl;
        }
        
        // display message
        //std::cout << "Received: " << std::string(buf, 0, bytesRecv);

        // return message
        //send(clientSocket, buf, bytesRecv+1, 0);
    	
    }

    close(clientSocket);*/

    return 0;

}
