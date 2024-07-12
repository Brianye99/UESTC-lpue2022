#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>


void List(char* path, char* string)
{
	struct dirent* ent = NULL;
	DIR* pDir;
	pDir = opendir(path);

	while (NULL != (ent = readdir(pDir)))
	{
		int type = ent->d_type;
		if (type == 8)  //如果是文件
		{
			char str[4096] = { 0 }; //用于扫描文件中的内容

			//用于拼接文件名，在文件名前面加上绝对路径
			char name[100];
			strcpy(name, path);
			strcat(name, "/");
			strcat(name, ent->d_name);
			//printf("file name is %s\n", name);
			FILE* fp = fopen(name, "r");   //文件名中包括绝对路径
			if (fp == NULL)
			{
				perror("fopen falid");
			}

			while (fgets(str, 4096, fp) != NULL)
			{
				//printf("str is %s\n", str);
				if (strstr(str, string) != NULL) //判断str中是否有string(main函数传入的参数)
				{
					printf("path is[%s]\n", path);
					printf("the file is %s\n",ent->d_name);
					close(fp); //关闭文件描述符
					return ;
					//exit(0); //若找到，直接退出程序。若使用return，则只会退出当前函数，还会继续进行扫描
				}
			}
			close(fp); //关闭文件描述符

		}
		else if (type == 4 && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..")) //如果是目录，并且跳过当前路径和上级目录(. ..)
		{
			//printf("child dir：%s\n", ent->d_name); //子目录名字，不包括绝对路径

			char tmp[100];     //用于拼接子路径
			strcpy(tmp, path);
			strcat(tmp, "/");
			strcat(tmp, ent->d_name);     //拼接路径，在List函数中传入绝对路径
			List(tmp, string); //递归执行

			//printf("return %s\n", ent->d_name);
		}
	}
	
}


int main(int argc, char* argv[])
{
	//文件返回值判断
	if (argc != 3)
	{
		printf("no directory input\n");
		exit(0);
	}

	List(argv[1], argv[2]);
}
