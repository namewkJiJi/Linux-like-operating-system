#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
#include "dev/dev.h"
#include "cpu/irq.h"

static int flag;//load内也有加载磁盘的操作，但不需要调用下面定义的中断处理程序中的notify
static mutex_t mutex;
static sem_t sem;
static disk_t disk_buf[DISK_NR];

static void disk_send_cmd(disk_t *disk,uint32_t start_sector,uint32_t sector_count,int cmd){
    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);		// 使用LBA寻址，并设置驱动器

	// 必须先写高字节
	outb(DISK_SECTOR_NR(disk), (uint8_t) (sector_count >> 8));	// 扇区数高8位
	outb(DISK_LBA_LOW(disk), (uint8_t) (start_sector >> 24));		// LBA参数的24~31位
	outb(DISK_LBA_MID(disk), 0);									// 高于32位不支持
	outb(DISK_LBA_HIGH(disk), 0);										// 高于32位不支持
	outb(DISK_SECTOR_NR(disk), (uint8_t) (sector_count));		// 扇区数量低8位
	outb(DISK_LBA_LOW(disk), (uint8_t) (start_sector >> 0));			// LBA参数的0-7
	outb(DISK_LBA_MID(disk), (uint8_t) (start_sector >> 8));		// LBA参数的8-15位
	outb(DISK_LBA_HIGH(disk), (uint8_t) (start_sector >> 16));		// LBA参数的16-23位

	// 选择对应的主-从磁盘
	outb(DISK_CMD(disk), (uint8_t)cmd);
}

static void disk_read_data(disk_t *disk,void * buf,int size){
    uint16_t * c = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        *c++ = inw(DISK_DATA(disk));
    }
}

static void disk_write_data(disk_t *disk,void * buf,int size){
    uint16_t * c = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        outw(DISK_DATA(disk),*c++);
    }
}

static int disk_wait_data(disk_t * disk){
    uint8_t status;
	do {
        // 等待数据或者有错误
        status = inb(DISK_STATUS(disk));
        if ((status & (DISK_BUSY | DISK_DRQ | DISK_ERR))
                        != DISK_BUSY) {
            break;
        }
    }while (1);

    // 检查是否有错误
    return (status & DISK_ERR) ? -1 : 0;
}

static void print_disk_info(disk_t *disk){
    log_printf("%s",disk->name);
    log_printf("  port base:%x",disk->port_base);
    log_printf("  total size:%d m",disk->sector_count * disk->sector_size/1024/1024);

    for(int i = 0; i < PARTINFO_NR;i++){
        partinfo_t * partinfo = disk->partinfo+i;
        if(partinfo->type != FS_INVALID){
            log_printf("        %s:type:%x,start sector:%d,sector count:%d",
                partinfo->name,partinfo->type,partinfo->start_sector,partinfo->sector_count
            );
        }
    }

}

static int detect_part_info(disk_t * disk){
    mbr_t mbr;

    disk_send_cmd(disk,0,1,DISK_READ);
    int err = disk_wait_data(disk);
    if(err < 0){
        log_printf("read mbr failed");
        return err;
    }

    disk_read_data(disk,&mbr,sizeof(mbr));

    part_item_t * item = mbr.part_item;
    partinfo_t * partinfo = disk->partinfo + 1;
    for(int i = 1;i<= MBR_PART_ITEM_NR;i++,item++,partinfo++){
        partinfo->type = item->system_id;
        if(partinfo->type == FS_INVALID){
            //分区无效
            partinfo->disk = (struct _disk_t *)0;
            partinfo->sector_count = 0;
            partinfo->start_sector = 0;
        }else{
            kernel_sprintf(partinfo->name,"%s%d",disk->name,i);
            partinfo->start_sector = item->relative_sectors;
            partinfo->sector_count = item->total_sectors;
            partinfo->disk = (struct _disk_t *)disk;
        }
    }

}


static int identify_disk(disk_t * disk){
    disk_send_cmd(disk,0,0,DISK_INDENTIFY);

    int err = inb(DISK_STATUS(disk));
    if(err == 0){
        return -1;
    }

    err = disk_wait_data(disk);
    if(err < 0){
        return err;
    }

    uint16_t buf[256];
    disk_read_data(disk,buf,sizeof(buf));
    disk->sector_count = *(uint32_t*)(buf+100);
    disk->sector_size = SECTOR_SIZE;
    disk->mutex = &mutex;
    disk->op_sem = &sem;

    //设置第0个分区的相关数据-分区0代表完整的一个磁盘
    partinfo_t * partinfo = disk->partinfo+0;
    partinfo->disk = (struct _disk_t *)disk;
    partinfo->start_sector = 0;
    partinfo->sector_count = disk->sector_count;
    partinfo->type = FS_INVALID;
    kernel_sprintf(partinfo->name,"%s%d",disk->name,0);



    detect_part_info(disk);


    return 0;
}


