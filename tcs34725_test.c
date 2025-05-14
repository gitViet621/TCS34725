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
#define TCS34725_IOCTL_RGBC_DATA _IOR(TCS34725_IOCTL_MAGIC, 1, struct tcs34725_color)

struct tcs34725_color{
    uint16_t clear;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};

int main(void){
	int fd;
    struct tcs34725_color color;

	// Open the device
	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
	   perror("Failed to open the device");
	   return errno;
	}

	while(1){
        //read color
        if (ioctl(fd, TCS34725_IOCTL_RGBC_DATA, &color) < 0) {
            perror("Failed to read clear light data");
            close(fd);
            return errno;
        }
        
        if (color.clear == 0) {
            color.clear=1;
        }

        color.red = (color.red*255)/color.clear;
        color.green = (color.green*255)/color.clear;
        color.blue = (color.blue*255)/color.clear;
        
        printf("Red: %u\n", color.red);
        printf("Green: %u\n", color.green);
        printf("Blue: %u\n", color.blue);
        delay(3000);
	}
	
	return 0;
}
