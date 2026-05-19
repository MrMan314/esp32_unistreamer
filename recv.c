#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <string.h>
#include <packet.h>

char errbuf[PCAP_ERRBUF_SIZE];

FILE *output_file;
uint8_t target_mac[] = {0x67, 0x41, 0x67, 0x41, 0x67, 0x41};
int prev_seq, prev_frag, prev_size, prev_buf_size;
char packet_valid = 0;
uint8_t *buf, *buf_ptr, *prev_buf;
int frag_count;

#define LOG_MODE_HUMAN

#ifdef LOG_MODE_HUMAN
#define LOG_ERROR_RATE(i, recv, total) fprintf(stderr, "%d broken (%d/%d)\n", i, recv, total);
#define LOG_SUCCESS(i, recvsize, realsize) fprintf(stderr, "%d %d %d\n", i, recvsize, realsize);
#elifdef LOG_MODE_CSV
#define LOG_ERROR_RATE(i, recv, total) fprintf(stderr, "%d, %lf\n", i, (double)(recv)/(total));
#define LOG_SUCCESS(i, recvsize, realsize) fprintf(stderr, "%d, 1\n", i);
#else
#define LOG_ERROR_RATE(i, recv, total)
#define LOG_SUCCESS(i, recvsize, realsize)
#endif

void packet_handler(unsigned char *args, const struct pcap_pkthdr *header, const unsigned char *packet_ptr) {
	packet_t *packet = (packet_t*) (packet_ptr + 0x15);
	if (!strncmp(target_mac, packet->packet_header.header.dest_addr, sizeof(target_mac))) {
		int cur_seq = packet->packet_header.header.seq_ctrl.sequence;
		int cur_frag = packet->packet_header.header.seq_ctrl.fragment;
		int cur_size = packet->frame_len;
		uint8_t *image = packet->data;

		if (cur_seq != prev_seq) {
			if (packet_valid) {
				if (buf_ptr - buf >= prev_size) {
					prev_buf_size = prev_size;
					fwrite(buf, sizeof(uint8_t), prev_size, output_file);
					memcpy(prev_buf, buf, prev_buf_size);
					LOG_SUCCESS(prev_seq, buf_ptr - buf, prev_size);
				} else {
					LOG_ERROR_RATE(prev_seq, frag_count, (prev_size + DATA_SIZE - 1)/DATA_SIZE);
					fwrite(prev_buf, sizeof(uint8_t), prev_buf_size, output_file);
				}
			} else {
				LOG_ERROR_RATE(prev_seq, frag_count, (prev_size + DATA_SIZE - 1)/DATA_SIZE);
				fwrite(prev_buf, sizeof(uint8_t), prev_buf_size, output_file);
			}
			prev_frag = -1;
			packet_valid = 1;
			buf_ptr = buf;
			frag_count = -1;
		}

		// juuuuust in case...
		if ((prev_frag + 1)%256 != (cur_frag)%256) {
			packet_valid = 0;
			//fprintf(stderr, "%d jump %d-%d\n", cur_seq, prev_frag, cur_frag);
			frag_count++;
		} else {
			memcpy(buf_ptr, image, DATA_SIZE);
			buf_ptr += DATA_SIZE;
		}
		frag_count++;

		prev_seq = cur_seq;
		prev_frag = cur_frag;
		prev_size = cur_size;
	} /*else {
		for (int i = 0; i < 64; i++) {
			printf(i % 16 ? "%02x " : "\n%02x ", packet_ptr[i + 0x15]);
		}

		putchar(10);
	}*/
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

