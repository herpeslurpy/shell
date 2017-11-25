#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <glob.h>
#include <fcntl.h>

//colors for fanciness
#define GRN "\x1B[38;5;10m"
#define NRM "\x1B[0m"

char* path;
char* username;
char* homedir;
char* tokens[64]; //tokenized path
char* command[64]; //tokenized command, split args and stuff

char* inputRedirects[64];

void checkthatbitchforerrors(char* msg, int cond){
	if(cond){
		perror(msg);
		exit(0);
	}
}

void findPath(char** env){
	int i = 0;
	while(env[i]){
		if(!strncmp(env[i], "PATH=", 5))
			path = env[i];
		if(!strncmp(env[i], "USER=", 5)) //get the username for the prompt while we're at it
			username = strtok(env[i], "USER=");
		if(!strncmp(env[i], "HOME=", 5))
			homedir = strtok(env[i], "HOME=");
		i++;
	}
}

void tokenizePath(){
	char* tmpDir = strtok(path, ":=");

	int cnt = 0;
	while(tmpDir){
		tokens[cnt] = tmpDir;
		tmpDir = strtok(NULL, ":=");
		cnt++;
	}
}

char* stripNL(char* input){ //Replace the new line with null char
	char* tmp = strstr(input, "\n");
	char* ret = input;
 	ret[tmp-input] = '\0';
	return ret;
}

char* getName(char* input){ //Return a pointer to the char after the last /
	char* ret = strrchr(input, '/')+1;
	return ret;
}

void tryExec(char* input){
	int i = 1;
	while(tokens[i]){
		char* tmpPath = (char*) malloc(1024);
		strcat(tmpPath, tokens[i]);
		strcat(tmpPath, "/");
		strcat(tmpPath, input);
		execv(tmpPath, command);
		tmpPath = ""; i++;
	}
	printf("Command not found\n");
}

void stripArgs(char* toStrip){
	char* tmp = strtok(toStrip, " ");

	char* homeRep;
	int cnt = 0;

	glob_t globules;
	while(tmp){
		glob(tmp, GLOB_NOCHECK | GLOB_TILDE, NULL, &globules);

		int i = 0;
		while(i < globules.gl_pathc){
			command[cnt] = (char*)malloc(64);
			strcpy(command[cnt], globules.gl_pathv[i]);
			cnt++; i++;
		}
		globfree(&globules);
		tmp = strtok(NULL, " ");
	}
	command[cnt] = NULL;
}

char* buildPrompt(){
	char* tmp=malloc(50);
	strcat(tmp, GRN);
	strcat(tmp, username);
	strcat(tmp, "@");
	strcat(tmp, "psh>");
	strcat(tmp, NRM);
	strcat(tmp, "\0");
	return tmp;
}

void process(char* input){
	stripArgs(input);
	if(input[0] == '/'){
		char* name = getName(command[0]);
		execv(command[0], command);
		printf("Command not found\n");
	}else{
		tryExec(command[0]);
	}
}

void forkToProcess(char* input){
	int status;
	int pid = fork();
	if(pid < 0){
		printf("FUCK\n");
		exit(0);
	}
	if(pid == 0){
		process(input);
		exit(0);
	}
	if(pid > 0){
		int r;
		do{
			r = waitpid(-1, &status, WNOHANG);
		}while(r >= 0);
	}
}

void queueueueCommands(char* cmd){
	char* queuedCommands[64];
	queuedCommands[0] = strtok(cmd, ";");
	int i = 1;
	while(queuedCommands[i-1]){
		queuedCommands[i] = strtok(NULL, ";");
		forkToProcess(queuedCommands[i-1]);
		i++;
	}
}

int main(int argc, char **argv){
	extern char** environ;
	findPath(environ);
	tokenizePath();
	int t; size_t maxLen = 128;
	char* in = NULL; char* prompt = buildPrompt();

	char histPath[64]; sprintf(histPath, "/home/%s/.psh_history", username);
	read_history(histPath);

	while(1){
		in = readline(prompt);
		char* expandedHistory[1024];
		history_expand(in, expandedHistory);
		strcpy(in, expandedHistory[0]);
		if(!in){
			break;
		}
		add_history(in);

		int status;
		int pid = fork();

		if(pid < 0){
			printf("FUCK\n");
			exit(0);
		}
		if(pid == 0){
			queueueueCommands(in);
			exit(0);
		}
		if(pid > 0){
			int r;
			do{
				r = waitpid(-1, &status, WNOHANG);
			}while(r >= 0);
		}
	}
	write_history(histPath);
	printf("^D\n");
}
