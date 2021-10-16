/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

/*
 * Number of times each animal enters kitchen
 */

#define TURNS 3

/*
 * All Semaphores
 */

static struct semaphore *mutex;
static struct semaphore *cats_queue;
static struct semaphore *mice_queue;
static struct semaphore *dish_mutex;
static struct semaphore *cat_done;
static struct semaphore *mouse_done;
static struct semaphore *no_cat_eat_mutex;
static struct semaphore *all_done;

/*
 * All boolean equivalent integers.
 */

int dish1_busy = 0;
int dish2_busy = 0;
volatile int no_cat_eat = 1;
volatile int all_dishes_avail = 1;
int first_cat_eat = 0;
int another_cat_eat = 0;
int first_mouse_eat = 0;
int another_mouse_eat = 0;
volatile int no_mouse_eat = 1;

/*
 * All integers used for counting.
 */

volatile int cat_wait_count = 0;
volatile int mice_wait_count = 0;


/*
 * 
 * Function Definitions
 * 
 */



/*
 * FirstCatNoMouse()
 *
 * Arguments:
 *      int catnum: holds the id of the cat
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function essentially makes the first cat claim a spot in the kitchen
 *
 */
static void FirstCatNoMouse(int catNum){

        P(mutex); //initial mutex to protext all_dishes_avail

        kprintf("\nCat %d is hungry", catNum);

        if(all_dishes_avail){ //if all dishes are available, this cat claims a dish
                all_dishes_avail = 0;
                V(cats_queue);
        }

        cat_wait_count++;
        V(mutex); 
        
        P(cats_queue); //holds cats here if all dishes are not available
        P(no_cat_eat_mutex); //mutex for no_cat_eat section to pace cats better

        if(no_cat_eat){ //if no cat is eating, then must be first cat
                no_cat_eat = 0;
                first_cat_eat = 1;
        } else {
                first_cat_eat = 0;
        }

        V(no_cat_eat_mutex);

}

/*
 * FirstCat()
 *
 * Arguments:
 *      int catnum: holds the id of the cat
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      First cat decides if another cat can join it in the kitchen
 *
 */
static void FirstCat(int catNum){

        if(first_cat_eat){
                P(mutex); //lock critical section of cat_wait_count
                if(cat_wait_count>=2){ //allows a second cat to join if there is one in queue
                        another_cat_eat = 1;
                        V(cats_queue);
                }
                V(mutex);
        }
        kprintf("\nCat %d is in the kitchen", catNum);

}

/*
 * CatDishes()
 *
 * Arguments:
 *      int *myDish: specifies which dish this cat will take
 *      int catnum: holds the id of the cat
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This cat in the kitchen claims a dish to eat from
 *
 */
static void CatDishes(int *myDish, int catNum){

        P(dish_mutex); //protect critical section of dish

        if(!dish1_busy){ //Claims dish 1 if not taken, else takes dish 2
                dish1_busy = 1;
                *myDish = 1;
        } else {
                assert(!dish2_busy);
                dish2_busy = 1;
                *myDish = 2;
        }

        V(dish_mutex);

        kprintf("\nCat %d is eating from dish %d", catNum, *myDish); //Cat eats
        clocksleep(1);
        kprintf("\nCat %d is finished eating from dish %d", catNum, *myDish);

}


/*
 * ReleaseDishCat()
 *
 * Arguments:
 *      int *myDish: specifies which dish this cat holds
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Releases the dish from the cat holding it
 *
 */
static void releaseDishCat(int *myDish){

        P(mutex); //protects cat_wait_count
        P(dish_mutex); //protects if block

        if(*myDish==1){ //releases dish 1 if that is dish taken by this cat, else dish 2
                dish1_busy = 0;
        } else { 
                assert(*myDish==2);
                dish2_busy = 0;
        }

        V(dish_mutex);
        cat_wait_count--;
        V(mutex);

}

/*
 * switchTurnsCat()
 *
 * Arguments:
 *      None
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Prioritizes mice if cats are done eating, then cats, then sets all dishes available
 *
 */
