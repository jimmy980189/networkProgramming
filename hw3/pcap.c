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
char *tcp_ftoa(u_int8_t flag);
int verbose;

int main(int argc, char **argv) {
    
    char *filename = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
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
        if(-1 == pcap_compile(handle, &fcode, argv[2], 1, mask)) {
            fprintf(stderr, "pcap_compile: %s\n", pcap_geterr(handle));
            pcap_close(handle);
            exit(1);
        }

        if(-1 == pcap_setfilter(handle, &fcode)) {
            fprintf(stderr, "pcap_pcap_setfilter: %s\n", pcap_geterr(handle));
            pcap_close(handle);
            exit(1);
        }

        pcap_freecode(&fcode);

        if (argc > 3)
            if (strcmp("-v", argv[3]) == 0)
                verbose = 1;
    }

    pcap_dispatch(handle, cnt, callback, NULL);
   
    pcap_close(handle);
    return 0;
}

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

    int i=0;
    static int count;

    struct tm *ltime;
    char timestr[32];
    time_t local_tv_sec;

    const struct ether_header* eptr = NULL;
    const struct ip* ipHeader;
    const struct ip6_hdr* ipv6Header;
    const struct tcphdr* tcpHeader;
    const struct udphdr* udpHeader;

    char sourceIp[INET6_ADDRSTRLEN];
    char destIp[INET6_ADDRSTRLEN];
    char srcMac[18];
    char dstMac[18];

    u_short ether_type;
    u_int sourcePort;
    u_int destPort;
    /*
     *u_char *data;
     *int dataLength = 0;
     */


    printf("\033[36m[No. %d]\033[0m\n", ++count);    /* Number of Packets */

    local_tv_sec = header->ts.tv_sec;
    ltime = localtime(&local_tv_sec);
    /*strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);*/
    strftime(timestr, sizeof(timestr), "%Y/%m/%d-%H:%M:%S", ltime);

    printf("Time: %s.%.6d\n", timestr, header->ts.tv_usec);
    printf("Length: %d bytes\n", header->len);
    printf("Capture length: %d bytes\n", header->caplen);

        
    eptr = (struct ether_header*) packet;
    ether_type = ntohs(eptr->ether_type);

    /*fprintf(stdout,  "Ethernet Frame:\n");*/
    fprintf(stdout,  "MAC address:\n");
    strcpy(srcMac, ether_ntoa((struct ether_addr*)eptr->ether_shost));
    strcpy(dstMac, ether_ntoa((struct ether_addr*)eptr->ether_dhost));
    fprintf(stdout, "\t%s -> %s\n", srcMac, dstMac);


    switch (ether_type) {
        case ETHERTYPE_IP:
            fprintf(stdout , "ETHER TYPE: IP\n");

            ipHeader = (struct ip*)(packet + sizeof(struct ether_header));
            inet_ntop(AF_INET, &(ipHeader->ip_src), sourceIp, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(ipHeader->ip_dst), destIp, INET_ADDRSTRLEN);

            switch (ipHeader->ip_p) {
            
                case IPPROTO_TCP:
                    tcpHeader = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
                    sourcePort = ntohs(tcpHeader->th_sport);
                    destPort = ntohs(tcpHeader->th_dport);

                    printf("Protocal: TCP\n");
                    printf("\t%s:%d -> %s:%d\n", sourceIp, sourcePort, destIp, destPort);

                    if (verbose == 1) {

                        u_int32_t sequence = ntohl(tcpHeader->th_seq);
                        u_int32_t ack = ntohl(tcpHeader->th_ack);
                        u_int8_t header_len = tcpHeader->th_off << 2;
                        u_int8_t flags = tcpHeader->th_flags;
                        u_int16_t window = ntohs(tcpHeader->th_win);
                        u_int16_t checksum = ntohs(tcpHeader->th_sum);
                        u_int16_t urgent = ntohs(tcpHeader->th_urp);

                        printf("+-------------------------+-------------------------+\n");
                        printf("| Source Port:       %5u| Destination Port:  %5u|\n", sourcePort, destPort);
                        printf("+-------------------------+-------------------------+\n");
                        printf("| Sequence Number:                        %10u|\n", sequence);
                        printf("+---------------------------------------------------+\n");
                        printf("| Acknowledgement Number:                 %10u|\n", ack);
                        printf("+------+-------+----------+-------------------------+\n");
                        printf("| HL:%2u|  RSV  |F:%8s| Window Size:       %5u|\n", header_len, tcp_ftoa(flags), window);
                        printf("+------+-------+----------+-------------------------+\n");
                        printf("| Checksum:          %5u| Urgent Pointer:    %5u|\n", checksum, urgent);
                        printf("+-------------------------+-------------------------+\n");
                    }

                    /*
                     *data = (u_char*)(packet + sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr));
                     *dataLength = pkthdr->len - (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct tcphdr));
                     */

                    break;

                case IPPROTO_UDP:
                    printf("Protocal: UDP\n");
                    udpHeader = (struct udphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
                    sourcePort = ntohs(udpHeader->uh_sport);
                    destPort = ntohs(udpHeader->uh_dport);
                    u_int16_t len = ntohs(udpHeader->uh_ulen);
                    u_int16_t checksum = ntohs(udpHeader->uh_sum);

                    printf("\t%s:%d -> %s:%d\n", sourceIp, sourcePort, destIp, destPort);

                    if (verbose == 1) {
                        printf("+-------------------------+-------------------------+\n");
                        printf("| Source Port:       %5u| Destination Port:  %5u|\n", sourcePort, destPort);
                        printf("+-------------------------+-------------------------+\n");
                        printf("| Length:            %5u| Checksum:          %5u|\n", len, checksum);
                        printf("+-------------------------+-------------------------+\n");

                    }
                    break;

                case IPPROTO_ICMP:
                    printf("Protocal: ICMP\n");
                    break;

                default:
                    printf("Protocal: Others\n");
            }
            break;

        case ETHERTYPE_IPV6:
            fprintf(stdout, "ETHER TYPE: IPV6\n");
            ipv6Header = (struct ip6_hdr*)(packet + sizeof(struct ether_header));
            inet_ntop(AF_INET6, &(ipv6Header->ip6_src), sourceIp, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &(ipv6Header->ip6_dst), destIp, INET6_ADDRSTRLEN);
            printf("\t%s -> %s\n", sourceIp, destIp);
            break;

        case ETHERTYPE_PUP:
            fprintf(stdout, "ETHER TYPE: PUP\n");
            break;

        case ETHERTYPE_ARP:
            fprintf(stdout, "ETHER TYPE: ARP\n");
            break;
        
        case ETHERTYPE_REVARP:
            fprintf(stdout, "ETHER TYPE: REVARP\n");
            break;

        case ETHERTYPE_VLAN:
            fprintf(stdout, "ETHER TYPE: VLAN\n");
            break;

        case ETHERTYPE_LOOPBACK:
            fprintf(stdout, "ETHER TYPE: LOOPBACK\n");
            break;      
    }

    
    if (verbose == 1) {
        printf("Payload:\n");                     /* And now the data */
        for(i = 0; i < header->len; i++) {
            if (isprint(packet[i]))                /* Check if the packet data is printable */
                printf("%c",packet[i]);          /* Print it */
            else
                printf(".");          /* If not print a . */
            if ((i % 16 == 0 && i != 0) || i == header->len - 1)
                printf("\n");
        }
    }

    printf("\n");
}

char *tcp_ftoa(u_int8_t flag) {
    static int  f[] = {'W', 'E', 'U', 'A', 'P', 'R', 'S', 'F'};
#define TCP_FLG_MAX (sizeof f / sizeof f[0])
    static char str[TCP_FLG_MAX + 1];
    u_int32_t mask = 1 << 7;
    int i;

    for (i = 0; i < TCP_FLG_MAX; i++) {
        if(mask & flag)
            str[i] = f[i];
        else
            str[i] = '-';
        mask >>= 1;
    }//end for
    str[i] = '\0';

    return str;
}//end tcp_ftoa
