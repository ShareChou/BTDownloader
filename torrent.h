#ifndef TORRENT_H
#define TORRENT_H
#include "tracker.h"

int download_upload_with_peers();

int print_peer_list();
void print_process_info();
void clear_connect_tracker();
void clear_connect_peer();
void clear_tracker_response();
void release_memory_in_torrent();
#endif