static void switchTurnsCat(){
        
        P(mutex); //protect critical section

        if(mice_wait_count>0){ //switches to mice turn if mice are waiting
                V(mice_queue);
        } else {
                if(cat_wait_count>0){ //keeps turn to cats if cats are waiting
                        V(cats_queue);
                } else {
                        all_dishes_avail = 1; //else free for all
                }
        }

        V(mutex);
}

/*
 * catsLeaving()
 *
 * Arguments:
 *      int catnum: holds the id of the cat
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Process for cats leaving the kitchen
 *
 */
static void catsLeaving(int catNum){

        if(first_cat_eat){

                if(another_cat_eat){
                        P(cat_done); //if first cat and there is another cat eating, must wait on other cat before first cat can leave
                }

                kprintf("\nCat %d is leaving the kitchen", catNum);
                no_cat_eat = 1;
                switchTurnsCat();

        } else {

                kprintf("\nCat %d is leaving the kitchen", catNum);
                V(cat_done); //other cat is ready to leave and signals to first cat

        }

}




/*
 * FirstMouseNoCat()
 *
 * Arguments:
 *      int mouseNum: holds the id of the mouse
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function essentially makes the first mouse claim a spot in the kitchen
 *
 */
static void FirstMouseNoCat(int mouseNum){

        P(mutex); //protects critical section
        kprintf("\nMouse %d is hungry", mouseNum);
        
        if(all_dishes_avail){ //if all dishes are available, this mouse claims one
                all_dishes_avail = 0;
                V(mice_queue);
        }
        
        mice_wait_count++;
        V(mutex);
        P(mice_queue); //holds mice here if they did not claim all_dishes_avail

        if(no_mouse_eat){ //if no mouse is eating, then must be first mouse.
                no_mouse_eat = 0;
                first_mouse_eat = 1;
        } else {
                first_mouse_eat = 0;
        }

}

/*
 * FirstMouse()
 *
 * Arguments:
 *      int mouseNum: holds the id of the mouse
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      First mouse decides if another mouse can join it in the kitchen
 *
 */
static void FirstMouse(int mouseNum){
        
        if(first_mouse_eat){
                P(mutex); //locks this critical section
                if(mice_wait_count>1){ //allows another mouse to join if there is one in the queue
                        another_mouse_eat = 1;
                        V(mice_queue);
                }
                V(mutex);
        }
        
        kprintf("\nMouse %d is in the kitchen", mouseNum);

}

/*
 * MouseDishes()
 *
 * Arguments:
 *      int *myDish: specifies which dish this cat will take
 *      int mouseNum: holds the id of the cat
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This cat in the kitchen claims a dish to eat from
 *
 */
static void MouseDishes(int *myDish, int mouseNum){

        P(dish_mutex); //protects critical section of dish

        if(!dish1_busy){ //claims dish 1 if not busy else dish 2
                dish1_busy = 1;
                *myDish = 1;
        } else {
                assert(!dish2_busy);
                dish2_busy = 1;
                *myDish = 2;
        }

        V(dish_mutex);

        kprintf("\nMouse %d is eating from dish %d", mouseNum, *myDish); //mouse eats
        clocksleep(1);
        kprintf("\nMouse %d is finished eating from dish %d", mouseNum, *myDish);
}

/*
 * ReleaseDishMouse()
 *
 * Arguments:
 *      int *myDish: specifies which dish this mouse holds
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Releases the dish from the mouse holding it
 *
 */
static void releaseDishMouse(int *myDish){

        P(mutex); //protects mice_wait_count
        P(dish_mutex); //protects if block

        if(*myDish==1){ //releases dish 1 if taken by mouse, else dish 2
                dish1_busy = 0;
        } else { 
                assert(*myDish==2);
                dish2_busy = 0;
        }

        V(dish_mutex);
        mice_wait_count--;
        V(mutex);
}

/*
 * switchTurnsMouse()
 *
 * Arguments:
 *      None
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Prioritizes cats if mice are done eating, then mice, then sets all dishes available
 *
 */
