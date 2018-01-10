#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#define DEVICE_NAME		"fb4qt" 
#define NEXUS_QT_FB_MAJOR 		170 

#define FBIOSET_ADDR        0x4620

#define FB_V_WIDTH	1280
#define FB_V_HEIGHT	720
#define FB_WIDTH	800
#define FB_HEIGHT	480
#define BPP			16
#define BP			(BPP/8)
#define FB_DATA_LENGTH	(FB_V_WIDTH*FB_V_HEIGHT*BP)

static char * fb_data = NULL;

static struct fb_var_screeninfo vfb_default = {
    .xres =         FB_WIDTH,
    .yres =         FB_HEIGHT,
    .xres_virtual = FB_V_WIDTH,
    .yres_virtual = FB_V_HEIGHT,
    .xoffset =		(FB_V_WIDTH-FB_WIDTH)/2,
    .yoffset =		(FB_V_HEIGHT-FB_HEIGHT)/2,
    .bits_per_pixel = BPP,
    .red =          { 11, 5, 0},
    .green =        {  5, 6, 0},
    .blue =         {  0, 5, 0 },
    .activate =     FB_ACTIVATE_TEST,
    .height =       -1,
    .width =        -1,
    .pixclock =     20000,
    .left_margin =  64,
    .right_margin = 64,
    .upper_margin = 32,
    .lower_margin = 32,
    .hsync_len =    64,
    .vsync_len =    2,
    .vmode =        FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo vfb_fix = {
    .id =           "fb4qt",
    .type =         FB_TYPE_PACKED_PIXELS,
    .visual =       FB_VISUAL_PSEUDOCOLOR,
    .xpanstep =     1,
    .ypanstep =     1,
    .ywrapstep =    1,
    .accel =        FB_ACCEL_NONE,
};

static u_long get_line_length(int xres_virtual, int bpp)
{
    u_long length;

    length = xres_virtual * bpp;
    length = (length + 31) & ~31;
    length >>= 3;
    return (length);
}

static int fb_open(struct inode *inode, struct file *filp)
{    
	return 0;
}

static long fb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    void __user *argp = (void __user *)arg;
    long ret = 0;

    switch (cmd) {
    case FBIOGET_VSCREENINFO:
        var = vfb_default;
        ret = copy_to_user(argp, &var, sizeof(var)) ? -EFAULT : 0;
        break;
        
    case FBIOGET_FSCREENINFO:
        fix = vfb_fix;
    	fix.line_length = get_line_length(vfb_default.xres_virtual, vfb_default.bits_per_pixel);
    	fix.smem_len = vfb_default.xres_virtual*vfb_default.yres_virtual*vfb_default.bits_per_pixel/8;
        ret = copy_to_user(argp, &fix, sizeof(fix)) ? -EFAULT : 0;
        break;
        
//    case FBIOSET_ADDR:
//		fb_data = (char *)arg;
//		break;

    default:
		break;
    }
    return ret;
}

static ssize_t
fb_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;

	u8  *src;
	int c, cnt = 0, err = 0;
	unsigned long total_size = FB_DATA_LENGTH;

	if (p >= total_size)
		return 0;

	if (count >= total_size)
		count = total_size;

	if (count + p > total_size)
		count = total_size - p;

	src = fb_data + p;

	while (count) {
		c  = (count > PAGE_SIZE) ? PAGE_SIZE : count;

		if (copy_to_user(buf, src, c)) {
			err = -EFAULT;
			break;
		}
		
		src += c;
		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	return (err) ? err : cnt;
}

static ssize_t
fb_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;

	u8 *dst;
	int c, cnt = 0, err = 0;
	unsigned long total_size = FB_DATA_LENGTH;

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}

	dst = fb_data + p;

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;

		if (copy_from_user(dst, buf, c)) {
			err = -EFAULT;
			break;
		}

		dst += c;
		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	return (cnt) ? cnt : err;
}

static int 
fb_mmap(struct file*filp, struct vm_area_struct *vma)
{
//	vma->vm_flags |= VM_IO;
//	vma->vm_flags |= VM_RESERVED;

	if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(fb_data)>>PAGE_SHIFT
						, vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return  -EAGAIN;
	}

	return 0;
}

static struct file_operations fb_fops=
{
	.owner = 			THIS_MODULE, 
	.open =				fb_open,	
	.unlocked_ioctl =	fb_ioctl, 
	.read = 			fb_read,
	.write = 			fb_write,
	.mmap =				fb_mmap,
};

int init_module(void)
{
	int ret;
	printk("fb4qt init_module\n");

	fb_data = kmalloc(FB_DATA_LENGTH, GFP_KERNEL);
	if (!fb_data)    /*…Í«Î ß∞‹*/
	{
		return  -ENOMEM;
	}
  
	ret = register_chrdev(NEXUS_QT_FB_MAJOR, DEVICE_NAME, &fb_fops);
	if (ret < 0)
	{
		printk(DEVICE_NAME " can't register major number\n");
		return ret;
	}

	return 0;
}

void cleanup_module(void)
{
	printk("fb4qt cleanup_module\n");

	kfree(fb_data);
	unregister_chrdev(NEXUS_QT_FB_MAJOR, DEVICE_NAME);
}

MODULE_LICENSE("GPL");

