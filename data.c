#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse_metafile.h"
#include "bitfield.h"
#include "sha1.h"
#include "policy.h"
#include "data.h"
#include "message.h"

extern char		*file_name;
extern files	*files_head;
extern int		file_lenght;
extern int 		piece_length;
extern char		*pieces;
extern int		pieces_length;

extern Bitmap	*bitmap;
extern int		download_piece_num;
extern Peer		*peer_head;

#define btcache_len	1024
Btcache *btcache_head = NULL;
Btcache	*last_piece = NULL;
int		last_piece_index = 0;
int 	last_piece_count = 0;
int		last_slice_len = 0;

int *fds = NULL;
int fds_len = 0;
int have_piece_index[64];
int end_mode = 0;

Btcache *initialize_btcache_node()
{
	Btcache *node;
	node = (Btcache *)malloc(sizeof(Btcache));
	if (node == NULL) return NULL;
	node->buff = (unsigned char *)malloc(16*1024);
	if (node->buff == NULL) {
		if (node != NULL) {
			free(node);
			return NULL;
		}
	}
	node->index = -1;
	node->begin = -1;
	node->length = -1;
	node->in_use = 0;
	node->read_write = -1;
	node->is_full = 0;
	node->is_writed = 0;
	node->access_count = 0;;
	node->next = NULL;

	return node;
}

int create_btcache()
{
	int i;
	Btcache *node, *last;

	for(i = 0; i < btcache_len; i++) {
		node = initialize_btcache_node();
		if (node == NULL) {
			printf("%s:%d create_btcache erro\n", __FILE__, __LINE__);
			release_memory_in_btcache();
		return -1;
		}
		if (btcache_head == NULL) {
			btcache_head = node;
			last = node;
		} else {
			last->next = node;
			last = node;
		}
	}

	int count = file_lenght % piece_length / (16 * 1024);
	if (file_lenght % piece_length % (16 * 1024) != 0) count++;
	last_piece_count = count;
	last_slice_len = file_lenght % piece_length % (16 * 1024);
	if (last_slice_len == 0) last_slice_len = 16 * 1024;
	last_piece_index = pieces_length / 20 - 1;
	while (count > 0) {
		node = initialize_btcache_node();
		if (node == NULL) {
			printf("%s:%d create_btcache erro\n", __FILE__, __LINE__);
			release_memory_in_btcache();
			return -1;
		}
		if (last_piece == NULL) {
			last_piece = node;
			last = node;
			
		} else {
			last->next = node;
			last = node;
		}
		count--;
	}
	for (i = 0; i < 64; i++) {
		have_piece_index[i] = -1;
	}
	return 0;
}

void release_memory_in_btcache()
{
	Btcache *p = btcache_head;
	while (p != NULL) {
		btcache_head = p->next;
		if (p->buff != NULL) free(p->buff);
		free(p);
		p = btcache_head;
	}
		
	release_last_piece();
	if (fds != NULL) free(fds);
}

void release_last_piece()
{
	Btcache *p = last_piece;
	while (p != NULL) {
		last_piece = p->next;
		if (p->buff != NULL) free(p->buff);
		free(p);
		p = last_piece;
	}
		
}

int get_files_count()
{
	int count = 0;

	if (is_multi_files() == 0) return 1;
	files *p = files_head;
	while (p != NULL) {
		count++;
		p = p->next;
	}
	return count;
}

int create_files()
{
	int ret, i;
	char buff[1] = {0x0};

	fds_len = get_files_count();
	if (fds_len < 0) return -1;
	fds = (int *)malloc(fds_len * sizeof(int));
	if (fds == NULL) return -1;

	if (is_multi_files() == 0) {	//待下载的为单文件
		*fds = open(file_name, O_RDWR|O_CREAT, 0777);
		if (*fds < 0) {
			printf("%s:%d error", __FILE__, __LINE__);
			return -1;
		}
		ret = lseek(*fds, file_lenght - 1, SEEK_SET);
		if (ret < 0) {
			printf("%s:%d error", __FILE__, __LINE__);
			return -1;
		}
		ret = write(*fds, buff, 1);
		if (ret != 1) {
			printf("%s:%d error", __FILE__, __LINE__);
			return -1;
		}
	} else {
		ret = chdir(file_name);
		if (ret < 0) {
			ret - mkdir(file_name, 0777);
			if (ret < 0) {
				printf("%s:%d error", __FILE__, __LINE__);
				return -1;
			}
			ret = chdir(file_name);
			if (ret < 0) {
				printf("%s:%d error", __FILE__, __LINE__);
				return -1;
			}
		}
		files *p = files_head;
		i = 0;
		while (p != NULL) {
			fds[i] = open(p->path, O_RDWR|O_CREAT, 0777);
			if (fds[1] < 0) {
				printf("%s:%d error", __FILE__, __LINE__);
				return -1;
			}
			ret = lseek(fds[i], p->length - 1, SEEK_SET);
			if (ret < 0) {
				printf("%s:%d error", __FILE__, __LINE__);
				return -1;
			}
			ret = write(fds[i], buff, 1);
			if (ret != 1) {
				printf("%s:%d error", __FILE__, __LINE__);
				return -1;
			}
			p = p->next;
			i++;
		}
	
	}

	return 0;
}

