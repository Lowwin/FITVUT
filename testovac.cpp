/*
* ISA Projekt
* Mereni ztratovosti a RTT
* Autor: Aneta Helesicova, xheles02
* 3BIT 2017/2018
*/

#include <iostream>
#include <sstream>
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
#include <ctype.h>
#include <algorithm>
#include <thread>

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
	"-w <timeout>      Time in which program waits for response when not retrieving one (seconds). Default 2s, then 2x calculated RTT.\n"
	"-p <port>         UPD port.\n"
	"-l <port>         UDP port to listen on.\n"
	"-r <value>        RTT value, if RTT > input value, packets exceeding RTT threshold will be reported.\n"
	"-v                Verbose mode - program acts like ping.\n"
	"<node>            IPv4/IPv6 adress or hostname.\n"
	;


typedef struct nodesStruct 
{
	std::string node;
	vector<float> rtts;
	float hourOk, hourSent;
	float tOk, tSent;
	float tLate, tLost;
	float w, r;
} nodesStruct;

//Input nodes
vector<nodesStruct> nodes;

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
	int error;
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
  		cout << "    Node " << i+1 << ": " << nodes[i].node <<endl;
}

bool isInt(char *str)
{
	int stringLen = strlen(str);
    int i = 0;
	bool returnVal = true;

    while(i < stringLen)
    {
        if(!isdigit(str[i]))
        {
            returnVal = false;
            break;
        }
        i++;
    }
    return returnVal;
}

bool isNumber(char *str)
{
	int stringLen = strlen(str);
    int i = 0;
	int dotCount=0;
	bool returnVal = true;

    while(i < stringLen)
    {
        if(!isdigit(str[i]))
        {
			if((str[i] == '.') && dotCount<1)
			{
				dotCount++;
			}
			else
            {
				returnVal = false;
            	break;
			}
        }
        i++;
    }
    return returnVal;
}

