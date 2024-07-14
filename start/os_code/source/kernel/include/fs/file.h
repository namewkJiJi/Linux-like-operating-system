#ifndef FILE_H
#define FILE_H

#include"comm/types.h"

#define FILE_NAME_SIZE  32
#define FILE_TABLE_SIZE 2048

typedef enum _file_type_t {
    FILE_UNKNOWN = 0,
    FILE_TTY = 1,
    FILE_DIR,
    FILE_NORMAL,
    
} file_type_t;

//不包含头文件，避免循环引用
struct _fs_t;

typedef struct _file_t
{
    char file_name[FILE_NAME_SIZE];
    file_type_t type;
    uint32_t size;
    int ref; // 打开次数
    int dev_id;
    int pos;
    int mode;
    struct _fs_t * fs;
    
    int index;  //index in dir
    int sblk; //start blk
    int cblk; //current blk
}file_t;

file_t * file_alloc (void) ;
void file_free (file_t * file);
void file_table_init (void);


#endif