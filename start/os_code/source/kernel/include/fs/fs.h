/**
 * 文件系统相关接口的实现
 *
 
 */
#ifndef FS_H
#define FS_H

#include"fs/file.h"
#include <sys/stat.h>
#include"tools/list.h"
#include"ipc/mutex.h"
#include "fs/fatfs/fatfs.h"
#include "applib/lib_syscall.h"

#define FS_MP_SIZE  512

struct _fs_t;


//回调函数表
typedef struct _fs_op_t{
    int(*mount)(struct _fs_t * fs,int major,int minor);
    void(*unmount)(struct _fs_t * fs);
    int(*open)(struct _fs_t * fs,const char* path,file_t * file);
    int (*read)(char* buf,int size,file_t * file);
    int (*write)(char* buf,int size,file_t * file);
    void(*close)(file_t * file);
    int(*seek)(file_t * file,uint32_t offset,int dir);
    int(*stat)(file_t *file,struct stat * st);
    int(*ioctl)(file_t *file,int cmd,int arg0,int arg1);

    int (*opendir)(struct _fs_t * fs,const char * name,DIR * dir);
    int (*readdir)(struct _fs_t * fs,DIR* dir,struct dirent * dirent);
    int (*closedir)(struct _fs_t * fs,DIR* dir);
    int (*unlink)(struct _fs_t * fs,const char* path);

}fs_op_t;


typedef enum _fs_type_t{
    FS_FAT16,
    FS_DEVFS,
    
}fs_types_t;

typedef struct _fs_t
{
    fs_op_t * op;
    char mount_piont[FS_MP_SIZE];
    fs_types_t type;
    void * data;//回调函数可能有临时的数据
    
    int dev_id;

    list_node_t node;
    mutex_t * mutex;

    //共用体：一个变量（相同的内存位置）可以存储多个多种类型的数据。
    union {
        fat_t fat_data;
    };

}fs_t;



int path_to_num(const char * path,int * num);
const char * path_next(const char* path);



int sys_open(const char *name, int flags, ...);
int sys_read(int file, char *ptr, int len);
int sys_write(int file, char *ptr, int len);
int sys_lseek(int file, int ptr, int dir);
int sys_close(int file);

int sys_isatty(int file);
int sys_fstat(int file, struct stat *st);

void fs_init(void);

int sys_dup(int file);
int sys_ioctl(int fd,int cmd,int arg0,int arg1);


int sys_opendir(const char* name,DIR* dir);
int sys_readdir(DIR* dir,struct dirent * dirent);
int sys_closedir(DIR* dir);
int sys_unlink(const char* path);


#endif

