/*********************************************************
ash              (Aditya SHell)
@author      :   Aditya Raj Gupta
@date        :   3/04/2020

**********************************************************/


#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<string.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

#define PR printf(RESET)
#define PB printf(BLUE)
#define PG printf(GREEN)
#define PY printf(YELLOW)
#define PRD printf(RED)
#define PC printf(CYAN)
#define PM printf(MAGENTA)

#define MEMORY_SIZE 100
#define BUILT_IN_s  6
#define BUFFER_SIZE 1024
#define HISTORY_LIMIT 10

#define MIN(x,y)	(((x)<(y))?(x):(y))

int number_of_commands=0;
pid_t *pids;
char *ex_commands[HISTORY_LIMIT];
int current_count=0;
int start=0;

void print(char *str,const char *color)
{
	char res[100];
	strcpy(res,color);
	strcat(res,str);
	strcat(res,RESET);
	printf("%s",res);
	return;
}


char *builtin_commands[BUILT_IN_s] =
{
	"cd"   ,
	"history",
	"pwd",
	"help" ,
	"echo",
	"exit"
};


int ash_help()
{
	int i;
	PB;printf("\t\tPrimitive Linux shell implementation \n");PR;
	PB;printf("\t\tAvailable internal commands\n");PR;
	for(i=0;i<BUILT_IN_s;i++)
	{
		if(i%3==0)
			printf("\n");
		PY;printf("%-14s",builtin_commands[i]);PR;
	}
	PB;printf("\n'echo' supports only the switch '-n'\n");PR;
	PB;
	printf("To execute multiple commands in a single line, you can use ;,||,&& to join them.\nHowever, please use space on both sides of the joining symbol.\nEg: command1 ; command2");
	PR;
	printf("\n");
	return 1;
}

void ash_history()
{
	if(current_count!=start)
	{
		int k=start-1;
		while(k>=0)
		{
			printf("%s\n",ex_commands[k]);
			k--;
		}
		k=HISTORY_LIMIT-1;
		while(k>=start)
		{
			printf("%s\n",ex_commands[k]);
			k--;
		}
	}
	else
	{
		int j=start-1;
		while(j>=0)
		{
			printf("%s\n",ex_commands[j]);
			j--;
		}
	}
}

int ash_pwd()
{
	long size;
	char *buf;
	char *ptr;
	size=pathconf(".",_PC_PATH_MAX);
	if(buf=(char *)malloc((size_t)size))
	{
		ptr=getcwd(buf,(size_t)size);
		if(ptr)
		{
			PY;printf("%s",buf);PR;
			free(buf);
			return 1;
		}
		else
		{
			perror("getcwd : ");
			return 0;
		}
	}
	else
	{
		perror("malloc : ");
		return 0;
	}	
}

int ash_cd(char *args)
{
	if(!args)
		return 1;
	if(chdir(args)==-1)
	{
		perror("cd in ash ");
	}
	return 1;
}


int ash_echo(char *arg,int flag)
{
	if(arg[0]=='$')										//if the user wishes to print the environment variables
	{
		char *buf=(char *)malloc((size_t)1024);
		if(buf)
		{
			int i;
			for(i=1;i<sizeof(arg);i++)
			{
				buf[i-1]=arg[i];
			}
			char *env;
			env=getenv(buf);
			if(env)
			{
				PY;printf("%s\n",env);PR;
				free(buf);
				// free(env);			//cannot free env because getenv returns pointer to the path variable, freeing it will remove the variable which is not allowed
				return 1;
			}
			else
			{
				perror("getenv ");
				return 0;
			}
		}
		else
		{
			perror("malloc ");
			return 0;
		}
	}
	else								//if the user wishes to print some normal strings
	{
		if(!flag)
		{
			PY;printf("%s\n",arg);PR;
		}
		else
		{
			PY;printf("%s",arg);PR;			
		}
		return 1;
	}
}

int ash_exit()							//self explanatory
{
	PG;printf("Exiting ash\n");PR;
	exit(0);	
}

char * readinput()						
{
	char *buf = (char *)malloc(sizeof(char)*BUFFER_SIZE);
	int c;
	int position=0,curr_size=BUFFER_SIZE;
	while(1)
	{
		c=getchar();
		if(c==EOF || c=='\n')
		{
			buf[position]='\0';
			return buf;
		}
		if(position>=curr_size)
		{
			curr_size+=BUFFER_SIZE;
			if(!(buf=realloc(buf,curr_size)))
			{
				perror("realloc ");
				return buf;
			}			
		}
		buf[position++]=c;
	}
}

char ** parser(char *input)
{
	int i=0,row=0,col=0;
	char ch;
	char **commands=(char **)malloc(sizeof(char *)* BUFFER_SIZE);
	for(i=0;i<BUFFER_SIZE;i++)
		commands[i]=(char *)malloc(sizeof(char)*BUFFER_SIZE);
	i=0;
	int inverted_comma=0,space=0;
	while((ch=input[i++])!='\0')				//this loop separates out the command from the given input
	{
		int j=i-1;
		if(!inverted_comma)
		{	
			if(ch==';')
			{
				commands[row][col]='\0';
				row++;
				i++;
				col=0;
			}
			else if(ch=='&' && input[i]=='&')	//command ending with && are appended with 'x'
			{

				commands[row][col++]='x';
				commands[row][col]='\0';
				i+=2;
				row++;
				col=0;
			}
			else if(ch=='|' && input[i]=='|')	//command ending with || are appened with 'y'
			{
				commands[row][col++]='y';
				commands[row][col++]='\0';
				i+=2;
				row++;
				col=0;
			}
			else if(ch=='"')
			{
				inverted_comma=1;
				commands[row][col++]=ch;	
			}
			else
			{
				commands[row][col++]=ch;					
			}
		}
		else
		{
			if(ch=='"')
			{
				inverted_comma=0;
			}	
			commands[row][col++]=ch;
		}	
	}
	commands[row][col]='\0' ;
	int x=0;
	number_of_commands=row+1;
	return commands;
}	

