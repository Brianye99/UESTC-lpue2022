#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
//#include<signal.h>
#include<errno.h>
//#include<assert.h>


#define MIN_WAIT_TASK_NUM 10/*���queue_size > MIN_WAIT_TASK_NUM ����µ��̵߳��̳߳�*/ 
#define DEFAULT_THREAD_VARY 3/*ÿ�δ����������̵߳ĸ���*/
#define true 1
#define false 0

typedef struct
{
	void *(*function) (void *);/* ����ָ�룬�ص����� */
	void *arg;/* ���溯���Ĳ��� */
} threadpool_task_t;/* �����߳�����ṹ�� */

/* �����̳߳������Ϣ */
typedef struct threadpool_t {
	pthread_mutex_t lock;               /* ������ס���ṹ�� ������������һ��ʹ�� */
	pthread_mutex_t thread_counter;     /* ��¼æ״̬�̸߳������� -- busy_thr_num */
	pthread_cond_t queue_not_full;      /* �����������ʱ�����������߳��������ȴ����������� */
	pthread_cond_t queue_not_empty;     /* ��������ﲻΪ��ʱ��֪ͨ�̳߳��еȴ�������߳� */

	pthread_t *threads;                 /* ����̳߳���ÿ���̵߳�tid������ */
	pthread_t adjust_tid;               /* ������߳�tid */
	threadpool_task_t *task_queue;      /* ������� */

	int min_thr_num;                    /* �̳߳���С�߳��� */
	int max_thr_num;                    /* �̳߳�����߳��� */
	int live_thr_num;                   /* ��ǰ����̸߳��� */
	int busy_thr_num;                   /* æ״̬�̸߳��� */
	int wait_exit_thr_num;              /* Ҫ���ٵ��̸߳��� */

	int queue_front;                    /* task_queue��ͷ�±� */
	int queue_rear;                     /* task_queue��β�±� */
	int queue_size;                     /* task_queue����ʵ�������� */
	int queue_max_size;                 /* task_queue���п��������������� */

	int shutdown;                       /* ��־λ���̳߳�ʹ��״̬��true��false */
} threadpool_t;

typedef struct Caculator
{
	int a;
	int b;

}Caculator;

void* threadpool_thread(void *threadpool);//�̳߳��й����߳�Ҫ�������顣
void *adjust_thread(void *threadpool);    //�������߳�Ҫ�������顣
int threadpool_free(threadpool_t *pool);    //����

//��һЩ��ʼ������
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
	int i;
	threadpool_t *pool = NULL;
	do {
		if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
			printf("malloc threadpool fail");	//�̳߳ش���ʧ�ܴ�ӡ
			break;/*����do while*/
		}

		pool->min_thr_num = min_thr_num;  //�̳߳���С�߳���
		pool->max_thr_num = max_thr_num;  //�̳߳�����߳���
		pool->busy_thr_num = 0;           //æ״̬���߳�����ʼֵΪ0
		pool->live_thr_num = min_thr_num; //���ŵ��߳��� ��ֵ=��С�߳��� 
		pool->wait_exit_thr_num = 0;      //Ҫ���ٵ��̸߳�����ʼֵҲ��ʼΪ0.


		pool->queue_size = 0;                      //�������ʵ��Ԫ�ظ�����ʼֵΪ0
		pool->queue_max_size = queue_max_size;     //����������Ԫ�ظ�����
		pool->queue_front = 0;
		pool->queue_rear = 0;
		pool->shutdown = false;                    //���ر��̳߳�

		/* ��������߳��������� �������߳����鿪�ٿռ�, ������ */
		pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*max_thr_num); //�̳߳����߳�������
		if (pool->threads == NULL) {
			printf("malloc threads fail");	//�߳�IDmallocʧ�ܴ�ӡ
			break;
		}
		memset(pool->threads, 0, sizeof(pthread_t)*max_thr_num);	//��ʼ���ڴ洢���߳�ID

		/* ���п��ٿռ� */
		pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size); //ÿ��Ԫ�ض���һ���ṹ�壬�ṹ������һ������ָ���һ��void*��ָ�롣
		if (pool->task_queue == NULL) {
			printf("malloc task_queue fail");	//�����ڴ�mallocʧ�ܴ�ӡ
			break;
		}

		/* ��ʼ������������������ */
		if (pthread_mutex_init(&(pool->lock), NULL) != 0 || 
			pthread_mutex_init(&(pool->thread_counter), NULL) != 0 ||
			pthread_cond_init(&(pool->queue_not_empty), NULL) != 0 || 
			pthread_cond_init(&(pool->queue_not_full), NULL) != 0)
		{
			printf("init the lock or cond fail");	//��ʼ������������������ʧ�ܴ�ӡ
			break;
		}

		
		pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool); /* �����������߳� */
		pthread_detach(pool->adjust_tid);  //�ѹ����߳�����Ϊ����ģ�ϵͳ����������Դ��
		printf("dajust_thread start\n");
		//�����߳�
		for (i = 0; i < min_thr_num; i++) {
			pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
			printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
		}
		

		sleep(1);  //�ȴ��̴߳�������ٻص��������С�
		return pool;

	} while (0);

	threadpool_free(pool);      /* ǰ��������ʧ��ʱ���ͷ�poll�洢�ռ� */

	return NULL;
}

