#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h> // Include errno header
#include <wiringPi.h>

#define DEVICE_PATH "/dev/tcs34725"
// IOCTL commands
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_CLEAR _IOR(TCS34725_IOCTL_MAGIC, 1, int)
#define TCS34725_IOCTL_RED _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
#define TCS34725_IOCTL_BLUE _IOR(TCS34725_IOCTL_MAGIC, 4, int)

int main(void){
	int fd;
	uint16_t data[4];
	uint8_t rgb[3];

	// Open the device
	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
	   perror("Failed to open the device");
	   return errno;
	}

	while(1){
	// Read clear light data
    if (ioctl(fd, TCS34725_IOCTL_CLEAR, &data[0]) < 0) {
        perror("Failed to read clear light data");
        close(fd);
        return errno;
    }

    if(data[0]==0) data[0]=1;

    // Read red data
    if (ioctl(fd, TCS34725_IOCTL_RED, &data[1]) < 0) {
        perror("Failed to read red data");
        close(fd);
        return errno;
    }

    // Read green data
    if (ioctl(fd, TCS34725_IOCTL_GREEN, &data[2]) < 0) {
        perror("Failed to read green data");
        close(fd);
        return errno;
    }

    // Read blue data
    if (ioctl(fd, TCS34725_IOCTL_BLUE, &data[3]) < 0) {
       perror("Failed to read blue data");
       close(fd);
       return errno;
    }

	rgb[0]=(data[1]*255)/data[0];
	rgb[1]=(data[2]*255)/data[0];
	rgb[2]=(data[3]*255)/data[0];
	
	printf("Red: %u\n", rgb[0]);
	printf("Green: %u\n", rgb[1]);
	printf("Blue: %u\n", rgb[2]);
	delay(5000);
	}
	
	return 0;
}