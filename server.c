
/*
Сервер отладочный - сделан неумело и исключительно для того, чтобы хоть как-то взаимодействовать с клиентом
*/

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <json-c/json.h>
#include <sys/sendfile.h>
#define PORT 49152
#define BUF_SIZE 65536
int main() {
    FILE *ver_fd;
    int conf_fd;
    char *ver_path = "version";
    char *conf_path = "config.json";
    char version[10];
    ver_fd = fopen(ver_path, "r");
    conf_fd = open(conf_path, O_RDONLY);
    int sock;
    int nsock;
    socklen_t *cl_ip_length;
    char cl_ip[INET_ADDRSTRLEN+1];
    struct sockaddr_in server;
    struct sockaddr_in client;
    cl_ip_length = (socklen_t *) sizeof(client);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
        printf("Порт занят\n");
        exit(1);
    }
    listen(sock, 5);
    for (;;) {
        int msg_len = 0;
        char msg[BUF_SIZE];
        cl_ip_length = (socklen_t *) sizeof(client);
        nsock = accept(sock, (struct sockaddr *) &client, (socklen_t *) &cl_ip_length);
        inet_ntop(AF_INET, &client.sin_addr, cl_ip, sizeof(cl_ip));
        printf("\nДанные клиента: %s:%hu\n", cl_ip, ntohs(client.sin_port));
        msg_len = read(nsock, &msg, BUF_SIZE);
        printf("Версия файла настроек клиента: ");
        for (int i = 0; i < msg_len; i++) printf("%c", msg[i]);
        fseek(ver_fd, 0, SEEK_SET);
        fgets(version, sizeof(version), ver_fd);
        version[strcspn(version, "\n")] = 0;
        printf("\nВерсия файла настроек на сервере: %s\n", version);
        if (strcmp(msg, version) == 0) {
            strcpy(msg, "0");
            write(nsock, &msg, strlen(msg));
            close(nsock);
            printf("\nВерсия файла та же, закрываю соединение\n");
            continue;
        }
        lseek(conf_fd, 0, SEEK_SET);
        sendfile(nsock, conf_fd, NULL, BUF_SIZE);
        msg_len = read(nsock, &msg, BUF_SIZE);
        if (strcmp(msg, "0") != 0) {
            for (int i = 0; i < msg_len; i++) printf("%c", msg[i]);
        }
        else {
            printf("\nНастройки успешно применены\n");
        }
        close(nsock);
    }
    fclose(ver_fd);
    close(conf_fd);
    exit(0);
}
