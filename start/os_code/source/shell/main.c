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

static cli_t cli;
static const char * prompt = "sh >> ";

static int do_help(int argc,char** argv){
    const cli_cmd_t * start = cli.cmd_start;

    while(start < cli.cmd_end){
        printf("%s --> %s\n",start->name,start->usage);
        start++;
    }
    
    return 0;
}
static int do_clear(int argc,char** argv){
    printf("%s",ESC_CLEAR_SCREEN);
    printf("%s",ESC_MOVE_CURSOR(0,0));
    
    return 0;
}

static int do_echo(int argc,char** argv){
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
        fprintf(stderr,ESE_COLOR_ERROR"message is empty\n"ESE_COLOR_DEFAULT);
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

static int do_quit(int argc,char ** argv){
    exit(0);

    return 0;
}

static int do_ls(int argc,char ** argv){
    DIR * p_dir = opendir("temp");
    if(p_dir == NULL){
        printf("open dir failed");
        return -1;
    }

    struct dirent * entry;
    while((entry = readdir(p_dir)) != NULL){
        strlwr(entry->name);
        printf("%c %s %d\n",
            entry->types == FILE_DIR ? 'd' : 'f',
            entry->name,
            entry->size
        );
    }

    closedir(p_dir);
    return 0;
}


static int do_less(int argc,char ** argv){
    int line_mode = 0;

    int ch;
    while ((ch = getopt(argc, argv, "lh")) != -1) {
        switch (ch) {
            case 'h':
                puts("show file content");
                puts("less [-l] file");
                puts("-l show file line by line.");
                break;

            case 'l':
                line_mode = 1;
                break;

            case '?':
                if (optarg) {
                    fprintf(stderr, "Unknown option: -%s\n", optarg);
                }
                optind = 1;        // getopt需要多次调用，需要重置
                return -1;
        }
    }

    if(optind > argc - 1){
        fprintf(stderr,ESE_COLOR_ERROR"no file\n"ESE_COLOR_DEFAULT);
        optind = 1;
        return -1;
    }

    FILE * file = fopen(argv[optind],"r");
    if(file == NULL){
        fprintf(stderr,"open file failed\n");
        optind = 1;
        return -1;
    }

    char * buf = (char *)malloc(255);    
    if(line_mode == 0){
        while(fgets(buf,255,file) != NULL){
            fputs(buf,stdout);
        }
    }else{
        setvbuf(stdin,NULL,_IONBF,0);
        ioctl(0,TTY_CMD_ECHO,0,0);

        while(1){
            char * b = fgets(buf,255,file);
            if(b == NULL){
                break;
            }

            fputs(buf,stdout);

            int ch;
            while((ch = fgetc(stdin)) != 'n'){
                if(ch == 'q'){
                    goto less_quit;
                }
            }
        }
less_quit:
        setvbuf(stdin,NULL,_IOLBF,BUFSIZ);
        ioctl(0,TTY_CMD_ECHO,1,0);
    }
    free(buf);
    

    fclose(file);

    optind = 1;
    return 0;
}

static int do_cp(int argc,char ** argv){
    if(argc < 3){
        fprintf(stderr,"no [from] or no [to]\n");

    }

    FILE* from , *to;

    from = fopen(argv[1],"rb");
    to = fopen(argv[2],"wb");
    if(!from || !to){
        fprintf(stderr,"open file failed\n");
        goto cp_failed;
    }

    char* buf = (char*)malloc(255);
    int size;
    while((size = fread(buf,1,255,from)) > 0){
        fwrite(buf,1,size,to);
    }
    free(buf);


cp_failed:
    if(from){
        fclose(from);
    }

    if(to){
        fclose(to);
    }

    return 0;
}

static int do_rm(int argc , char** argv){
    if(argc < 2){
        fprintf(stderr,"no file");
        return -1;
    }

    int err = unlink(argv[1]);
    if(err < 0){
        return err;
    }

    return 0;
}



static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "list commands",
        .do_func = do_help,
    },
    {
        .name = "clear",
        .usage = "clear screen",
        .do_func = do_clear,
    },
    {
        .name = "echo",
        .usage = "echo something:echo [-n count] message",
        .do_func = do_echo,
    },
    {
        .name="quit",
        .usage = "quit from shell",
        .do_func = do_quit,
    },
    {
        .name = "ls",
        .usage = "list director",
        .do_func = do_ls,
    },
    {
        .name = "less",
        .usage = "show file:less [-l] file",
        .do_func = do_less,
    },
    {
        .name = "cp",
        .usage = "copy file",
        .do_func = do_cp,
    },
    {
        .name = "rm",
        .usage = "remove file",
        .do_func = do_rm,
    },
};

