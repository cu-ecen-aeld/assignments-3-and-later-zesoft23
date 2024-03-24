#include <stdio.h>
#include <syslog.h>

// arg 1 is path to file
// arg 2 is string to write to file
int main(int argc, char **argv)
{
    openlog(NULL, 0, LOG_USER);

    if (argc < 3)
    {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
        printf("Too few args passed\n");
        printf("Usage: %s filepath writestring\n", argv[0]);
        return 1;
    }

    FILE *myfile = fopen(argv[1], "w");
    fprintf(myfile, argv[2]);
    syslog(LOG_DEBUG, "Writing %s to %s", argv[1], argv[2]);
    fclose(myfile);

    closelog();
    return 0;
}