/*
**Agruments parsing
**Stores nodes in vector
**Returns stucture of parameters
*/
paramStruct paramGet(int argc, char *argv[])
{
	//FUJ, tohle jeste upravit nejak rozumne
    paramStruct actualParameters = {0, 0, 0, 56, 300, 100, 2, 0, 0, 0, 0};
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
	    	if(i+1 < argc)
			{
				if(isInt(argv[i+1]))
				{
					actualParameters.dataSize=atoi(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use integer value with argument -t" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use int value with argument -s" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-t")))
	    {
			if(i+1 < argc)
			{
				if(isNumber(argv[i+1]))
				{
					actualParameters.t=atof(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use value with argument -t" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use value with argument -t" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-i")))
	    {
			if(i+1 < argc)
			{
				if(isNumber(argv[i+1]))
				{
					actualParameters.i=atof(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use value with argument -i" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use value with argument -i" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-w")))
	    {
			if(i+1 < argc)
			{
				if(isNumber(argv[i+1]))
				{
					actualParameters.w=atof(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use value with argument -w" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use value with argument -w" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-p")))
	    {
			if(i+1 < argc)
			{
				if(isInt(argv[i+1]))
				{
					actualParameters.portUdp=atoi(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use int value with argument -p" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use int value with argument -p" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-l")))
	    {
			if(i+1 < argc)
			{
				if(isInt(argv[i+1]))
				{
					actualParameters.listenUdp=atoi(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use int value with argument -tl" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use int value with argument -l" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else if(!(strcmp(argv[i],"-r")))
	    {
			if(i+1 < argc)
			{
				if(isNumber(argv[i+1]))
				{
					actualParameters.rtt=atof(argv[i+1]);
	    			i++;
				}
				else
				{
					cout << "Error: use value with argument -r" << endl;
					actualParameters.error=1;
					return actualParameters;
				}
			}
	    	else
			{
				cout << "Error: use value with argument -r" << endl;
				actualParameters.error=1;
				return actualParameters;
			}
	    }
	    else
	    	{
				nodesStruct newNode;
				newNode.node = argv[i];
				vector<float> rtts;
				newNode.rtts = rtts;
	    		nodes.push_back(newNode);
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

	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	if( nleft == 1 ) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w ;
		sum += u;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

std::string getStatistics(int nodeNumber)
{
	float min = 10000000.0;
	float max = -10000000.0;
	float sum = 0;
	float sum2 = 0;
	float mean, smean, mdev;
	for(int i=0;i<nodes[nodeNumber].rtts.size();i++)
    {
        if(nodes[nodeNumber].rtts[i]<min)
        	min=nodes[nodeNumber].rtts[i];
		if(nodes[nodeNumber].rtts[i]>max)
			max=nodes[nodeNumber].rtts[i];
		sum += nodes[nodeNumber].rtts[i];
		sum2 += nodes[nodeNumber].rtts[i]*nodes[nodeNumber].rtts[i];
    }
	float avg = sum/nodes[nodeNumber].rtts.size();
	mean = sum/nodes[nodeNumber].rtts.size();
	smean = sum2/nodes[nodeNumber].rtts.size();
	mdev = sqrt(smean-(mean*mean));

	std::ostringstream ret;
	ret << std::fixed << std::setprecision(3) << min << "/" 
	<< std::fixed << std::setprecision(3) << avg << "/" 
	<< std::fixed << std::setprecision(3) << max << "/"
	<< std::fixed << std::setprecision(3) << mdev;
	
	return ret.str();
}

void tOutput(int nodeNumber)
{
	time_t curTimer;
	struct timeval checkTimer;
	struct tm* tm_info;
	char timeBuffer[26];
	float overflowPacks = 0.0;

	gettimeofday(&checkTimer,0);

	if(nodes[nodeNumber].tOk>nodes[nodeNumber].tSent)
	{	
		overflowPacks = nodes[nodeNumber].tOk-nodes[nodeNumber].tSent;
		nodes[nodeNumber].tOk=nodes[nodeNumber].tSent;
	}
	if(nodes[nodeNumber].tOk != nodes[nodeNumber].tSent)
	{
		time(&curTimer);
		tm_info = localtime(&curTimer);
		strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

		float loss =(nodes[nodeNumber].tSent-nodes[nodeNumber].tOk-nodes[nodeNumber].tLate)/(nodes[nodeNumber].tSent/100);
		cout << timeBuffer << "." << std::fixed << std::setprecision(2)
			<< lrint(checkTimer.tv_usec/1000)<< " " << nodes[nodeNumber].node <<": ";
		if(lrint(nodes[nodeNumber].tLate) != 0)
		{
			float lateness = (nodes[nodeNumber].tLate)/(nodes[nodeNumber].tSent/100);
			cout << std::fixed << std::setprecision(3) << lateness << "% (" 
				<< std::fixed << std::setprecision(0) << nodes[nodeNumber].tLate
				<< ") packets exceeded RTT threshold " << nodes[nodeNumber].r
				<< "ms " << endl;
		}
		if(lrint(loss)>=100.0)
		{
			cout << "status down" << endl;
		}
		else
		{
			cout << std::fixed << std::setprecision(3) << loss
				<< "% packet loss, " << std::fixed << std::setprecision(0) 
				<< nodes[nodeNumber].tSent-nodes[nodeNumber].tOk
				<< " packet lost" << endl;
		}
	}
	nodes[nodeNumber].tOk   = overflowPacks;
	nodes[nodeNumber].tSent = 0.0;
	nodes[nodeNumber].tLost = 0.0;
	nodes[nodeNumber].tLate = 0.0;
}

void hourOutput(int nodeNumber)
{
	time_t curTimer;
	struct timeval checkTimer;
	struct tm* tm_info;
	char timeBuffer[26];
	float overflowPacks =0.0;
	
	gettimeofday(&checkTimer,0);
	
	time(&curTimer);
    tm_info = localtime(&curTimer);
    strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	if(nodes[nodeNumber].hourOk>nodes[nodeNumber].hourSent)
	{
		overflowPacks=nodes[nodeNumber].hourOk-nodes[nodeNumber].hourSent;
		nodes[nodeNumber].hourOk=nodes[nodeNumber].hourSent;
	}
	float loss =(nodes[nodeNumber].hourSent-nodes[nodeNumber].hourOk)/(nodes[nodeNumber].hourSent/100);
	cout << timeBuffer << "." << std::fixed << std::setprecision(2)
		<< lrint(checkTimer.tv_usec/1000)<< " " << nodes[nodeNumber].node <<": ";
	if(lrint(loss)>=100.0)
	{
		cout << "status down" << endl;
	}
	else
	{
		std::string statistics = getStatistics(nodeNumber);
		cout << std::fixed << std::setprecision(0) << loss
			<< "% packet loss, rtt min/avg/max/mdev "
			<< statistics << " ms" << endl;
	}
	nodes[nodeNumber].rtts.clear();
	nodes[nodeNumber].hourOk   = overflowPacks;
	nodes[nodeNumber].hourSent = 0.0;
}


int listenTo4(paramStruct parameters, int nodeNumber)
{
	int length;
	char buffer[65000];
	sockaddr_in receiveSockAddr;
	socklen_t size;
	hostent *host;
	iphdr *ip;
	icmphdr *icmpRecv;
	unsigned short int pid = getpid();
	timeval tv;
	int sock;
	unsigned int ttl = 255;
	char *addrString;
	time_t curTimer;
	char timeBuffer[26];
	struct tm* tm_info;
	struct timeval konec, send;
	double timer;
	
	tv.tv_sec = parameters.w;
    tv.tv_usec = 0;
	nodes[nodeNumber].w = parameters.w;
	nodes[nodeNumber].r = parameters.rtt;


	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
    	cerr << "Error when creating listen socket" << endl;
    	return -1;
    }
	setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));

    size = sizeof(sockaddr_in);
	while(1)
    {
		length = 0;
		if ((length = recvfrom(sock, buffer, 65000, 0, (sockaddr *)&receiveSockAddr, &size)) <= 0)
		{
			cerr << "Error when accepting data." << endl;
			break;
		}

		ip = (iphdr *) buffer;
		icmpRecv = (icmphdr *) (buffer + ip->ihl * 4);

		if ((icmpRecv->type == ICMP_ECHOREPLY) && (icmpRecv->un.echo.id == pid))
		{
			char recvTime[16];
			char *bufTimePointer = buffer;
			bufTimePointer+=28;
			memcpy(recvTime,bufTimePointer,sizeof(timeval));

			struct timeval rTime = (timeval &)recvTime;

			addrString = strdup(inet_ntoa(receiveSockAddr.sin_addr));
			host = gethostbyaddr(&receiveSockAddr.sin_addr, 4, AF_INET);
					
			time(&curTimer);
			tm_info = localtime(&curTimer);
			strftime(timeBuffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

			gettimeofday(&konec,0);
			
			timer = ((konec.tv_sec-rTime.tv_sec)*1000000 + (konec.tv_usec - rTime.tv_usec));
			nodes[nodeNumber].rtts.push_back(timer/1000);
			
			if(parameters.verbose)
			{
				cout << timeBuffer << "." << lrint(konec.tv_usec/10000)
					<< " "<< length << " bytes from "
					<< nodes[nodeNumber].node.c_str()
					<< " (" << addrString << ")"
					<< " time=" << std::fixed << std::setprecision(2) << timer/1000 << " ms" << endl;
			}
			
			if((timer/1000) > nodes[nodeNumber].w*2)
			{
				nodes[nodeNumber].tLost++;
			}
			else if((nodes[nodeNumber].r != 0) && ((timer/1000) > nodes[nodeNumber].r))
			{
				nodes[nodeNumber].hourOk++;
				nodes[nodeNumber].tLate++;
			}
			else
			{
				nodes[nodeNumber].tOk++;
				nodes[nodeNumber].hourOk++;
			}
			nodes[nodeNumber].w=timer/1000;
		}
	}
}

int doPing6(paramStruct parameters, int nodeNumber)
{	
	socklen_t size;
	hostent *host;
	icmp6_hdr *icmp, *icmpRecv;
	iphdr *ip;
	int sock, total, lenght;
	unsigned int ttl = 255;
	struct sockaddr_in6 sendSockAddr, receiveSockAddr;
	char buffer[BUFSIZE];
	timeval tv;
	char *addrString;
	in_addr addr;
	unsigned short int pid = getpid();
	time_t curTimer;
	char timeBuffer[26];
	struct tm* tm_info;
	struct timeval konec, send;
	double timer;
	int datasize = parameters.dataSize;

	struct timeval hourOTimer, tOTimer, checkTimer;

	nodes[nodeNumber].hourOk = 0;
	nodes[nodeNumber].hourSent = 0;
	nodes[nodeNumber].tOk = 0;
	nodes[nodeNumber].tSent = 0;

	gettimeofday(&hourOTimer,0);
	gettimeofday(&tOTimer,0);

	int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in6 *h;
    int rv;
	char ipaddr[100];
 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_RAW;
 
    if ( (rv = getaddrinfo( nodes[nodeNumber].node.c_str() , NULL , &hints , &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
 
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        h = (struct sockaddr_in6 *) p->ai_addr;
        //strcpy(ipaddr , inet_ntoa( h->sin6_addr ) );
    }
	cout<<"trace to "<<nodes[nodeNumber].node.c_str()<<" ("
	<<inet_ntop(servinfo->ai_family, &((struct sockaddr_in6*)servinfo->ai_addr)->sin6_addr.s6_addr, ipaddr, 100)<<")"<<endl;
     
    freeaddrinfo(servinfo);

	if ((sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) == -1)
	{
    	cerr << "Unable to create socket." << endl;
    	return -1;
	}
	
	if(setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (const char *)&ttl, sizeof(ttl)) != 0)
	{
		cerr << "Error setsockopt IPv6 ping" << endl;
		return -1;
	}
	
	/*memset(&sendSockAddr, 0, sizeof(sendSockAddr));
	sendSockAddr.sin6_family = AF_INET6;
	sendSockAddr.sin6_port = 0;
	inet_pton(AF_INET6, nodes[nodeNumber].node.c_str(),&(sendSockAddr.sin6_addr));*/

	memset(&sendSockAddr, 0, sizeof(sendSockAddr));
	// copy address
	memcpy(&sendSockAddr, servinfo->ai_addr, servinfo->ai_addrlen);

	while (1)
	{
		timer =0;
		char icmpBuffer[65000];
		char *bufPointer = icmpBuffer;
		char str[datasize-16];
		const char alphanum[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";

		for (int i = 0; i < datasize; ++i)
		{
			str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
		}
		char timestampBuf[16];
		gettimeofday((timeval*)timestampBuf,0);
		gettimeofday(&send,0);
		//cout << "Send time: " << send.tv_sec << "." << send.tv_usec << endl;
		icmp = (icmp6_hdr *) icmpBuffer;
		icmp->icmp6_type = ICMP6_ECHO_REQUEST;
		icmp->icmp6_code = 0;
		icmp->icmp6_id = pid;
		icmp->icmp6_cksum = 0;
		icmp->icmp6_seq = 0;

		bufPointer+= sizeof(icmp);
		for (int i=0; i<16;i++)
		{
			*bufPointer = timestampBuf[i];
			bufPointer++;
		}
		for (int counter=0; counter<strlen(str);counter++)
		{
			*bufPointer = str[counter];
			bufPointer++;
		}

		icmp->icmp6_cksum = checksum((u_short *)icmpBuffer, sizeof(icmp6_hdr)+sizeof(str)-1);
		int errsend = sendto(sock,  (char *)icmpBuffer, sizeof(icmp6_hdr)+sizeof(str)-1,
							 0, (sockaddr *)&sendSockAddr, sizeof(sockaddr));
		if(errsend<=0)
		{	
			fprintf(stderr, "Sendto: %s\n", gai_strerror(errsend));
			return -1;
		}
		
		nodes[nodeNumber].hourSent++;
		nodes[nodeNumber].tSent++;

    	//Print statistics, if time is correct
    	gettimeofday(&checkTimer, 0);
		if ((checkTimer.tv_sec-tOTimer.tv_sec)>=parameters.t)
		{
			tOutput(nodeNumber); 
			gettimeofday(&tOTimer,0);
		}	
		if((checkTimer.tv_sec-hourOTimer.tv_sec)>=5)
		{
			hourOutput(nodeNumber); 
			gettimeofday(&hourOTimer,0);
		}
    	
		usleep(parameters.i*1000);
	}
	close(sock);
	free(icmp);
	return 0;
}

/*
**Sends packets to addresses
*/
int doPing4(paramStruct parameters, int nodeNumber)
{
	nodes[nodeNumber].hourOk = 0;
	nodes[nodeNumber].hourSent = 0;
	nodes[nodeNumber].tOk = 0;
	nodes[nodeNumber].tSent = 0;	
	socklen_t size;
	hostent *host;
	icmphdr *icmp, *icmpRecv;
	//iphdr *ip;
	int sock, total, lenght;
	unsigned int ttl = 255;
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
	struct timeval konec, send;
	double timer;
	int datasize = parameters.dataSize;

	struct timeval hourOTimer, tOTimer, checkTimer;

	gettimeofday(&hourOTimer,0);
	gettimeofday(&tOTimer,0);


	cout << "ICMP header size: " <<sizeof(icmphdr) << endl;
	if ((host = gethostbyname(nodes[nodeNumber].node.c_str())) == NULL)
	{
		cerr << "Bad address." << endl;
	    return -1;
	}
	if ((sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
	{
    	cerr << "Unable to create socket." << endl;
    	return -1;
	}
	
	setsockopt(sock, IPPROTO_IP, IP_TTL, (const char *)&ttl, sizeof(ttl));
	
	sendSockAddr.sin_family = AF_INET;
	sendSockAddr.sin_port = 0;
	memcpy(&(sendSockAddr.sin_addr), host->h_addr, host->h_length);

	while (1)
	{
		timer =0;
		char icmpBuffer[65000];
		char *bufPointer = icmpBuffer;
		
		char str[datasize-15];
		const char alphanum[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";

		for (int i = 0; i < datasize; ++i)
		{
			str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
		}
		char timestampBuf[16];
		gettimeofday((timeval*)timestampBuf,0);
		gettimeofday(&send,0);
		//cout << "Send time: " << send.tv_sec << "." << send.tv_usec << endl;
		icmp = (icmphdr *) icmpBuffer;
		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = pid;
		icmp->checksum = 0;
		icmp->un.echo.sequence = 0;

		bufPointer+= sizeof(icmp);
		for (int i=0; i<=16;i++)
		{
			*bufPointer = timestampBuf[i];
			bufPointer++;
		}
		cout << "String size "<< strlen(str) << endl;
		for (int counter=0; counter<strlen(str);counter++)
		{
			*bufPointer = str[counter];
			bufPointer++;
		}

		cout << "ICMP head " << sizeof(icmphdr) << " String " << sizeof(str)-1 << " ip hdr" << sizeof(iphdr) 
			<< " Timestamp 16" << " icmpBuffer" << sizeof(icmpBuffer) << endl;

		icmp->checksum = checksum((u_short *)icmpBuffer, sizeof(icmphdr)+sizeof(str)-1);
		if(sendto(sock,  (char *)icmpBuffer, sizeof(icmphdr)+sizeof(str)-1, 0, (sockaddr *)&sendSockAddr, sizeof(sockaddr)) <= 0)
			cout << "DID NOT SEND A THING." << endl;
		nodes[nodeNumber].hourSent++;
		nodes[nodeNumber].tSent++;

    	//Print statistics, if time is correct
    	gettimeofday(&checkTimer, 0);
		if ((checkTimer.tv_sec-tOTimer.tv_sec)>=parameters.t)
		{
			tOutput(nodeNumber); 
			gettimeofday(&tOTimer,0);
		}	
		if((checkTimer.tv_sec-hourOTimer.tv_sec)>=5)
		{
			hourOutput(nodeNumber); 
			gettimeofday(&hourOTimer,0);
		}
    	
		usleep(parameters.i*1000);
	}
	close(sock);
	free(icmp);
	return 0;
}

int main(int argc, char *argv[])
{
	vector<std::thread> threadsPing, threadsListen;
    paramStruct parameters = paramGet(argc, argv);
	struct timeval outputTimer;
	struct addrinfo hint, *res = NULL;

	memset(&hint, '\0', sizeof hint);

    hint.ai_family = PF_UNSPEC;

	gettimeofday(&outputTimer,0);
	if(parameters.error==1)
		return -1;
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

    for(int nodeCounter = 0; nodeCounter<nodes.size(); nodeCounter++)
    {
		if(getaddrinfo(nodes[nodeCounter].node.c_str(), NULL, &hint, &res))
		{
			cerr << "Invalid address." << endl;
			return -1;
		}
		if(res->ai_family == AF_INET)
		{
			cout << "Jsem IPv4" << endl;
			threadsPing.push_back(std::thread(doPing4, parameters, nodeCounter));
			threadsListen.push_back(std::thread(listenTo4, parameters, nodeCounter));
		}
		else if(res->ai_family == AF_INET6)
		{
			cout << "Jsem IPv6" <<endl;
			threadsPing.push_back(std::thread(doPing6,parameters,nodeCounter));
		}
		else
		{
			cout << "What is this address?" << endl;
			return -1;
		}
		memset(&hint, '\0', sizeof hint);
    }
    
	for (std::thread& t : threadsPing)
    	t.join();

	for (std::thread& t : threadsListen)
    	t.join();

    
    cout << "--------------DEBUG--------------" << endl;
    printParameters(parameters);
    return 0;
}

