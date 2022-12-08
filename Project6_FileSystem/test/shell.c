/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#define SHELL_BEGIN 20 //分屏，使用下半部分
#define BUF_LEN 50
#define CMD_LEN 10
#define OP_NUM 10
#define OP_LEN 20
#define DIR_LEVEL 5
#define DIR_LEN 20
char strbuf[BUF_LEN];
int strptr;
char cmdbuf[CMD_LEN];
int cmdptr;
char opbuf[OP_NUM][OP_LEN];
int opptr;
char *argv[OP_NUM];
int argc;
int isbackground; //后台进程
char dir[DIR_LEVEL][DIR_LEN];
int dir_level = 0;
// TODO [P6-task1]: mkfs, statfs, cd, mkdir, rmdir, ls
// TODO [P6-task2]: touch, cat, ln, ls -l, rm
void backspace();
void clear();
void parse();
void ps();
pid_t exec();
void kill();
void taskset();
void mkfs();
void statfs();
void cd();
void mkdir();
void rmdir();
void ls();
void touch();
void cat();
void ln();
void rm();
int split_path(char *path,char str[3][20]);
char oplist[][10]=  {
                    "clear",
                    "ps",
                    "exec",
                    "kill",
                    "taskset",
                    "mkfs",
                    "statfs",
                    "cd",
                    "mkdir",
                    "rmdir",
                    "ls",
                    "touch",
                    "cat",
                    "ln",
                    "rm"
                    };
int main(void)
{

    // for (int i = 0;; i++)
    // {
    //     sys_move_cursor(0, SHELL_BEGIN);
    //     printf("> [TASK] This task is to test scheduler. (%d)", i);
    //     sys_yield();
    // }

    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    //printf("> root@UCAS_OS: ");
    int ch;
    while (1)
    {
        // TODO [P3-task1]: call syscall to read UART port
        //init buf
        bzero(strbuf,BUF_LEN);
        strptr=0;
        printf("> root@UCAS_OS:~");
        for(int i=0;i<dir_level;i++)
            printf("/%s",dir[i]);
        printf("$ ");
        while(1){
            while((ch = sys_getchar())==-1) //;
            {
                // for(int j=0;j<1000000;j++)
                // ;
            }
                //loop until getting a char
            if(ch == '\r'){
                strbuf[strptr++]= '\0';
                printf("\n");
                break;
            }
            else if(ch == 8 || ch == 127){
                if(strptr == 0)
                    continue;
                strbuf[--strptr] = ' ';
                backspace();
            }
            else{
                strbuf[strptr++] = ch;
                printf("%c",ch);
            }
        }

        // TODO [P3-task1]: parse input
        // note: backspace maybe 8('\b') or 127(delete)
        // TODO [P6-task1]: mkfs, statfs, cd, mkdir, rmdir, ls
        // TODO [P6-task2]: touch, cat, ln, ls -l, rm
        parse();
        if     (strcmp(cmdbuf,oplist[ 0])==0) clear();
        else if(strcmp(cmdbuf,oplist[ 1])==0) ps();
        else if(strcmp(cmdbuf,oplist[ 2])==0) exec();
        else if(strcmp(cmdbuf,oplist[ 3])==0) kill();
        else if(strcmp(cmdbuf,oplist[ 4])==0) taskset();
        else if(strcmp(cmdbuf,oplist[ 5])==0) mkfs();
        else if(strcmp(cmdbuf,oplist[ 6])==0) statfs();
        else if(strcmp(cmdbuf,oplist[ 7])==0) cd();
        else if(strcmp(cmdbuf,oplist[ 8])==0) mkdir();
        else if(strcmp(cmdbuf,oplist[ 9])==0) rmdir();
        else if(strcmp(cmdbuf,oplist[10])==0) ls();
        else if(strcmp(cmdbuf,oplist[11])==0) touch();
        else if(strcmp(cmdbuf,oplist[12])==0) cat();
        else if(strcmp(cmdbuf,oplist[13])==0) ln();
        else if(strcmp(cmdbuf,oplist[14])==0) rm();
        else if(cmdbuf[0]=='\0') ;                      //允许直接换行
        else   printf("Error: Unknown Command %s\n",cmdbuf); 
        //printf("%s %s\n",cmdbuf,filebuf);
        // TODO [P3-task1]: ps, exec, kill, clear 
        // sys_yield();       
    }

    return 0;
}

void backspace(){
    sys_backspace();
    sys_reflush();
}
void clear(){
    sys_clear();
    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}
