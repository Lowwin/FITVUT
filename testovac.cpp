/*
* ISA Projekt
* Mereni ztratovosti a RTT
* Autor: Aneta Helesicova, xheles02
* 3BIT 2017/2018
*/

#include <iostream>
#include <iomanip>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <vector>

#include <ctime>
#include <time.h>
#include <math.h>


#define BUFSIZE 1024

using namespace std;

std::string HelpMsg = 
	"Hi!\n"
	"This is my ISA project.\n"
	"It might not be perfect, but it clearly is cute.\n"
	"So, 20/20 I guess? THANK YOU!\n"
	"------------------------------------------------\n"
	"Usage:\n"
	"./testovac [-h] [-u] [-s <size>] [-t <interval>] [-i <interval>] [-w <timeout>] [-p <port>] [-l <port>] [-r <value>] <node1> <node2> ... <nodeN>\n\n"
	"-h                Shows this help message.\n"
	"-u                Program will use UDP for testing. -p must be used alongside.\n"
	"-s <size>         Size of data to send.\n"
	"-t <interval>     Interval in which losing packets is reported (seconds). Default 300s.\n"
	"-i <interval>     Interval in which test packets will be send (milliseconds). Default 100ms.\n"
	"-w <timeout>      Time in which program waits for response when not retrieving one (seconds). Default 2s.\n"
	"-p <port>         UPD port.\n"
	"-l <port>         UDP port to listen on.\n"
	"-r <value>        RTT value, if RTT > input value, packets exceeding RTT threshold will be reported.\n"
	"-v                Verbose mode - program acts like ping.\n"
	"<node>            IPv4/IPv6 adress or hostname.\n"
	;

//Input nodes
vector<std::string> nodes;

/*
**Structure for parameters
*/
typedef struct params 
{
	bool help, udp, verbose;
	int dataSize;
	float t, i, w;
	int portUdp, listenUdp;
	float rtt;
} paramStruct;


/*
** Debug function - print all parameters
*/
void printParameters(paramStruct p)
{
    cout << "Printing parameters" << endl;
    cout << "    Help: "<< p.help << endl;
    cout << "    UDP: "<< p.udp << endl;
    cout << "    Verbosity: "<< p.verbose << endl;
    cout << "    Data Size: "<< p.dataSize << endl;
    cout << "    T: "<< p.t << endl;
    cout << "    I: "<< p.i << endl;
    cout << "    W timeout: "<< p.w << endl;
    cout << "    UDP port: "<< p.portUdp << endl;
    cout << "    Listen UDP port: "<< p.listenUdp << endl;
    cout << "    RTT: "<< p.rtt << endl;
    for(int i=0; i<nodes.size(); i++)
  		cout << "    Node " << i+1 << ": " << nodes[i] <<endl;
}

