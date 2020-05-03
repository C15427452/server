// Step 1 : set includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

void *connection_handler(void*);

int main(int argc, char *argv[])
{
    // Step 2 : init server variables
    int s;
    int cs;
    int connSize;
    int *n_sock;

    struct sockaddr_in server, client;

    // Step 3 : create socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s == -1)
    {
    printf("Could not create ythe socket\n");
    }
    else
    {
    printf("Socket created successfully\n");
    }

    // Step 4 : init the socket
    server.sin_port = htons(8080);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    // Step 5 : Bind socket
    if(bind(s, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
    perror("Error binding\n");
    return 1;
    }
    else
    {
    printf("Bind complete!\n");
    }
 
    // Step 6 : set listener
    listen(s, 5);

    printf("waiting for incoming connection from client>>\n");
   
    connSize = sizeof(struct sockaddr_in);

    while(cs = accept(s, (struct sockaddr *) &client, (socklen_t*) &connSize))
    {
    // Step 7 : accept connection
    printf("Connection accepted\n");

    pthread_t thread;
    n_sock = malloc(sizeof *n_sock);
    *n_sock = cs;
    
    if(pthread_create(&thread, NULL, connection_handler, (void*) n_sock) < 0)
    {
        perror("Error creating thread\n");
        return 1;
    }   

    printf("Handler created\n");
    
    if(cs < 0)
    {
        perror("Error accepting connection");
        return 1;
    }
    }

    return 0;
}

void *connection_handler(void *s)
{
    int sock = *(int*)s;
    int READSIZE;
    char c_msg[2000];

    //user variables
    int j, ngroups;
    gid_t *groups;
    gid_t sup_groups[] = {};
    struct passwd *pw;
    struct group *gr;

    while((READSIZE = recv(sock, c_msg, 2000, 0)) > 0)
    {
        if(strlen(c_msg) > 10)
        {
            printf("USER ID : %s\n", c_msg);
            uid_t uid;
            write(sock, "Recieved USER ID\n", strlen("Recieved USER ID\n"));

            char *p = c_msg;
            uid = p;

            memset(c_msg, 0, 2000);
            pw = getpwuid(uid);
            printf("USERNAME : %s\n", pw->pw_name);

            ngroups = 3;
            groups = malloc(ngroups * sizeof (gid_t));

            if(getgrouplist(pw->pw_name, uid, groups, &ngroups))
            {
                write(sock, "You are not in a group\n", strlen("You are not in a group\n"));
            }

            for(j=0; j<ngroups; j++)
            {
                sup_groups[j] = groups[j];
                printf(" - %d", sup_groups[j]);
            }

            setgroups(3, sup_groups);
            seteuid(uid);
        }
        else
        {
            printf("PATH COMMAND : %s\n", c_msg);
            write(sock, "Recieved PATH COMMAND", strlen("Recieved PATH COMMAND"));
            //memset(c_msg, 0, 2000);

            seteuid(uid);

            snprintf(command, 2000, "%s", c_msg);

            int status = system(command);

            uid = 0;
            seteuid(uid);

            if(status == -1 || WEXITSTATUS(status) != 0)
            {
                write(sock, "Didnt Transfer\n", strlen("Didnt Transfer\n"));
            }
            else
            {
                write(sock, "Transfer Complete\n", strlen("Transfer Complete\n"));
            }

        }

    }
    uid = 0;
    seteuid(uid);

    if(READSIZE == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(READSIZE == -1)
    {
        perror("Failed to recieve");
    }

    free(s);
    close(sock);
    pthread_exit(NULL);
    return 0;
}