void disk_init(void){
    kernel_memset(disk_buf,0,sizeof(disk_buf));

    mutex_init(&mutex);
    sem_init(&sem,0);


    for(int i = 0;i<DISK_NR_PER_CHANNEL;i++){
        disk_t * disk = disk_buf + i;

        kernel_sprintf(disk->name,"sd%c",i+'a');
        disk->drive = (i==0) ? DISK_MASTER : DISK_SLAVE;
        disk->port_base = IOBASE_PRIMARY;

        int err = identify_disk(disk);
        if(err == 0){
            print_disk_info(disk);
        }
    }

}

//-------
int disk_open(device_t * dev){
    //次设备号类似于0xa0 a代表磁盘，数字代表分区
    int disk_idx = (dev->minor >> 4) - 0xa; 
    int part_idx = (dev->minor) &0xF;

    disk_t * disk = disk_buf + disk_idx;
    if(disk->sector_count == 0){
        log_printf("disk open failed");
        return -1;
    }

    partinfo_t * partinfo = disk->partinfo + part_idx;
    if(partinfo->sector_count == 0){
        log_printf("part info failed");
        return -1;
    }
    
    dev->data = partinfo;//保存分区信息

    irq_install(IRQ14_HARDDISK_PRI,exception_handler_ide_primary);
    irq_enable(IRQ14_HARDDISK_PRI);

    return 0;
}

int disk_read(device_t* dev,int addr,char* buf,int size){
    partinfo_t * partinfo = (partinfo_t *)dev->data;

    if(!partinfo){
        return -1;
    }

    disk_t * disk = (disk_t *)partinfo->disk;
    if(disk == (disk_t*)0){
        return -1;
    }

    //占用端口：上锁
    mutex_lock(disk->mutex);
    flag = 1;
    disk_send_cmd(disk,partinfo->start_sector+addr,size,DISK_READ);
    
    int cnt = 0;
    for(;cnt<size;cnt++,buf+=disk->sector_size){
        //在进程的初始化阶段也会调用读取磁盘的操作-挂载fat16文件系统时。
        //只有在进程中读取磁盘时才需要等待
        if(task_current()){
            sem_wait(disk->op_sem);
        }

        int err = disk_wait_data(disk);
        if(err < 0){
            break;
        }

        disk_read_data(disk,buf,disk->sector_size);

    }

    mutex_unlock(disk->mutex);
    return cnt;
}

int disk_write(device_t* dev,int addr,char* buf,int size){
    partinfo_t * partinfo = (partinfo_t *)dev->data;

    if(!partinfo){
        return -1;
    }

    disk_t * disk = (disk_t *)partinfo->disk;
    if(disk == (disk_t*)0){
        return -1;
    }

    //占用端口：上锁
    mutex_lock(disk->mutex);
    flag = 1;
    disk_send_cmd(disk,partinfo->start_sector+addr,size,DISK_WRITE);
    
    int cnt = 0;
    for(;cnt<size;cnt++,buf+=disk->sector_size){
        disk_write_data(disk,buf,disk->sector_size);

        if(task_current()){
            sem_wait(disk->op_sem);
        }

        int err = disk_wait_data(disk);
        if(err < 0){
            break;
        }

    }

    mutex_unlock(disk->mutex);
    return cnt;
}

int disk_control(device_t* dev ,int cmd,int arg0,int arg1){
    return -1;
}

void disk_close(device_t * dev){

}

dev_desc_t dev_disk_desc={
    .name = "disk",
    .major = DEV_DISK,
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .control = disk_control,
    .close = disk_close,

};

void do_handler_ide_primary(exception_frame_t * frame){
    pic_send_eoi(IRQ14_HARDDISK_PRI);

    //既不是loader时加载磁盘文件
    //不是挂载fat16时加载磁盘文件

    //在多进程时才发送信号量
    if(flag == 1 && task_current()){
        sem_notify(&sem);
    }
}
