#ifndef PEER_H
#define PEER_H
#include <string.h>
#include <time.h>
#include "bitfield.h"

#define INITIAL			-1 //表明处于初始化状态
#define HALFSHAKED		0	//表明处于半握手状态
#define HANDSHAKED		1	//表明处于全握手状态
#define SENDBITFIELD	2	//表明处于已发送位图状态
#define RECVBITFIELD	3	//表明处于已接收位图状态
#define DATE			4	//表明处于与peer交换数据的状态
#define CLOSING			5	//表明处于即将已peer断开的状态

//发送和接受缓冲区的大小，16K可以存放一个slice， 2K用来存放其它消息
#define MSG_SIZE	(2*1024+16*1024)

typedef struct _Request_piece {
	int		index;		//请求得piece索引
	int 	begin;		//请求得piece的偏移
	int 	length;		//请求的长度，一般为16KB
	struct _Request_piece *next;
} Request_piece;	//定义数据请求队列的结点

typedef struct _Peer {
	int			socket;		//通过该socket与peer进行通信
	char 		ip[16];		//peer的ip地址
	unsigned short port;	//peer的端口号
	char		id[21];		//peer的id

	int 		state;		//当前所处的状态
	int			am_choking;	//是否将peer阻塞
	int			am_interested;	//是否对peer感兴趣
	int 		peer_choking;	//是否被peer阻塞
	int			peer_interested;	//是否被peer感兴趣

	Bitmap		bitmap;			//存放peer的位图

	char		*in_buff;		//存放从peer处获取的消息
	int 		buff_len;
	char		*out_msg;
	int			msg_len;
	char 		*out_msg_copy;
	int			msg_copy_len;
	int			msg_copy_index;

	Request_piece *Request_piece_head;
	Request_piece *Requested_piece_head;

	unsigned int	down_total;
	unsigned int 	up_total;

	time_t		start_timestamp;
	time_t		recet_timestamp;
	time_t		last_down_timestamp;
	time_t		last_up_timestamp;
	long long 	down_count;
	long long 	up_count;
	float		down_rate;
	float 		up_rate;

	strutct _Peer	*next;
} Peer;

int		initialize_peer(Peer *peer);
Peer* add_peer_node();
int del_peer_node(Peer *peer);
void free_peer_node(Peer *node);
int cancel_request_list(Peer *node);
int cancel_requested_list(Peer *node);
void release_memory_in_peer();
void print_peers_data();

#endif