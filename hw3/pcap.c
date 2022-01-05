#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char **argv) {
    
    char *filename = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];  /* buffer to hold error text */
    int cnt = -1;
    pcap_t *handle = NULL;

    bpf_u_int32 net, mask;
    struct bpf_program fcode;

    filename = argv[1];
    handle = pcap_open_offline(filename, errbuf);
    if (!handle) {
        perror("pcap_open_offline");
    }
    
    if (argc < 2) {
       fprintf(stderr, "[usage] ./pcap <filename>\n"); 
       fprintf(stderr, "[usage] ./pcap <filename> <tcp/udp>\n"); 
       exit(1);
    }
    else if (argc > 2) {
        if (strcmp("tcp", argv[2]) == 0) {
            if(-1 == pcap_compile(handle, &fcode, "tcp", 1, mask)) {
                fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
                pcap_close(handle);
                exit(1);
            }//end if
        }
        else if (strcmp("udp", argv[2]) == 0) {
            if(-1 == pcap_compile(handle, &fcode, "udp", 1, mask)) {
                fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
                pcap_close(handle);
                exit(1);
            }//end if
        }
        else {
           fprintf(stderr, "[usage] ./pcap <filename>\n"); 
           fprintf(stderr, "[usage] ./pcap <filename> <tcp/udp>\n"); 
           exit(1);
        }

        if(-1 == pcap_setfilter(handle, &fcode)) {
            fprintf(stderr, "pcap_pcap_setfilter: %s\n", pcap_geterr(handle));
            pcap_close(handle);
            exit(1);
        }

        //free code
        pcap_freecode(&fcode);
    }

    pcap_dispatch(handle, cnt, callback, NULL);
   
    pcap_close(handle);
    return 0;
}

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

    int i=0;
    static int count=0;
	char timestr[5000];

    printf("\033[36m[No. %d]\033[0m ", ++count);    /* Number of Packets */

	struct tm* ltime;
    ltime = localtime(&header->ts.tv_sec);
    strftime(timestr, sizeof(timestr), "%Y/%m/%d %H:%M:%S", ltime);
	printf("%s\n",timestr);
    printf("Recieved Packet Size: %d\n", header->len);    /* Length of header */

    const struct ether_header* eptr = (struct ether_header*) packet;
    const struct ip* ipHeader;
    const struct ip6_hdr* ipv6Header;
    const struct tcphdr* tcpHeader;
    const struct udphdr* udpHeader;

    char sourceIp[INET6_ADDRSTRLEN];
    char destIp[INET6_ADDRSTRLEN];

    u_short ether_type;
    u_int sourcePort;
    u_int destPort;
    /*
     *u_char *data;
     *int dataLength = 0;
     */

    ether_type = ntohs(eptr->ether_type);

    fprintf(stdout,"MAC address: \n");
    fprintf(stdout,"\t%s -> %s\n",
            ether_ntoa((struct ether_addr*)eptr->ether_shost),
            ether_ntoa((struct ether_addr*)eptr->ether_dhost));


    if (ether_type == ETHERTYPE_IP) {
        fprintf(stdout , "ETHER TYPE: IP\n");

        ipHeader = (struct ip*)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET, &(ipHeader->ip_src), sourceIp, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ipHeader->ip_dst), destIp, INET_ADDRSTRLEN);
        /*printf("\t%s -> %s\n", sourceIp, destIp);*/

        if (ipHeader->ip_p == IPPROTO_TCP) {
            printf("\tProtocal: TCP\n");
            tcpHeader = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
            sourcePort = ntohs(tcpHeader->th_sport);
            destPort = ntohs(tcpHeader->th_dport);

            /*printf("\t%d -> %d\n", sourcePort, destPort);*/
            /*
             *data = (u_char*)(packet + sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr));
             *dataLength = pkthdr->len - (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr));
             */

            printf("\t\t%s:%d -> %s:%d\n", sourceIp, sourcePort, destIp, destPort);
        }
        else if (ipHeader->ip_p == IPPROTO_UDP) {
            printf("\tProtocal: UDP\n");
            udpHeader = (struct udphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
            sourcePort = ntohs(udpHeader->uh_sport);
            destPort = ntohs(udpHeader->uh_dport);

            /*printf("\t%d -> %d\n", sourcePort, destPort);*/
            printf("\t\t%s:%d -> %s:%d\n", sourceIp, sourcePort, destIp, destPort);
        }
        else {
            printf("\tProtocal: Others\n");
        }
    }
    else if (ether_type == ETHERTYPE_IPV6){
        fprintf(stdout, "ETHER TYPE: IPV6\n");
        ipv6Header = (struct ip6_hdr*)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET6, &(ipv6Header->ip6_src), sourceIp, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(ipv6Header->ip6_dst), destIp, INET6_ADDRSTRLEN);
        printf("\t%s -> %s\n", sourceIp, destIp);
    }
    else if (ether_type == ETHERTYPE_PUP){
        fprintf(stdout, "ETHER TYPE: PUP\n");
    }
    else if (ether_type == ETHERTYPE_ARP){
        fprintf(stdout, "ETHER TYPE: ARP\n");
    }
    else if (ether_type == ETHERTYPE_REVARP){
        fprintf(stdout, "ETHER TYPE: REVARP\n");
    }
    else if (ether_type == ETHERTYPE_VLAN){
        fprintf(stdout, "ETHER TYPE: VLAN\n");
    }
    else if (ether_type == ETHERTYPE_LOOPBACK){
        fprintf(stdout, "ETHER TYPE: LOOPBACK\n");
    }      

    
    printf("Payload:\n");                     /* And now the data */
    for(i = 0; i < header->len; i++) {
        if (isprint(packet[i]))                /* Check if the packet data is printable */
            printf("%c",packet[i]);          /* Print it */
        else
            printf(".");          /* If not print a . */
        if ((i % 16 == 0 && i != 0) || i == header->len - 1)
            printf("\n");
    }

    printf("\n");
}
