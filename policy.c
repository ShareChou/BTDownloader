#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "parse_metafile.h"
#include "peer.h"
#include "data.h"
#include "message.h"
#include "policy.h"

long long total_down = 0L, total_up = 0L; 	//总的下载量和上传量
float total_down_rate = 0.0F, total_up_rate = 0.0F;
int total_peers = 0;
Unchoke_peers unchoke_peers;

extern int end_mode;
extern Bitmap *bitmap;
extern Peer	*peer_head;
extern int pieces_length;
extern int piece_length;

extern Btcache *btcache_head;
extern int last_piece_index;
extern int last_piece_count;
extern int last_slice_len;
extern int download_piece_num;

void init_unchoke_peers()
{

}

int is_in_unchoke_peers(Peer *node)
{
}

int get_last_index(Peer **array, int len)
{

}

int select_unchoke_peer()
{
}

int *rand_num = NULL;
int get_rand_numbers(int length)
{
	int i, index, piece_count, *temp_num;

	if (length == 0) return -1;
	piece_count = length;

	rand_num = (int *)malloc(piece_count * sizeof(int));
	if (rand_num == NULL) return -1;

	temp_num = (int *)malloc(piece_count * sizeof(int));
	if (temp_num == NULL) return -1;
	for (i = 0; i < piece_count; i++) {
		index = (int)((float)(piece_count - i) * rand() / (RAND_MAX + 1.0));
		rand_num[i] = temp_num[index];
		temp_num[index] = temp_num[piece_count - 1 - i];
	}

	if (temp_num != NULL) free(temp_num);
	return 0;
}

int select_optunchoke_peer()
{

}

int compute_rate()
{
	Peer *p = peer_head;
	time_t time_now = time(NULL);
	long t = 0;

	while (p != NULL) {
		if (p->last_down_timestamp == 0) {
			p->down_rate = 0.0f;
			p->down_count = 0;
		} else {
			t = time_now - p->last_down_timestamp;
			if (t == 0) printf("%s:%d time is 0\n", __FILE__,__LINE__);
			else {
				p->down_rate = p->down_count / t;

			}
			p->down_count = 0;
			p->last_down_timestamp = 0;
		}

		if (p->last_up_timestamp == 0) {
			p->up_rate = 0.0f;
			p->up_count = 0;
		} else {
			t = time_now - p->last_up_timestamp;
			if (t == 0) printf("%s:%d time is 0\n", __FILE__,__LINE__);
			else p->up_rate = p->up_count / t;
			p->up_count = 0;
			p->last_up_timestamp = 0;
		}

		p = p->next;
	}
	return 0;
}

int compute_total_rate()
{
	Peer *p = peer_head;

	total_peers = 0;
	total_down = 0;
	total_up = 0;
	total_down_rate = 0.0f;
	total_up_rate = 0.0f;

	while (p != NULL) {
		total_down += p->down_total;
		total_up += p->up_total;
	total_down_rate += p->down_rate;
		total_up_rate += p->up_rate;

		total_peers++;
		p = p->next;
	}
	return 0;
}

int is_seed(Peer * node)
{

}

int create_req_slice_msg(Peer * node)
{
	int index, begin, length = 16 * 1024;
	int i, count = 0;

	if (node == NULL) return -1;
	if (node->peer_choking == 1 || node->am_interested == 0) return -1;

	Request_piece *p = node->Requested_piece_head, *q = NULL;
	if (p != NULL) {
		while (p->next != NULL) {
			p = p->next;
		}
		int last_begin = piece_length - 16 * 1024;
		if (p->index == last_piece_count) {
			last_begin = (last_piece_count - 1) * 16 * 1024;
		}

		if (p->begin < last_begin) {
			index = p->index;
			begin = p->begin + 16 * 1024;
			count = 0;

			while (begin != piece_length && count < 1) {
				if (p->index == last_piece_index) {
					if (begin == ((last_piece_count - 1) * 16 * 1024))
						length = last_slice_len;
				}

				create_request_msg(index, begin, length, node);
				q = (Request_piece *)malloc(sizeof(Request_piece));
				if (q == NULL) {
					printf("%s:%d error\n", __FILE__, __LINE__);
					return -1;
				}
				q->index = index;
				q->begin = begin;
				q->length = length;
				q->next = NULL;
				p = q;
				begin += 16 * 1024;
				count++;
			}
			return 0;
		}
	}

	if (get_rand_numbers(pieces_length / 20) == -1) {
		printf("%s:%d error\n", __FILE__, __LINE__);
					return -1;
	}

	for (i = 0; i < pieces_length/20; i++) {
		index = rand_num[i];

		if (get_bit_value(&(node->bitmap), index) != 1) continue;
		if (get_bit_value(bitmap, index) == 1) continue;

		Peer 	*peer_ptr = peer_head;
		Request_piece *reqt_ptr;
		int find = 0;
		while (peer_ptr != NULL) {
			reqt_ptr = peer_ptr->Requested_piece_head;
			while (reqt_ptr != NULL) {
				if (reqt_ptr->index == index) {
					find = 1;
					break;
				}
				reqt_ptr = reqt_ptr->next;
			}
			if (find == 1) break;
			peer_ptr = peer_ptr->next;
		}
		if (find == 1) continue;
		break;
	}

	if (i == pieces_length /20) {
		if (end_mode == 0) end_mode = 1;
		for (i = 0; i < pieces_length / 20; i++) {
			if (get_bit_value(bitmap, i) == 0) {
				index = i;
				break;
			}
		}
		if (i == pieces_length / 20) {
			printf("cant not find an index to IP: %s\n", node->ip);
			return -1;
		}
	}

	begin = 0;
	count = 0;
	p = node->Requested_piece_head;
	if (p != NULL)
		while (p->next != NULL) p = p->next;
		while (count < 4) {
			if (index == last_piece_index) {
				if (count + 1 > last_piece_count)
					break;
			}
			if (begin == (last_piece_c - 1) * 16 * 1024)
				length = last_slice_len;
		}

		create_request_msg(index, begin, length, node);
		q = (Request_piece *)malloc(sizeof(Request_piece));
		if (q == NULL) {
			printf("%s:%d error\n", __FILE__, __LINE__);
			return -1;
		}
		q->index = index;
		q->begin = begin;
		q->length = length;
		q->next = NULL;
		if (node->Requested_piece_head == NULL) {
			node->Requested_piece_head = q;
			p = q;
		} else {
			p->next = q;
			p = q;
		}
	
		begin += 16 * 1024;
		count++;

		if (rand_num != NULL) {
			free(rand_num);
			rand_num = NULL;
		
		}
		return 0;
}