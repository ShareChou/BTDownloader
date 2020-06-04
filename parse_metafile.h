#ifndef PARSE_METAFILE
#define PARSE_METAFILE

//保存从种子文件中获取的tracker的url
typedef struct _announce_list {
	char	announce[128];
	struct	_announce_list *next;
} announce_list;

//保存各个待下载文件的路径和长度
typedef struct _files {
	char	path[256];
	long	length;
	struct _files *next;
} files;

int read_metafile(char *metafile_name);			//读取种子文件
int find_keyword(char *keyword, longg *position);	//在种子文件中查找某个关键词
int read_announce_list();	//获取各个tracker服务器的地址
int add_an_announce(char *url);		//向tracker列表添加一个url

int get_piece_length();		//获取每个piece的长度，一般为256kb
int get_pices();			//读取各个piece的哈希值

int is_multi_files();		//判断下载的是单个文件还是多个文件
int get_file_name();		//获取文件名， 对于过文件，获取的是目录名
int get_file_lenght();		//获取待下载文件的总长度
int get_files_lenght_path();	//获取文件的路径和长度，对多文件种子有效

int get_info_hash();		//由info关键词对应的值计算info_hash
int get_peer_id();			//生成peer_id，每个peer都有一个20字节的peer_id

void release_memory_in_parse_metafile();	//释放parse_metafile.c中动态分配的内存
int parse_metafile(char *metafile);		//调用本文件中定义的函数，完成解析种子文件

#endif
