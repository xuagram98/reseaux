#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const char *real_address(const char *address, struct sockaddr_in6 *rval){

    int err;

    struct addrinfo *hints = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
    if(hints == NULL){
      return "Erreur d'alloction de mémoire.";
    }
    struct addrinfo *results;

    //memset(hints, 0, sizeof(hints)); inutile si on fait calloc() au-dessus
    hints->ai_family = AF_INET6;
    hints->ai_protocol = IPPROTO_UDP;
		hints->ai_socktype = SOCK_DGRAM; // Car UDP packets
		// hints.ai_socktype = SOCK_STREAM; si TCP packets

    err = getaddrinfo(address, NULL, hints, &results);
    if(err != 0){
        return "Erreur d'exécution de la fonction addrinfo.";
    }

    struct sockaddr *temp = results->ai_addr;
    struct sockaddr_in6 *final = (struct sockaddr_in6 *) temp;
    *rval = *final;

    freeaddrinfo(results);
    return NULL;
}
