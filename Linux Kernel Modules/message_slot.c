#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       
#include <linux/uaccess.h>  
#include <linux/string.h>  
#include "message_slot.h"
#include <linux/slab.h>


MODULE_LICENSE("GPL");

/*Define a struct for each open file descriptor saving it's channel id and censorship state*/
struct fd_state {
  unsigned int channel_id;
  unsigned int censorship_mode;
};

/*Define a struct for a message channel*/
struct channel_info {
  unsigned int channel_id;
  char message[128];     /*The actual message (max 128 bytes)*/
  int message_len;       /*How long the current message is*/
  struct channel_info *next;
};

/*Define a struct for a message slot with a specific minor number*/
struct slot_info {
  unsigned int minor_num;
  struct channel_info *channels; /* Head of the channels linked list*/
  struct slot_info *next;  /* Next minor number slot*/
};

/*Global pointer to head of slots linked list*/
static struct slot_info *slots_head = NULL;


//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,struct file*  file )
{
  struct slot_info* curr_slot;
  struct fd_state* state;
  struct slot_info* new_slot;
  int minor;
  printk("Invoking device_open(%p)\n", file);
  state = (struct fd_state*)kmalloc(sizeof(struct fd_state),GFP_KERNEL);
  if(state==NULL){
    return -ENOMEM;
  }
  state->channel_id=0;
  state->censorship_mode=0;
  file->private_data=(void *)state;
  minor = iminor(file_inode(file));
  curr_slot=slots_head;
  while(curr_slot!=NULL){
    if(curr_slot->minor_num==minor){ /*Slot already exists*/
      return 0;
    }
    curr_slot=curr_slot->next;
  }
  /*Create new slot*/
  new_slot = (struct slot_info*)kmalloc(sizeof(struct slot_info),GFP_KERNEL);
  if(new_slot==NULL){
    kfree(state);
    file->private_data = NULL;
    return -ENOMEM;
  }
  /*Initiate new slot and push it into beginning of linked list*/
  new_slot->minor_num=minor;
  new_slot->channels=NULL;
  new_slot->next = slots_head;
  slots_head = new_slot;

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode, struct file*  file)
{
printk("Invoking device_release(%p,%p)\n", inode, file);
if(file->private_data!=NULL){
  kfree(file->private_data);
}
return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer, size_t length,loff_t* offset ){

  struct fd_state *state;
  struct slot_info *curr_slot;
  struct channel_info *curr_channel;
  int minor;

  printk("Invoking device_read(%p,%ld)\n", file, length);

  if (buffer == NULL) {
    return -EINVAL;
  }

  if (file->private_data == NULL) {
      return -EINVAL;
  }

  state = (struct fd_state *)file->private_data;
  /*Illegal channel id number*/
  if(state->channel_id==0){ 
    return -EINVAL;
  }
  /*Reach required slot using minor number*/
  curr_slot = slots_head;
  minor = iminor(file_inode(file));
  while(curr_slot!=NULL && curr_slot->minor_num!=minor){
    curr_slot = curr_slot->next;
  }

  if (curr_slot == NULL) {
    return -EWOULDBLOCK; 
  }
  /*Reach required chanel inside slot*/
  curr_channel=curr_slot->channels;
  while(curr_channel!=NULL && curr_channel->channel_id!=state->channel_id){
    curr_channel=curr_channel->next;
  }

  if(curr_channel==NULL || curr_channel->message_len==0){/*No message exists on the channel*/
    return -EWOULDBLOCK;
  }

  if(length<curr_channel->message_len){ /*Provided buffer too small*/
    return -ENOSPC;
  }
  /*Try to read into buffer, preserving atomic requirement*/
  if (copy_to_user(buffer, curr_channel->message, curr_channel->message_len) != 0) {
    return -EFAULT;
  }
  return curr_channel->message_len;

}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file* file, const char __user* buffer,size_t length,loff_t* offset){
  struct fd_state *state;
  struct slot_info *curr_slot;
  struct channel_info *curr_channel;
  char temp_buffer[128];
  int minor;
  int i;
  printk("Invoking device_write(%p,%ld)\n", file, length);

  if (buffer == NULL) {
    return -EINVAL;
  }

/*Illegal buffer length*/
  if(length==0 || length>128){ 
    return -EMSGSIZE;
  }

/*Confirm assigned channel id*/
  if (file->private_data == NULL) {
      return -EINVAL;
  }

  state = (struct fd_state *)file->private_data;
  /*Illegal channel id number*/
  if(state->channel_id==0){ 
    return -EINVAL;
  }
  /*Reach required slot using minor number*/
  curr_slot = slots_head;
  minor = iminor(file_inode(file));
  while(curr_slot!=NULL && curr_slot->minor_num!=minor){
    curr_slot = curr_slot->next;
  }

  if (curr_slot == NULL) {
    return -EINVAL; 
  }
  /*Reach required chanel inside slot*/
  curr_channel=curr_slot->channels;
  while(curr_channel!=NULL && curr_channel->channel_id!=state->channel_id){
    curr_channel=curr_channel->next;
  }

  /*Check if channel exists and create one if not*/
  if(curr_channel==NULL){
    curr_channel=(struct channel_info*)kmalloc(sizeof(struct channel_info), GFP_KERNEL);
    /*Memory allocation failure*/
    if (curr_channel == NULL) {
      return -ENOMEM; 
    }
    /*Initialize new channel and add it to head of channel list*/
    curr_channel->channel_id=state->channel_id;
    curr_channel->message_len=0;
    curr_channel->next=curr_slot->channels;
    curr_slot->channels=curr_channel;
  }


  /*Copy message with correct censorship*/
  for(i=0;i<length;i++){
    if(get_user(temp_buffer[i],&buffer[i])!=0){ /*check address like in recitation*/
      return -EFAULT;
    }
  }
  /*Write according to censorship*/
  for(i=0;i<length;i++){
    if(state->censorship_mode==1 && (i % 4 == 3)){
      curr_channel->message[i]='#';
    }
    else{
      curr_channel->message[i]=temp_buffer[i];
    }
  }
  curr_channel->message_len=length;
return length;
}


