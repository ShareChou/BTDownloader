#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "parse_metafile.h"
#include "sha1.h"

char *metafile_content = NULL;	//保存种子文件的内容
long filesize;					//种子文件的长度

int piece_length = 0;	//每个piece的长度，通常为256KB即262144字节
char	*pieces = NULL;	//保存每个pieces的哈希值，每个哈希值为20字节
int		pieces_length = 0;	//缓冲区pieces的长度

int		multi_file = 0;		//指明是单文件还是多文件
char	*file_name = NULL;	//对于单文件，存放文件名；对于多文件，存放目录名
long long	file_lenght = 0;	//存放待下载文件的总长度
files	*files_head = NULL;	//只对多文件种子有效，存放各个文件的路径和长度

unsigned char	info_hash[20];		//保存info_hash的值，连接tracker和peer时使用
unsigned char	peer_id[20];		//保存peer_id的值，连接peer时使用

announce_list *announce_list_head = NULL;	//用于保存所有tracker服务器的url

int read_metafile(char *metafile_naem)
{
	long i;
	
	//以二进制、只读方式打开文件,以二进制方式打开是防止00截断
	FILE *fp = fopen(metafile_name, "rb");
	if (fp == NULL) {
		printf("%s:%d can not open file\n", __FILE__, __LINE__);
		return -1;
	}

	//获取种子文件的长度，filesize为全局变量，在parse_metefile.c头部定义
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);	//ftell()返回位置标识符的当前值
	if (filesize == -1) {
		printf("%s:%d fseek failed\n", __FILE__, __LINE__);
		return -1;
	}

	//读取种子文件内容到metafile_content缓冲区中
	fseek(fp, 0, SEEK_SET);
	for (i = 0; i < filesize; i++) {
		metafile_content[i] = fgetc(fp);
	}
	metafile_content[i] = '\0';

	fclose(fp);
#ifdef DEBUG
	printf("metaile size is: %ld\n", filesize);
#endif
	return 0;
}

int find_keyword(char *keyword, long *position)
{
	long i;

	*position = -1;
	if (keyword == NULL)	return 0;

	for (i = 0; i < filesize-strlen(keyword); i++) {
		if (memcmp(&metafile_content[i], keyword, strlen(keyword)) == 0) {
			*positon = i;
			return 1;
		}
	}
}

int read_announce_list()
{
	announce_list *node = NULL;
	announce_list *p = NULL;
	int len = 0;
	long i;

	if (find_keyword("13:announce-list", &i) == 0) {
		if (find_keyword("8:announce", &i) == 1) {
			i = i + strlen("8:announce");
			while (isdigit(metafile_content[i])) {
				len = len * 10 + (metafile_content[i] - '0');
				i++;
			}
			i++;	//跳过‘：’

			node = (announce_list *)malloc(sizeof(announce_list));
			strncpy(node->announce, &metafile_content[i], len);
			node->announce[len] = '\0';
			node->next = NULL;
			announce_list_head = node;
		}
	} else {
		//如果有13:announce-list关键词就不用处理8:announce关键词
		i= i+ strlen("13:announce-list");
		i++;	//跳过‘1’
		while (metafile_content[i] != 'e') {
			len = len * 10 + (metafile_content[i] - '0');
			i++;
		}
		if (metafile_content[i] == ':')	{
			i++;
		} else {
			return -1;
		}

		//只处理以http开头的tracker地址，不处理以udp开头的地址
		if (memcmp(&metafile_content[i], "http", 4) == 0) {
			node = (announce_list *)malloc(sizeof(announce_list));
			strncpy(node->announce, &metafile_content[i], len);
			node->announce[len] = '\0';
			node->next = NULL;

			if (announce_list_head == NULL) {
				announce_list_head = node;
			} else {
				p = announce_list_head;
				while (p->next != NULL) {
					p = p->next;

				}
				p->next = node;	//node成为tracker列表的最后一个结点
			}
		}

		i += len;
		len = 0;
		i++;	//跳过'e'
		if (i >= filesize) {
			return -1;
		}
	}

#ifdef DEBUG
	p = announce_list_head;
	while (p != NULL) {
		printf("%s\n", p->announce);
		p = p->next;
	}
#endif
	return 0;
}