/* �̳߳��и��������߳� */
void *threadpool_thread(void *threadpool)
{
	threadpool_t *pool = (threadpool_t*)threadpool;

	threadpool_task_t task; //

	while (true)
	{
		/*�մ������̣߳��ȴ���������� �����񣬷��������ȴ������������������ٻ��ѽ�������*/
		pthread_mutex_lock(&(pool->lock));

		/*queue_size == 0 ˵��û�����񣬵� wait ����������������, ��������������while*/
		while ((pool->queue_size == 0) && (!pool->shutdown)) //�̳߳�û�������Ҳ��ر��̳߳ء�
		{
			printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
			pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));//�߳��������������������

			/*���ָ����Ŀ�Ŀ����̣߳����Ҫ�������̸߳�������0�������߳�*/
			if (pool->wait_exit_thr_num > 0)  /* Ҫ���ٵ��̸߳�������0 */
			{
				pool->wait_exit_thr_num--;

				/*����̳߳����̸߳���������Сֵʱ���Խ�����ǰ�߳�*/
				if (pool->live_thr_num > pool->min_thr_num) {
					printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
					pool->live_thr_num--;
					pthread_mutex_unlock(&(pool->lock));
					pthread_exit(NULL);
				}
			}
		}

		/*���ָ����true��Ҫ�ر��̳߳����ÿ���̣߳������˳�����*/
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->lock));
			printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
			pthread_exit(NULL);     /* �߳����н��� */
		}

		//������������ȡ������һ�����Ӳ���
		task.function = pool->task_queue[pool->queue_front].function;
		task.arg = pool->task_queue[pool->queue_front].arg;

		pool->queue_front = ((pool->queue_front + 1) % pool->queue_max_size);//��ͷָ������ƶ�һλ
		pool->queue_size--;													//�ﵽ��������ʱ��ȡ�����0�ص����

		/*��������г���һ��Ԫ�أ�����λ�� ������������������������ϵ��̣߳�����֪ͨ�������µ�������ӽ���*/
		pthread_cond_broadcast(&(pool->queue_not_full)); //queue_not_full��һ������������

		/*����ȡ���������� �̳߳��� �ͷ�*/
		pthread_mutex_unlock(&(pool->lock));

		/*ִ������*/
		printf("thread 0x%x start working\n", (unsigned int)pthread_self());
		pthread_mutex_lock(&(pool->thread_counter));         /*æ״̬�߳���������*/
		pool->busy_thr_num++;                                /*æ״̬�߳���+1*/
		pthread_mutex_unlock(&(pool->thread_counter));
		(task.function)(task.arg);  /*ִ�лص����������൱��process(arg)  */

		/*�����������*/
		printf("thread 0x%x end working\n\n", (unsigned int)pthread_self());
		//usleep(10000);

		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thr_num--;                 /*�����һ������æ״̬���߳���-1*/
		pthread_mutex_unlock(&(pool->thread_counter));

	}
	pthred_exit(NULL);
}

int is_thread_alive(pthread_t tid)
{
	int kill_rc = pthread_kill(tid, 0);     //��0���źţ������߳��Ƿ���
	if (kill_rc == ESRCH) {
		return false;
	}

	return true;
}

/* �����߳� */
void *adjust_thread(void *threadpool)
{
	int i;
	threadpool_t *pool = (threadpool_t *)threadpool;

	while (!(pool->shutdown)) //�̳߳�û�йر�
	{
		sleep(1);                                    /*��ʱ ���̳߳ع���*/
		

		pthread_mutex_lock(&(pool->lock));
		int queue_size = pool->queue_size;                      /* ��ע ������ */
		int live_thr_num = pool->live_thr_num;                  /* ��� �߳��� */
		pthread_mutex_unlock(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));
		int busy_thr_num = pool->busy_thr_num;
		pthread_mutex_unlock(&(pool->thread_counter));                 /* æ�ŵ��߳��� */
		//printf("busy_thr_num is %d\n", busy_thr_num);

		/* �������߳� �㷨�� ������������С�̳߳ظ���, �Ҵ����߳�����������̸߳���ʱ �磺30>=10 && 40<100*/
		if (queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num)
		{
			printf("create new thread\n");

			pthread_mutex_lock(&(pool->lock));
			int add = 0;

			/*һ������ DEFAULT_THREAD ���߳�*/
			for (i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY
				&& pool->live_thr_num < pool->max_thr_num; i++)
			{
				if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i]))
				{
					pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
					add++;
					pool->live_thr_num++;
				}
			}
			pthread_mutex_unlock(&(pool->lock));

		}

		/* ���ٶ���Ŀ����߳� �㷨��æ�߳�X2 С�� �����߳��� �� �����߳��� ���� ��С�߳���ʱ*/
		if ((busy_thr_num * 2) < live_thr_num  &&  live_thr_num > pool->min_thr_num)
		{
			printf("delete pthread\n");

			/* һ������DEFAULT_THREAD���߳�, �S�C10������ */
			pthread_mutex_lock(&(pool->lock));
			pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
			pthread_mutex_unlock(&(pool->lock));

			for (i = 0; i < DEFAULT_THREAD_VARY; i++)
			{
				/* ֪ͨ���ڿ���״̬���߳�, ���ǻ�������ֹ*/
				pthread_cond_signal(&(pool->queue_not_empty));
			}
		}
	}
	return NULL;
}

