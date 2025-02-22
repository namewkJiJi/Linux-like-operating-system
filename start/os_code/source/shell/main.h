#ifndef MAIN_H
#define MAIN_H

#define CLI_INPUT_SIZE  1024
#define CLI_MAX_PARAM   10


#define ESC_CMD2(n,cmd)     "\x1b["#n#cmd

#define ESC_CLEAR_SCREEN     ESC_CMD2(2,J)
#define ESE_COLOR_ERROR      ESC_CMD2(31,m)
#define ESE_COLOR_DEFAULT      ESC_CMD2(39,m)


#define ESC_MOVE_CURSOR(row,col)      "\x1b["#row";"#col"H"

typedef struct _cli_cmd_t
{
    const char * name;
    const char * usage;
    int(*do_func)(int argc,char** argv);
}cli_cmd_t;


//command line interface
typedef struct _cli_t
{
    char curr_input[CLI_INPUT_SIZE];
    const cli_cmd_t* cmd_start;
    const cli_cmd_t * cmd_end;
    const char * prompt;
}cli_t;


#endif