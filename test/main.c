#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <elf.h>

#ifdef __APPLE__
#define CDECL_NORM(x) x
#else
#define CDECL_NORM(x) _ ## x
#endif /* __APPLE__ */

extern uint8_t CDECL_NORM(payload);
extern uint64_t CDECL_NORM(payload_size);

#define payload_p &CDECL_NORM(payload)
#define payload_size_p CDECL_NORM(payload_size)

uint16_t port = 0;
uint32_t addr_ip = 0;

int main(int argc, char *argv[], char *envp[]) {
    port = htons(3002);
    addr_ip = htonl(INADDR_ANY);
    printf("port: %#x %#x\n", port, addr_ip);


	int fd = open("testprog", O_RDWR);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	struct stat st;
	if (fstat(fd, &st) == -1) {
		perror("fstat");
		return 1;
	}

	void *map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;
	Elf64_Phdr *phdr = (Elf64_Phdr *)(map + ehdr->e_phoff);

	int last_phdr = -1;

	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == PT_LOAD) {
			printf("found PT_LOAD\n");
			phdr[i].p_flags = PF_R | PF_W | PF_X;
			last_phdr = i;
		}
	}

	if (last_phdr == -1) {
		fprintf(stderr, "no PT_LOAD found\n");
		return 1;
	}

	Elf64_Phdr *last_phdr_p = &phdr[last_phdr];

	int last_section = -1;

	for (int j = 1; j < elf->e_shnum; j++)
	{
		// find the last section in the last segment
		if (shdr[j].sh_addr >= last_phdr_p->p_vaddr &&
			shdr[j].sh_addr < last_phdr_p->p_vaddr + last_phdr_p->p_memsz)
		{
			last_section = j;
		}
	}

	printf("last_phdr: %d\n", last_phdr);
	printf("last_section: %d\n", last_section);

	munmap(map, st.st_size);
	close(fd);

    return 0;
}
