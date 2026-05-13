#include <stdint.h>

typedef struct __attribute__((packed)) {
	uint16_t protocol_version	: 2;
	uint16_t type			: 2;
	uint16_t subtype		: 4;
	uint16_t to_ds			: 1;
	uint16_t from_ds		: 1;
	uint16_t more_frag		: 1;
	uint16_t retry			: 1;
	uint16_t power_mgmt		: 1;
	uint16_t more_data		: 1;
	uint16_t protected_frame	: 1;
	uint16_t order			: 1;
} frame_control_t;

// this however, is actually not the case (it is actually 4,12)
typedef struct __attribute__((packed)) {
	uint16_t fragment		: 8;
	uint16_t sequence		: 8;
} seq_ctrl_t;

typedef struct __attribute__((packed)) {
	frame_control_t frame_control;
	uint16_t duration_id;
	uint8_t dest_addr[6];
	uint8_t src_addr[6];
	uint8_t bssid[6];
	seq_ctrl_t seq_ctrl;
} dot11_header_t;

typedef struct __attribute__((packed)) {
	uint8_t dsap;
	uint8_t ssap;
	uint8_t control;
} llc_header_t;

typedef struct __attribute__((packed)) {
	uint8_t dsap;
	uint8_t ssap;
	uint8_t control;
	uint8_t oui[3];
	uint16_t ethertype;
} llc_snap_header_t;

typedef struct __attribute__((packed)) {
	dot11_header_t header;
	llc_snap_header_t snap_header;
} packet_header_t;

#define DATA_SIZE (2328-sizeof(packet_header_t) - 4)

typedef struct __attribute__((packed)) {
	packet_header_t packet_header;
	int frame_len;
	uint8_t data[DATA_SIZE];
} packet_t;
