#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BUF_LEN 1024

int main(int argc, char **argv)
{
	int rv1 = -1;
	int rv2 = -1;	
	int fd1 = -1;
	int fd2 = -1;
	char buf[BUF_LEN];
	int count = 0;

	//判断源文件存在性
	if (access(argv[1], F_OK < 0))
	{
		printf("source file '%s' is not exist\n", argv[1]);
		return -1;
	}
	else
	{
		printf("source file is exist\n", argv[1]);
	}	
	//判断源文件可读性
	if (access(argv[1], R_OK) < 0)
	{
		printf("source file '%s' unreadable\n", argv[1]);
		return -2;
	}
	else
	{
		printf("source file '%s' is ready\n", argv[1]);
	}

	//打开源文件
	if ((fd1 = open (argv[1], O_RDONLY))<0)
	{
		printf("open source file '%s' failure : %s", argv[1], strerror(errno));
	       return -3;	
	}	
	else 
	{
		printf("open source file '%s' fd[%d] successfully\n", argv[1], fd1);
	}
	//打开目标文件
	if ((fd2 = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
	{
		printf("open/creat destination file '%s' fail: %s\n", argv[2], fd2);
		goto cleanup;
	}
	else
	{
		printf("open/creat destination file successfully\n", argv[2], fd2);
	}
	//读源文件到buffer，写buffer到目标文件中
	while ((rv1 = read(fd1, buf, sizeof(buf))) !=0)
	{
		if((rv2 = write(fd2, buf ,rv1)) < 0)
		{
			printf("count[%d] write fail\n", count);
		}
	count++;
	}
	printf("the file has been copied and executed %d times\n", count);

cleanup:
	if (fd1 > 0)
		close(fd1);
	if (fd2 > 0)
		close(fd2);
	
	return 0;
}	
