#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){

  int err; // Variable pour error check

  // Création du socket
  int socketfd = socket(AF_INET6, SOCK_DGRAM, 0);
  if(socketfd < 0){
    fprintf(stderr, "ERROR : fonction socket()\n");
    perror("socket");
    return -1;
  }

  // Liaison à la source
  if(source_addr != NULL && src_port > 0){

    struct addrinfo *hints = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
    if(hints == NULL){
      fprintf(stderr, "ERROR : fonction calloc()\n");
      return -1;
    }
    hints->ai_family = AF_INET6;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = 0;

    struct addrinfo *res;

    err = getaddrinfo("::1", NULL, hints, &res);
    if(err != 0){
      perror("getaddrinfo");
      free(hints);
      return -1;
    }

    source_addr->sin6_port = htons(src_port); // Utilisation de htons pour convertir en Network byte order
    err = bind(socketfd, (struct sockaddr*) source_addr, res->ai_addrlen);
    // Error check
    if(err != 0){
      fprintf(stderr, "ERROR : fonction bind() source\n");
      perror("bind");
      return -1;
    }
  }

  // Liaison à la destination

  struct addrinfo *hints = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
  if(hints == NULL){
    fprintf(stderr, "ERROR : fonction calloc()\n");
    return -1;
  }
  hints->ai_family = AF_INET6;
  hints->ai_socktype = SOCK_DGRAM;
  hints->ai_protocol = 0;

  struct addrinfo *res;

  err = getaddrinfo("::1", NULL, hints, &res);
  if(err != 0){
    perror("getaddrinfo");
    free(hints);
    return -1;
  }

  if(dest_addr != NULL && dst_port > 0){
    dest_addr->sin6_port = htons(dst_port); // Utilisation de htons pour convertir en Network byte order
    err = connect(socketfd, (struct sockaddr*) dest_addr, res->ai_addrlen);
    // Error check
    if(err != 0){
      fprintf(stderr, "ERROR : fonction connect() destination\n");
      perror("connect");
      return -1;
    }
  }

  return socketfd;
}
