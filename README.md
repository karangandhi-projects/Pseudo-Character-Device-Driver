# Description
- A basic Pseudo Character Device Driver with 4 devices that support read, write, open, release and lseek operations.
- Linux - ubuntu
- Kernel - 6.11.0-19-generic
- Cross compiler - gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabih
  
# Init
- Please edit the make file based on your system. when in the repo use below command to remove all the make files
- $ make clean
- Once done you can use $ make host
- Then use $ sudo insmod <filename>.ko in this case its pcd_n.ko
- You can check the devices using $ ls -l /dev/pcdev- + tab
  
# Operations
- In this case the first device is RDONLY, second device is WRONLY, third and fourth devices are RDWR
- you can use various different commands to test the supported operations
- the ones i used:
- Read - cat /dev/pcdev-3 , dd if=/dev/pcdev-3 of=file.txt count=1 bs=100
- Write - echo, dd if=pcd_n.c of=/dev/pcdev-3 count=1 bs=100
- Lseek - used the dev_read.c to check the SEEK_SET, SEEK_CUR, SEEK_END

# Credits
- This project was part of Fastbit Embedded Brain Academy course on Udemy
