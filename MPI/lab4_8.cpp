#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>

#define GNUPLOT
#define R 5  // Сопротивление
#define C 1  // Емкость 
#define UN 0  // Начальные условия
#define UG 100  // Граничные йсловия
#define ht 1  // Шаг по времени
FILE* out, * fp;  // Файл с данными для визуализации и файл gnuplot скрипта для отрисовки


void arg_abort(const char* msg)  // Завершение программы при ошибке
{
	printf("%s\n", msg);
	MPI_Abort(MPI_COMM_WORLD, 0);
	exit(0);
}

void gen_gnuplot(int time_interval, int element_c)  // Генерация файла-скрипта для отображения графики
{
	fprintf(fp, "set cbrange [0.9:1]\n");
	fprintf(fp, "set xrange [0:%d]\n", element_c + 1);
	fprintf(fp, "set yrange [-10:110]\n");
	fprintf(fp, "set palette defined (1 '#ce4c7d')\n");
	fprintf(fp, "set style line 1 lc rgb '#b90046' lt 1 lw 0.5\n");
	fprintf(fp, "do for [i=0:%d]{\n", time_interval);
	fprintf(fp, "plot 'out.txt' index i using 1:2 smooth bezier title 'U(t)'\npause 0.1}\npause -1\n");
}

void to_file(double* U, int element_c, double t)  // Вывод в файл значений напряжений
{
	int i, k;
	for (i = 0; i < element_c + 2; i++)
		fprintf(out, "%d\t%lf\n", i, U[i]);
	fprintf(out, "\n");
	fprintf(out, "\n\n");
}

void begin_values(double *U_old_all, double *U_first, double *U_last, int total, long long element_c)  // Задание начальных и граничных условий
{
	for (long long i = 1; i <= element_c; i++)
		U_old_all[i] = UN;
	U_old_all[0] = UG;
	U_old_all[element_c + 1] = UG;
	for (int i = 1; i < total - 1; i++)
	{
		U_first[i] = UN;
		U_last[i] = UN;
	}
	U_first[0] = UG;
	U_last[total] = UG;
}

void calculate(double* U_new, double* U_old, double *first, double* last, long long n_per_proc)  // Вычисление напряжений на новом шаге
{
	for (long long i = 0; i < n_per_proc; i++)
	{
		if (i == 0)
			U_new[i] = (first[0] - 2 * U_old[i] + U_old[i + 1]) * ht / R / C + U_old[i];
		else if (i == n_per_proc - 1)
			U_new[i] = (U_old[i - 1] - 2 * U_old[i] + last[0]) * ht / R / C + U_old[i];
		else
			U_new[i] = (U_old[i - 1] - 2 * U_old[i] + U_old[i + 1]) * ht / R / C + U_old[i];
	}
}

int main(int argc, char* argv[])
{
	double sum_sec = 0.0;  // Переменная, запоминающая время, потраченное на отрисовку
	int udif;  // Переменная для вычисления разностей времен в микросекундах
	double* U_old;  // Значения напряжений в узлах на одном отрезке на предыдущем шаге
	double* U_new;  // Значения напряжений в узлах на одном отрезке на новом шаге
	double* U_old_all;  // Значения напряжений во всех узлах
	double* U_first;  // Значения напряжений в узлах прямо перед началами отрезков
	double* U_last;  // Значения напряжений в узлах прямо после концов отрезков
	double first[1];  // Значение напряжения в узле перед началом отрезка
	double last[1];  // Значение напряжения в узле после конца отрезка
	long long n_per_proc;  // Количество узлов на процесс (длина отрезка)
	long long intBuf[2];  // Для передачи данных из аргументов вызова
	int total, myrank;  // Количество процессов и идентификатор конкретного процесса

	timeval tv_s, tv1_e, tv2_e;  // Время начала, конца выводов в файл и конца выполнения программы
	timezone tz;  // Временная зона

	MPI_Init(&argc, &argv);  // Инициализация MPI
	MPI_Comm_size(MPI_COMM_WORLD, &total);  // Получение количества процессов
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);  // Получение идентификатора процесса

	if (argc < 3)  // Проверка на количество аргументов запуска
	{
		if (!myrank)
			arg_abort("Insert amount of elements that is divisible by 8 and time interval.");
	}
#ifdef GNUPLOT
	// Открытие файлов, нужных для визуализации
	out = fopen("out.txt", "w");
	fp = fopen("script.dat", "w");
	if (fp == NULL || out == NULL)  // Проверка на успешное открытие
	{
		arg_abort("Can't open file for writing.");
	}
