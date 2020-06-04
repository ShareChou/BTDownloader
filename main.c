#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include "parse_metafile.h"
#include "signal_hander.h"
#include "bitfiel.h"
#include "data.h"
#include "policy.h"
#include "tracker.h"
#include "torrent.h"
#include "log.h"

#define DEBUG

int main(int argc, char *argv[])
{
	int ret;

	if (argc != 2) {
		printf("usage:%s metafile\n", argv[0]);
		exit(-1);
	}
	
	ret = set_signa_hander();
	if (ret != 0) {
		printf("%s:%d error\n", __FILE__, __LINE__);
		return -1;
	}
	
	ret = parse_metafile(argv[1]);
	if (ret != 0) {
		printf("%s:%d error\n", __FILE__, __LINE__);
		return -1;
	}
	
	ret = create_files();
	if (ret != 0) {
		printf("%s:%d error\n", __FILE__, __LINE__);
		return -1;
	}
	
	ret = create_bitfield();
	if (ret != 0) {
		printf("%s:%d error\n", __FILE__, __LINE__);
		return -1;
	}

	ret = create_btcache();
	if (ret != 0) {
		printf("%s:%d error\n", __FILE__, __LINE__);
		return -1;
	}

	init_unchoke_peers();

	download_upload_with_peers();

	do_clear_work();
	return 0;
}