static void switchTurnsMouse(){

        P(mutex); //protect critical section

        if(cat_wait_count>0){ //switches turn to cats if cats are waiting
                V(cats_queue);
        } else {
                if(mice_wait_count>0){ //keeps turn to mice if mice are waiting
                        V(mice_queue);
                } else {
                        all_dishes_avail = 1; //else free for all
                }
        }
        V(mutex);
}

/*
 * mouseLeaving()
 *
 * Arguments:
 *      int mousenum: holds the id of the mouse
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Process for mouse leaving the kitchen
 *
 */
static void mouseLeaving(int mouseNum){

        if(first_mouse_eat){

                if(another_mouse_eat){ 
                        P(mouse_done); //if another mouse is eating, first cat waits until it is done
                }

                no_mouse_eat = 1;
                kprintf("\nMouse %d is leaving the kitchen", mouseNum);
                switchTurnsMouse();

        } else {

                kprintf("\nMouse %d is leaving the kitchen", mouseNum);
                V(mouse_done); //if another mouse is eating, signals to first mouse that it is done

        }


}


/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;

        int myDish = 0; 
        int i;
        for(i = 0; i<TURNS; i++){
                clocksleep(2); //cat plays
                FirstCatNoMouse(catnumber);
                FirstCat(catnumber);
                CatDishes(&myDish, catnumber);
                releaseDishCat(&myDish);
                catsLeaving(catnumber);
                switchTurnsCat();
        }
        kprintf("\nCat %d is done", (int)catnumber);
        V(all_done); //signals that this cat is done


}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) mousenumber;

        int myDish = 0;
        int i;
        for(i = 0; i<TURNS; i++){
                clocksleep(2); //mouse plays
                FirstMouseNoCat(mousenumber);
                FirstMouse(mousenumber);
                MouseDishes(&myDish, mousenumber);
                releaseDishMouse(&myDish);
                mouseLeaving(mousenumber);
                switchTurnsMouse();
        }
        kprintf("\nMouse %d is done", (int)mousenumber);
        V(all_done); //signals that this mouse is done




}

/*
 * setup()
 *
 * Arguments:
 *      none
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      initializes all semaphores
 *
 */
void setup(){
        mutex = sem_create("mutex", 1);
        if(mutex==NULL){
                panic("Out Of Memory");
        }
        cats_queue = sem_create("cats queue", 0);
        if(cats_queue==NULL){
                panic("Out Of Memory");
        }
        mice_queue = sem_create("mice queue", 0);
        if(mice_queue==NULL){
                panic("Out Of Memory");
        }
        dish_mutex = sem_create("dish mutex", 1);
        if(dish_mutex==NULL){
                panic("Out Of Memory");
        }
        cat_done = sem_create("cat done", 0);
        if(cat_done==NULL){
                panic("Out Of Memory");
        }
        mouse_done = sem_create("mouse done", 0);
        if(mouse_done==NULL){
                panic("Out Of Memory");
        }
        no_cat_eat_mutex = sem_create("no cat eat mutex", 1);
        if(no_cat_eat_mutex==NULL){
                panic("Out Of Memory");
        }
        all_done = sem_create("all done", 0);
        if(all_done==NULL){
                panic("Out Of Memory");
        }

}

/*
 * cleanup()
 *
 * Arguments:
 *      none
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      deletes all semaphores
 *
 */
void cleanup(){
        int i;
        for(i = 0; i<NCATS+NMICE; i++){
                P(all_done); //waits for signal from each thread to indicate simulation is done
        }
        sem_destroy(mutex);
        sem_destroy(cats_queue);
        sem_destroy(mice_queue);
        sem_destroy(dish_mutex);
        sem_destroy(cat_done);
        sem_destroy(mouse_done);
        sem_destroy(no_cat_eat_mutex);
        sem_destroy(all_done);
        kprintf("\nSimulation is complete\n");
}
/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
        setup();
        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        cleanup();
        return 0;
}


/*
 * End of catsem.c
 */