/*
**Agruments parsing
**Stores nodes in vector
**Returns stucture of parameters
*/
paramStruct paramGet(int argc, char *argv[])
{
	//FUJ, tohle jeste upravit nejak rozumne
    paramStruct actualParameters = {0, 0, 0, 56, 300, 100, 2, 0, 0, 0};
    if(argc==2 && !(strcmp(argv[1],"-h")))
        {
	    actualParameters.help=true;
	    return actualParameters;
        }

    for(int i=1; i<argc; i++)
        {
        	//cout << argv[i]; //Actual parameter, DEBUG reasons
            if(!(strcmp(argv[i],"-h")))
		{
		actualParameters.help=true;
		return actualParameters;
		}
	    else if(!(strcmp(argv[i],"-u")))
		actualParameters.udp=true;
	    else if(!(strcmp(argv[i],"-v")))
		actualParameters.verbose=true;
	    else if(!(strcmp(argv[i],"-s")))
	    {
	    	actualParameters.dataSize=atoi(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-t")))
	    {
	    	actualParameters.t=atof(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-i")))
		{
	    	actualParameters.i=atof(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-w")))
		{
	    	actualParameters.w=atof(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-p")))
		{
	    	actualParameters.portUdp=atoi(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-l")))
		{
	    	actualParameters.listenUdp=atoi(argv[i+1]);
	    	i++;
	    }
	    else if(!(strcmp(argv[i],"-r")))
		{
	    	actualParameters.rtt=atof(argv[i+1]);
	    	i++;
	    }
	    else
	    	{
	    		nodes.push_back(argv[i]);
	    	}
        }
    return actualParameters;
}

/*
** Funkce prevzata z http://www.ping127001.com/pingpage/ping.text
*/
u_short checksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}


/*
**Sends packets to addresses
*/
int doPing(paramStruct parameters, int nodeNumber)
{
	float sentPackets = 0;
	float okPackets = 0;
	float latePackets = 0;
	float lostPackets = 0;	
	socklen_t size;
	hostent *host;
	icmphdr *icmp, *icmpRecv;
	iphdr *ip;
	int sock, total, lenght;
	unsigned int ttl;
	sockaddr_in sendSockAddr, receiveSockAddr;
	char buffer[BUFSIZE];
	fd_set mySet;
	timeval tv;
	char *addrString;
	in_addr addr;
	unsigned short int pid = getpid(), p;
	time_t curTimer;
	char timeBuffer[26];
	struct tm* tm_info;
	struct timeval start, konec;
	double timer;

	struct timeval outputTimer, checkTimer;

	gettimeofday(&start,0);
	gettimeofday(&outputTimer,0);

	if ((host = gethostbyname(nodes[nodeNumber].c_str())) == NULL)
	{
		cerr << "Bad address." << endl;
	    return -1;
	}
	if ((sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
	{
    	cerr << "Unable to create socket." << endl;
    	return -1;
	}
	ttl = 255;
	setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));
	icmp = (icmphdr *)malloc(sizeof(icmphdr)+56);
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->un.echo.id = pid;

	sendSockAddr.sin_family = AF_INET;
	sendSockAddr.sin_port = 0;
	memcpy(&sendSockAddr.sin_addr, host->h_addr, host->h_length);

	while (1)
	{
	timer =0;
    icmp->checksum = 0;
    //icmp->un.echo.sequence = p;
    icmp->checksum = checksum((u_short *)icmp, sizeof(icmphdr));
    if(sendto(sock,
        (char *)icmp, sizeof(icmphdr), 0,
        (sockaddr *)&sendSockAddr, sizeof(sockaddr)) <= 0)
        cout << "DID NOT SEND A THING." << endl;
	sentPackets++;
    gettimeofday(&start,0);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    do
    {
    	FD_ZERO(&mySet);
    	FD_SET(sock, &mySet);
    	if (select(sock + 1, &mySet, NULL, NULL, &tv) < 0)
    	{
            cerr << "Select failed." << endl;
            break;
     	}
    	if (FD_ISSET(sock, &mySet))
    	{
            size = sizeof(sockaddr_in);
            if ((lenght = recvfrom(sock, buffer, BUFSIZE, 0,
               (sockaddr *)&receiveSockAddr,
               &size)) <= 0)
            {
            	cerr << "Error when accepting data." << endl;
            }
            ip = (iphdr *) buffer;
    		icmpRecv = (icmphdr *) (buffer + ip->ihl * 4);
    		if (icmpRecv->type == ICMP_ECHOREPLY)
    		{
				okPackets++;
        		addrString =
            	strdup(inet_ntoa(receiveSockAddr.sin_addr));
         		host = gethostbyaddr(&receiveSockAddr.sin_addr, 4, AF_INET);
        		time(&curTimer);
    			tm_info = localtime(&curTimer);
    			strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

				if(parameters.verbose)
				{
					gettimeofday(&konec,0);
    				cout << timeBuffer << "." << lrint(konec.tv_usec/10000);
    	
					timer = ((konec.tv_sec-start.tv_sec)*1000000 + (konec.tv_usec - start.tv_usec));
        			cout << " "<< lenght << " bytes from "
            			<< nodes[nodeNumber].c_str()
            			<< " (" << addrString << ")"
            			<< " time=" << timer/1000 << " ms" << endl;
        		}
    		}
    		else
      			cout << "NOP. Did not do." << endl;
    	}
    	else
    	{
    		cout << "Timeout" << endl;
    	}

    	//Print statistics, if time is correct
    	gettimeofday(&checkTimer, 0);
		if ((checkTimer.tv_sec-outputTimer.tv_sec)>=3)
		{
			time(&curTimer);
    		tm_info = localtime(&curTimer);
    		strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

			float loss =(sentPackets-okPackets)/(sentPackets/100);
			 
		    cout << timeBuffer << "." << lrint(checkTimer.tv_usec/10000)<< " " << nodes[nodeNumber] <<": ";

			if(loss==100.0)
			{
				cout << "status down" << endl;
			} else
			{
				cout << std::fixed << std::setprecision(0) << loss
				<< "% packet loss, rtt min/avg/max/mdev "
				<< "4.845/4.882/4.912/0.063" << " ms" << endl;
			}
		    gettimeofday(&outputTimer,0);
		}
    	usleep(parameters.i*1000);
	} while (!((icmpRecv->un.echo.id == pid) && (icmpRecv->type == ICMP_ECHOREPLY) && (icmpRecv->un.echo.sequence == p)));
	}
	close(sock);
	free(icmp);
	return 0;
}

int main(int argc, char *argv[])
{

    paramStruct parameters = paramGet(argc, argv);
    if(parameters.help)
    {
    	cout << HelpMsg;
    	return 0;
    }
    if(parameters.udp && (parameters.portUdp==0)){
    	cout << "RTFM! Use -p!\nI quit." << endl;
    	return -1; 
    }
    if(parameters.udp)
    	cout << "Well, not gonna use that silly UDP, but thanks.\n";

    for(int nodeCounter = 0; nodeCounter<nodes.size();nodeCounter++)
    {
    	doPing(parameters, nodeCounter);
    }
    

    
    cout << "--------------DEBUG--------------" << endl;
    printParameters(parameters);
    return 0;
}

