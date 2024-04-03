#include "systemcalls.h"
#include "stdlib.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

    int return_code = system(cmd);

    if (return_code != 0) {
        return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    char * commandargs[count];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        if (i > 0) {
            commandargs[i - 1] = va_arg(args, char *);
        }
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    int status;
    pid_t pid;

    // if the command does not start with "/" it's not absolute
    if (command[0][0] != '/') {
        return false;
    }

    pid = fork();
    if (pid == -1) {
        return false;
    } else if (pid == 0) {
        execv(command[0], commandargs);
    }

    waitpid(pid, &status, 0);
    printf("my pid value %d\n", status);
/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    va_end(args);

    if (status == 0) {
        return true;
    } else {
        return false;
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    char * commandargs[count];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        if (i > 0) {
            // commandargs[i - 1] = va_arg(args, char *);
        }
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    if (command[0][0] != '/') {
        printf("command is not absolute");
        return false;
    }



    int status;
    pid_t pid;

    fflush(stdout);
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return false;
    } else if (pid == 0) {
        // open the file for stdout redirection
        int fd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC);
        // fflush(stdout);
        int dup_value = dup2(fd, 1);
        close(fd);
        if (dup_value < 0) {
            perror("dup2");
            return false;
        }
        printf("hello i am hrere....\n\n");
        status = execv(command[0], commandargs);
        printf("and not here....\n\n");

    }

    waitpid(pid, &status, 0);


    va_end(args);

    if (status == 0) {
        return true;
    } else {
        return false;
    }
}
