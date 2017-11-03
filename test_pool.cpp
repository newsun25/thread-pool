#include <iostream>
#include <queue>
#include <pthread.h>
#include <cstdlib>

using namespace std;

#define MAX_TASK_NUM	1024


template <typename _Ty>
class CPool
{
public:
	CPool()=default;
	CPool(int num_que, int que_len);
	~CPool();
	int init();

	int enter(_Ty &e);

private:
	static void *worker_cb(void *arg);
	void proc(long arg);

private:
	int m_num_que;
	int m_que_len;
	queue<_Ty> * m_que_manager;
	int m_cur_que;
	pthread_mutex_t m_mtx_cur;
	pthread_mutex_t * m_mtx_manager;
	pthread_cond_t	* m_cond_manager;
};

template <typename _Ty>
struct _t_task_func
{
	void (CPool<_Ty>::*func)(long arg);	
	CPool<_Ty> *pobj;
	int ipara;
};

typedef _t_task_func<int> t_task_func;

t_task_func g_task[MAX_TASK_NUM] = {0x0};

#if 0
template <typename _Ty>
pthread_mutex_t * CPool<_Ty>::m_mtx_manager = NULL;

template <typename _Ty>
pthread_cond_t * CPool<_Ty>::m_cond_manager = NULL;

template <typename _Ty>
queue<_Ty> * CPool<_Ty>::m_que_manager = NULL;
#endif

template <typename _Ty>
CPool<_Ty>::CPool(int num_que, int que_len):
	m_num_que(num_que),
	m_que_len(que_len),
	m_cur_que(0),
#if 1
	m_mtx_manager(NULL),
	m_cond_manager(NULL),
	m_que_manager(NULL)
#endif
{
	m_que_manager = new queue<_Ty>[num_que];

	pthread_mutex_init(&m_mtx_cur, NULL);
	m_mtx_manager = new pthread_mutex_t[num_que];
	m_cond_manager = new pthread_cond_t[num_que];
	for (int i = 0; i!= m_num_que; ++i)
	{
		pthread_mutex_init(&(m_mtx_manager[i]), NULL);
		pthread_cond_init(&(m_cond_manager[i]), NULL);
	}
}

template <typename _Ty>
CPool<_Ty>::~CPool()
{
	pthread_mutex_destroy(&m_mtx_cur);
	for (int i = 0; i!= m_num_que; ++i)
	{
		pthread_mutex_destroy(&(m_mtx_manager[i]));
		pthread_cond_destroy(&(m_cond_manager[i]));
	}
	
	if (m_mtx_manager)
	{
		delete [] m_mtx_manager;
		m_mtx_manager = NULL;
	}

	if (m_que_manager)
	{
		delete [] m_que_manager;
		m_que_manager = NULL;
	}

	if (m_cond_manager)
	{
		delete [] m_cond_manager;
		m_cond_manager = NULL;
	}
}

template <typename _Ty>
void CPool<_Ty>::proc(long arg)
{
	long num = (long)arg;
	for (;;)
	{
		printf("debug: this[%p]\n", this);
		pthread_mutex_lock(&(m_mtx_manager[num]));
		while(m_que_manager[num].size()<=0)
		{
			pthread_cond_wait(&(m_cond_manager[num]), &(m_mtx_manager[num]));
		}
		
		if (m_que_manager[num].size())
		{
			_Ty e = m_que_manager[num].front();
			m_que_manager[num].pop();

			cout << "handle que element: " << e << endl;
		}
		pthread_mutex_unlock(&(m_mtx_manager[num]));
	}
}


typedef void (*t_func)(long arg);

typedef struct
{
	void *pobj;
	int	i;
} t_arg;

template <typename _Ty>
void *CPool<_Ty>::worker_cb(void *arg)
{
	t_arg *parg = (t_arg *)arg;
	CPool<_Ty> *pobj = (CPool<_Ty> *)parg->pobj;
	long i = parg->i;
	
	printf("debug: before func\n");
	pobj->proc(i);

	if (parg)
		free(parg);
}

template <typename _Ty>
int CPool<_Ty>::init()
{
	for (int i=0; i!=m_num_que && i<MAX_TASK_NUM; ++i)
	{
		t_arg *parg = (t_arg *)malloc(sizeof(t_arg));
		if (!parg)
			return -1;
		parg->pobj = this;
		parg->i = i;

		pthread_t pid;
		pthread_create(&pid, NULL, worker_cb, (void *)parg);
	}

	return 0;
}

template <typename _Ty>
int CPool<_Ty>::enter(_Ty &e)
{
	pthread_mutex_lock(&m_mtx_cur);
	m_cur_que = (m_cur_que+1)%m_num_que;
	pthread_mutex_unlock(&m_mtx_cur);

	pthread_mutex_lock(&(m_mtx_manager[m_cur_que]));
	m_que_manager[m_cur_que].push(e);
	pthread_cond_signal(&(m_cond_manager[m_cur_que]));
	pthread_mutex_unlock(&(m_mtx_manager[m_cur_que]));

	return 0;
}

int main()
{
	CPool<int> pool(10, 10);
	pool.init();

	int e = 23;

	pool.enter(e);

	sleep(100);

	return 0;
}

