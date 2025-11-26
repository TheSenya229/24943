#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



int main(int argc, char const *argv[])
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execlp("cat", "cat", "../README.md", NULL);
    }
    else
    {
        wait(NULL);
        printf("Text.\n");
    }
    return 0;
}