//----------------------------------------------------------------
static long device_ioctl( struct   file* file,unsigned int   ioctl_command_id,unsigned long  ioctl_param ){
// Switch according to the ioctl called
struct fd_state *state;

/*Get the state assigned to this specific file descriptor*/    
if (file->private_data == NULL) {
    return -EINVAL;
}
    
state = (struct fd_state *)file->private_data;

switch (ioctl_command_id)
{
case MSG_SLOT_CHANNEL: /*Need to set channel ID*/
  if(ioctl_param==0){
    return -EINVAL;
  }
  printk( "Invoking ioctl: setting channel id "
    "flag to %ld\n", ioctl_param );
  state->channel_id = ioctl_param;
  break;

case MSG_SLOT_SET_CEN:
  if (ioctl_param != 0 && ioctl_param != 1) { /*Assert param is legal cennsorship*/
    return -EINVAL;
  }
  printk( "Invoking ioctl: setting encryption "
    "flag to %ld\n", ioctl_param );
  state->censorship_mode = ioctl_param;
  break;

default: /*Passed command is not MSG_SLOT_CHANNEL or MSG_SLOT_SET_CEN*/
  return -EINVAL;
}
return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
.owner	  = THIS_MODULE, 
.read           = device_read,
.write          = device_write,
.open           = device_open,
.unlocked_ioctl = device_ioctl,
.release        = device_release,
};


// Initialize the module - Register the character device
static int __init message_slot_init(void){
    int rc = -1;
    slots_head = NULL;

    rc = register_chrdev(MAJOR_NUM, "message_slot", &Fops);
    if (rc < 0) {
      printk(KERN_ERR "%s: registration failed for major %d\n", DEVICE_FILE_NAME, MAJOR_NUM);        
      return rc;
    }

    printk(KERN_INFO "%s: registration successful\n", DEVICE_FILE_NAME);    
    return 0; 
}

// Unregister the device and free all allocated memory
// Should always succeed
static void __exit message_slot_cleanup(void){
  struct channel_info *curr_channel;
  struct channel_info *next_channel;
  struct slot_info *curr_slot;
  struct slot_info *next_slot;

  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
  /*Free all channels and slots that were allocated*/
  curr_slot = slots_head;
  while(curr_slot!=NULL){
    curr_channel = curr_slot->channels;
    while(curr_channel!=NULL){
      next_channel = curr_channel->next;
      kfree(curr_channel);
      curr_channel = next_channel;
    }
    next_slot = curr_slot->next;
    kfree(curr_slot);
    curr_slot = next_slot;
  }

}

//---------------------------------------------------------------
module_init(message_slot_init);
module_exit(message_slot_cleanup);
//========================= END OF FILE =========================