/*
设备管理
*/



#include "dev/dev.h"
#include "cpu/irq.h"
#include"tools/klib.h"

#define DEV_TABLE_SIZE      128

extern dev_desc_t dev_tty_desc;
extern dev_desc_t dev_disk_desc;


static  dev_desc_t * dev_desc_tbl[]={
    &dev_tty_desc,
    &dev_disk_desc,
};

static device_t dev_tbl[DEV_TABLE_SIZE];

static int devid_is_bad(int dev_id){
    if(dev_id < 0 || dev_id >= DEV_TABLE_SIZE){
        return 1;
    }

    if(dev_tbl[dev_id].desc == (dev_desc_t *)0){
        return 1;
    }

    return 0;
}


int dev_open (int major,int minor,void * data){
    irq_state_t state = irq_enter_protection();
    
    device_t * free_dev = (device_t*)0;
    //便利设备表，分配新表项或增加已打开设备的打开次数
    for(int i =0;i < sizeof(dev_tbl)/sizeof(device_t);i++){
        device_t * dev = dev_tbl + i;
        
        if(dev->open_count == 0){
            free_dev = dev;//若无已打开的设备，则最后一个open count为0的表项为分配给该设备的表项
        }else if(dev->minor == minor && dev->desc->major == major){
            dev->open_count++;
            irq_leave_protection(state);
            
            //若存在已打开的设备，不需要调用打开函数
            return i;
        }
    }

    //打开新设备

    //检查该设备是否已在dev_desc_tbl中注册
    dev_desc_t * desc = (dev_desc_t*)0;
    for(int i = 0;i< sizeof(dev_desc_tbl) / sizeof(dev_desc_t *);i++){
        dev_desc_t * _desc = dev_desc_tbl[i];
        if(major == _desc->major){
            desc = _desc;
            break;
        }
    }

    //新分配的设别已注册
    if(free_dev && desc){
        //对表项进行设置
        free_dev->minor = minor;
        free_dev->desc = desc;
        free_dev->data = data;

        //调用接口
        int err = desc->open(free_dev);
        if(err == 0){
            free_dev->open_count = 1;
            irq_leave_protection(state);
            return free_dev - dev_tbl;//返回设备在数组的下标
        }
    }
    
    irq_leave_protection(state);
    return -1;
}
int dev_read(int dev_id,int addr,char *buf,int size)
{
    if(devid_is_bad(dev_id)){
        return -1;
    }

    device_t * device = dev_tbl + dev_id;

    return device->desc->read(device,addr,buf,size);
}

int dev_write (int dev_id,int addr,char *buf,int size)
{
    if(devid_is_bad(dev_id)){
        return -1;
    }

    device_t * device = dev_tbl + dev_id;

    return device->desc->write(device,addr,buf,size);
}

int dev_control(int dev_id, int cmd, int arg0, int arg1)
{
    if(devid_is_bad(dev_id)){
        return -1;
    }

    device_t * device = dev_tbl + dev_id;

    return device->desc->control(device,cmd,arg0,arg1);
}

void dev_close(int dev_id){
    if(devid_is_bad(dev_id)){
        return;
    }
    device_t * device = dev_tbl + dev_id;

    irq_state_t state = irq_enter_protection();
    if(--device->open_count == 0){
        device->desc->close(device);
        kernel_memset((void*)device,0,sizeof(device));
    }
    irq_leave_protection(state);
}

