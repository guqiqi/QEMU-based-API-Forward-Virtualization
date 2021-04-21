#include <linux/virtio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_pci.h>
#include <linux/virtio_config.h>
#include <linux/scatterlist.h>
#include <linux/random.h>
#include <linux/io.h>
#include <linux/sort.h>

#include "virtio_gl_common.h"

#define error(fmt, arg...) \
	printk("### func= %-30s ,line= %-4d ," fmt, __func__,  __LINE__, ##arg)

#if 1
#define ptrace(fmt, arg...) \
	printk(KERN_INFO "    ### " fmt, ##arg)
#else
#define ptrace(fmt, arg...)
#endif

struct virtio_gl
{
	struct virtio_device	*vdev;
	struct virtqueue        *vq;
	spinlock_t	 	 	 	lock;
};

struct virtio_gl *vgl;

////////////////////////////////////////////////////////////////////////////////
///	General Function
////////////////////////////////////////////////////////////////////////////////
/** 
 * copy_from_user_safe() and copy_to_user_safe()
 * Copy data butween user space and kernel space 
 * */
static inline unsigned long copy_from_user_safe(void *to, const void __user *from, unsigned long n)
{
	unsigned long err;

	if( from==NULL || n==0 )
	{
		memset(to, 0, n);
		return 0;
	}

	err = copy_from_user(to, from, n);
	if( err ){
		error("copy_from_user is could not copy  %lu bytes\n", err);
		BUG_ON(1);
	}

	return err;
}

static inline unsigned long copy_to_user_safe(void __user *to, const void *from, unsigned long n)
{
	unsigned long err;

	if( to==NULL || n==0 )
		return 0;

	err = copy_to_user(to, from, n);
	if( err ){
		error("copy_to_user is could not copy  %lu bytes\n", err);
	}

	return err;
}

static inline void* kzalloc_safe(size_t size)
{
	void *ret;

	ret = kzalloc(size, GFP_KERNEL);
	if( !ret ){
		error("kzalloc failed, size= %lu\n", size);
		BUG_ON(1);
	}

	return ret;
}

static inline void* kmalloc_safe(size_t size)
{
	void *ret;

	ret = kmalloc(size, GFP_KERNEL);
	if( !ret ){
		error("kmalloc failed, size= %lu\n", size);
		BUG_ON(1);
	}

	return ret;
}

static uint64_t user_to_gpa(uint64_t from, uint32_t size){
	void *gva;
	gva = kmalloc_safe(size);
	
	if(from)
		copy_from_user_safe(gva, (const void*)from, size);

	return (uint64_t)virt_to_phys(gva);
}

static void kfree_gpa(uint64_t pa, uint32_t size){
	kfree(phys_to_virt((phys_addr_t)pa));
}


// ##################################################################################
// OpenGL Operations
// ##################################################################################
/**
 * Send Virtual Cmd to virtio device
 * @req: struct include command and arguments
 * if the function is corrent, it return 0. otherwise, is -1
 * */
static int virtio_gl_misc_send_cmd(VirtioGLArg *req)
{
    struct scatterlist *sgs[2], req_sg, res_sg;
	unsigned int len;
	int err;
	VirtioGLArg *res;

	res = kmalloc_safe(sizeof(VirtioGLArg));
	memcpy(res, req, sizeof(VirtioGLArg)); // dest, src, size

	sg_init_one(&req_sg, req, sizeof(VirtioGLArg));
	sg_init_one(&res_sg, res, sizeof(VirtioGLArg));

	sgs[0] = &req_sg;
	sgs[1] = &res_sg;

	err =  virtqueue_add_sgs(vgl->vq, sgs, 1, 1, req, GFP_ATOMIC);

ptrace("cmd req: %u, %lu, %u, %lu, %u", req->cmd, req->pA, req->pASize, req->pB, req->pBSize);

virtqueue_kick(vgl->vq);


	return err;
}

static int virtio_gl_misc_send_cmd_no_arg(VirtioGLArg *arg){
	return (int)virtio_gl_misc_send_cmd(arg);
}

static int virtio_gl_misc_send_cmd_two_args(VirtioGLArg *arg){
	int *a = arg->pA;
	char *b = arg->pB;
	ptrace("a: %d! %u\n", *a, arg->pASize);
	ptrace("b: %s! %u\n", b, arg->pBSize);
ptrace("logical arg: %u, %lu, %u, %lu, %u", arg->cmd, arg->pA, arg->pASize, arg->pB, arg->pBSize);
	arg->pA = user_to_gpa(arg->pA, arg->pASize);
	arg->pB = user_to_gpa(arg->pB, arg->pBSize);

	ptrace("pysical arg: %u, %lu, %u, %lu, %u", arg->cmd, arg->pA, arg->pASize, arg->pB, arg->pBSize);
	
	//kfree_gpa(arg->pA, arg->pASize);
	//kfree_gpa(arg->pB, arg->pBSize);

	return (int)virtio_gl_misc_send_cmd(arg);
}

// ##################################################################################
// Machine Driver
// ##################################################################################

static long virtio_gl_misc_ioctl(struct file *filp, unsigned int _cmd, unsigned long _arg){
ptrace("send message!\n");    
ptrace("cmd: %d!\n", _cmd);
    VirtioGLArg *arg;
    int err;

    arg = kmalloc_safe(sizeof(VirtioGLArg));
    copy_from_user_safe(arg, (void*)_arg, sizeof(VirtioGLArg));

    arg->cmd = _cmd;

    switch(arg->cmd){
	case VIRTGL_CMD_WRITE:
	    err = (int)virtio_gl_misc_send_cmd_two_args(arg);
	    break;
	case VIRTGL_OPENGL_CREATE_WINDOW:
	    err = (int)virtio_gl_misc_send_cmd_no_arg(arg);
	    break;
	default:
	    ptrace("unknown cmd!\n");
	    break;
    }

    

    //kfree(arg);

    return err;
}

static int virtio_gl_misc_open(struct inode *inode, struct file *filp){
	ptrace("device open!\n");
    return 0;
}

static int virtio_gl_misc_release(struct inode *inode, struct file *filp){
	ptrace("device release!\n");
    return 0;
}

static int virtio_gl_misc_mmap(struct file *filp, struct vm_area_struct *vma){
	ptrace("device mmap!\n");
    return 0;
}

// device file operation
struct file_operations gl_misc_fops = {
	.owner   		= THIS_MODULE,
	.open    		= virtio_gl_misc_open,
	.release 		= virtio_gl_misc_release,
	.unlocked_ioctl = virtio_gl_misc_ioctl, // user space interface
	.mmap			= virtio_gl_misc_mmap,
};

static struct miscdevice gl_misc_driver = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "virtio-gl",
	.fops  = &gl_misc_fops,
};

// ##################################################################################
// Virtio Operations
// ##################################################################################
/**
 * callback function
 * */
static void vgl_virtio_cmd_vq_cb(struct virtqueue *vq){
 ptrace("callback!\n");
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_GL, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static unsigned int features[] = {};

static int gl_virtio_probe(struct virtio_device *dev){
    ptrace("probe module!\n");

    int err;

    vgl = kzalloc_safe(sizeof(struct virtio_gl));
    if(!vgl){
        err = -ENOMEM;
        return err;
    }

    dev->priv = vgl;
    vgl->vdev = dev;

    vgl->vq = virtio_find_single_vq(dev, vgl_virtio_cmd_vq_cb, 
			"request_handle");
    // TODO err handler

    err = misc_register(&gl_misc_driver);
	if (err)
	{
		error("virthm: register misc device failed.\n");
	}

    return 0;
}

static void gl_virtio_remove(struct virtio_device *dev){
	printk(KERN_ALERT "remove module!\n");
    // int err;

    misc_deregister(&gl_misc_driver);
printk(KERN_ALERT "remove misc!\n");
	// if( err ){
	// 	error("misc_deregister failed\n");
	// }

	vgl->vdev->config->reset(vgl->vdev);
	vgl->vdev->config->del_vqs(vgl->vdev);
	//kfree(vgl->vq);
	//kfree(vgl);
}

static struct virtio_driver virtio_gl_driver = {
    .id_table           = id_table,
	.feature_table      = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name        = KBUILD_MODNAME,
	.driver.owner       = THIS_MODULE,
	.probe              = gl_virtio_probe,
	.remove             = gl_virtio_remove,
};

static int __init init(void)
{
	int ret;

	ret = register_virtio_driver(&virtio_gl_driver);
	ptrace("register virtio gl driver ret: %d\n", ret);
	if( ret < 0 ){
		error("register virtio driver faild (%d)\n", ret);
	}
	return ret;
	return 0;
}

static void __exit mod_exit(void)
{
	ptrace("bye module!\n");
	unregister_virtio_driver(&virtio_gl_driver);
}

module_init(init);
 module_exit(mod_exit);

// replace module_init and module_exit function
//module_virtio_driver(virtio_gl_driver);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Qemu Virtio GL driver");
MODULE_LICENSE("GPL");
