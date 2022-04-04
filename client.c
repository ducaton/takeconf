#include <unistd.h>
#include <arpa/inet.h>
#include <bsd/string.h>
#include <stdio.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <json-c/json.h>
#include <sys/sendfile.h>
#define BUF_SIZE 65536

// Компилирование - cc -o client client.c -l json-c -l bsd
// Выполнение - ./client 2> >(tee /tmp/takeconf.log >&2)

// При наличие ошибок надо отчитаться о них серверу
void is_err_happen(int sock, int err) {
    if (err) {
        fprintf(stderr, "\nПри применении настроек произошла ошибка. Клиент оставляет старые файлы настроек (если они есть)\n");
        // !Некоторые изменения так медленно записывались в файл, что на сервер отправлялся неплоный лог :)
        sync();
        char msg[BUF_SIZE];
        int log;
        log = open("/tmp/takeconf.log", O_RDONLY);
        if (log != -1)
            sendfile(sock, log, NULL, BUF_SIZE);
        else {
            strlcpy(msg, "\nЛоги записать не удалось.\nПри применении настроек произошла ошибка. Клиент оставляет старые файлы настроек (если они есть)\n", sizeof(msg));
            write(sock, &msg, strlen(msg));
        }
        remove("/tmp/takeconf.log");
        close(log);
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    int err = 0;
    // Проверка правильности выполнения программы
    if (argc != 1) {
        fprintf(stderr, "\nПрограмма должна быть запущена без аргументов");
        err = 1;
    }

    //Проверка прав доступа
    if (!opendir("/etc/takeconf")) {
        fprintf(stderr, "\nНе удаётся получить доступ к каталогу /etc/takeconf");
        err = 1;
    }
    FILE *chmod_fd;
    if (chmod_fd = (fopen("/etc/takeconf/test", "a+"))) {
        fclose(chmod_fd);
        remove("/etc/takeconf/test");
    }
    else {
        fprintf(stderr, "\nНет доступа на запись и чтение в каталоге /etc/takeconf");
        err = 1;
    }

    // Инициализация файлов из /etc. Если файла version нет или он пустой, версия равна нулю
    char local_ver[10] = "0";
    char server_name[254] = "";
    int port = 0;
    FILE *ver_fd_r;
    FILE *ver_fd_w;
    FILE *program_conf;
    char *ver_path = "/etc/takeconf/version";
    char *conf_path = "/etc/takeconf/current.json";
    char *program_conf_path = "/etc/takeconf/takeconf.conf";
    // Чтение файла с версией
    ver_fd_r = fopen(ver_path, "r");
    if (ver_fd_r) {
        fgets(local_ver, sizeof(local_ver), ver_fd_r);
        //Удаление переноса строки в конце
        local_ver[strcspn(local_ver, "\n")] = 0;
        if(!strlen(local_ver))
            strlcpy(local_ver, "0", sizeof(local_ver));
        fclose(ver_fd_r);
    }

    // Чтение файла настроек программы
    program_conf = fopen(program_conf_path, "r");
    if (program_conf) {
        char settings_string[4097];
        while (fgets(settings_string, sizeof(settings_string), program_conf)) {
            if (strcmp(&settings_string[0], "#") == 0)
                continue;
            settings_string[strcspn(settings_string, "\n")] = 0;
            char* token = strtok(settings_string, "=");
            if (strcmp(token, "server_name") == 0) {
                token = strtok(NULL, "");
                if (token)
                    strlcpy(server_name, token, sizeof(server_name));
                continue;
            }
            if (strcmp(token, "port") == 0) {
                token = strtok(NULL, "");
                if (token) {
                    char *end;
                    port = (int)strtoumax(token, &end, 10);
                }
                continue;
            }
        }
        fclose(program_conf);
        if (strcmp(server_name, "") == 0) {
            fprintf(stderr, "Не указано имя хоста\n");
            remove("/tmp/takeconf.log");
            exit(1);
        }
        if (port == 0) {
            fprintf(stderr, "Не указан порт\n");
            remove("/tmp/takeconf.log");
            exit(1);
        }
    }

    // Создание TCP соединения с сервером
    int sock;
    struct sockaddr_in server;
    struct in_addr *srv;
    unsigned long addr_s;
    char msg[BUF_SIZE];
    struct hostent *resolv_struct;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    srv=&(server.sin_addr);
    resolv_struct = gethostbyname(server_name);
    if (!resolv_struct) {
        fprintf(stderr, "Не удалось resolve DNS\n");
        remove("/tmp/takeconf.log");
        exit(1);
    }
    addr_s = inet_addr(inet_ntoa(*((struct in_addr *)resolv_struct->h_addr)));
    (*srv).s_addr = addr_s;
    server.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *) &(server), sizeof(server)) == -1) {
        fprintf(stderr, "Невозможно установить соединение с сервером\n");
        remove("/tmp/takeconf.log");
        exit(1);
    }
    // Соединение с сервером установлено
    is_err_happen(sock, err);

    // Первый обмен данными это проверка версии файла настроек
    strlcpy(msg, local_ver, sizeof(msg));
    write(sock, &msg, strlen(msg));
    read(sock, &msg, BUF_SIZE);
    if (strcmp(&msg[0], "0") == 0) {
        // Конфигурация не изменилась, выходим
        close(sock);
        remove("/tmp/takeconf.log");
        exit(0);
    }

    // Конфигурация изменилась, читаем и применяем
    struct json_object *local_conf = json_object_from_file(conf_path);
    struct json_object *net_conf = json_tokener_parse(msg);
    char net_ver[10];
    char templates_path[4097];
    strlcpy(net_ver, json_object_get_string(json_object_object_get(net_conf, "ver")), sizeof(net_ver));
    strlcpy(templates_path, json_object_get_string(json_object_object_get(net_conf, "templatesPath")), sizeof(templates_path));
    struct json_object *local_pc_conf;
    struct json_object *local_users_conf;
    struct json_object *local_user_conf;
    struct json_object *net_pc_conf;
    struct json_object *net_users_conf;
    char string_value[4097];
    char command[16284];

    // Применение настроек компьютера
    json_object_object_get_ex(local_conf, "pc", &local_pc_conf);
    json_object_object_get_ex(net_conf, "pc", &net_pc_conf);
    json_object_object_foreach (net_pc_conf, setting, value) {
        strlcpy(string_value, json_object_get_string(value), sizeof(string_value));
        // Если в локальных настройках значение такое же, то применение пропускается
        if (local_conf)
            if (json_object_get_string(json_object_object_get(local_pc_conf, setting)))
                if (strcmp(string_value, json_object_get_string(json_object_object_get(local_pc_conf, setting))) == 0)
                    continue;
        snprintf(command, sizeof(command), "%s/pc/%s.sh %s", templates_path, setting, string_value);
        fprintf(stderr, "---\n");
        // Если произошла ошибка
        if (system(command)) {
            err = 1;
            fprintf(stderr, "\n-Строка настроек: %s: %s\n-Команда: %s\n", setting, string_value, command);
        }
    }

    // Применение пользовательских настроек
    json_object_object_get_ex(local_conf, "users", &local_users_conf);
    json_object_object_get_ex(net_conf, "users", &net_users_conf);
    json_object_object_foreach (net_users_conf, user, settings) {
        json_object_object_foreach (settings, setting, value) {
            strlcpy(string_value, json_object_get_string(value), sizeof(string_value));
            if (local_conf)
                if (json_object_object_get_ex(local_users_conf, user, &local_user_conf))
                    if (json_object_get_string(json_object_object_get(local_user_conf, setting)))
                        if (strcmp(string_value, json_object_get_string(json_object_object_get(local_user_conf, setting))) == 0)
                            continue;
            snprintf(command, sizeof(command), "%s/users/%s.sh %s %s", templates_path, setting, user, string_value);
            fprintf(stderr, "---\n");
            // Если произошла ошибка
            if (system(command)) {
                err = 1;
                fprintf(stderr, "\n-Пользователь: %s\n-Строка настроек: %s: %s\n-Команда: %s\n", user, setting, string_value, command);
            }
        }
    }

    is_err_happen(sock, err);
    // Если ошибок нет, посылаем 0
    strlcpy(msg, "0", sizeof(msg));
    write(sock, &msg, strlen(msg));
    // Настроки применены успешно, старые файлы заменяются
    ver_fd_w = fopen(ver_path, "w+");
    fprintf(ver_fd_w, "%s", net_ver);
    json_object_to_file(conf_path, net_conf);
    fclose(ver_fd_w);
    remove("/tmp/takeconf.log");
    exit(0);
}