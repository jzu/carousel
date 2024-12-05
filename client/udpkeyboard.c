/*
 * udpkeyboard.c - send a character at a time to an address using UDP
 * gcc udpkeyboard.c -o udpkeyboard
 */

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <termios.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
   
#define ADDRESS "10.42.0.1"
#define PORT    1234 
#define MAXLINE 1024 
   
int main() { 

    int n;  
    char buffer[MAXLINE]; 
    int c, 
        rc;
    struct termios orig,
                   now;
    int sockfd; 
    socklen_t len; 
    struct sockaddr_in servaddr; 
   
    if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror ("socket creation failed"); 
        exit (EXIT_FAILURE); 
    } 

    memset (&servaddr, 0, sizeof (servaddr)); 
    servaddr.sin_family      = AF_INET; 
    servaddr.sin_port        = htons (PORT); 
    servaddr.sin_addr.s_addr = inet_addr (ADDRESS);

    tcgetattr (0, &orig);
    now = orig;
    now.c_lflag &= ~(ISIG|ICANON|ECHO);
    now.c_cc[VMIN] = 1;
    now.c_cc[VTIME] = 2;
    tcsetattr (0, TCSANOW, &now);

    while (1) {
        c = getchar();
        sprintf (buffer, "%c", c);
        sendto (sockfd, (const char *) buffer, 
                1, 
                MSG_CONFIRM, 
                (const struct sockaddr *) &servaddr,  
                sizeof (servaddr)); 
        if ((c >= ' ') && (c <= '~'))
          printf ("[Keyboard] %c\t→ server.\n", c);
        else
          printf ("[Keyboard] 0x%02x\t→ server.\n", c);
    }
    tcsetattr (0, TCSANOW, &orig);

    close (sockfd); 
    return 0; 
}
