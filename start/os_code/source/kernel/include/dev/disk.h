#ifndef DISK_H
#define DISK_H

#include"comm/types.h"
#include "ipc/mutex.h"
#include "ipc/sem.h"

#define PARTINFO_NAME_SIZE  32
#define DISK_NAME_SIZE      32
#define PARTINFO_NR         (4+1)//为什么是五个
#define DISK_NR             2//磁盘数量
#define DISK_NR_PER_CHANNEL 2
#define MBR_PART_ITEM_NR    4


#define IOBASE_PRIMARY  0x1F0
#define DISK_DATA(disk) (disk->port_base + 0)   
#define DISK_ERROR(disk) (disk->port_base + 1)
#define DISK_SECTOR_NR(disk) (disk->port_base + 2)   
#define DISK_LBA_LOW(disk) (disk->port_base + 3)   
#define DISK_LBA_MID(disk) (disk->port_base + 4)   
#define DISK_LBA_HIGH(disk) (disk->port_base + 5)   
#define DISK_DRIVE(disk) (disk->port_base + 6)   
#define DISK_STATUS(disk) (disk->port_base + 7)   
#define DISK_CMD(disk) (disk->port_base + 7)   

#define DISK_INDENTIFY  0xEC
#define DISK_READ   0x24
#define DISK_WRITE  0x34

#define DISK_ERR    (1<<0)
#define DISK_DRQ    (1<<3)
#define DISK_DF     (1<<5)
#define DISK_BUSY   (1<<7)

#define DISK_DRIVE_BASE 0xE0

#pragma pack(1)
//分区信息表
typedef struct _part_item_t
{
    uint8_t boot_active;
    uint8_t start_header;
    uint16_t start_;
    uint8_t system_id;
    uint8_t end_header;
    uint16_t end_;
    uint32_t relative_sectors;
    uint32_t total_sectors;
}part_item_t;


//MBR:LBA格式下磁盘的第一个扇区
typedef struct _mbr_t{
    uint8_t code[446];
    part_item_t part_item[MBR_PART_ITEM_NR];
    uint8_t boot_sig[2];
}mbr_t;
#pragma pack()


struct _dist_t;
//分区信息
typedef struct _partinfo_t{
    char name[PARTINFO_NAME_SIZE];
    struct _disk_t * disk;
    int start_sector;
    int sector_count;

    enum{
        FS_INVALID = 0x00,
        FS_FAT16_0 = 0x6,
        FS_FAT16_1 = 0xE,
    }type;

}partinfo_t;

//代表磁盘
typedef struct _dist_t{
    char name[DISK_NAME_SIZE];

    enum{
        DISK_MASTER = (0<<4),
        DISK_SLAVE = (1<<4),
    }drive;

    uint16_t port_base;

    int sector_size;//512字节
    int sector_count;//扇区数量
    partinfo_t partinfo[PARTINFO_NR];

    mutex_t * mutex;
    sem_t * op_sem;
}disk_t;


void disk_init(void);

#endif