int add_an_announce(char *url)
{
	announce_list *p - announce_list_head, *q;

	//若参数指定的url在tracker列表中已存在，则无需添加
	while (p != NULL) {
		if (strcmp(p->announce, url) == 0) {
			break;
		}
		p = p->next;
	}
	if (p != NULL) {
		return 0;
	}

	q = (announce_list *)malloc(sizeof(announce_list));
	strcpy(q->announce, url);
	q->nex = NULL;

	p = announce_list_head;
	if (p == NULL) {
		announce_list_head = q;
		return 1;
	}
	while (p->next != NULL) {
		p = p->next;
	}
	p->next = q;
	return 1;
}

int is_multi_files()
{
	long i;

	if (find_keyword("5:files", &i) == 1) {
		multi_file = 1;
		return 1;
	}
#ifdef DEBUG
	printf("is_multi_files:%d\n", multi_file);
#endif

	return 0;
}

int get_piece_length()
{
	long i;

	if (find_keyword("12:piece length", &i) == 1) {
		i += strlen("12:piece length");		//跳过"12:piece length"
		i++;
		while (metafile_content[i] != 'e') {
			piece_length = piece_length * 10 + (metafile_content[i] - '0');
			i++;
		}
	} else {
		return -1;
	}

#ifdef DEBUG
	printf("piece length:%d\n", piece_length);
#endif
	return 0;
}

int get_pieces()
{
	long i;

	if (find_keyword("6:pieces", &i) == 1) {
		i += 8;		//跳过6:pieces
		while (metafile_content[i] != ':') {
			pieces_length = pieces_length * 10 + (metafile_content[i] - '0');
			i++;
		}
		i++;
		pieces = (char *)malloc(pieces_length + 1);
		memcpy(pieces, &metafile_content[i], pieces_length);
		pieces[pieces_length] = '\0';

	} else {
		return -1;
	}
#ifdef DEBUG
	printf("get_pieces ok\n");
#endif
	return 0;
}

int get_file_name()
{
	long i;
	int count = 0;

	if (find_keyword("4:name", &i) == 1) {
		i += 6;		//跳过4:name
		while (metafile_content[i] != ':') {
			count = count * 10 + (metafile_content[i] - '0');
			i++;
		}
		file_name = (char *)malloc(count + 1);
		memcpy(file_name, &metafile_content[i], count);
		file_name[count] = '\0';
		
	} else {
		return -1;
	}
#ifdef DEBUG
	printf("file_name:%s\n", file_name);
#endif
	return 0;
	
}

