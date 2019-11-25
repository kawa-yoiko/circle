#include <linux/kthread.h>
#include <linux/bug.h>
#include <circle/sched/task.h>
#include <circle/sched/scheduler.h>

class CKThread : public CTask
{
public:
	CKThread (int (*threadfn) (void *data), void *data)
	:	m_threadfn (threadfn),
		m_data (data)
	{
	}

	void Run (void)
	{
		(*m_threadfn) (m_data);
	}

private:
	int (*m_threadfn) (void *data);
	void *m_data;
};

struct task_struct *current = 0;

static int next_pid = 1;

#define MAX_THREADS     16
static task_struct tasks[MAX_THREADS];

struct task_struct *kthread_create (int (*threadfn)(void *data),
				    void *data,
				    const char namefmt[], ...)
{
	int pid = next_pid++;
	BUG_ON (pid >= MAX_THREADS);

	task_struct *task = &tasks[pid];
	task->pid = pid;

	CTask *ctask = new CKThread (threadfn, data);
	ctask->SetUserData ((void *) pid);

	return task;
}

void set_user_nice (struct task_struct *task, long nice)
{
}

// TODO: task is waken by default
int wake_up_process (struct task_struct *task)
{
	return 0;
}

void flush_signals (struct task_struct *task)
{
}

static void task_switch_handler (CTask *ctask)
{
	int pid = (int) ctask->GetUserData ();
	BUG_ON (pid < 0 || pid >= MAX_THREADS);
	current = &tasks[pid];
}

int linuxemu_init_kthread (void)
{
	// init a task_struct for the first kthread, which is not created with kthread_create()
	task_struct *task = new task_struct;

	task->pid = next_pid++;

	CTask *ctask = CScheduler::Get ()->GetCurrentTask ();
	ctask->SetUserData (task);

	current = task;

	CScheduler::Get ()->RegisterTaskSwitchHandler (task_switch_handler);

	return 0;
}
