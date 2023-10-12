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
#define GET "�������� �� �����.\n"
#define CONNECTION_WAIT "�������� �����������.\n"

int s, s_new;

void finish(int a = NULL)  // ������� ���������� ������
{
    printf("��������� ������.\n");
    close(s);
    close(s_new);
    exit(0);
}

int main()
{
    socklen_t from_len;
    char buf[BUF_SIZE];
    struct sockaddr_in sin, from_sin;

    signal(SIGINT, finish);  // ��������������� ��������
    signal(SIGKILL, finish);
    signal(SIGTERM, finish);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)  // �������� ������
    {
        printf("������ �������� ������.\n");
        exit(-1);
    }
    memset((char*)&sin, 0, sizeof(sin));  // ��������� ������ ��� ������ � ��� �������������
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = SRV_PORT;

    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0)  // ����������� � ������
    {
        printf("������ ���������� � �������.\n");
        exit(-1);
    }

    write(1, CONNECTION_WAIT, strlen(CONNECTION_WAIT));
    
    if (listen(s, 1) < 0)  // �������� ����������
    {
        printf("������ �������� � ����� ������.\n");
        close(s);
        exit(-1);
    }
    
    from_len = sizeof(from_sin);
    if ((s_new = accept(s, (struct sockaddr*)&from_sin, &from_len)) < 0)  // �������� ������� �� ����������
    {
        printf("������ ���������� � ��������.\n");
        finish();
    }
    while (1)
    {
        write(1, GET, strlen(GET));
        from_len = read(s_new, buf, BUF_SIZE);  // ������ ��������� �� ������
        write(1, buf, from_len);  // ����� ���������
        if (from_len == 0)  // ���� ������ ��������� ������� ����� - �����
            finish();

        from_len = read(0, buf, BUF_SIZE);  // ���������� ������ ���������
        write(s_new, buf, from_len);  // ������ ��������� � �����
        if (from_len == 0)  // ���� ��������� ����� ������� ����� - �����
            finish();
    }
    close(s_new);  // ������� ������ ������
}