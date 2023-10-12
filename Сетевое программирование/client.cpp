#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define SRV_PORT 1234
#define CLNT_PORT 1235
#define BUF_SIZE 128
#define GET "Перехожу на прием.\n"

int s;

void finish(int a = NULL)  // Функция завершения работы
{
    printf("Звершение работы.");
    close(s);
    exit(0);
}

int main(int argc, char* argv[])
{
    int from_len;
    char buf[BUF_SIZE];
    struct hostent* hp;
    struct sockaddr_in clnt_sin, srv_sin;

    signal(SIGINT, finish);  // Переопределение сигналов
    signal(SIGKILL, finish);
    signal(SIGTERM, finish);

    if (argc < 2)
    {
        printf("В параметрах запуска введите IP-адресс сервера.\n");
        exit(-1);
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // Создание сокета
    {
        printf("Ошибка создания сокета.\n");
        exit(-1);
    }
    memset((char*)&clnt_sin, 0, sizeof(clnt_sin));  // Выделение памяти для сокета и его инициализация
    clnt_sin.sin_family = AF_INET;
    clnt_sin.sin_addr.s_addr = INADDR_ANY;
    clnt_sin.sin_port = CLNT_PORT;
    if ((bind(s, (struct sockaddr*)&clnt_sin, sizeof(clnt_sin))) < 0)  // Подключение к сокету
    {
        printf("Ошибка соединения с адресом.\n");
        finish();
    }

    memset((char*)&srv_sin, 0, sizeof(srv_sin));
    hp = gethostbyname(argv[1]);  // Получение информации о сервере по ip
    srv_sin.sin_family = AF_INET;
    memcpy((char*)&srv_sin.sin_addr, hp->h_addr, hp->h_length);
    srv_sin.sin_port = SRV_PORT;
    if (connect(s, (struct sockaddr*)&srv_sin, sizeof(srv_sin)) < 0)  // Соединение с сервером
    {
        printf("Ошибка соединения с сервером.\n");
        finish();
    }
    while (1)
    {
        // Цикл работает также как в сервере, но последовательность ввода и вывода в сокет изменена
        // Смотри текст программы-сервера
        from_len = read(0, buf, BUF_SIZE);
        write(s, buf, from_len);
        if (from_len == 0)
            finish();
        write(1, GET, sizeof(GET));
        from_len = read(s, buf, BUF_SIZE);
        write(1, buf, from_len);
        if (from_len == 0)
            finish();
    }
    close(s);  // Очистка памяти сокета
    exit(0);
}