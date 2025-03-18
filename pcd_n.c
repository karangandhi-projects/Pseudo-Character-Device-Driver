#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

#define NO_OF_DEVICES 4

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

/* pseudo device's memory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];


/* Device priavte data structure */
struct pcdev_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/* Driver private data structure */
struct pcdrv_private_data
{
	int total_devices;
	/* this holds the device number */
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data  pcdev_data[NO_OF_DEVICES];
};

#define RDONLY 0X01
#define WRONLY 0X10
#define RDWR   0X11

struct pcdrv_private_data pcdrv_data = 
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {

		[0] = {
			.buffer = device_buffer_pcdev1,
			.size 	= MEM_SIZE_MAX_PCDEV1,
		       	.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY /* RDONLY */
		},

		[1] = {
                        .buffer = device_buffer_pcdev2,
                        .size   = MEM_SIZE_MAX_PCDEV2,
                        .serial_number = "PCDEV2XYZ123",
                        .perm = WRONLY /* WRONLY */
		},

		[2] = {
                        .buffer = device_buffer_pcdev3,
                        .size   = MEM_SIZE_MAX_PCDEV3,
                        .serial_number = "PCDEV3XYZ123",
                        .perm = RDWR /* RDWR */
                },

		[3] = {
                        .buffer = device_buffer_pcdev4,
                        .size   = MEM_SIZE_MAX_PCDEV4,
                        .serial_number = "PCDEV4XYZ123",
                        .perm = RDWR /* RDWR */
                }
	}	
};

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *filp);
int check_permission(int dev_perm, int access_mode);


loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	
	int max_size = pcdev_data->size;
	
	loff_t temp;

	pr_info("lseek requested \n");
	pr_info("Current value of file postion = %lld \n", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if( (offset > max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if( (temp > max_size) || (temp < 0) )
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:	
			temp = max_size + offset;
			if( (temp > max_size) || (temp < 0) )
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return - EINVAL;
	}

	pr_info("New value of the file postion = %lld \n ", filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;

	pr_info("read requested for %zu bytes \n",count);
	pr_info("current file position = %lld",*f_pos);

	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;	 	

	/* copy to user */
	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count)){
		return -EFAULT;
	}

	/* update the current file position */
	*f_pos += count;

	pr_info("Number of bytes successfully read: %zu \n", count);
	pr_info("Updated file position  = %lld \n",*f_pos);
	
	/* return number of bytes which have been successfully read*/
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;

	pr_info("write requested for %zu bytes \n",count);
	pr_info("current file position = %lld",*f_pos);

	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}	

	/* copy from user */
	if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count)){
		return -EFAULT;
	}

	/* update the current file position */
	*f_pos += count;

	pr_info("Number of bytes successfully written: %zu \n", count);
	pr_info("Updated file position  = %lld \n",*f_pos);
	
	/* return number of bytes which have been successfully written */
	return count;
}

int check_permission(int dev_perm, int access_mode)
{
	if(dev_perm == RDWR)
		return 0;

	/* ensures readonly access*/
	if ((dev_perm == RDONLY) & ((access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE)))
		return 0;
	
	/* ensures writeonly access */	
	if ((dev_perm == WRONLY) & ((access_mode & FMODE_WRITE) && !(access_mode & FMODE_READ)))
		return 0;

	return -EPERM;

}

int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	
	int minor_n;

	struct pcdev_private_data *pcdev_data;

	/* find out ton which device file open was attempted by the user */

	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n", minor_n);

	/* get devices's private data struture */
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	
	/* to supply device private data to other methods of the driver */
	filp->private_data = pcdev_data;

	/* check permission */
	pr_info(" perm = %d , access_mode = %d ", pcdev_data->perm,filp->f_mode);
	ret = check_permission(pcdev_data->perm,filp->f_mode);

	(!ret)?pr_info("open was successful \n"):pr_info("open was unsucessful \n");
	return ret;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("close was successful \n");
	return 0;
}

/* file operations of the driver */
struct file_operations pcd_fops = 
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};


static int __init pcd_driver_init(void)
{
	int ret;
	int i;

	/* Dynamically allocate a device number */
	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,NO_OF_DEVICES,"pcd_devices");
	if(ret < 0)
	{
		pr_err("Alloc chrdev failed \n");
		goto out;
	}
	
	/* create device class under /sys/class/ */
	pcdrv_data.class_pcd = class_create("pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed \n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}
	

	for(i=0;i<NO_OF_DEVICES;i++)
	{
		pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));
		
		/* Initialize the cdev structure with fops */
		cdev_init(&pcdrv_data.pcdev_data[i].cdev,&pcd_fops);
		
		/* Register cdev structure with the VFS */
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);
		if(ret < 0)
		{
			pr_err("Cdev add failed \n");
			goto cdev_del;
		}

		/* populate the sysfs with device information*/
		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev-%d",i+1);
		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_err("Device create failed \n");
			ret = PTR_ERR(pcdrv_data.device_pcd);
			goto class_del;
		}
		pr_info("Device creation complete \n");
	}
	pr_info("Out of for loop \n");

	pr_info("Module init was successful \n ");

	pr_info(" Right before return \n");

	return 0;

cdev_del:
class_del:
	for(;i>=0;i--)
	{
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
	pr_info("Module Insertion failed \n");
	return ret;
}



static void __exit pcd_driver_cleanup(void)
{
	int i;

	for(i=0;i<NO_OF_DEVICES;i++)
	{
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
	
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);

	pr_info("module unloaded \n");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KARAN");
MODULE_DESCRIPTION("A pseudo character driver which handles n devices");
