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
		if (type == 8)  //������ļ�
		{
			char str[4096] = { 0 }; //����ɨ���ļ��е�����

			//����ƴ���ļ��������ļ���ǰ����Ͼ���·��
			char name[100];
			strcpy(name, path);
			strcat(name, "/");
			strcat(name, ent->d_name);
			//printf("file name is %s\n", name);
			FILE* fp = fopen(name, "r");   //�ļ����а�������·��
			if (fp == NULL)
			{
				perror("fopen falid");
			}

			while (fgets(str, 4096, fp) != NULL)
			{
				//printf("str is %s\n", str);
				if (strstr(str, string) != NULL) //�ж�str���Ƿ���string(main��������Ĳ���)
				{
					printf("path is[%s]\n", path);
					printf("the file is %s\n",ent->d_name);
					close(fp); //�ر��ļ�������
					return ;
					//exit(0); //���ҵ���ֱ���˳�������ʹ��return����ֻ���˳���ǰ�����������������ɨ��
				}
			}
			close(fp); //�ر��ļ�������

		}
		else if (type == 4 && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..")) //�����Ŀ¼������������ǰ·�����ϼ�Ŀ¼(. ..)
		{
			//printf("child dir��%s\n", ent->d_name); //��Ŀ¼���֣�����������·��

			char tmp[100];     //����ƴ����·��
			strcpy(tmp, path);
			strcat(tmp, "/");
			strcat(tmp, ent->d_name);     //ƴ��·������List�����д������·��
			List(tmp, string); //�ݹ�ִ��

			//printf("return %s\n", ent->d_name);
		}
	}
	
}


int main(int argc, char* argv[])
{
	//�ļ�����ֵ�ж�
	if (argc != 3)
	{
		printf("no directory input\n");
		exit(0);
	}

	List(argv[1], argv[2]);
}
