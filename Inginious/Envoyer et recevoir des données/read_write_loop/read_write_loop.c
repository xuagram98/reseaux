#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define STD_IN 0
#define STD_OUT 1


/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(int sfd){

    int err;
    int pret;
    int length;
    struct pollfd fds[2];
    int timeout;

    // Initialisation des buffers
    char* send_buf = (char*) malloc(1024*sizeof(char));
    if(send_buf == NULL){
      fprintf(stderr, "ERROR : fonction malloc()\n");
      return;
    }
    char* rcv_buf = (char*) malloc(1024*sizeof(char));
    if(rcv_buf == NULL){
      fprintf(stderr, "ERROR : fonction malloc()\n");
      free(send_buf);
      return;
    }

    while(1){
        fds[0].fd = sfd;
        fds[0].events = POLLIN;

        fds[1].fd = sfd;
        fds[1].events = POLLOUT;

        timeout = 10000;

        memset((void*) send_buf, 0, 1024);
        memset((void*) rcv_buf, 0, 1024);

        pret = poll(fds, 2, timeout);
        if(fgets(send_buf, 1024, stdin) == NULL){
            free(send_buf);
            free(rcv_buf);
            return;
        }
        length = strlen(send_buf);
        send(sfd, send_buf, length, 0);
        if(pret != 0){
            if(fds[0].revents != 0){
                recv(sfd, rcv_buf, 1024, 0);
            }
            if(fds[1].revents != 0){
                err = write(STD_OUT, rcv_buf, length);
                if(err < 0){
                    free(send_buf);
                    free(rcv_buf);
                    return;
                }
            }
        }

    }

    free(send_buf);
    free(rcv_buf);
    return;
}
