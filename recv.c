#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <string.h>
#include "camera_example/main/packet.h"

char errbuf[PCAP_ERRBUF_SIZE];

FILE *output_file;
uint8_t target_mac[] = {0x67, 0x41, 0x67, 0x41, 0x67, 0x41};
int prev_seq, prev_frag, prev_size, prev_buf_size;
char packet_valid = 0;
#define FRAG_SIZE (DATA_SIZE - 4)
uint8_t *buf, *buf_ptr, *prev_buf;
int frag_count;

#define LOG_MODE_HUMAN

void packet_handler(unsigned char *args, const struct pcap_pkthdr *header, const unsigned char *packet_ptr) {
	packet_t *packet = (packet_t*) (packet_ptr + 18);
	if (!strncmp(target_mac, packet->packet_header.header.dest_addr, sizeof(target_mac))) {
		int cur_seq = packet->packet_header.header.seq_ctrl.sequence;
		int cur_frag = packet->packet_header.header.seq_ctrl.fragment;
		int *cur_size = (int*)packet->data;
		uint8_t *image = packet->data + 4;

		if (cur_seq != prev_seq) {
			if (packet_valid) {
				if (buf_ptr - buf >= prev_size) {
					prev_buf_size = prev_size;
					fwrite(buf, sizeof(uint8_t), prev_size, output_file);
					memcpy(prev_buf, buf, prev_buf_size);
#if defined(LOG_MODE_HUMAN)
					fprintf(stderr, "%d %d %d\n", prev_seq, buf_ptr - buf, prev_size);
				} else {
					fprintf(stderr, "%d short frame (%d/%d)\n", prev_seq, prev_frag, (prev_size + FRAG_SIZE - 1)/FRAG_SIZE);
#elif defined(LOG_MODE_CSV)
					fprintf(stderr, "%d, 1\n", prev_seq);
				} else {
					fprintf(stderr, "%d, %lf\n", prev_seq, (double)prev_frag/((prev_size + FRAG_SIZE - 1)/FRAG_SIZE));
#else
				} else {
#endif
					fwrite(prev_buf, sizeof(uint8_t), prev_buf_size, output_file);
				}
			} else {
#if defined(LOG_MODE_HUMAN)
				fprintf(stderr, "%d discontinuous frame (%d/%d)\n", cur_seq, frag_count, (prev_size + FRAG_SIZE - 1)/FRAG_SIZE);
#elif defined(LOG_MODE_CSV)
				fprintf(stderr, "%d, %lf\n", cur_seq, (double)frag_count/((prev_size + FRAG_SIZE - 1)/FRAG_SIZE));
#endif
				fwrite(prev_buf, sizeof(uint8_t), prev_buf_size, output_file);
			}
			prev_frag = -1;
			packet_valid = 1;
			buf_ptr = buf;
			frag_count = 0;
		}

		if (prev_frag + 1 != cur_frag) {
			packet_valid = 0;
		} else {
			memcpy(buf_ptr, image, FRAG_SIZE);
			buf_ptr += FRAG_SIZE;
		}
		frag_count++;

		prev_seq = cur_seq;
		prev_frag = cur_frag;
		prev_size = *cur_size;
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		return -1;
	}
//	output_file = fopen("output", "w");
	output_file = stdout;
	pcap_t *handle = pcap_open_live(
		argv[1],
		BUFSIZ,
		1,
		0,
		errbuf
	);

	if (handle == NULL) {
		fprintf(stderr, "Error opening device: %s\n", errbuf);
		return 1;
	}

	buf = malloc(8e6);
	prev_buf = malloc(8e6);
	buf_ptr = buf;

	pcap_set_immediate_mode(handle, 1);
	pcap_loop(handle, 0, packet_handler, NULL);
	pcap_close(handle);
	return 0;
}

