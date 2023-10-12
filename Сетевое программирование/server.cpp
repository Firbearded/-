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
#define BUF_SIZE 128
#define GET "Перехожу на прием.\n"
#define CONNECTION_WAIT "Ожидание подключения.\n"

int s, s_new;

void finish(int a = NULL)  // Функция завершения работы
{
    printf("Звершение работы.\n");
    close(s);
    close(s_new);
    exit(0);
}

int main()
{
    socklen_t from_len;
    char buf[BUF_SIZE];
    struct sockaddr_in sin, from_sin;

    signal(SIGINT, finish);  // Переопределение сигналов
    signal(SIGKILL, finish);
    signal(SIGTERM, finish);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // Создание сокета
    {
        printf("Ошибка создания сокета.\n");
        exit(-1);
    }
    memset((char*)&sin, 0, sizeof(sin));  // Выделение памяти для сокета и его инициализация
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = SRV_PORT;

    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0)  // Подключение к сокету
    {
        printf("Ошибка соединения с адресом.\n");
        exit(-1);
    }

    write(1, CONNECTION_WAIT, strlen(CONNECTION_WAIT));
    
    if (listen(s, 1) < 0)  // Ожидание соединения
    {
        printf("Ошибка перехода в режим приема.\n");
        close(s);
        exit(-1);
    }
    
    from_len = sizeof(from_sin);
    if ((s_new = accept(s, (struct sockaddr*)&from_sin, &from_len)) < 0)  // Принятие запроса на соединение
    {
        printf("Ошибка соединения с клиентом.\n");
        finish();
    }
    while (1)
    {
        write(1, GET, strlen(GET));
        from_len = read(s_new, buf, BUF_SIZE);  // Чтение сообщения из сокета
        write(1, buf, from_len);  // Вывод сообщения
        if (from_len == 0)  // Если пришло сообщение нулевой длины - конец
            finish();

        from_len = read(0, buf, BUF_SIZE);  // Считывание нового сообщения
        write(s_new, buf, from_len);  // Запись сообщения в сокет
        if (from_len == 0)  // Если сообщение имело нулевую длину - конец
            finish();
    }
    close(s_new);  // Очистка памяти сокета
}