#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <readline/history.h>
#include <readline/readline.h>

//colors for fanciness
#define GRN "\x1B[38;5;10m"
#define NRM "\x1B[0m"

char* path;
char* username;
char* homedir;
char* tokens[64]; //tokenized path
char* command[64]; //tokenized command, split args and stuff

char* inputRedirects[64];

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
		char* tmpPath = (char*) malloc(256);
		strcat(tmpPath, tokens[i]);
		strcat(tmpPath, "/");
		strcat(tmpPath, input);
		printf("trying *%s*\n", tmpPath);
		execv(tmpPath, command);
		tmpPath = ""; i++;
	}
	printf("Command not found\n");
}

void stripArgs(char* toStrip){
	char* tmp = strtok(toStrip, " ");

	char* homeRep;
	int cnt = 0;
	while(tmp){
		if((homeRep = strstr(tmp, "~")) != NULL){
			tmp = strcat(homedir, tmp+1);
			homeRep[0] = '\0';
		}
		command[cnt] = tmp;
//		strcat(command[cnt], "\0");
		tmp = strtok(NULL, " ");
		cnt++;
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
	//inputRedirect(input);
	stripArgs(input);
	if(input[0] == '/'){
		char* name = getName(command[0]);
		execv(command[0], command);
		printf("Command not found\n");
	}else{
		tryExec(command[0]);
	}
}

int main(int argc, char **argv){
	extern char** environ;
	findPath(environ);
	tokenizePath();
	int t; size_t maxLen = 128;
	char* in = NULL; char* prompt = buildPrompt();
	while(1){
		//printf("%s%s@%s%s", GRN, username, prompt, NRM);
		in = readline(prompt);

		if(!in){
			break;
		}
		add_history(in);

		int status;
		int pid = fork()
;
		if(pid < 0){
			printf("FUCK\n");
			exit(0);
		}
		if(pid == 0){
			process(in);
			exit(0);
		}
		if(pid > 0){
			int r;
			do{
				r = waitpid(-1, &status, WNOHANG);
			}while(r >= 0);
		}
	}
	printf("^D\n");
}