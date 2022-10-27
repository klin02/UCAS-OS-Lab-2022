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

#define SHELL_BEGIN 20 //分屏，使用下半部分
#define BUF_LEN 50
#define CMD_LEN 10
#define OP_NUM 10
#define OP_LEN 20
char strbuf[BUF_LEN];
int strptr;
char cmdbuf[CMD_LEN];
int cmdptr;
char opbuf[OP_NUM][OP_LEN];
int opptr;
char *argv[OP_NUM];
int argc;
int isbackground; //后台进程

void backspace();
void clear();
void parse();
void ps();
pid_t exec();
void kill();

char oplist[][10]=  {
                    "clear",
                    "ps",
                    "exec",
                    "kill"
                    };
int main(void)
{
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
        printf("> root@UCAS_OS: ");
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
        parse();
        if     (strcmp(cmdbuf,oplist[0])==0) clear();
        else if(strcmp(cmdbuf,oplist[1])==0) ps();
        else if(strcmp(cmdbuf,oplist[2])==0) exec();
        else if(strcmp(cmdbuf,oplist[3])==0) kill();
        else if(cmdbuf[0]=='\0') ;                      //允许直接换行
        else   printf("Error: Unknown Command %s\n",cmdbuf); 
        //printf("%s %s\n",cmdbuf,filebuf);
        // TODO [P3-task1]: ps, exec, kill, clear        
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
    bzero(argv,OP_NUM);
    argc=0;
    isbackground =0;

    int index=0;
    for(;index<strptr;index++)
        if(strbuf[index] == ' ' || strbuf=='\0')
            break;
        else{
            cmdbuf[cmdptr++] = strbuf[index];
        }
    
    int tmpptr=0;
    int space=1; //允许连续空格
    index++;
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