int write_piece_to_harddisk(int sequence, Peer * peer)
{
	Btcache *node_ptr = btcache_head, *p;
	unsigned char piece_hash1[20], piece_hash2[20];
	int slice_count = piece_length / (16 * 1024);
	int index, index_copy;

	if (peer == NULL) return -1;
	int i = 0;
	while (i < sequence) {
		node_ptr = node_ptr->next;
		i++;
	}
	p = node_ptr;

	SHA1_CTX ctx;
	SHA1Init(&ctx);
	while (slice_count > 0 && node_ptr != NULL) {
		SHA1Update(&ctx, node_ptr->buff, 16 * 1024);
		slice_count--;
	node_ptr = node_ptr->next;
	}
	SHA1Final(piece_hash1, &ctx);
	index = p->index * 20;
	index_copy = p->index;
	for (i = 0; i < 20; i++) piece_hash2[i] = pieces[index + 1];
	int ret = memcmp(piece_hash1, piece_hash2, 20);
	if (ret != 0) {
		printf("piece_has is wrong\n");
		return -1;
	}
	node_ptr = p;
	slice_count = piece_length / (16 * 1024);
	while (slice_count > 0) {
		write_btcache_node_to_harddisk(node_ptr);
		Request_piece *req_p = peer->Request_piece_head;
		Request_piece *req_q = peer->Request_piece_head;
		while (req_p != NULL) {
			if (req_p->begin == node_ptr->begin && req_p->index == node_ptr->index) {
				if (req_q == peer->Request_piece_head) {
					peer->Request_piece_head = req_p->next;
				} else {
					req_q->next = req_p->next;
				}
					free(req_p);
					req_p = req_q = NULL;
					break;
				}
				req_q = req_p;
				req_p = req_p->next;
		}
		node_ptr->index = -1;
		node_ptr->begin = -1;
		node_ptr->length = -1;
		node_ptr->in_use = 0;
		node_ptr->read_write = -1;
		node_ptr->is_full = 0;
		node_ptr->is_writed = 0;
		node_ptr->access_count = 0;
		node_ptr = node_ptr->next;
		slice_count--;
	}
	if (end_mode == 1) delete_request_end_mode(index_copy);
	set_bit_value(bitmap, index_copy, 1);
	for (i = 0; i < 64; i++) {
		if (have_piece_index[i] == -1) {
			have_piece_index[i] = index_copy;
			break;
		}
	}
	download_piece_num++;
	if (download_piece_num % 10 == 0) restore_bitmap();
	printf("%%%%%% Total piece download:%d %%%%%%\n", download_piece_num);
	printf("writed piece index:%d \n", index_copy);
	return 0;
	
}

int read_piece_from_harddisk(Btcache * p, int index) {

};

int write_btcache_to_haddisk(Peer * peer) {
	Btcache *p = btcache_head;
	int slice_count = piece_length / (16 * 1024);
	int index_count = 0;
	int full_count = 0;
	int first_index;

	while(p != NULL) {
		if(index_count % slice_count == 0) {
			full_count = 0;
			first_index = index_count;
		}
		if ((p->in_use == 1) && (p->read_write == 1) && 
			(p->is_full == 1) && (p->is_writed == 0)) {
			full_count++;
		}
		if (full_count == slice_count) {
			write_piece_to_harddisk(first_index, peer);
		}
		index_count++;
		p = p->next;
	}
	return 0;
}

