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
#include <sys/queue.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#define PORT "9000"                            // the port users will be connecting to
#define BACKLOG 10                             // the port users will be connecting to
#define FILELOCATION "/var/tmp/aesdsocketdata" // location to write data to
#define BUFFER_SIZE 1024

FILE *myfile;
int new_fd;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
timer_t timerid;

struct thread_data
{

    pthread_mutex_t *write_mutex;
    int sock_fd;
    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    bool thread_complete_success;

    // These are just for the timing components
    time_t start_time;
};

struct entry
{
    struct thread_data *thread_data;
    pthread_t *thread_id;
    SLIST_ENTRY(entry)
    entries; // singly linked list
};

SLIST_HEAD(slisthead, entry);

void daemonize()
{
    pid_t pid, sid;

    // Fork off the parent process
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    // Create a new SID for the child process
    sid = setsid();
    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Change the current working directory
    if ((chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void sigchld_handler(int s)
{

    if (s == SIGINT)
        printf("SIGINT received. ");
    else if (s == SIGTERM)
        printf("SIGTERM received. ");
    else if (s == SIGALRM)
        printf("SIGALRM received");

    if (s == SIGALRM)
    {
        time_t current_time;
        struct itimerspec its;
        its.it_value.tv_sec = 10; // 10 seconds
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        timer_settime(timerid, 0, &its, NULL);

        current_time = time(NULL);

        char time_str[128];

        // 0 out the array to end the write
        memset(time_str, 0, sizeof(time_str));
        pthread_mutex_lock(&write_mutex);

        FILE *localfile = fopen(FILELOCATION, "a+");

        strftime(time_str, sizeof(time_str), "timestamp: %a, %d %b %Y %T %z\n", localtime(&current_time));

        fwrite(time_str, sizeof(char), sizeof(time_str), localfile);

        fflush(localfile);

        fclose(localfile);

        pthread_mutex_unlock(&write_mutex);


    }
    else {
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

void *receive_write_send(void *thread_param)
{

    syslog(LOG_DEBUG, "inside of my thread");

    int total_size = 0;
    bool packet_finished = false;
    char buf[BUFFER_SIZE];
    int i;
    struct thread_data *thread_func_args = (struct thread_data *)thread_param;
    // // Receive via file descriptor until \n
    packet_finished = false;

    pthread_mutex_lock(thread_func_args->write_mutex);

    FILE *localfile = fopen(FILELOCATION, "a+");

    // I think I can just spawn after getting a connection, less things to pass thru

    while (packet_finished != true)
    {

        int received_size = recv(thread_func_args->sock_fd, buf, sizeof buf, 0);

        if (received_size == -1)
        {
            perror("recv");
            // return -1;
        }

        total_size += received_size;

        fwrite(buf, sizeof(char), received_size, localfile);

        fflush(localfile);

        for (i = 0; i < received_size; i++)
        {
            if (buf[i] == '\n')
            {
                packet_finished = true;
            }
        }
    }

    printf("received %s\n", buf);

    char *source = NULL;
    long file_size;
    if (localfile != NULL)
    {
        fseek(localfile, 0, SEEK_END);
        file_size = ftell(localfile);
        /* Allocate our buffer to that size. */
        source = malloc(sizeof(char) * (file_size + 1));

        /* Go back to the start of the file. */
        fseek(localfile, 0, SEEK_SET);

        /* Read the entire file into memory. */
        size_t newLen = fread(source, sizeof(char), file_size, localfile);
        if (ferror(localfile) != 0)
        {
            fputs("Error reading file", stderr);
        }
        else
        {
            source[newLen++] = '\0'; /* Just to be safe. */
        }
    }

    // printf("sending %s\n", source);

    send(thread_func_args->sock_fd, source, file_size, 0);

    close(thread_func_args->sock_fd); // parent doesn't need this
    free(source);                     // call free to buffer used for reading the file
    fclose(localfile);                // Something may be messed up here with sending back between threads, but the mutex should deal with it

    pthread_mutex_unlock(thread_func_args->write_mutex);

    thread_func_args->thread_complete_success = true;

    return thread_param;
}

// void *ten_second_timer(void *thread_param)
// {
//     // Influence taken from timer_thread.c in the lecture files
//     // https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/timer_thread.c

//     struct thread_data *thread_func_args = (struct thread_data *)thread_param;
//     // // Receive via file descriptor until \n

//     // double elapsed_time;

//     time_t current_time;

//     // I think I can just spawn after getting a connection, less things to pass thru

//     while (1)
//     {
//         // elapsed_time = difftime(current_time, thread_func_args->start_time);

//         sleep(10);
//         current_time = time(NULL);

//         char time_str[128];

//         // 0 out the array to end the write
//         memset(time_str, 0, sizeof(time_str));
//         pthread_mutex_lock(thread_func_args->write_mutex);

//         FILE *localfile = fopen(FILELOCATION, "a+");

//         strftime(time_str, sizeof(time_str), "timestamp: %a, %d %b %Y %T %z\n", localtime(&current_time));

//         fwrite(time_str, sizeof(char), sizeof(time_str), localfile);

//         fflush(localfile);

//         fclose(localfile);

//         pthread_mutex_unlock(thread_func_args->write_mutex);

//         thread_func_args->start_time = time(NULL);
//     }

//     return thread_param;
// }

// arg 1 is path to file
// arg 2 is string to write to file
int main(int argc, char *argv[])
{
    time_t start_time = time(NULL);
    int daemonflag = 0;
    int sockfd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // singly list inits
    struct slisthead head;
    SLIST_INIT(&head); /* Initialize the queue */

    int c;
    // Get the args -d arg if it exists
    while ((c = getopt(argc, argv, "d")) != -1)
        switch (c)
        {
        case 'd':
            daemonflag = 1;
            break;
        default:
            abort();
        }

    if (daemonflag)
    {
        daemonize();
    }

    // Create the timer thread
    pthread_t p_thread;
    struct thread_data *thread_param = malloc(sizeof(struct thread_data));
    thread_param->write_mutex = &write_mutex;
    thread_param->start_time = start_time;
    thread_param->thread_complete_success = false;
    thread_param->sock_fd = 0;

    // timer initilization
    int clock_id = CLOCK_MONOTONIC;
    bool timer_started = false;
    struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(clock_id, &sev, &timerid) != 0)
    {
        printf("Error %d (%s) creating timer!\n", errno, strerror(errno));
    }

    // pthread_create(&p_thread, NULL,
    //                &ten_second_timer, thread_param);

    struct entry *thread_linked_list_entry = malloc(sizeof(struct entry));
    thread_linked_list_entry->thread_data = thread_param;
    thread_linked_list_entry->thread_id = &p_thread;

    SLIST_INSERT_HEAD(&head, thread_linked_list_entry, entries);

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

    // Set the handler for SIGALRM
    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("sigaction");
        // exit(1);
    }

    printf("server: waiting for connections...\n");

    struct entry *np; // used for iteration

    while (1)
    { // main accept() loop
        sin_size = sizeof their_addr;

        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        if (!timer_started) {
            struct itimerspec its;
            // Set up timer expiration time
            its.it_value.tv_sec = 10; // 10 seconds
            its.it_value.tv_nsec = 0;
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;
            timer_settime(timerid, 0, &its, NULL);
            timer_started = true;
        }

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

        struct thread_data *thread_param = malloc(sizeof(struct thread_data));

        pthread_t p_thread;
        thread_param->write_mutex = &write_mutex;
        thread_param->sock_fd = new_fd;
        thread_param->thread_complete_success = false;

        // Put some things in the list
        struct entry *thread_linked_list_entry = malloc(sizeof(struct entry));
        thread_linked_list_entry->thread_data = thread_param;
        thread_linked_list_entry->thread_id = &p_thread;

        syslog(LOG_DEBUG, "server: created thread");

        pthread_create(&p_thread, NULL, &receive_write_send, thread_param);

        SLIST_INSERT_HEAD(&head, thread_linked_list_entry, entries);

        // janky and doesn't assumes first in first out until it can join
        np = SLIST_FIRST(&head);
        if (np->thread_data->thread_complete_success)
        {
            pthread_join(*(np->thread_id), NULL);
            SLIST_REMOVE_HEAD(&head, entries);
            free(np->thread_data);
            free(np);
        }
    }

    return rv;
}
