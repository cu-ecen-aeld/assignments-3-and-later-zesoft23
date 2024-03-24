#include <stdio.h>
#include <syslog.h>



// arg 1 is path to file
// arg 2 is string to write to file
int main(int argc, char **argv)
{
    if (argc < 3) {
        syslog(LOG_ERR, "Too few args passed");
        printf("Too few args passed\n");
        printf("Usage: %s filepath writestring\n", argv[0]);
        return 1;
    }
    openlog("writer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

    FILE* myfile = fopen(argv[1], "w");
    fprintf(myfile, argv[2]);
    syslog(LOG_DEBUG, "Writing %s to %s\n", argv[1], argv[2]);
    fclose(myfile);

    closelog();
    return 0;
}