int release_read_btcache_node(int base_count)
{
	Btcache *p = btcache_head;
	Btcache *q = NULL;
	int count = 0;
	int used_count = 0;
	int slice_count = piece_length / (16 * 1024);

	if (base_count < 0) return -1;
	while (p != NULL) {
		if (count % slice_count == 0) used_count += p->access_count;
		if (used_count == base_count) break;

		count++;
		p = p->next;
	}

	if (p != NULL) {
		p = q;
		while (slice_count > 0) {
			p->index = -1;
			p->begin = -1;
			p->length = -1;
			p->in_use = 0;
			p->read_write = -1;
			p->is_full = 0;
			p->is_writed = 0;
			p->access_count = 0;

			slice_count--;
			p = p->next;
		}
	}
	return 0;
}

int is_a_complete_piece(int index, int *sequence) 
{
	
}

void clear_btcache()
{	
	Btcache *node = btcache_head;
	while (node != NULL) {
		node->index = -1;
		node->begin = -1;
		node->length = -1;
		node->in_use = 0;
		node->read_write = -1;
		node->is_full = 0;
		node->is_writed = 0;
		node->access_count = 0;
		node = node->next;
	}
}

int write_slice_to_btcache(int index, int begin, int length, unsigned char * buff, int len, Peer * peer)
{
	int count = 0, slice_count, unuse_count;
	Btcache *p = btcache_head, *q = NULL;

	if (p == NULL) return -1;
	if (index >= pieces_length / 20 || begin > piece_length - 16 * 1024) retutn -1;
	if (buff == NULL || peer == NULL) return -1;
	if (index == last_piece_index) {
		write_slice_to_last_piece(index, begin, length, buff, len, peer);	
		return 0;
	}
	if (end_mode == 1) {
		if (get_bit_value(bitmap, index) == 1) return 0;
	}

	slice_count = piece_length / (16 * 1024);
	while (p != NULL) {
		if(count % slice_count == 0) {
			q = p;
		}
		if (p->index == index && p->in_use == 1) break;
		count++;
		p = p->next;
	}
	// p非空说明当前slice所在的piece的部分数据已经下载
	if (p != NULL) {
		count = begin / (16b * 1024);
		p = q;
		while (count > 0) {
			p = p->next;
			count--;
		}
		if (p->begin == begin && p->in_use == 1 && p->read_write == 1 && p->is_full == 1) {
			return 0;
		}

		node->index = index;
		node->begin = begin;
		node->length = length;
		node->in_use = 1;
		node->read_write = 1;
		node->is_full = 1;
		node->is_writed = 0;
		node->access_count = 0;

		memcpy(p->buff, buff, len);
		printf("+++++ write a slice to btcache index:%-6d begin:%-6d begin +++++", index, begin);
		if (download_piece_num < 10) {
			int sequence;
			int ret;
			ret = is_a_complete_piece(index, &sequence);
			if (ret == 1) {
				printf("###### begin write a piece to harddisk ######\n");
				write_piece_to_harddisk(sequence, peer);
				printf("###### end write a piece to harddisk ######\n");
			}
		}
		return 0;
	}

	int i = 4;
	while (i > 0) {
		slice_count = piece_length / ( 16 * 1024);
		count = 0;
		unuse_count = 0;
		Btcache *q;
		p = btcache_head;
		while (p != NULL) {
			if (count % slice_count == 0) {
				unuse_count =0;
				q = p;
			}
			if (p->in_use == 0) unuse_count++;
			if (unuse_count == slice_count) break;
			count++;
			p->next;
		}
		if (p != NULL) {
			count = begin / (16b * 1024);
			p = q;
			while (count > 0) {
				p = p->next;
				count--;
			}

			node->index = index;
			node->begin = begin;
			node->length = length;
			node->in_use = 1;
			node->read_write = 1;
			node->is_full = 1;
			node->is_writed = 0;
			node->access_count = 0;

			memcpy(p->buff, buff, len);
			printf("+++++ write a slice to btcache index:%-6d begin:%-6d begin +++++", index, begin);
			return 0;
		}

		if (i == 4) write_btcache_to_haddisk(peer);
		if (i == 3)	release_read_btcache_node(16);
		if (i == 2)	release_read_btcache_node(8);
		if (i == 1)	release_read_btcache_node(0);
		i--;
	}

	printf("+++++ write a slice to btcashe FAILED : NO BUFFER +++++\n");
	clear_btcache();
	return 0;
}

int read_slice_for_send(int index, int begin, int length, Peer * peer)
{

}