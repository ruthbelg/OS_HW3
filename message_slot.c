#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include "message_slot.h"
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/ioctl.h>
#include <linux/kdev_t.h>


#include <linux/slab.h>
MODULE_LICENSE("GPL");

//for the device_open
struct info{
	int minor;
	unsigned long int channelId;
};


//a struct for the message
struct msg{
	char buff[128]; //max of 128 bytes
	int length; //the actual length of the message
};

//a struct for the minors array
struct slot
{
	struct msg* message;
	struct info* info;//information of the slot about it's minor & channel
	struct slot* next;
};

//array of 256 linked lists - each linked list represents a slot
static struct slot** minors;

static int device_open(struct inode* inode, struct file* file)
{
	unsigned long int minor = iminor(inode);//MINOR(inode->i_rdev);//iminor(inode);
	struct info* infoStruct;
	//struct slot* newSlot = kmalloc(sizeof(struct slot), GFP_KERNEL);

	infoStruct = kmalloc(sizeof(struct info), GFP_KERNEL);//allocating kernel memory for the file 
	if(!infoStruct)
	{
		printk(KERN_ERR "error allocating kernel memory\n");
		return -1;//returns -1 if failed
	}
	printk("updating data\n");

	infoStruct->channelId = 0;
	infoStruct->minor = minor;
	file->private_data = (void*)infoStruct;//assocoating the FD was invoked on - for the ioctl
	
	
	printk("im here\n");
	//if no one ever opened this minor number
	if(minors[minor]->next == NULL)
	{
		minors[minor]->next = kmalloc(sizeof(struct slot), GFP_KERNEL);
		minors[minor]->next->next = NULL;

		printk("1\n");
		//allocating memory for the new node
		minors[minor]->next->message = kmalloc(sizeof(struct msg), GFP_KERNEL);
		//minors[minor]->next->message = newSlot->message;

		printk("1\n");
		minors[minor]->next->info = kmalloc(sizeof(struct info), GFP_KERNEL);
		printk("1\n");
		minors[minor]->next->info = infoStruct;

		//newSlot->message->length = 0;
		printk("1\n");
		//newSlot->info = infoStruct;
		//minors[minor]->next = newSlot;

	}
	return 0;//returns 0 on success
}


static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{	
	if(ioctl_param == 0 || ioctl_command_id != MSG_SLOT_CHANNEL)
		return -EINVAL;

	((struct info*)file->private_data)->channelId = ioctl_param;
	return 0;
}


static int device_release(struct inode* inode, struct file* file)
{
	//releasing the struct from the private data
	kfree((struct info*)file->private_data);
	return 0;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
	int minor = ((struct info*)file->private_data)->minor; //=get iminor...
	unsigned long int channel = ((struct info*)file->private_data)->channelId;
	int i = 0;
	struct msg* msg;// = minors[minor].message;
	struct slot* slot = minors[minor];//should write the message of this minor
	struct slot* tmp = slot;
	int found = 0;
	
	if(((struct info*)file->private_data)->channelId == 0)//no channel has been set to the FD
		return -EINVAL;

	if(length > 128 || length == 0)
		return -EMSGSIZE;

	//getting the slot and adding a message
	while(tmp->next != NULL && !found)
	{
		if(tmp->info->channelId == channel)
			found = 1;
		tmp = tmp->next;
	}
	if(!found)//no channel 
	{
		msg = kmalloc(sizeof(struct msg), GFP_KERNEL);
		msg->length = 0;

		//iserting the message
		tmp->next = kmalloc(sizeof(struct slot), GFP_KERNEL);
		tmp->next->info = kmalloc(sizeof(struct info), GFP_KERNEL);
		tmp->next->message = kmalloc(sizeof(struct msg), GFP_KERNEL);
		tmp->next->info = ((struct info*)file->private_data);
		tmp->next->message = msg;
		tmp->next->next = NULL;
	}
	else
		msg = tmp->message;
	
	printk("jhvgvuvujhbhvuvjhvhgjhbjnnjhbjn\n");
	for(i = 0; i < length && i < 128; i++)
		get_user(msg->buff[i], &buffer[i]);//writting from the user space

	//the number of written characters
	msg->length = i;
	return i;
}

static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
	int minor = ((struct info*)file->private_data)->minor; //=get iminor...
	unsigned long int channel = ((struct info*)file->private_data)->channelId;
	int i = 0;
	struct msg* msg;// = minors[minor].message;
	struct slot* slot = minors[minor];//should write the message of this minor
	struct slot* tmp = slot;
	int found = 0;

	if(((struct info*)file->private_data)->channelId == 0)//no channel has been set to the FD
		return -EINVAL;

	if(buffer == NULL)
		return -EWOULDBLOCK;

	//getting the relevant slot 
	while(tmp->next != NULL && !found)
	{
		if(tmp->info->channelId == channel)
			found = 1;
		tmp = tmp->next;
	}
	if(!found)//no channel 
	{
		msg = kmalloc(sizeof(struct msg), GFP_KERNEL);
		msg->length = 0;

		//iserting the message
		tmp->next = kmalloc(sizeof(struct slot), GFP_KERNEL);

		tmp->next->info = ((struct info*)file->private_data);
		tmp->next->message = msg;
		tmp->next->next = NULL;
	}
	else
		msg = tmp->message;

	//if the buffer isn't large enough
	if (length < msg->length)
		 return -ENOSPC;

	for(i = 0; i < msg->length && i < 128; i++)
		put_user(msg->buff[i], &buffer[i]);

	msg->length = i;
	return i;
}

static struct file_operations _fops =
{
	.owner = THIS_MODULE, 
	.read = device_read,
  	.write = device_write,
  	.open = device_open,
  	.unlocked_ioctl = device_ioctl,
  	.release = device_release,
};


//init function for the module
static int __init initModule(void)
{
	int i = 0; 
	minors = kmalloc(256 * sizeof(struct slot*), GFP_KERNEL);
	if(!minors)
	{
		printk(KERN_ERR "error allocating kernel memory\n");
		return -1;//returns -1 if failed
	}

	//creating 256 linked lists
	for(i = 0; i < 256; i++)
	{
		minors[i] = kmalloc(sizeof(struct slot), GFP_KERNEL);
		//setting the head of each list to point to NULL
		minors[i]->next = NULL;
	}
		
	if(register_chrdev(240, "message_slot", &_fops) < 0)
	{
		printk(KERN_ERR "error registering the module\n");
		return -1;//returns -1 if failed
	}
	else {
		printk("Everything went just fine\n");
	}
	return 0;//returns 0 on success
}


static void __exit cleanup(void)
{
	int i;
	struct slot* tmp;
	//releasing the struct array
	for(i = 0; i < 256; i++)
	{
		if(minors[i]->next != NULL)
		{
			minors[i] = minors[i]->next;
			//freeing the list in minors[i]
			while(minors[i] != NULL)
			{
				kfree(minors[i]->message);
				kfree(minors[i]->info);
				tmp = minors[i];
				minors[i] = minors[i]->next;
				kfree(tmp);
			}
			kfree(minors[i]);
			
		}
				
	}
	kfree(minors);
	//unregister the device
	unregister_chrdev(240, "message_slot");
}

module_init(initModule);
module_exit(cleanup);

