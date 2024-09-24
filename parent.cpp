#include <unistd.h>     // fork, execve, pipe, dup2
#include <sys/wait.h>   // waitpid
#include <fcntl.h>      // open, O_CREAT, O_WRONLY
#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit
#include <sys/types.h>  // pid_t
#include <string>       // std::string
#include <cstring>      // strlen, strcspn
#include <iostream>     // std::cout, std::cin

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        char* argv[] = {"./child", NULL};  // Программа для execve
        execve("./child", NULL, NULL);

        perror("Ошибка execve");
        exit(1);
    } else {
        std::cout << "Parent process!\n";
    }
    return 0;
}