#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define true 1
#define false 0
#define maxlen 512 //the define max lensize per line
#define debug() printf("debug\n");exit(0);
#define doit() puts("3");exit(0);

//report the error
void report_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
//read a string line from input
char *read_cmd(){
	char *buffer=(char *)malloc(sizeof(char)*maxlen);
	if(!buffer){
		report_error();
	}
	memset(buffer,0,sizeof(buffer));
	if(fgets(buffer,maxlen,stdin)==NULL) return NULL;
	if(strlen(buffer)>maxlen) report_error();
	return buffer;
}
//split the string into serveal commands
char **split_cmd(int *cnt,int *pipepos,int *redpos,char *cmd){
	*cnt=0;
	int len=strlen(cmd),i=0,j=0,pos=0,c,flag=0;
	char **ret=(char **)malloc(sizeof(char *)*maxlen);
	if(!ret){
		report_error();
	}
	while(cmd[i]){
		pos=0;
		ret[*cnt]=(char *)malloc(sizeof(char)*maxlen);
		if(!ret[*cnt]){
			report_error();
		}
		while(cmd[i]!=' '&&cmd[i]!='\n'&&cmd[i]!='>'
			&&cmd[i]!='|'&&cmd[i]!='&'&&cmd[i]!='$'){
			ret[*cnt][pos++]=cmd[i++];
		}
		if(pos>0){
			(*cnt)++;
		}
		if(cmd[i]=='>'||cmd[i]=='&'||cmd[i]=='$'||cmd[i]=='|'){
			ret[*cnt]=(char *)malloc(sizeof(char)*maxlen);
			if(!ret[*cnt]){
				report_error();
			}
			if(cmd[i]=='|') *pipepos=i;
			if(cmd[i]=='>'&&(*redpos)==-1) *redpos=i;
			ret[(*cnt)++][0]=cmd[i];
		}
		//move position
		i++;
	}
	ret[*cnt]=NULL;
	return ret;
}
//pipe by myself
void myPipe(int cnt,int pipepos,char **cmd){
	int fd[2];
	if(pipe(fd)<0){
		report_error();
		return ;
	}
	int lastpos=pipepos-1;
	while(lastpos>=0&&strcmp(cmd[lastpos],"|")) lastpos--;
	int pid=fork();
	if(pid<0){
		report_error();
	}
	else if(pid==0){
		dup2(fd[1],STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		if(lastpos>=0){
			myPipe(lastpos,pipepos,cmd);
		}
		else{
			cmd[pipepos]=NULL;
			if(execvp(cmd[0],cmd)==-1){
				report_error();
			}
		}
	}
	else{
		cmd[cnt-1]=NULL;
		dup2(fd[0],STDIN_FILENO);
		close(fd[0]);
		close(fd[1]);
		while(wait(NULL)>0);
		if(execvp(cmd[pipepos+1],&cmd[pipepos+1])==-1){
			report_error();
		}
	}
}
//function to redirection
void Redirection(char *s,char *str){
	FILE *fp=fopen(s,"w");
	if(fp==NULL) report_error();
	if(strlen(str)!=0) fprintf(fp, "%s\n",str);
	if(fclose(fp)!=0) report_error();
}
//function to change directory
void Change_Directory(char *s){
	if(chdir(s)==-1){
		report_error();
	}
}
int Set_Flag(int *cnt,char **s){
	int flag=0;
	while((*cnt)>0&&strcmp(s[(*cnt)-1],"&")==0){
		s[--(*cnt)]=NULL;
		flag=1;
	}
	return flag;
}
//execute with command strings
void Execute_Command(int *n,int *pipepos,int *redpos,char **cmd){
	int cnt=*n,fork_flag=Set_Flag(&cnt,cmd),status,fd[2];
	if(strcmp(cmd[0],"cd")==0){
		if(cnt==1){
			Change_Directory(getenv("HOME"));
		}
		else if(cnt==2){
			Change_Directory(cmd[1]);
		}
		else if(cnt==3&&strcmp(cmd[1],">")==0){
			Redirection(cmd[2],"");
			Change_Directory(getenv("HOME"));
		}
		else if(cnt==4&&strcmp(cmd[2],">")==0){
			Redirection(cmd[3],"");
			Change_Directory(cmd[1]);
		}
		else report_error();
		return ;
	}
	int pid=fork();
	if(pid<0){
		report_error();
	}
	else if(pid==0){
		if((*pipepos)>=0){
			doit();
			if((*redpos)>=0){
				if(cnt-(*redpos)!=2) report_error();
				else{
					close(STDOUT_FILENO);
                    int fd = open(cmd[(*redpos) + 1],O_CREAT|O_TRUNC|O_WRONLY,(S_IRWXU^S_IXUSR)|S_IRGRP|S_IROTH);
                    if (fd == -1) {
                    	report_error();
                        exit(0);
                    }
                    cmd[*redpos]=NULL;
                    myPipe(*pipepos,cnt,cmd);
                    exit(0);
				}
			}
			else{
				myPipe(*pipepos,cnt,cmd);
			}
			exit(0);
		}
		if(strcmp(cmd[0],"exit")==0){
			if(cnt==3&&strcmp(cmd[1],">")==0){
				Redirection(cmd[2],"");
			}
			exit(0);
		}
		else if(strcmp(cmd[0],"pwd")==0){
			getcwd(cmd[0],maxlen);
			if(cnt==1){
				printf("%s\n",cmd[0]);
			}
			else if(cnt==3&&strcmp(cmd[1],">")==0){
				Redirection(cmd[2],cmd[0]);
			}
		}
		else if(strcmp(cmd[0],">")==0){
			if(cnt==2){
				Redirection(cmd[1],"");
			}
		}
		else if(strcmp(cmd[0],"wait")==0){
			exit(0);
		}
		else{
			if(execvp(cmd[0],cmd)==-1){
				report_error();
			}
		}
		exit(0);
	}
	else{
		if(!fork_flag)//no need to wait
			waitpid(pid,NULL,0);
        if(strcmp(cmd[0],"wait")==0){
            if(cnt!=1){
            	report_error();
            }
            wait(NULL);
        }
        else if(strcmp(cmd[0],"exit")==0){
        	exit(0);
        }
	}
}

int main(int argc, char const *argv[])
{
	int fd,flag=0,i=0;
	char *input_line;//contain the input string
	char **cmd;//contain the seperate command string
	FILE *fp=NULL;
	if(argc>1){
		for(i=1;i<argc;i++){
			fd=open(argv[i],O_RDWR|O_CREAT,0777);
			if(fd==-1){
				report_error();
			}
			else{
				dup2(fd,STDIN_FILENO);
				flag=1;
			}
		}
	}
	while(true){
		//to show like a shell
		if(!flag) printf("mysh> ");
		//get the input command string
		int pipepos=-1,cnt=0,redpos=-1;
		input_line=read_cmd();
		//empty file, so break
		if(input_line==NULL){
			break;
		}//empty line, so continue
		else if(strcmp(input_line,"\n")==0){
			continue;
		}
		//spilt the command stirng
		cmd=split_cmd(&cnt,&pipepos,&redpos,input_line);
		//execute the command	
		Execute_Command(&cnt,&pipepos,&redpos,cmd);
	}
	return EXIT_SUCCESS;
}