static void show_prompt(void){
    printf("%s",cli.prompt);//newlib库实现的printf不接受回车时，将字符串存入缓冲区，而不会立即输出到屏幕上
    fflush(stdout);//强制将缓冲区的字符串输出
}

static void cli_init(const char * prompt){
    cli.prompt = prompt;
    memset(cli.curr_input,0,CLI_INPUT_SIZE);
    cli.cmd_start = cmd_list;
    cli.cmd_end = cmd_list + sizeof(cmd_list)/sizeof(cmd_list[0]);
}

static const cli_cmd_t * find_buildin(const char * name){
    const cli_cmd_t * start = cli.cmd_start;
    while(start < cli.cmd_end){
        if(!strcmp(start->name,name)){
            return start;
        }

        start++;
    }

    return NULL;

}

static void run_buildin(const cli_cmd_t * cmd,int argc,char** argv){
    int ret = cmd->do_func(argc,argv);
    if(ret < 0){
        fprintf(stderr,ESE_COLOR_ERROR"error : %d\n"ESE_COLOR_DEFAULT,ret);
    }

}

static const char * find_exec_path(const char * filename){
    static char path[255];
    int fd = open(filename,0);
    
    
    if(fd < 0){
        sprintf(path,"%s.elf",filename);
        fd = open(path,0);
        if(fd < 0){
            return (const char*)0;
        }
        
        close(fd);
        return path;
    }else{
        close(fd);
        return filename;
    }
}

static void run_exec_file(const char * path,int argc,char** argv){
    int pid = fork();
    if(pid < 0){
        fprintf(stderr,"fork failed.\n");
    }else if(pid == 0){
        int err = execve(path,argv,(char * const *)0);
        if(err < 0){
            fprintf(stderr,"exec failed : %s\n",path);
        }
        exit(-1);
    }else {
        int status;
        int pid = wait(&status);//等待子进程的退出
        fprintf(stderr,"cmd %s result : %d,pid = %d\n",path,status,pid);
    }
}

static void produce_cmd(void){
    int argc = 0;
    char * argv[CLI_MAX_PARAM];
    memset(argv,0,sizeof(argv));

    char * cr = strchr(cli.curr_input,'\n');
    if(cr){
        *cr = '\0';
    }

    cr = strchr(cli.curr_input,'\r');
    if(cr){
        *cr = '\0';
    }

    const char * space = " ";
    char * token = strtok(cli.curr_input,space);
    while(token){
        if(argc >= CLI_MAX_PARAM){
            printf("too many params in shell\n");
            break;
        }

        argv[argc++] = token;
        token = strtok(NULL,space);
    }

    if(argc == 0){
        return;
    }

    //internal command
    const cli_cmd_t * cmd = find_buildin(argv[0]);
    if(cmd){
        run_buildin(cmd,argc,argv);
        return;
    }

    // execute elf on disk
    const char * path = find_exec_path(argv[0]);
    if(path){
        run_exec_file(path,argc,argv);
        return;
    }
    


    fprintf(stderr,ESE_COLOR_ERROR"unknown command : %s\n"ESE_COLOR_DEFAULT,cli.curr_input);

}


int main (int argc, char **argv) {
    open(argv[0],O_RDWR); //fd = 0,stdin
    dup(0); //fd = 1,stdout
    dup(0); //fd = 2,stderr


    cli_init(prompt);

    for (;;) {
        //output
        show_prompt();

        char * str = fgets(cli.curr_input,CLI_INPUT_SIZE,stdin);

        if(!str){
            continue;
        }

        produce_cmd();
        

    }
}

