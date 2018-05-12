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
struct minorNode {
	unsigned long value;
	bool open;
	char channels[512];
	ssize_t length[4];
	struct minorNode * nextNode;
};


struct message_slot_info
{
  int size;
  struct minorNode * head;
};

static struct message_slot_info device_info;

// The message the device will give when asked
static char message[BUF_LEN] = {0};

// Functions of the lisnked list
void insertAtTheBeginingOfTheList(unsigned long value) {
	int i;
	struct minorNode * first = (struct minorNode *)kmalloc(sizeof(struct minorNode), GFP_KERNEL);
	first -> value = value;
	first -> open = true;
	for (i = 0; i <= 3; i++) {
		first -> length[i] = 0;
	}
	first -> nextNode = device_info.head;
	device_info.head = first;
	device_info.size++;
}

struct minorNode* getMinorNode(unsigned long minor) {
	struct minorNode * curr;
	if (device_info.head == NULL) {
		return NULL;
	}
	curr = device_info.head;
	while ( curr -> value != minor) {
		if (curr -> nextNode == NULL) {
			return NULL;
		}
		curr = curr -> nextNode;
	}
	return curr;
}

int cleanList(void) {
	struct minorNode * temp = NULL;
	while (device_info.head != NULL) {
		temp  = device_info.head;
		device_info.head = temp -> nextNode;
		kfree(temp);
	}
	return 0;
}
//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned long minor = (unsigned long)iminor(inode);
  struct minorNode *relevantNode = getMinorNode(minor);
  printk("Invoking device_open(%p)\n", file);
  if (relevantNode == NULL) {
	  insertAtTheBeginingOfTheList(minor);
  } else {
	  relevantNode -> open = true;
  }
  file -> private_data = (void*)-1;
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file) {
	unsigned long minor = (unsigned long)iminor(inode);
	struct minorNode *relevantNode = getMinorNode(minor);
	printk("Invoking device_release(%p,%p)\n", inode, file);
	if (relevantNode != NULL) {
		relevantNode -> open = false;
	}
      	return SUCCESS;
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
	 ssize_t totalRead;
	 struct minorNode * relevantNode;
       	 if (channel == -1 ) {
		 printk(KERN_ALERT "Not a valid Channel");
       		 return -EINVAL;
       	 }
       	 relevantNode = getMinorNode(minor);
	 if (relevantNode -> open != true) {
		 printk(KERN_ALERT "The file is not open");
		 return -EINVAL;
	 }
	 if (relevantNode -> length[channel] > length) {
		 printk(KERN_ALERT "Too small buffer\n");
		 return -EINVAL;
	 }
       	 printk("Invoking device_read(%p,%d)\n", file, length);
       	 for( i = (128*channel); i < relevantNode -> length[channel]+(128*channel); ++i ) {
		 put_user(relevantNode -> channels[i], &buffer[i-(128*channel)]);
		 message[i] += 1;
	 }
       	 //invalid argument error
	 totalRead = (ssize_t)(i-(128*channel));
	 return totalRead;
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
	struct minorNode * relevantNode;
	if (channel == -1  || sizeof(buffer) > MAX_BUFFER_SIZE || length < sizeof(buffer)) {
		printk(KERN_ALERT "Channel is not valid OR length of massage is more than 128 bytes\n");
		return -EINVAL;
	}
	relevantNode = getMinorNode(minor);
      	printk("Invoking device_write(%p,%d)", file, length);
       	if (relevantNode -> open != true) {
		printk(KERN_ALERT "File is not open");
		return -EINVAL;
	}
      	for( i = (128*channel); i < length+(128*channel) && i < BUF_LEN+(128*channel); ++i ) {
		get_user(relevantNode -> channels[i], &buffer[i-(128*channel)]);
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
    printk( "Invoking ioctl\n");
    file -> private_data = (void*)ioctl_param;
    return SUCCESS;
  }
  return -EINVAL;
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
static int __init simple_init(void)
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
  
  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  cleanList();
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
