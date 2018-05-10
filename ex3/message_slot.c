// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h> /*for kmalloc and kfree*/
MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

//Creating an int linked list for the slot's names
typedef struct node {
	unsigned long value;
	bool open;
	char channels[512];
	ssize_t length[4];
	struct node * nextNode;
} minorNode;


struct message_slot_info
{
  int size;
  minorNode * head;
};

static struct message_slot_info device_info;

// The message the device will give when asked
static char message[BUF_LEN] = {0};

static minorNode* getMinorNode(unsigned long minor) {
	minorNode * curr = device_info.head;
	if (device_info.size > 0) {
		while ( curr != NULL) {
			if (curr -> value == minor && curr -> open == true) {
				return curr;
			}
			curr = curr -> nextNode;
		}
	}
	printk("123456789minor does not exist\n");
	return NULL;
}
//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned long minor = (unsigned long)iminor(inode);
  printk("123456789Invoking device_open(%p)\n", file);
  if (device_info.size == 0) {
	  device_info.head = kmalloc(sizeof(minorNode),GFP_KERNEL);
	  if (device_info.head == NULL) {
		  printk("123456789Could not kmalloc head\n");
		  return -1;
	  }
	  device_info.head -> value = minor;
	  device_info.head -> open = true;
	  device_info.head -> nextNode = NULL;
	  device_info.size = 1;
  }
  else {
	  minorNode * curr = device_info.head;
	  while (curr -> nextNode != NULL) {
		  if (curr -> value == minor) {
			  curr -> open = true;
			  return SUCCESS;
		  }
		  curr = curr -> nextNode;
	  }
	  curr -> nextNode = kmalloc(sizeof(minorNode),GFP_KERNEL);
	  curr -> nextNode -> value = minor;
	  device_info.head -> open = true;
	  curr -> nextNode -> nextNode = NULL;
	  device_info.size++;
  }
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  minorNode * curr = device_info.head;
  unsigned long minor = iminor(inode);
  printk("123456789Invoking device_release(%p,%p)\n", inode, file);
  if (device_info.size > 0) {
	  while ( curr != NULL) {
		  if (curr -> value == minor) {
			  curr -> open = false;
			  return SUCCESS;
		  }
		  curr = curr -> nextNode;
	  }
  }
  printk("123456789Could not release because the minor does not exist\n");
  return -1;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset ) {
      	// read doesnt really do anything (for now)
       	 int channel = (int) file -> private_data;
       	 unsigned long minor = iminor(file_inode(file));
	 int i;
	 ssize_t totalWritten;
	 minorNode * relevantNode;
 	 printk( "123456789Invocing device_read\n");
       	 if (channel == -1 ) {
       		 printk("123456789Channel is -1\n");
       		 return -EINVAL;
       	 }
       	 relevantNode = getMinorNode(minor);
	 if (relevantNode -> length[channel] < sizeof(buffer) || relevantNode -> length[channel] < length) {
		 printk("123456789Too small buffer\n");
		 return -EINVAL;
	 }
       	 printk("123456789Invoking device_write(%p,%d)\n", file, length);
       	 for( i = (128*channel); i < length && i < BUF_LEN; ++i ) {
		 put_user(relevantNode -> channels[i], &buffer[i]);
		 message[i] += 1;
	 }
       	 //invalid argument error
	 totalWritten = (ssize_t)(i-(128*channel));
	 return totalWritten;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset) {
	int channel = (int) file -> private_data;
	unsigned long minor = iminor(file_inode(file));
	int i;
	ssize_t totalWritten;
	minorNode * relevantNode;
	if (channel == -1  || sizeof(buffer) > MAX_BUFFER_SIZE || length < sizeof(buffer)) {
		printk("123456789Channel is -1 OR length of massage is more than 128 bytes\n");
		return -EINVAL;
	}
	relevantNode = getMinorNode(minor);
      	printk("123456789Invoking device_write(%p,%d)\n", file, length);
      	for( i = (128*channel); i < length && i < BUF_LEN; ++i ) {
		get_user(relevantNode -> channels[i], &buffer[i]);
      		message[i] += 1;
      	}
	// return the number of input characters used
	totalWritten = (ssize_t)(i-(128*channel));
	relevantNode -> length[channel] = totalWritten;
	return totalWritten;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  // Switch according to the ioctl called
  if( IOCTL_SET_ENC == ioctl_command_id && (ioctl_param >=0 && ioctl_param <=3)) {
    // Get the parameter given to ioctl by the process
    printk( "123456789Invoking ioctl\n");
    file -> private_data = (void*)ioctl_param;
    return SUCCESS;
  }
  return -1;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .release        = device_release,
  .unlocked_ioctl = device_ioctl
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init message_slot_init(void)
{
  int rc = -1;
  // init dev struct
  memset( &device_info, 0, sizeof(struct message_slot_info));
  device_info.size = 0;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }
  
  printk( "123456789Registeration is successful. ");
  printk( "123456789If you want to talk to the device driver,\n" );
  printk( "123456789you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
}

static minorNode* freeNode(minorNode * currNode) {
	minorNode * tmpNext;
	tmpNext = currNode -> nextNode;
	kfree(currNode);
	return tmpNext;
}

//---------------------------------------------------------------
static void __exit message_slot_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  minorNode * currNode = device_info.head;
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
  if (device_info.size > 0) {
	  while (currNode != NULL) {
		  currNode = freeNode(currNode);
	  }
  }
}

//---------------------------------------------------------------
module_init(message_slot_init);
module_exit(message_slot_cleanup);

//========================= END OF FILE =========================
