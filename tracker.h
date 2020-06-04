#ifndef TRACKER_H
#define TRACKER_H
#include <netinet/in.h>
#include "parse_metafile.h"

typedef struct _Peer_addr {
	char			ip[16];
	unsigned short	port;
	struct _Peer_addr *next;
} Peer_addr;

int http_encode(unsigned char *in, int len1, char *out, int len2);

int get_tracker_name(announce_list *node, char *name, int len);

int get_tracker_port(announce_list *node, unsigned short *port);

int create_request(char * request, int len , announce_list *node,
					unsigned short port, long long down, long long up,
						long long left, int numwant);

int prepare_connect_tracker(int *max_sockfd);

int prepare_connect_peer(int *max_sockfd);

int get_response_type(char *buffer, int len, int *total_length);

int parse_tracker_response1(char *buffer, int ret, char *redirection, int len);

int parse_tracker_response2(char *buffer, int ret);

int add_peer_node_to_peerlist(int *sock, struct sockaddr_in saptr);

void free_peer_addr_head();

#endif