void parse(){
    bzero(cmdbuf,CMD_LEN);
    cmdptr=0;
    for(int i=0;i<OP_NUM;i++)
        bzero(opbuf[i],OP_LEN);
    opptr=0;
    // bzero(argv,OP_NUM);
    for(int i=0;i<OP_NUM;i++)
        argv[i] = NULL;
    argc=0;
    isbackground =0;

    int index=0;
    //跳过开头空格
    for(;index<strptr;index++)
        if(strbuf[index]!=' ')
            break;

    for(;index<strptr;index++)
        if(strbuf[index] == ' ' || strbuf=='\0')
            break;
        else{
            cmdbuf[cmdptr++] = strbuf[index];
        }
    
    int tmpptr=0;
    int space=1; //允许连续空格
    index++;
    //跳过开头空格
    for(;index<strptr;index++)
        if(strbuf[index]!=' ')
            break;

    for(;index<strptr;index++)
    {
        if(strbuf[index] == '\0'){
            if(space==0) opptr++;
            break;            
        }
        else if(strbuf[index]==' ' && space==0) // 之前非空格
        {
            opptr++;
            space=1;
            tmpptr=0;
        }
        else if(strbuf[index]=='&')
        {
            isbackground = 1;
            break;
        }
        else
        {
            space=0;
            opbuf[opptr][tmpptr++] = strbuf[index];   
        }
    }

    for(int i=0;i<opptr;i++)
        argv[argc++] = opbuf[i];

    // printf("len:%d\n",strptr);
    // if(opptr!=0){
    //     //printf("\n");
    //     printf("cmd: %s\n",cmdbuf);
    //     printf("file: %s\n",filebuf);
    //     printf("OPNUM:%d\n",opptr);
    //     for(int i=0;i<opptr;i++)
    //     printf("%s ",opbuf[i]);
    //     printf("\n");
    // }
    //printf("len:%d",strlen(opbuf[0]));
}
void ps() { sys_ps(); }
pid_t exec(){
    pid_t pid;
    //printf("%s %d\n",argv[0],argc);
    if(isbackground==1){
        pid = sys_exec(argv[0],argc,argv);
        if(pid==0)
            printf("Error: %s: no such file\n",argv[0]);
        else 
            printf("Info : execute %s successfully, pid = %d\n",argv[0],pid);
        return pid;
    }
    else
    {
        // printf("!!no back\n");
        pid = sys_exec(argv[0],argc,argv);
        if(pid==0)
            printf("Error: %s: no such file\n",argv[0]);
        else 
            sys_waitpid(pid);
        //printf("wait done\n");
        return pid;
    }
    // while(1) 
    //     sys_yield();
    // printf("end!!!!!");
}
void kill(){
    int killpid = atoi(argv[0]);
    int res = sys_kill(killpid);
    if(res==1)
        printf("Info : kill successfully\n");
    else 
        printf("Error: the pid is no allocated\n");
}

void taskset(){
    char *pstr = "-p";
    if(strcmp(argv[0],pstr)==0 && strlen(argv[1])==3 && argc==3){
        int mask;
        char ch = argv[1][2]; //0x后的第三个字符
        if(ch>='0' && ch<='9')
            mask = (int)(ch-'0');
        else if(ch >='a' && ch <='f')
            mask = (int)(ch-'a'+10);
        else 
            printf("Err!");
        if(mask < 0 || mask > 3)
            printf("Mask is out of range\n");
        pid_t pid = atoi(argv[2]);
        sys_setmask(pid,mask);
    }
    else if(strlen(argv[0])==3 && argc==2){
        int mask;
        char ch = argv[0][2]; //0x后的第三个字符
        if(ch>='0' && ch<='9')
            mask = (int)(ch-'0');
        else if(ch >='a' && ch <='f')
            mask = (int)(ch-'a'+10);
        else 
            printf("Err!");
        if(mask < 0 || mask > 3)
            printf("Mask is out of range\n");
        sys_runmask(argv[1],argc-1,argv+1,mask);
    }
    else{
        printf("Unknown Taskset\n");
    }
}

void mkfs(){sys_mkfs();}
void statfs(){sys_statfs();}
void cd(){
    int status; //最低第i位表示第i个路径作用，1有效，0无效
    int lvl;
    char str[3][20];
    int mask;
    status = sys_cd(argv[0]);
    lvl = split_path(argv[0],str);
    for(int i=0;i<lvl;i++){
        mask = 1<<i;
        if((status & mask) !=0){//work
            if(strcmp(str[i],"..")==0)
                dir_level --;
            else if(strcmp(str[i],".")==0)
                ;
            else
                strcpy(dir[dir_level++],str[i]);
        }
    }
}
void mkdir(){sys_mkdir(argv[0]);}
void rmdir(){sys_rmdir(argv[0]);}
void ls(){
    char *lstr = "-l";
    int option = 0;
    if(strcmp(lstr,argv[0])==0)
        sys_ls(argv[1],1);
    else
        sys_ls(argv[0],0);
}
void touch(){sys_touch(argv[0]);}
void cat(){sys_cat(argv[0]);}
void ln(){sys_ln(argv[0],argv[1]);}
void rm(){sys_rm(argv[0]);}

int split_path(char *path,char str[3][20]){
    int level = 0;
    int ptr = 0;
    for(int i=0;i<level;i++)
            bzero(str[i],20);
    for(int i=0;i<strlen(path);i++){
            if(path[i] == '/'){
                    str[level][ptr] = '\0';
                    ptr = 0;
                    level++;
            }
            else
                    str[level][ptr++] = path[i];
    }
    str[level][ptr] = '\0';
    level++;
    return level;
}