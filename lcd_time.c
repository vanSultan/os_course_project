#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define PATH_DEV "/dev/pcd8544"
#define LEN_BUF 256
#define LEN_MSG 87
#define LINE_COUNT 6

int main(void)
{
	int fd_dev = open(PATH_DEV, O_WRONLY);

	if (fd_dev != -1)
	{
		int line_num = 0;
		char buf[LEN_BUF];
		long int s_time;
		struct tm *m_time;

		for (int i = 0; i < LINE_COUNT * LINE_COUNT; i++)
		{
			s_time = time(NULL);
			m_time = localtime(&s_time);
			snprintf(buf, LEN_BUF, "%d %0.2d:%0.2d:%0.2d %0.2d.%0.2d", line_num, m_time->tm_hour,
					m_time->tm_min, m_time->tm_sec, m_time->tm_mday, m_time->tm_mon + 1);
			write(fd_dev, buf, LEN_BUF);
			sleep(4);

			write(fd_dev, "-1 -1 0", LEN_BUF);
			sleep(0.2);

			line_num = line_num % LINE_COUNT + 1;
		}


		close(fd_dev);
	}
	else
		printf("Can't open file to read: %s\n", PATH_DEV);

	return 0;
}
