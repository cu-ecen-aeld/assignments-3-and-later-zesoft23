#include <stdio.h>
#include <syslog.h>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#define PORT "9000"                            // the port users will be connecting to
#define BACKLOG 10                             // the port users will be connecting to
#define FILELOCATION "/var/tmp/aesdsocketdata" // location to write data to
#define BUFFER_SIZE 1024

FILE *myfile;
int new_fd;

void sigchld_handler(int s)
{

    if (s == SIGINT)
        printf("SIGINT received. ");
    else if (s == SIGTERM)
        printf("SIGTERM received. ");

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    // Attempt to delete the file
    if (remove(FILELOCATION) == 0)
    {
        printf("File %s deleted successfully.\n", FILELOCATION);
    }
    else
    {
        printf("Unable to delete the file %s.\n", FILELOCATION);
        perror("remove");
    }

    syslog(LOG_DEBUG, "Caught signal, exiting");
    printf("Caught signal, exiting\n");

    errno = saved_errno;

    exit(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// arg 1 is path to file
// arg 2 is string to write to file
int main(void)
{
    int sockfd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    char buf[BUFFER_SIZE];
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    bool packet_finished = false;
    int i;
    int itemswritten;
    int total_size = 0;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // Set the handler for SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // Set the handler for SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1)
    { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        syslog(LOG_DEBUG, "server: got connection from %s\n", s);

        // // Receive via file descriptor until \n
        packet_finished = false;
        FILE *myfile = fopen(FILELOCATION, "a+");

        while (packet_finished != true)
        {

            int received_size = recv(new_fd, buf, sizeof buf, 0);

            if (received_size == -1)
            {
                perror("recv");
                return -1;
            }

            total_size += received_size;

            printf("client: received '%s' of size %d\n", buf, received_size);

            itemswritten = fwrite(buf, sizeof(char), received_size, myfile);

            fflush(myfile);

            printf("wrote %d\n", itemswritten);

            // this may be dumb if we don't have the new line early enough
            for (i = 0; i < received_size; i++)
            {
                if (buf[i] == '\n')
                {
                    packet_finished = true;
                }
            }

        }

        // char *return_buf = malloc(total_size);
        // if (buf == NULL)
        // {
        //     exit(1);
        // }

        char *source = NULL;
        if (myfile != NULL)
        {
            // /* Go to the end of the file. */
            // if (fseek(myfile, 0L, SEEK_END) == 0)
            // {
            //     /* Get the size of the file. */
            //     long bufsize = ftell(myfile);
            //     if (bufsize == -1)
            //     { /* Error */
            //     }

                /* Allocate our buffer to that size. */
                source = malloc(sizeof(char) * (total_size + 1));

                /* Go back to the start of the file. */
                fseek(myfile, 0, SEEK_SET);

                /* Read the entire file into memory. */
                size_t newLen = fread(source, sizeof(char), total_size, myfile);
                if (ferror(myfile) != 0)
                {
                    fputs("Error reading file", stderr);
                }
                else
                {
                    source[newLen++] = '\0'; /* Just to be safe. */
                }
        }

        send(new_fd, source, total_size, 0);

        // printf("numb ytes%ld \n", numbyte_written);

        // if (!fork()) { // this is the child process
        //     close(sockfd); // child doesn't need the listener
        //     if (send(new_fd, "Hello, world!", 13, 0) == -1)
        //         perror("send");
        //     close(new_fd);
        //     exit(0);
        // }
        close(new_fd); // parent doesn't need this
        free(source); /* Don't forget to call free() later! */
        fclose(myfile);


    }
    return 0;
}
