#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

const char* INPUT_ERROR = "Ошибка ввода.\n";
const int MAX_N = 10000;
char proc_mode[2][2];
pid_t pid1, pid2;
int fds01[2], fds12[2];

void set_mode()
{
    	char proc[2], mode[2], c;
    	const char* WELLCOME = "Приветствую в режиме изменения режима процессов. Введите номер процесса (1 или 2).\n";
    	const char* CHOOSE_MODE_MES = "Выберите режим работы процесса:\n";
    	const char* PROC_MODES[2] = {
"0 - трансляция строки в неизменном виде,\n1 - инвертирование строки - первый символ становится последним и т.д.,\n2 - обмен соседних символов - нечетный становится на место четного и наоборот,\n3 - перевод в КОИ-8 - установление в 1 старшего (8-ого) бита каждого символа.\n", 
"0 - трансляция строки в неизменном виде,\n1 - перевод всех символов строки в верхний регистр,\n2 - перевод всех символов строки в нижний регистр,\n3 - смена регистра всех символов строки.\n"};
	const char* FINISH = "Выбор сделан.\n";
	
    	write(1, WELLCOME, strlen(WELLCOME));
    	while (read(0, &proc, MAX_N) != 2 || (mode[0] != '0' && proc[0] != '1' && proc[0] != '2') || proc[1] != '\n')
    	{
		write(1, INPUT_ERROR, strlen(INPUT_ERROR));
    	}
    	write(1, CHOOSE_MODE_MES, strlen(CHOOSE_MODE_MES));
    	write(1, PROC_MODES[proc[0] - '1'], strlen(PROC_MODES[proc[0] - '1']));

    	while (read(0, &mode, MAX_N) != 2 || (mode[0] != '0' && mode[0] != '1' && mode[0] != '2' && mode[0] != '3') || mode[1] != '\n')
    	{
		write(1, INPUT_ERROR, strlen(INPUT_ERROR));
    	}
    	proc_mode[proc[0] - '1'][0] = mode[0];

	write(1, FINISH, strlen(FINISH));
}

int isprep(char c)
{
	char prep[29] = {'!', '%', '^', '&', '*', '(', ')', '-', '+', 
'=', '{', '}', '|', '~', '[', ']', 92, 59, 39, ':', 34, '<', '>', '?', ',', '.', '/', '#', ' '};
	for (int i = 0; i < 29; i++)
		if (c == prep[i])
			return 1;
	return 0;

}

int proc0()
{
	char str[MAX_N], clear_str[MAX_N], mode_str[MAX_N];
	const char *MSG = "Результат процесса 0: ";
			
	int str_len = 0, clear_str_len = 0;

	for (int i = strlen(str); i > -1; i--)
		str[i] = '\0';
	for (int i = strlen(mode_str); i > -1; i--)
		mode_str[i] = '\0';

	if ((str_len = read(0, &str, MAX_N)) == 0)
		return 0;
	
	for (int i = 0; i < str_len; i++)
	{
		if (isprep(str[i]) || str[i] == '\n' || isalnum(str[i]))
			clear_str[clear_str_len++] = str[i];
	}

	write(1, MSG, strlen(MSG));
	write(1, clear_str, clear_str_len);

	strcat(mode_str, proc_mode[0]);
	strcat(mode_str, proc_mode[1]);
	strcat(mode_str, clear_str);

 	write(fds01[1], mode_str, str_len + 2);
	
	kill(pid1, SIGUSR1);

	return str_len;
}

void proc1()
{
	const char *MSG = "Результат процесса 1: ";
	char buf[MAX_N];
	
	for (int i = strlen(buf); i > -1; i--)
		buf[i] = '\0';

	read(fds01[0], &proc_mode[0], 1);
	read(fds01[0], &proc_mode[1], 1);
	int len_buf = read(fds01[0], &buf, MAX_N);

	char new_buf[len_buf], new_buf_mode[len_buf + 1];

	for (int i = strlen(new_buf); i > -1; i--)
		new_buf[i] = '\0';
	for (int i = strlen(new_buf_mode); i > -1; i--)
		new_buf_mode[i] = '\0';

	switch (proc_mode[0][0])
	{
		case '0': 
			strcpy(new_buf, buf);
			break;
	    	case '1':
			for (int i = 0; i < len_buf - 1; i++)
		    		new_buf[i] = buf[len_buf - 2 - i];
			new_buf[len_buf - 1] = '\n';
			break;
	    	case '2':
			for (int i = 0; i < len_buf; i++)
    				new_buf[i] = buf[i + (-1) * (i % 2 == 1) * (i != (len_buf - 1)) + (i % 2 == 0) * (i < (len_buf - 2))];
			break;
 		case '3':
			for (int i = 0; i < len_buf; i++)
				new_buf[i] = buf[i] | 256;
			break;
		default:
			write(1, "Error1\n", 7);
			break;
	}
	new_buf_mode[0] = proc_mode[1][0];
	strcat(new_buf_mode, new_buf);

	write(1, MSG, strlen(MSG));
	write(1, new_buf, len_buf);
	
	write(fds12[1], new_buf_mode, len_buf + 1);
	kill(pid2, SIGUSR2);
}

void proc2()
{
	const char *MSG = "Результат процесса 2: ";
	char buf[MAX_N];

	for (int i = strlen(buf); i > -1; i--)
		buf[i] = '\0';

	read(fds12[0], &proc_mode[1], 1);
	int len_buf = read(fds12[0], &buf, MAX_N);

	char new_buf[len_buf];
	switch (proc_mode[1][0])
	{
		case '0': 
			strcpy(new_buf, buf);
			break;
		case '1':
			for (int i = 0; i < len_buf; i++)
   				new_buf[i] = toupper(buf[i]);
			break;
		case '2':
			for (int i = 0; i < len_buf; i++)
    				new_buf[i] = tolower(buf[i]);
			break;
		case '3':
			for (int i = 0; i < len_buf; i++)
    				if (isupper(buf[i]))
    					new_buf[i] = tolower(buf[i]);
    				else
					new_buf[i] = toupper(buf[i]);
			break;
		default:
			write(1, "Error2\n", 7);
			break;
	}
	write(1, MSG, strlen(MSG));
	write(1, new_buf, len_buf);
}

int main(int argc, char** argv)
{
    	int str_len = 1;
	
	pipe(fds01);
    	pipe(fds12);

	proc_mode[0][0] = '0';
	proc_mode[0][1] = '\0';
	proc_mode[1][0] = '0';
	proc_mode[1][1] = '\0';
	
	if ((pid2 = fork()) == 0)
    	{
		signal(SIGUSR2, proc2);
		signal(SIGINT, SIG_IGN);
	}
    	else if ((pid1 = fork()) == 0)
    	{
		signal(SIGUSR1, proc1);
		signal(SIGINT, SIG_IGN);
	}
	else if (pid1 != 0 && pid2 != 0)
		signal(SIGINT, set_mode);
	
    	while (str_len != 0)
	{
		if (pid1 && pid2)
		{
			str_len = proc0();
			if (str_len == 0)
			{
				kill(pid1, SIGTERM);
				kill(pid2, SIGTERM);
				close(fds01[1]);
				close(fds01[0]);
				close(fds12[1]);
				close(fds12[0]);
				exit(0);
			}
		}
		else
			pause();
	}
	exit(0);
}