/* ����������У� ���һ������ */
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg), void *arg)
{
	pthread_mutex_lock(&(pool->lock));

	/* ==Ϊ�棬�����Ѿ����� ��wait���� */
	while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown))
	{
		pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
	}

	if (pool->shutdown)
	{
		pthread_mutex_unlock(&(pool->lock));
	}

	/* ��� �����߳� ���õĻص����� �Ĳ���arg */
	if (pool->task_queue[pool->queue_rear].arg != NULL)
	{
		free(pool->task_queue[pool->queue_rear].arg);
		pool->task_queue[pool->queue_rear].arg = NULL;
	}

	/*����������������*/
	pool->task_queue[pool->queue_rear].function = function; //�ڶ��е�β�����Ԫ��
	pool->task_queue[pool->queue_rear].arg = arg;
	pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size; /* ��βָ���ƶ�, ģ�⻷�� */
	pool->queue_size++;

	/*���������󣬶��в�Ϊ�գ�����������Ϊ�յ��Ǹ������������е��߳�*/
	pthread_cond_signal(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->lock));

	return 0;
}

int threadpool_free(threadpool_t *pool)
{
	if (pool == NULL) {
		return -1;
	}

	if (pool->task_queue) {   //�ͷ��������
		free(pool->task_queue);
	}
	if (pool->threads) {   //�ͷ��̳߳������Լ���������
		free(pool->threads);
		pthread_mutex_unlock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_mutex_unlock(&(pool->thread_counter));
		pthread_mutex_destroy(&(pool->thread_counter));
		pthread_cond_destroy(&(pool->queue_not_empty));
		pthread_cond_destroy(&(pool->queue_not_full));
	}
	free(pool);
	pool = NULL;

	return 0;
}

int threadpool_destroy(threadpool_t *pool)
{
	int i;
	if (pool == NULL) {
		return -1;
	}
	pool->shutdown = true;
	
	/*֪ͨ���еĿ����߳�, ����Ҫѭ���㲥*/
	pthread_cond_broadcast(&(pool->queue_not_empty));
	for (i = 0; i < pool->live_thr_num; i++) {
		pthread_join(pool->threads[i], NULL);
	}
	threadpool_free(pool);

	return 0;
}

/* �̳߳��е��̣߳�ģ�⴦��ҵ�� */
void *process(void *arg)
{
	Caculator ans = *(Caculator*)arg;
	int num1 = ans.a;
	int ca = rand() % 4 + 1;	//1->+,2->-,3->*,4->/
	int num2 = ans.b;
	double answer = 0;

	//���Ӽ��˳�
	//�����쳣��ӡ...
	if (ca == 4 && num2 == 0)
	{
		printf("%d / %d is not available\n", num1, num2);
	}

	switch (ca)
	{
	case 1:
		answer = num1 + num2;
		printf("%d + %d = %f\n", num1, num2, answer);
		break;
	case 2:
		answer = num1 - num2;
		printf("%d - %d = %f\n", num1, num2, answer);
		break;
	case 3:
		answer = num1 * num2;
		printf("%d * %d = %f\n", num1, num2, answer);
		break;
	case 4:

		answer = (float)num1 / num2;

		printf("%d / %d = %f\n", num1, num2, answer);
		break;
	}

	sleep(1);
}

int main()
{
	printf("pool inited\n");
	threadpool_t *thp = threadpool_create(10, 100, 100);/*�����̳߳أ�������С3���̣߳����100���������100*/
	
	for (int i = 0; i < 1000; i++)
	{
		
		char c_str[100];
		Caculator* list = (Caculator*)malloc(sizeof(Caculator));
		//printf("for cirle has run for %d times\n", i);

		list->a = rand() % 100 + 1;
		list->b = rand() % 100 + 1;

		threadpool_add(thp, process, list);     /* ������������������ */
		
	}

	while (true) {
		pthread_mutex_lock(&(thp->thread_counter));
		int busyNum = thp->busy_thr_num;
		int taskNum = thp->queue_size;
		pthread_mutex_unlock(&(thp->thread_counter));

		/* �ͷ��̳߳� */
		if (busyNum == 0 && taskNum == 0) {
			printf("busyNum == 0 && taskNum == 0, clean up thread pool\n");
			threadpool_destroy(thp);
			return 0;
		}
	}

	
}
/* gcc threadpool.c -o threadpool -lpthread */
