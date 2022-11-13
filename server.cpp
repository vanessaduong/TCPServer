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
#define MAX_ClIENT_ALLOWED 2

bool running{true};
std::mutex m;

void getId(int& fileDescriptor)
{
	time_t seconds;
	char buffer[200] = { 0 };
	while (running)
	{
		//m.lock();
		seconds = time (NULL);
		int uniqueId = seconds + fileDescriptor*100000;
		//m.unlock();
		int len = snprintf(buffer, sizeof(buffer), "%d\n", uniqueId);
		int ret = send(fileDescriptor, buffer, len+1, 0);
		if (ret != len+1)
		{
			fileDescriptor = 0;
			return;
		}
		uniqueId++;
		sleep(1);
	}
	send(fileDescriptor, "Thank you\n", 10, 0);
    close(fileDescriptor);
    fileDescriptor=0;
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

    if (bind(listening, (struct sockaddr *)&hint, sizeof(hint)) == -1) 
    {
        std::cerr << "Can't bind to IP/port";
        return -2;
    }

    if (listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Can't listen !";
        return -3;
    }

    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    signal(SIGINT, signalHandler);

	//int fileDescriptor[6] = {};
	int fileDescriptors[MAX_ClIENT_ALLOWED] = {};
	while (running)
	{
		int clientSocket = accept(listening, (struct sockaddr *)&client, &clientSize);

		if (std::find(std::begin(fileDescriptors), std::end(fileDescriptors), 0) != std::end(fileDescriptors))
		{
			for (int i = 0; i < MAX_ClIENT_ALLOWED; i++)
			{
				if (fileDescriptors[i] == 0)
				{
					fileDescriptors[i] = clientSocket;
					std::thread th (getId, std::ref(fileDescriptors[i]));
					th.detach();
					break;
				}
				
			}
		}
		else
		{
			send(clientSocket, "Max connexions reached.\n", 24, 0);
            close(clientSocket);
		}
	}

	close(listening);

    return 0;

}
