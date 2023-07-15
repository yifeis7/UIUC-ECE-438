#define WINDOW_SIZE 256
#define DATA_SIZE 1400

struct seg {
	int32_t len;
	int32_t seq;
	int32_t eot; // indicate the end of transfer
	char data[DATA_SIZE];
};

struct ack {
	int32_t seq;
};