void executioner(char **commands)
{
	int itr;
	for(itr=0;itr<number_of_commands;itr++)
	{	
		int inverted_comma=0,i,num_fields=0,and=0,or=0,isecho=0;
		long int curr_command_size=strlen(commands[itr]);
		int *splits=(int *)malloc(sizeof(int)*curr_command_size);
		memset(splits,0,sizeof(int)*curr_command_size);
		if(curr_command_size>2)											//this section checks for 'or' , 'and' in the command
		{	
			if(commands[itr][curr_command_size-2]==' ')
			{
				if(curr_command_size>=6)
				{	
					if(commands[itr][0]=='e'&&commands[itr][1]=='c'&&commands[itr][2]=='h'&&commands[itr][3]=='o'&&commands[itr][4]==' ')
						isecho=1;
				}	
				if(!isecho)
				{	
					if(commands[itr][curr_command_size-1]=='x')
					{
						and=1;
						curr_command_size-=2;
					}	
					else if(commands[itr][curr_command_size-1]=='y')
					{
						or=1;
						curr_command_size-=2;
					}

				}	 
			}
		}

		//this section removes trailing spaces if any
		int remove_spaces=curr_command_size-1;
		while(commands[itr][remove_spaces--]==' ')
			curr_command_size--;
		//
		
		for(i=0;i<curr_command_size;i++)			//this loop marks the positions of the command that separates it into arguments
		{
			if(commands[itr][i]=='\\')
			{
				i++;
				continue;
			}
			if(inverted_comma)
			{	
				if(commands[itr][i]=='"')
					inverted_comma=0;
			}
			else
			{
				if(commands[itr][i]=='"')
					inverted_comma=1;	
			}
			if(!inverted_comma)
			{

				if(commands[itr][i]==' ')
				{	
					num_fields++;
					splits[i]=1;
				}	
			}
		}
		num_fields+=2;
		char **args=(char **)malloc(sizeof(char *)*(num_fields));
		for(i=0;i<num_fields;i++)
			args[i]=(char *)malloc(sizeof(char)*50);
		args[num_fields-1]=NULL;
		int row=0,col=0;
		for(i=0;i<curr_command_size;i++)				//this loop transforms the given command into a 2d null terminated array of arguments to be passed to execvp
		{
			if(splits[i])
			{
				args[row][col]='\0';
				row++;
				col=0;
			}
			else
			{
				args[row][col++]=commands[itr][i];
			}
		}
		args[row][col]='\0';
		int flag=1;
		
		//this section keeps a track of all commands issued
		strcpy(ex_commands[start],args[0]);
		start=(start+1)%HISTORY_LIMIT;
		current_count=MIN(HISTORY_LIMIT,current_count+1);
		//

		for(i=0;i<BUILT_IN_s;i++)											//this loop handles the case of internal commands
		{
			if(!strcmp(args[0],builtin_commands[i]))
			{

					flag=0;
					if(!strcmp(builtin_commands[i],"cd"))
					{
						if(!ash_cd(args[1]));
						{
						}	
					}
					if(!strcmp(builtin_commands[i],"pwd"))
					{
						if(!ash_pwd())
						{
							printf("Unable to get current directory\n");

						}
						printf("\n");
					}
					if(!strcmp(builtin_commands[i],"help"))
					{
						ash_help();
					}
					if(!strcmp(builtin_commands[i],"exit"))
					{
						exit(0);
					}	
					if(!strcmp(builtin_commands[i],"history"))
					{
						ash_history();
					}
					if(!strcmp(builtin_commands[i],"echo"))
					{
						char *for_echo=(char *)malloc(sizeof(char)*1024);

						if(!strcmp(args[1],"-n"))
						{
							int it,sth=0;
							for(it=8;it<curr_command_size;it++)
								for_echo[sth++]=commands[itr][it];
							for_echo[sth]='\0';
							ash_echo(for_echo,1);
						}
						else
						{
							int it,sth=0;
							for(it=5;it<curr_command_size;it++)
								for_echo[sth++]=commands[itr][it];
							for_echo[sth]='\0';
							ash_echo(for_echo,0);

						}
					}						 
							
			}
		}
		if(flag)										//this section handles the external commands
		{	
			pid_t pid=fork();
			if(pid==0)
			{
					execvp(args[0],args);
					printf("Unable to execute %s\n",args[0]);
			}
			else
			{
				int status;
				waitpid(pid,&status,0);
				if(or)
				{
					int es=WEXITSTATUS(status);				
					if(es==0)						//if the program is successful then we can skip the next command
						itr++;
				}
				else if(and)
				{
					int es=WEXITSTATUS(status);		//if the program fails then we can skip the next command	
					if(es!=0)
						itr++;	
				}
			}
		}

	}	
}


void Ash_loop()
{
	for(int i=0;i<HISTORY_LIMIT;i++)
		ex_commands[i]=(char *)malloc(sizeof(char)*50);
	while(1)
	{
		ash_pwd();
		printf("$ ");
		char *line=readinput();
		char **commands = parser(line);
		executioner(commands);
	}

}



int main(int argc, char const *argv[])
{
	Ash_loop();
	return 0;
}