#include <stdint.h>
#include <arpa/inet.h>

extern void _payload(void);

uint16_t port = 0;
uint32_t addr_ip = 0;

int main()
{
	port = htons(3002);
	addr_ip = htonl(INADDR_ANY);
	_payload();

	return 0;
}
