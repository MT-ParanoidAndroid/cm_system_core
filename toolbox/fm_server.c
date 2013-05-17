#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

#define LOCAL_SOCKET  "/data/fmsocket"

struct sockaddr_un 
{
    sa_family_t sun_family;
    char        sun_path[UNIX_PATH_MAX];
};

int recv_check2(int sockid, int msecToWait)
{
    int ret;
    int len;
    fd_set rfd;
    struct timeval to;
 
    FD_ZERO(&rfd);
    FD_SET(sockid, &rfd);
    to.tv_sec = msecToWait/1000;
    to.tv_usec = (msecToWait%1000)*1000;

    ret = select(sockid+1, &rfd, NULL, NULL, &to);
    if (SOCKET_ERROR == ret)
    {
        printf("[fm server] select failed with %d\n", errno);
        return 0;
    }
 
    if (!FD_SET(sockid, &rfd))
    {
        return 0;
    }
 
    ret = ioctl(sockid, FIONREAD, &len);
    if (SOCKET_ERROR == ret)
    {
        printf("[fm server] ioctlsocket(FIONREAD) failed with %d\n",errno );
        return 0;
    }
    return len;
}

int fm_server_main(int argc, char *argv[])
{
    int sockid;  // main socket id
    int sockid2; // socket id to
    int addr2_len = 0;
    struct sockaddr_un addr = {0};
    struct sockaddr_un addr2 = {0};

    printf("[fm server] fm_server running...\n");

    unlink(LOCAL_SOCKET);

    // prepare main socket
    sockid = socket(PF_UNIX, SOCK_STREAM, 0);  // aslo known as PF_LOCAL
    if (SOCKET_ERROR == sockid)
    {
        printf("[fm server] socket failed with %d\n",errno);
        return 0;
    }

    addr.sun_family = PF_UNIX;
    sprintf(addr.sun_path, LOCAL_SOCKET);

    if (SOCKET_ERROR == bind(sockid, (struct sockaddr*)&addr, sizeof(addr)))
    {
        close(sockid);
        sockid = SOCKET_ERROR;
        printf("[fm server] bind %s failed with %d\n", addr.sun_path,errno);
        return -1;
    }

    // start service
    while (1)
    {
        if (SOCKET_ERROR == listen(sockid, 2))
        {
            close(sockid);
            sockid = SOCKET_ERROR;
            printf("[fm server] listen failed with %d\n",errno);
            return -1;
        }

        sockid2 = accept(sockid, (struct sockaddr*) &addr2, &addr2_len);
        if (SOCKET_ERROR == sockid2)
        {
            close(sockid);
            sockid = SOCKET_ERROR;
            unlink(LOCAL_SOCKET);
            printf("[fm server] accept failed with %d\n",errno);
            return -1;
        }

        // service the local socket
        int len = recv_check2 (sockid2, 3000);
        printf("[fm server] cmd_len=%d\n", len);

        if (SOCKET_ERROR == len || 0 == len)
        {
            close (sockid2 );
            continue;
        }

        // read command
        char* cmdline = (char*) malloc (1+len);
        int num_recv = recv(sockid2, cmdline, len, 0);

        cmdline[len] = 0;

        printf("[fm server] cmdline=(%s)\n", cmdline);

        int ret = system(cmdline);
        printf("[fm server send command ret = %d\n", ret);

        free(cmdline);
        close(sockid2);
    }

    close(sockid);
    unlink(LOCAL_SOCKET);

    return 0;
}
