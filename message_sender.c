
#include <errno.h>
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>       
#include <unistd.h>   
#include "message_slot.h"

int main(int argc, char *argv[])
{
	int fd, length, num_written;

	if (argc != 4) {
		fprintf(stderr, "not enough/too many arguments\n");
		exit(1);
	}
	fd = open( argv[1], O_WRONLY );
	if( fd < 0)
	{
		printf("ERROR: open failed. %s\n", strerror(errno));
		exit(1);
	}

	if (ioctl( fd, MSG_SLOT_CHANNEL, atoi(argv[2])) != 0) 
	{
		perror("ioctl failed");
		exit(1);
	}

	num_written = write(fd,argv[3], strlen(argv[3]));
	if (num_written < 0) 
	{
	    	printf("ERROR: write failed. %s\n", strerror(errno));
	    	close(fd);
		exit(1);
    	}

	length = strlen(argv[3]);
	if (write( fd, argv[3], length) != length) {
		perror("write failed");
		exit(1);
	}

	close(fd); 
	return 0;
}