#endif
	long long element_c = atoi(argv[1]);  // Получение количества узлов из аргументов
	long long time_interval = atoi(argv[2]);  // Получение временного промежутка из аргументов
	if (element_c <= 0 || element_c % 8 != 0 || time_interval <= 0)  // Проверка на соответствие аргументов
	{
		if (!myrank)
			arg_abort("Insert amount of elements that is divisible by 8 and time interval.");
	}

#ifdef GNUPLOT
	if (!myrank)
		gen_gnuplot(time_interval, element_c);  // Генерация скрипта для визуализации
#endif

	if (!myrank) {
		n_per_proc = element_c / total;
		intBuf[0] = time_interval;
		intBuf[1] = n_per_proc;
	};
	MPI_Bcast((void*)intBuf, 2, MPI_LONG_LONG, 0, MPI_COMM_WORLD);  // Рассылка данных, потлученных в аргументах вызова
	time_interval = intBuf[0];
	n_per_proc = intBuf[1];
	// Выделение памяти
	U_new = (double*)malloc(sizeof(double) * n_per_proc);
	U_old = (double*)malloc(sizeof(double) * n_per_proc);
	U_old_all = (double*)malloc(sizeof(double) * (element_c + 2));
	U_first = (double*)malloc(sizeof(double) * (total + 1));
	U_last = (double*)malloc(sizeof(double) * (total + 1));
	if (!myrank)
	{
		begin_values(U_old_all, U_first, U_last, total, element_c);  // Определение начальных значений
	}

	if (!myrank)
	{
#ifdef GNUPLOT
		to_file(U_old_all, element_c, 0);
#endif
		gettimeofday(&tv_s, &tz);
	}
	// Рассылка начальных значений в отрезках
	MPI_Scatter((void*)(U_old_all + 1), n_per_proc, MPI_DOUBLE, (void*)U_old, n_per_proc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	for (long long t = 1; t <= time_interval; t += ht)
	{
		// Рассылка значений, идущих прямо перед отрезками
		MPI_Scatter((void*)U_first, 1, MPI_DOUBLE, (void*)first, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		// Рассылка значений, идущих прямо после отрезков
		MPI_Scatter((void*)(U_last + 1), 1, MPI_DOUBLE, (void*)last, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		calculate(U_new, U_old, first, last, n_per_proc);  // Вычисление значений на новом шаге
		for (long long i = 0; i < n_per_proc; i++)  // Теперь значения на новом шаге - значения на предыдущем шаге
			U_old[i] = U_new[i];
		// Подготовка для отправки новых предельных значений для других отрезков 
		last[0] = U_new[n_per_proc - 1];
		first[0] = U_new[0];
#ifdef GNUPLOT
		// Сбор всех полученных значений для вывода в файл
		MPI_Gather((void*)U_new, n_per_proc, MPI_DOUBLE, (void*)(U_old_all + 1), n_per_proc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		if (!myrank)
		{
			// Подсчет времени на вывод
			gettimeofday(&tv1_e, &tz);
			to_file(U_old_all, element_c, t);
			gettimeofday(&tv2_e, &tz);
			udif = tv2_e.tv_usec - tv1_e.tv_usec;
			sum_sec += tv2_e.tv_sec - tv1_e.tv_sec - (udif < 0) + (udif + (udif < 0) * 1000000) / 1000000.0;
		}
#endif
		// Сбор новых предельных значений
		MPI_Gather((void*)last, 1, MPI_DOUBLE, (void*)(U_first + 1), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Gather((void*)first, 1, MPI_DOUBLE, (void*)(U_last), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}
	if (!myrank)
	{
		// Вычисление времени работы программы с учетом выводов в файл
		gettimeofday(&tv1_e, &tz);
		udif = tv1_e.tv_usec - tv_s.tv_usec;
		printf("Processing time: %lf s.\n", tv1_e.tv_sec - tv_s.tv_sec - (udif < 0) + (udif + (udif < 0) * 1000000) / 1000000.0 - sum_sec);
	}
#ifdef GNUPLOT
	// Закрытие файлов для визуализации
	fclose(out);
	fclose(fp);
#endif
	// Очистка памяти
	free(U_old_all);
	free(U_old);
	free(U_new);
	free(U_last);
	free(U_first);
	MPI_Finalize();  // Завершение работы с MPI
	exit(0);
}
