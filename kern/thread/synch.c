/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name); // sets name of lock
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
	lock->holder = NULL; // sets thread holder to nothing initially
	// add stuff here as needed
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	// add stuff here as needed
	kfree(lock->holder); // destroys everything associated with lock
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{

	int spl = splhigh(); // turn off interrupts

	if(lock_do_i_hold(lock)) // panics if current process is already being held
	{ 
		panic("lock %s at %p: Deadlock\n", lock->name, lock);
	}

	while(lock->holder != NULL) // sleeps lock if it is holding a thread
	{ 
		thread_sleep(lock);
	}

	lock->holder = curthread; // sets lock to hold current thread
	splx(spl); // restore interrupts 

}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	int spl = splhigh(); // disable interrupts
	
	thread_wakeup(lock); // wakes up thread
	
	lock->holder = NULL; // releases thread from lock
	
	splx(spl); // restores interrupt
}

int
lock_do_i_hold(struct lock *lock)
{
	assert(lock != NULL);

	int spl = splhigh(), same = 0; // disables interrupts

	if(lock->holder != NULL && lock->holder == curthread)
	{
		same = 1; // sets same to 1 (true) if lock is holding current thread
	}

	splx(spl); // restores interrupts

	return same; 

}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// nothing to be added
	// add stuff here as needed
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	// nothing to be added
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{

	assert(cv != NULL);
	assert(lock != NULL);

	int spl = splhigh(); // disables interrupts

	if(!lock_do_i_hold(lock)){ // panics if lock is not holding a thread and attempts to wait
		panic("lock %s at %p: Deadlock\n", lock->name, lock);
	}
	

	lock_release(lock); // releases lock 

	thread_sleep(cv); // sleeps the cv

	lock_acquire(lock); // reacquires lock

	splx(spl); // enables interrupts
}

void
cv_signal(struct cv *cv, struct lock *lock)
{

	assert(cv != NULL);
	assert(lock != NULL);

	int spl = splhigh(); // disables interrupts

	if(!lock_do_i_hold(lock)){ //panics is lock does not hold current thread and is attempting to signal
		panic("cv_signal error: cv %s at %p, lock %s at %p.\n", cv->name, cv, lock->name, lock);
	}
	
	
	thread_wakeup_one(cv); // wakes up one thread

	splx(spl); // restores interrupts
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	int spl = splhigh(); // disables interrupts

	thread_wakeup(cv); //wakes up all threads
	
	splx(spl); // enables interrupts
}
