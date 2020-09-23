#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PATH_DEV "/dev/pcd8544"
#define LEN_BUF 256
#define LEN_MSG 87
#define LINE_COUNT 6

void print_menu(void)
{
	printf("1 - Menu\n");
	printf("2 - Set pixel\n");
	printf("3 - Unset pixel\n");
	printf("4 - Write string\n");
	printf("5 - Clear screen\n");
	printf("0 - Exit\n");
}

int get_choice(void)
{
	int choice;

	while (printf("[1 - menu]> ") > 0 && scanf("%d", &choice) != 1);

	return choice;
}

int get_x_y(int *x, int *y)
{
	int ret = 0;

	printf("Input coordinates x, y: ");
	if (scanf("%d %d", x, y) != 2)
	{
		printf("Two integer numbers are required!\n");
		ret = -1;
	}

	return ret;
}

int get_line_number(int *line_number)
{
	int ret = 0;
	char buf[LEN_BUF];
	
	printf("Input line number [1-%d](next default)> ", LINE_COUNT);
	if (fgets(buf, LEN_BUF, stdin))
	{
		if ((sscanf(buf, "%d\n", line_number) != 1) || (*line_number < 1 || *line_number > LINE_COUNT))
			*line_number = 0;
	}
	else
	{
		printf("Error while reading line number!\n");
		ret = -1;
	}

	return ret;
}

int get_message(char *msg_buf, int len)
{
	int ret = 0;
	
	fgetc(stdin);
	printf("Input message[length<%d]> ", len);
	if (fgets(msg_buf, len, stdin) == 0)
	{
		printf("Error while reading message!\n");
		ret = -1;
	}
	else
	{
		int pos_newline = strlen(msg_buf) - 1;
		if (msg_buf[pos_newline] == '\n')
			msg_buf[pos_newline] = 0;
	}

	return ret;
}

int main(void)
{
	int fd_dev = open(PATH_DEV, O_WRONLY);

	if (fd_dev != -1)
	{
		int choice;
		int x, y;
		int line_num;
		char msg[LEN_MSG];
		char buf[LEN_BUF];

		while ((choice = get_choice()) > 0)
		{
			switch (choice)
			{
				case 1:
					print_menu();
					break;
				case 2:
				{
					if (get_x_y(&x, &y) == 0)
					{
						buf[0] = 0;
						snprintf(buf, LEN_BUF, "%d %d 1", x, y);
						write(fd_dev, buf, LEN_BUF);
					}
					
					break;
				}
				case 3:
				{
					if (get_x_y(&x, &y) == 0)
					{
						buf[0] = 0;
						snprintf(buf, LEN_BUF, "%d %d 0", x, y);
						write(fd_dev, buf, LEN_BUF);
					}
					
					break;
				}
				case 4:
				{
					if (get_message(msg, LEN_MSG) == 0)
						if (get_line_number(&line_num) == 0)
						{	
							buf[0] = 0;
							snprintf(buf, LEN_BUF, "%d %s", line_num, msg);
							write(fd_dev, buf, LEN_BUF);
						}
							
					break;
				}
				case 5:
				{
					write(fd_dev, "-1 -1 0", LEN_BUF);

					break;
				}
			}
		}

		close(fd_dev);
	}
	else
		printf("Can't open file to read: %s\n", PATH_DEV);

	return 0;
}
