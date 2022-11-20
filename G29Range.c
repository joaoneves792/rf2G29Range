//#include "PluginObjects.h"
//#include "InternalsPlugin.h"

#include <fcntl.h>
#include <unistd.h>
#include <windef.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
/*libudev*/
#include <libudev.h>

#include <unistd.h>
#include <fcntl.h>
#define O_RDWR		     02
#define O_NONBLOCK	  04000

#include <inttypes.h>

#define SUBSYSTEM "hidraw"
#define MAX_DEV_PATH_CHARS 1024

#define VENDOR "046d"
#define PRODUCT "c24f"

int g_wheelAngle = 0;



int find_g29_device(struct udev* udev, char* device, int c){
  int found = 0;
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_subsystem(enumerate, SUBSYSTEM);
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry* entry;

  udev_list_entry_foreach(entry, devices) {
      const char* path = udev_list_entry_get_name(entry);
      struct udev_device* dev = udev_device_new_from_syspath(udev, path);
      if (dev) {
        path = udev_device_get_devnode(dev);
        if (path){
           dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
           if(!dev){
             udev_device_unref(dev);
             continue;
           }
           const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
           const  char* product = udev_device_get_sysattr_value(dev, "idProduct");
           if(!strncmp(vendor, VENDOR, 4) && !strncmp(product, PRODUCT, 4)){
              printf("Found G29 Wheel\n");
              found = 1;
              strncpy(device, path, c);
              udev_device_unref(dev);
              goto end;
           }
        }
        udev_device_unref(dev);
      }
  }
end:
  udev_enumerate_unref(enumerate);
  return found;
}

void set_range(uint16_t range){

	int fd;
	int res = 0;
	char buf[256];
	char device[MAX_DEV_PATH_CHARS];

  struct udev* udev = udev_new();
  if (!udev) {
    fprintf(stderr, "udev_new() failed\n");
    return;
  }

  if(!find_g29_device(udev, device, MAX_DEV_PATH_CHARS)){
    udev_unref(udev);
    return;
  }
  udev_unref(udev);

	printf("%s\n", device);
  
  fd = open(device, O_RDWR | O_NONBLOCK);

	if (fd < 0) {
		perror("Unable to open device");
		return; 
	}

	memset(buf, 0x0, sizeof(buf));


	/* Get Raw Name */
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWNAME");
	else
		printf("Raw Name: %s\n", buf);
  
  uint8_t cmd[7];
  
  printf("Setting range to %d\n", range);
  
  cmd[0] = 0xf8;
  cmd[1] = 0x81;
  cmd[2] = range & 0x00ff;
  cmd[3] = (range & 0xff00) >> 8;
  cmd[4] = 0x00;
  cmd[5] = 0x00;
  cmd[6] = 0x00;
	
  res = write(fd, cmd, 7);
	if (res < 0) {
		printf("Error: %d\n", errno);
		perror("write");
	} else {
		printf("write() wrote %d bytes\n", res);
	}

}


void WINAPI ProxyEnterRealtime(){
  g_wheelAngle = 0;
}

void WINAPI ProxyExitRealtime(){
  g_wheelAngle = 0;
}

void WINAPI ProxyUpdateTelemetry(void *info){
  if(!g_wheelAngle){
    /*
    Offset calculated with:
    offsetof(TelemInfoV01, mPhysicalSteeringWheelRange) 
    692
    */
#define OFFSET 692
#define G29_MAX_RANGE 900
#define G29_MIN_RANGE 40

    if(info == NULL){
      return;
    }

    uint8_t* infoBytes = (uint8_t*) info;
    float rangeInfo = (float) (*(float*) (infoBytes+OFFSET));
    uint16_t range = (uint16_t) rangeInfo;
    if( range >= G29_MIN_RANGE && range <= G29_MAX_RANGE){  
      set_range(range);
      g_wheelAngle = 1;
    }
    
  }

}
