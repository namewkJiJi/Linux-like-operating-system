/**
 * 简单的命令行解释器
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_syscall.h"
#include"main.h"
#include <getopt.h>
#include <sys/file.h>
#include "fs/file.h"
#include "dev/tty.h"


//do_echo
int main(int argc,char** argv){
    if(argc == 1){
        char msg_buf[128];
        fgets(msg_buf,sizeof(msg_buf),stdin);
        msg_buf[sizeof(msg_buf)/sizeof(char) - 1] = '\0';
        puts(msg_buf);
        return 0;
    }


    //完整的echo指令
    int count = 1;
    int ch;



    while((ch = getopt(argc,argv,"n:h")) != -1){
        switch (ch)
        {
        case 'h':
            puts("echo something:echo [-n count] message");
            optind = 1;
            return 0;
        
           case 'n':
                count = atoi(optarg);
                break;

        case '?':
            if (optarg) {
                fprintf(stderr, "Unknown option: -%s\n", optarg);
            }
            optind = 1;
            return -1;

        default:
            break;
        }
    }

    if(optind > argc - 1){
        fprintf(stderr,"message is empty\n");
        optind = 1;
        return -1;
    }

    char * msg = argv[optind];
    for(int i = 0;i < count;i++){
        puts(msg);
    }
    
    optind = 1;
    return 0;
}
