#include "includes/famine.h"

void remove_shm(void)
{
	shmctl(shmget(FM_SHM_KEY, sizeof(int), 0666), IPC_RMID, NULL);
}

int main(void)
{
	remove_shm();

	char *const args[] = {
		"pkill",
		"-9",
		"famine",
		NULL
	};

	execve("/bin/pkill", args, NULL);
}