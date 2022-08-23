#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>

// Set packet size here
#define PKT_SIZE 64
// Set port number here
#define PORT_NO 0
// Timeout threshold (in seconds,milliseconds)
#define TIMEOUT_THRESH_s 1
#define TIMEOUT_THRESH_ms 0 

#define MSG_SIZE PKT_SIZE-sizeof(struct icmphdr)

// ICMP header and message together as the ping_packet
struct ping_packet
{
	struct icmphdr icmp_header;
	char msg[MSG_SIZE];
};
// Checks if IP address is valid
int isValidIPAddr(char* ip_addr){
    struct in_addr dst;
    // IPv4, 1 if address is valid, 0,-1 otherwise
    int valid = inet_pton(AF_INET, ip_addr, &dst);
    if (valid != 1){
        return 0;
    }
    return 1;
}
// AS observed in official ping.c
#if BYTE_ORDER == LITTLE_ENDIAN
# define ODDBYTE(v)	(v)
#elif BYTE_ORDER == BIG_ENDIAN
# define ODDBYTE(v)	((unsigned short)(v) << 8)
#else
# define ODDBYTE(v)	htons((unsigned short)(v) << 8)
#endif
// Implementing ICMP checksum
// Similar to ping.c
static unsigned short checksum(void *pckt, int len)
{
	unsigned int sum = 0;
    int length = len;
    unsigned short answer;
    unsigned short *p = pckt; 
    //Sum of 16 bit words
    for(int i = 0; length > 1; length-=2){
        sum += *p++;
    }
    //If odd length, shift last byte to make length 2 bytes, according to endianness
	if (length == 1)
	{
    	sum += ODDBYTE(*(unsigned char *)p);
    }
	sum = (sum >> 16) + (sum & 0xffff);	// add hi 16 to low 16 
	sum += (sum >> 16);			        // add carry 
	answer = ~sum;				        // ones complement 
	return (answer);
}

// Ping the ip address once
void ping_once(int sockfd, struct sockaddr_in *ping_addr, char *ip_addr)
{
	struct ping_packet packet;          //To store packet being sent, received
	struct sockaddr_in rec_addr;        //To store received addr, in proper format
    int rec_size = sizeof(rec_addr);    //Its length
	struct timespec start, end;         //Used to calculate rtt, timespec as it is used by clock.gettime()
    struct timeval timeout;             //timeval set to TIMEOUT_THRESH
    long double rtt = 0;
    timeout.tv_sec = TIMEOUT_THRESH_s;
    timeout.tv_usec = 1000*TIMEOUT_THRESH_ms;

    //Set Timeout value for the socket
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
    //Prepare packet before sending, set header, set msg to 0s, do checksum
    bzero(&packet, sizeof(packet));	
	packet.icmp_header.type = ICMP_ECHO;
    memset(packet.msg,0,sizeof(packet.msg));
    packet.icmp_header.checksum = checksum(&packet, sizeof(packet));

    //Get start time before sending packet
    clock_gettime(CLOCK_MONOTONIC, &start);
    //Send packet
    if (sendto(sockfd, &packet, sizeof(packet), 0,(struct sockaddr*)ping_addr, sizeof(*ping_addr)) <= 0){
        printf("Request timed out or host unreacheable\n");
        return;
    }
    //time_t max_time = time(0)+TIMEOUT_THRESH_s+(double)(timeout.tv_usec/100000.0);

    //Any error in receiveing or timeout, return
    if(recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&rec_addr, &rec_size) <= 0){
        printf("Request timed out or host unreacheable\n");
        return;
    }
    //Get finish time
    clock_gettime(CLOCK_MONOTONIC, &end);

    //Calc rtt in milliseconds
    rtt = (end.tv_sec-start.tv_sec) * 1000 + (double)(end.tv_nsec -start.tv_nsec)/1000000.0;	
	printf("Reply from %s RTT = %Lf ms\n",ip_addr,rtt);
}


int main(int argc, char **argv)
{
	int sockfd;
	char *ip_addr;
    struct sockaddr_in ping_addr;
    struct in_addr saddr;
    // Sanity check command line args count
	if(argc!=2)
	{
		printf("Invalid format. Use  %s <IP Address>\n", argv[0]);
		return 0;
	}

    //Check if valid IP address is given
    ip_addr = argv[1];
    if(!isValidIPAddr(ip_addr)){
        printf("Bad hostname\n");
		return 0;
    }
	printf("Pinging IP: %s\n", ip_addr);
    // Get s_addr using inet_aton, to set sin_addr for ping address
    int res = inet_aton(ip_addr,&saddr);

    //We will never enter here
    if(res==0){
        printf("Bad hostname\n");
    }
    // Configure ping_addr
    ping_addr.sin_family = AF_INET;
    ping_addr.sin_addr.s_addr =  saddr.s_addr;
    ping_addr.sin_port = htons(PORT_NO);

    //Create socket
    //IPPROTOCOL_ICMP helps us ignore ICMP identifier, bookkeeping done by kernel
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sockfd<0)
	{
		printf("\nSocket creation failed\n");
		return 0;
	}
	//Ping once
	ping_once(sockfd, &ping_addr, ip_addr);
	
	return 0;
}
