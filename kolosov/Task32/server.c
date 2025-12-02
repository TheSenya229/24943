#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <aio.h>
#include <time.h>

#define SOCK_PATH "socket_path"
#define MAX_CLIENTS 16
#define BUF_SIZE 1024

int main(void)
{
    int srv, cli, i, n;
    int clients[MAX_CLIENTS];
    struct aiocb cbs[MAX_CLIENTS];
    char bufs[MAX_CLIENTS][BUF_SIZE];
    struct sockaddr_un addr;

    char outbuf[65536];
    int outlen = 0;
    int delayed = 1;
    time_t start = time(NULL);

    srv = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);
    unlink(SOCK_PATH);

    bind(srv, (struct sockaddr *)&addr, sizeof(addr));
    listen(srv, 5);

    fcntl(srv, F_SETFL, O_NONBLOCK);

    for (i = 0; i < MAX_CLIENTS; i++)
        clients[i] = -1;

    while (1) {
        if (delayed && time(NULL) - start >= 10) {
            delayed = 0;
            if (outlen > 0)
                write(1, outbuf, outlen); 
            break; 
        }

        while (1) {
            cli = accept(srv, NULL, NULL);
            if (cli < 0)
                break;

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) {
                    clients[i] = cli;
                    memset(&cbs[i], 0, sizeof(cbs[i]));
                    cbs[i].aio_fildes = cli;
                    cbs[i].aio_buf = bufs[i];
                    cbs[i].aio_nbytes = BUF_SIZE;
                    aio_read(&cbs[i]);
                    break;
                }
            }
        }

        const struct aiocb *list[MAX_CLIENTS];
        int cnt = 0;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1)
                list[cnt++] = &cbs[i];
        }

        if (cnt == 0) { 
            struct timespec ts = {1, 0};
            nanosleep(&ts, NULL);
            continue;
        }

        struct timespec ts = {1, 0};
        aio_suspend(list, cnt, &ts);

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1)
                continue;

            int err = aio_error(&cbs[i]);
            if (err == 0){
                n = aio_return(&cbs[i]); 
                if (n <= 0) {
                    close(clients[i]);
                    clients[i] = -1;
                } else {
                    int j;
                    for (j = 0; j < n; j++)
                        bufs[i][j] =
                            (char)toupper((unsigned char)bufs[i][j]);

                    if (delayed) {
                        if (outlen + n < (int)sizeof(outbuf)) {
                            memcpy(outbuf + outlen, bufs[i], n);
                            outlen += n;
                        }
                    } else {
                        write(1, bufs[i], n);
                    }

                    memset(&cbs[i], 0, sizeof(cbs[i]));
                    cbs[i].aio_fildes = clients[i];
                    cbs[i].aio_buf = bufs[i];
                    cbs[i].aio_nbytes = BUF_SIZE;
                    aio_read(&cbs[i]);
                }
            }
        }
    }

    for (i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] != -1)
            close(clients[i]);
    close(srv);
    unlink(SOCK_PATH);
    return 0;
}
