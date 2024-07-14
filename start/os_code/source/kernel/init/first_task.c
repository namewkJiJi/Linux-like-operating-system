/**
 * 内核初始化以及测试代码
 *
 
 */
#include "applib/lib_syscall.h"
#include "dev/tty.h"

int first_task_main (void) {
    // int count = 3;

    // int pid = getpid();
    // print_msg("first task id=%d", pid);

    // pid = fork();
    // if (pid < 0) {
    //     print_msg("create child proc failed.", 0);
    // } else if (pid == 0) {
    //     print_msg("child: %d", count);

    //     char * argv[] = {"arg0", "arg1", "arg2", "arg3"};
    //     execve("/shell.elf", argv, (char **)0);
    // } else {
    //     print_msg("child task id=%d", pid);
    //     print_msg("parent: %d", count);
    // }

    // pid = getpid();


    for(int i = 0;i< TTY_NR;i++){
        int pid = fork();
        if(pid <0){
            print_msg("create shell failed.",0);
            break;
        }else if (pid == 0){
            char tty_num[]= "/dev/tty?";
            tty_num[sizeof(tty_num)-2] = i+'0';
            char* argv[] = {tty_num,(char*)0};
            execve("shell.elf", argv, (char **)0);
            while(1){
                msleep(1000);
            }
        }
    }



    for (;;) {
        // print_msg("task id = %d", pid);
        // 收容孤儿进程，释放资源
        int status;
        wait(&status);



        // msleep(1000);
    }

    return 0;
} 