#include "message_slot.h"


#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_SLOT_CHANNEL _IOW(240, 0, unsigned int)

int main(int argc, char *argv[])
{
	int fd, ret_val;
	char buffer[128];

	if (argc != 3) {
		fprintf(stderr, "not enough/too many arguments\n");
		exit(1);
	}

	fd = open( argv[1], O_RDONLY );
	if( fd < 0 ) {
		perror("failed opening");
		exit(1);
	}


	if (ioctl( fd, MSG_SLOT_CHANNEL, atoi(argv[2])) != 0) 
	{
		perror("failed ioctl");
		exit(1);
    	}
	ret_val = read( fd, buffer, 128);
	if ( ret_val < 0) {
		perror("read");
		exit(1);
	}
	 else 
	{
		if (write(STDOUT_FILENO, buffer, ret_val) == -1) {
			perror("write to console");
		}
	}

    close(fd); 
    return 0;
}

