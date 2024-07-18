#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int arg, char **av)
{
	struct stat stats;
	int fd = open(av[0], O_RDONLY);
	fstat(fd, &stats);

	void *me = mmap(NULL, stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	unlink(av[0]);
	sleep(5);
	close(fd);
	fd = open(av[0], O_CREAT | O_WRONLY, 0755);
	write(fd, (char *)me, stats.st_size);
}