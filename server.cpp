#include "iostream"
#include "sys/types.h"
#include "unistd.h"
#include "sys/socket.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "string"
#include "thread"
#include "vector"
#include "mutex"
#include "ctime"
#include "chrono"
#include "signal.h"
#include "algorithm"

#define MAX_ClIENT_ALLOWED 6

bool running{true};
std::mutex openedTCPmutex;
unsigned int openedTCP=0;

class TimeHelper
{
private:
	std::chrono::high_resolution_clock::time_point start{std::chrono::high_resolution_clock::now()};
public:
	auto awake_time() {
	    using std::chrono::operator""ms;
	    return start + 1000ms;
	}
};


class Utils
{
public:
	static void threadActions(int& fileDescriptor)
	{
		timeval selectTimeVal;
	    selectTimeVal.tv_sec=0;
	    fd_set rset;

	    openedTCPmutex.lock();
	    openedTCP++;
	    openedTCPmutex.unlock();

		unsigned int increment = (unsigned int)time (NULL) + (unsigned int)fileDescriptor*100000;
	    char buffer[200] = { 0 };
		while (running)
		{
			int ret;
	        /// write
	        int len = snprintf(buffer, sizeof(buffer), "%d\n", increment);
	        ret = send(fileDescriptor, buffer, len+1, 0);
	        TimeHelper start;
	        if (ret != len+1)
	        {
	            openedTCPmutex.lock();
	            openedTCP--;
	            openedTCPmutex.unlock();
	            fileDescriptor = 0;
	            return;
	        }
	        /// read
	        selectTimeVal.tv_usec=100000;
	        FD_SET(fileDescriptor, &rset);
	        ret = select(fileDescriptor+1, &rset, NULL, NULL, &selectTimeVal);
	        if (FD_ISSET(fileDescriptor, &rset) && ret>0) {
	            bool backslashN=false;
	            char buf[4096];
	            int bytesRecv = recv(fileDescriptor, buf, 4096, 0);
	            for (int i=0;i<bytesRecv;i++) {
	                if (buf[i]=='\n') {backslashN=true;break;}
	            }
	            if (backslashN) {
	                int len2 = snprintf(buffer, sizeof(buffer), "%d TCP connection currently active\n", openedTCP);
	                send(fileDescriptor, buffer, len2+1, MSG_NOSIGNAL);
	            }
	        }
	        /// increment ID and sleep to the next second
	        increment++;
	        std::this_thread::sleep_until(start.awake_time());
		}
		send(fileDescriptor, "Thank you\n", 20, MSG_NOSIGNAL);
	    close(fileDescriptor);
	    fileDescriptor=0;

	    openedTCPmutex.lock();
	    openedTCP--;
	    openedTCPmutex.unlock();
	}

	static void signalHandler(int signalNum)
	{
		if (signalNum == SIGINT)
		{
			running = false;
		}
	}
};


int main()
{

    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        //std::cerr << "Can't create a socket!";
        return -1;
    }

    struct sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    if (bind(listening, (struct sockaddr *)&hint, sizeof(hint)) == -1) 
    {
        //std::cerr << "Can't bind to IP/port";
        return -2;
    }

    if (listen(listening, SOMAXCONN) == -1)
    {
        //std::cerr << "Can't listen !";
        return -3;
    }

    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Utils::signalHandler);

	int fileDescriptors[MAX_ClIENT_ALLOWED] = {};
	fd_set rset;
	timeval selectTimeVal;
	selectTimeVal.tv_sec=0;
    int ret;
	while (running)
	{
		selectTimeVal.tv_usec=100000;
        FD_SET(listening, &rset);
        ret = select(listening+1, &rset, NULL, NULL, &selectTimeVal);
        if (FD_ISSET(listening, &rset) && ret>0)
        {
        	int clientSocket = accept(listening, (struct sockaddr *)&client, &clientSize);
			if (std::find(std::begin(fileDescriptors), std::end(fileDescriptors), 0) != std::end(fileDescriptors))
			{
				for (int i = 0; i < MAX_ClIENT_ALLOWED; i++)
				{
					if (fileDescriptors[i] == 0)
					{
						fileDescriptors[i] = clientSocket;
						std::thread th (&Utils::threadActions, std::ref(fileDescriptors[i]));
						th.detach();
						break;
					}
					
				}
			}
			else
			{
				send(clientSocket, "Max connexions reached.\n", 24, MSG_NOSIGNAL);
	            close(clientSocket);
			}
        }	
		
	}

	while (openedTCP>0) usleep(5000);
	close(listening);

    return 0;

}
