#ifndef DATA_H
#define DATA_H
#include "peer.h"

//每个Btcache结点维护一个长度为16KB的缓冲区，该缓冲区保存要给slice的数据
typedef struct _Btcache {
	unsigned char 	*buff;		//指向缓冲区的指针
	int				index;		//数据所在的piece块的索引
	int				begin;		//数据在piece块中的起始位置
	int				length;		//数据的长度

	unsigned char	in_use;		//该缓冲区是否在使用中
	unsigned char	read_write;	//是发送给peer的数据还是接收到的数据
								//若数据是从硬盘读出，值为0
								//若数据将要写入硬盘，1

	unsigned char is_full;
	unsigned char is_writed;
	int				access_count;
	struct _Btcache	*next;
	
} Btcache;

Btcache* initialize_btcache_node();
int create_btcache();
void release_memory_in_btcache();

int get_files_count();
int create_files();

int write_btcache_node_to_harddisk(Btcache *node);
int read_slice_from_harddisk(Btcache *node);
int write_piece_to_harddisk(int sequence, Peer *peer);
int read_piece_from_harddisk(Btcache *p, int index);

int write_btcache_to_haddisk(Peer *peer);
int release_read_btcache_node(int base_count);
void clear_btcache_before_peer_close(Peer *peer);
int write_slice_to_btcache(int index, int begin, int length, unsigned char *buff, int len, Peer *peer);
int read_slice_for_send(int index, int begin, int length, Peer *peer);

int write_last_piece_to_btcache(Peer *peer);
int write_slice_to_last_piece(int index, int begin, int length, unsigned char *buff, int len, Peer *peer);
int read_last_piece_from_harddisk(Btcache *p, int index);
int read_slice_for_send_last_piece(int index, int bedin, int length, Peer *peer);
void release_last_piece();

#endif