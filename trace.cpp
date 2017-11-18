/*
*  IPK 2016/2017 
*  Projekt 2 - verze Traceroute
*  Autor: Aneta Helešicová - xheles02
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/errqueue.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <arpa/inet.h>

int paramCheck(int argc, char* argv[], int *ttl, int *maxttl){
	
	if (argc != 6 && argc != 4 && argc != 2){
		fprintf(stderr, "Nespravne zadane argumenty\n");
		return -1;
	}
	else{
		for(int i=1;i<argc;i+=1){
			if(strcmp(argv[i],"-f")==0){
				*ttl=atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-m")==0)
				*maxttl=atoi(argv[i+1]);
		}
	}
	return 0;
}

int tracev4(int src_sock, int ttl, int maxttl, struct sockaddr_in sendTo){
	int res = 0;
	char cbuf[512];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg                  ;
	struct icmphdr icmph;
	struct sockaddr_in target;
	struct timeval start, konec;
	const char* sendmsg = "Msg";
	double time, timer;
	char hostname[NI_MAXHOST];	
	
	for (;ttl<=maxttl;ttl++){
		time =0;
		timer =0;
		if (!(setsockopt(src_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)))) {
		} else {
			printf("Error setting TTL: %s\n", strerror(errno));
			return -1;
		}

		if ((sendto(src_sock, sendmsg, strlen(sendmsg), 0, (struct sockaddr*)&sendTo, sizeof(struct sockaddr_in))) > 0) {
			gettimeofday(&start,0);
		} else {
			fprintf(stderr, "Error sending msg: %s\n", strerror(errno));
			return -1;
		}

		int cosi=2;
		if (!(setsockopt(src_sock, SOL_IP, IP_RECVERR, &cosi, sizeof(cosi)))) {
		} else {
			printf("Error setting IP_RECVERR: %s\n", strerror(errno));
			return -1;
		}

		//Cekani na odpoved
		while (timer < 2000.0){
			iov.iov_base = &icmph;
			iov.iov_len = sizeof(icmph);
			msg.msg_name = &target;
			msg.msg_namelen = sizeof(target);
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			msg.msg_flags = 0;
			msg.msg_control = cbuf;
			msg.msg_controllen = sizeof(cbuf);
			res = recvmsg(src_sock, &msg, MSG_ERRQUEUE);
			//Casovac
			gettimeofday(&konec,0);
			time = ((konec.tv_sec-start.tv_sec)*1000000 + (konec.tv_usec - start.tv_usec));
			timer = time/1000;
			if (res<0)
				continue;
			for (cmsg = CMSG_FIRSTHDR(&msg);cmsg; cmsg =CMSG_NXTHDR(&msg, cmsg)){
				if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR){
					struct sock_extended_err *e = (struct sock_extended_err*)CMSG_DATA(cmsg);
					if (e && e->ee_origin == SO_EE_ORIGIN_ICMP) {
						//Ulozeni adresy
						struct sockaddr_in *sin = (struct sockaddr_in *)(e+1);
						char adresa[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(sin->sin_addr), adresa, INET_ADDRSTRLEN);
						
						//Ziskani Hostname z IP adresy
						struct sockaddr *sa = (struct sockaddr *) sin;
						getnameinfo (sa, sizeof (*sin), hostname, NI_MAXHOST, NULL, 0, 0);
						
						//Kontrola typu odpovedi
						if (e->ee_type == ICMP_DEST_UNREACH && e->ee_code == ICMP_PORT_UNREACH){
							return 0;
						}
						if (e->ee_type == ICMP_TIME_EXCEEDED){
							printf("%d    %s(%s)    %.3f ms\n",ttl,hostname, adresa, timer);
							break;
						}
						if (e->ee_type == ICMP_DEST_UNREACH && e->ee_code == ICMP_NET_UNREACH){
							printf("%d    %s    N!\n",ttl, adresa);
							break;
						}
						if (e->ee_type == ICMP_DEST_UNREACH && e->ee_code == ICMP_HOST_UNREACH){
							printf("%d    %s    H!\n",ttl, adresa);
							break;
						}
						if (e->ee_type == ICMP_DEST_UNREACH && e->ee_code == ICMP_PROT_UNREACH){
							printf("%d    %s    P!\n",ttl, adresa);
							break;
						}
						if (e->ee_type == ICMP_DEST_UNREACH && e->ee_code == 13){
							printf("%d    %s    X!\n",ttl, adresa);
							break;
						}
					}
				}
			}
			break;
		}
		if(timer >= 2000.0)
			printf("%d    *\n",ttl);
	}
return 0;
}
int tracev6(int src_sock, int ttl, int maxttl, struct sockaddr_in sendTo){
	int res = 0;
	char cbuf[512];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg                  ;
	struct icmphdr icmph;
	struct sockaddr_in6 target;
	struct timeval start, konec;
	const char* sendmsg = "Msg";
	double time, timer;
	char hostname[NI_MAXHOST];

	for (;ttl<=maxttl;ttl++){
		time =0;
		timer =0;
		if (!(setsockopt(src_sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &ttl, sizeof(ttl)))) {
		} else {
			printf("Error setting TTL: %s\n", strerror(errno));
			return -1;
		}

		if ((sendto(src_sock, sendmsg, strlen(sendmsg), 0, (struct sockaddr*)&sendTo, sizeof(struct sockaddr_in6))) > 0) {
			gettimeofday(&start,0);
		} else {
			fprintf(stderr, "Error sending msg: %s\n", strerror(errno));
			return -1;
		}

		int cosi=2;
		if (!(setsockopt(src_sock, SOL_IPV6, IPV6_RECVERR, &cosi, sizeof(cosi)))) {
		} else {
			printf("Error setting IP_RECVERR: %s\n", strerror(errno));
			return -1;
		}

		
		while (timer < 2000.0){
			iov.iov_base = &icmph;
			iov.iov_len = sizeof(icmph);
			msg.msg_name = &target;
			msg.msg_namelen = sizeof(target);
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			msg.msg_flags = 0;
			msg.msg_control = cbuf;
			msg.msg_controllen = sizeof(cbuf);
			res = recvmsg(src_sock, &msg, MSG_ERRQUEUE);
			gettimeofday(&konec,0);
			time = ((konec.tv_sec-start.tv_sec)*1000000 + (konec.tv_usec - start.tv_usec));
			timer = time/1000;
			if (res<0)
				continue;
			for (cmsg = CMSG_FIRSTHDR(&msg);cmsg; cmsg =CMSG_NXTHDR(&msg, cmsg)){
				if (cmsg->cmsg_level == SOL_IPV6 && cmsg->cmsg_type == IPV6_RECVERR){
					struct sock_extended_err *e = (struct sock_extended_err*)CMSG_DATA(cmsg);
					if (e && e->ee_origin == SO_EE_ORIGIN_ICMP6) {
						struct sockaddr_in6 *sin = (struct sockaddr_in6 *)(e+1);//TADY JE ULOZENA ADRESA
						char adresa[100];
						inet_ntop(AF_INET6, &(sin->sin6_addr), adresa, 100);
						
						struct sockaddr *sa = (struct sockaddr *) sin;
						getnameinfo (sa, sizeof (*sin), hostname, NI_MAXHOST, NULL, 0, 0);
						
						if (e->ee_type == 1 && e->ee_code == 4){
							return 0;
						}
						else if (e->ee_type == 3){
							printf("%d    %s(%s)    %.3f ms\n",ttl,hostname, adresa, timer);
							break;
						}
						else if  (e->ee_type == 1 && e->ee_code == 0){
							printf("%d    %s    N!\n",ttl, adresa);
							break;
						}
						else if  (e->ee_type == 1 && e->ee_code == 3){
							printf("%d    %s    H!\n",ttl, adresa);
							break;
						}
						else if  (e->ee_type == 4 && e->ee_code == 1){
							printf("%d    %s    P!\n",ttl, adresa);
							break;
						}
						else if  (e->ee_type == 1 && e->ee_code == 13){
							printf("%d    %s    X!\n",ttl, adresa);
							break;
						}
						printf("EE_TYPE %d, EE_CODE %d\n",e->ee_type,e->ee_code);
					}
				}
			}
			break;
		}
		if(timer >= 2000.0)
			printf("%d    *\n",ttl);
	}
return 0;
}

int main(int argc, char *argv[]){ 
struct addrinfo hints;
struct addrinfo* ret;
char ipv4[100];
int status = 0;
int ttl=1;
int maxttl=30;
int src_sock = 0;
const char* dest_port = "33434";

if(paramCheck(argc,argv,&ttl,&maxttl)!=0)
	return -1;

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_DGRAM;


if ((status = getaddrinfo(argv[argc-1], dest_port, &hints, &ret)) != 0) {
    printf("getaddrinfo: %s\n", gai_strerror(status));
    return -1;
}

if(ret->ai_family==AF_INET){
	struct sockaddr_in* ip = (struct sockaddr_in *)ret->ai_addr;
	inet_ntop(ret->ai_family, &(ip->sin_addr), ipv4, 100);
}
else{
	struct sockaddr_in6* ip = (struct sockaddr_in6 *)ret->ai_addr;
	inet_ntop(ret->ai_family, &(ip->sin6_addr), ipv4, 100);
}

if ((src_sock = socket(ret->ai_family, ret->ai_socktype, 
                ret->ai_protocol)) < 0) {
    fprintf(stderr, "Error creating host socket: %s\n", strerror(errno));
    return -1;
}


struct sockaddr_in sendTo;
sendTo.sin_family = ret->ai_family;
sendTo.sin_port=htons(33434);
inet_aton(argv[argc-1],&sendTo.sin_addr);

if (ret->ai_family==AF_INET)
	if(tracev4(src_sock,ttl,maxttl, sendTo)==0)
		return 0;
	else 
		return -1;
else
	if(tracev6(src_sock,ttl,maxttl,sendTo)==0)
		return 0;
	else 
		return -1;
}