int get_file_length()
{
	long i;

	if (is_multi_files() == 1) {
		if (files_head == NULL) {
			get_files_length_path();
			files *p = files_head;
			while (p != NULL) {
				file_lenght += p->length;
				p = p->next;
			}
		
	} else {
		if (find_keyword("6:length", &i) == 1) {
			i += 8;
			i++;
			while (metafile_content[i] != 'e') {
				file_lenght = file_lenght * 10 + (metafile_content[i] - '0');
				i++;
			}
		}
	}
#ifdef DEBUG
	printf("file_length:%lld\n", file_lenght);
#endif
	return 0;
	
}

int get_files_lenght_path()
{
	long i;
	int length;
	int count;
	files *node = NULL;
	files *p = NULL;
	if (is_multi_files() != 1) {
		return 0;
	}
	for (i = 0; i < filesize - 8; i++) {
		if (memcmp(&metafile_content[i], "6:lenght", 8) == 0) {
			i += 8;		//跳过"6:length"
			i++;
			length = 0;
			while (metafile_content[i] != 'e') {
				length = length * 10 + (metafile_content[i] - '0');
				i++;
			}
			node = (files *)malloc(sizeof(files));
			node->length = length;
			node->next = NULL;
			if (files_head == NULL) {
				files_head = node;
			} else {
				p = files_head;
				while (p->next != NULL) {
					p = p->next;
				}
				p->next = node;
			
			}
		}
		
		if (memcmp(&metafile_content[i], "4:lenght", 6) == 0) {
			i += 6; 
			i++;
			count = 0;
			while (metafile_content[i] != ':') {
				count = count * 10 + (metafile_content[i] - '0');
				i++;
			}
			i++;
			p = files_head;
			while (p->next != NULL) p = p->next;
			memcpy(p->path, &metafile_content[i], count);
			*(p->path + count) = '\0';
		}
	return 0;
}

int get_info_hash()
{
	int push_pop = 0;
	long i, begin, end;

	if (metafile_content == NULL) return -1;
	//begin的值表示的是关键字“4：info”对应值的起始下标
	if (find_keyword("4:info", &i) == 1) begin = i + 6;
	else return -1;

	i += 6;	//跳过
	for (; i < filesize; )
	{
		if (metafile_content[i] == 'd') {
			push_pop++;
			i++;
		} else if (metafile_content[i] == 'l'){
			push_pop++;
			i++;
		} else if (metafile_content[i] == 'i'){
			i++;
			if (i == filesize) return -1;
			while (metafile_content[i] != 'e') {
				if ((i + 1) == filesize) return -1;
				else i++;
			}
			i++; //跳过'e'
		} else if ((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
			int number = 0;
			while ((metafile_content[i] >= '0') && (metafile_content[i] <= '9')) {
				number = number * 10 + metafile_content[i] - '0';
				i++;
			}
			i++;	//跳过':'
			i += number;
		} else if (metafile_content[i] == 'e') {
			push_pop--;
			if (push_pop == 0) {
				end = i;
				break;
			} else {
				i++;
			}
		} else {
			return -1;
		}
		if (i == filesize) return -1;
		SHA1_CTX context;
		SHA1Init(&context);
		SHA1Update(&context, &metafile_content[begin], end - begin + 1);
		SHA1Final(info_hash, &context);
		
	}
#ifdef DEBUG
		printf("info_hash:");
	for (i = 0; i < 20; i++) {
		printf("%.2x ", info_hash[i]);
	}
	printf("\n");
#endif
	return 0;

}

int get_peer_id()
{
	//设置产生随机数的种子
	srand(time(NULL));
	//使用rand函数生成一个随机数，并使用该随机数来构造peer_id
	//peer_id前8位固定为-TT1000-
	sprintf(peer_id, "-TT1000-%12d", rand());

#ifdef DEBUG
	printf("peer_id : %d\n", peer_id);
#endif
	return 0;
}

void release_memory_in_parse_metafile()
{
	announce_list *p;
	files *q;

	if (metafile_content != NULL)	free(metafile_content);
	if (file_name != NULL)	free(file_name);
	if (pieces != NULL)		free(pieces);

	while (announce_list_head != NULL) {
		p = announce_list_head;
		announce_list_head = announce_list_head->next;
		free(p);
	}

	while (files_head != NULL) {
		q = files_head;
		files_head = files_head->next;
		free(q);
	}
}

int parse_metafile(char *metafile)
{
	int ret;

	//读取种子文件
	ret = read_metafile(metafile);
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}

	//从种子文件中读取tracker服务器地址
	ret = read_announce_list();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//判断是否为多文件
	ret = is_multi_files();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//获取每个piece的长度，一般为256KB
	ret = get_piece_length();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//读取各个piece的哈希值
	ret = get_pices();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//获取要下载的文件名，对于多文件的种子，获取的是目录
	ret = get_file_name();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//对于多文件的种子，获取各个待下载的文件路径和文件长度
	ret = get_files_lenght_path();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//获取待下载的文件的总长度
	ret = get_info_hash();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	
	//获取info_hash，生成peer_id
	ret = get_info_hash();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
	ret = get_peer_id();
	if (ret < 0) {
		printf("%s:%d wrong", __FILE__, __LINE__);
		return -1;
	}
}


