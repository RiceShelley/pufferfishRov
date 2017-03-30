/*-----------------------------------------------------------------
 * code for puffer fish ROV controler                             *
 *                                                                *
 * - what is a puffer fish ROV? it's a pbc ROV that               * 
 * is used in high school ROV compitions                          *
 *                                                                *
 * <--Info-->                                                     *
 * - compled with gcc on a 32bit ARM distro of Arch Linux         *
 *                                                                *
 * - dependences ptheads and wiringPi <- little raspberry pi lib  *
 *   for controling gpio pins                                     *
 *                                                                *
 * - joystick used (Logitech Attack 3) <- other joysticks might   *
 *   work but program was built for this one and using any other  *
 *   could cause a less than satisfactory performance             *
 *                                                                *
 * <--Personal-->                                                 *
 * Hey you! I like your face, and constructive criticism          *
 * want to critique me, or fight me for using 8 space tabs?       *
 * email me -> rootieDev@gmail.com                                *
 *----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/joystick.h>
#include <stdbool.h>
#include <pthread.h>
#include "motor_ctl.h"

#define PRESS_TIME_L 40

// Direction of movment
int dir_of_movment = 0;

// Is recording in process flag
bool mot_rec = false;

// Time of presses <- max of 10 recoradable presses
__u32 rec_press_time[PRESS_TIME_L];

// Var for playing back recording
bool play_back_rec = false;

// Threading varibles forgive me father for making these global
pthread_t thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * set all indexe's of rec_press_time to -1 
 * this is done so other functions know
 * when to stop reading from the array
 */
void clear_rec_presses() 
{
	for (int i = 0; i < PRESS_TIME_L; i++)
		rec_press_time[i] = -1;
}


/*
 * Read data from usb joystick
 */
int ctl_listen(int *fd, struct js_event *e) 
{
	size_t s = read(*fd, e, sizeof(*e));
	return (s =! sizeof(*e));
}

/*
 * mimic button presses made by user
 * this func loops until global var play_back_rec is false
 * NOTE run this func via passing it to a 
 *  pthread_create() call 
 * Refenced functions such as ver_mot() can be found
 * in motor_ctl.h <- these functions just change
 * the states of motors on the pufferfish ROV
 */
void *do_recording(void *p_temp) 
{
	int real_l;
	pthread_mutex_lock(&mutex);
	__u32 *p = (__u32 *) p_temp;
	__u32 rpt[PRESS_TIME_L];
	for (int i = 0; i < PRESS_TIME_L; i++) {
		if (p[i] == -1) {
			real_l = i - 1;
			break;
		}
		rpt[i] = p[i];
	}
	pthread_mutex_unlock(&mutex);
	while (true) {
		usleep((rpt[1] - rpt[0]) * 1000);
		vert_mot(0);
		for (int i = 2; i < real_l; i++) {
			usleep(((int) (rpt[i] - rpt[i - 1])) * 1000);
			if ((i % 2) == 0) {
				vert_mot(-1);
			} else {
				vert_mot(0);
			}
		}
		usleep((rpt[real_l] - rpt[real_l - 1]) * 1000);
		pthread_mutex_lock(&mutex);
		if (!play_back_rec) 
			break;
		pthread_mutex_unlock(&mutex);	
	}
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}

/*
 * axis() function interprets data from a 
 * js_event struct having to do with
 * the movment of the ROV joystick
 * func uses this data to tell
 * ROV what to do via the motor_ctl.h header file
 */
void axis(struct js_event *e) 
{
	switch(e->number) {
	case 1:
		if (e->value < -25000) {
			horizontal_mov(FORWARD);
			dir_of_movment = 1;
		} else if (e->value > 25000) {
			horizontal_mov(BACKWARD);
			dir_of_movment = -1;
		} else {
			if (dir_of_movment != 0)		
				horizontal_mov(NIL);
			dir_of_movment = 0;
		}
		break;
	case 0: 
		if (!dir_of_movment) {
			if (abs(e->value) > 30000) {
				if (e->value < 0)
					horizontal_mov(H_LEFT);
				else
					horizontal_mov(H_RIGHT);
			} else if (abs(e->value) > 5000) {
				if (e->value < 0)
					horizontal_mov(LEFT);
				else
					horizontal_mov(RIGHT);
			} else
				horizontal_mov(NIL);
		}
		break;
	}
}

/*
 * processes js_event struct data
 * and does operations based off
 * what buttons are pressed
 * on joystick
 */
void button(struct js_event *e) 
{
	switch(e->number) {
	case 0:
		if (mot_rec) {
			int i = 0;
			while (rec_press_time[i] != -1) 
				i++;
			rec_press_time[i] = e->time;
		}
		if (e->value)
			vert_mot(0);
		else
			vert_mot(-1);
		break;
	case 2:
		if (e->value || rec_press_time[1] == -1)
			break;
		if (!play_back_rec) {
			play_back_rec = true;
			pthread_create(&thread, NULL, do_recording, (void *) rec_press_time);
		} else {
			play_back_rec = false;
			pthread_join(thread, NULL);
		}
		break;
	case 3:
		if (e->value) {
			clear_rec_presses();
			rec_press_time[0] = e->time;
			mot_rec = true;
		} else {
			int i = 0; 
			while (rec_press_time[i] != -1)
				i++;
			rec_press_time[i] = e->time;
			mot_rec = false;
		}
		break;
	}
}

int main() 
{
	pthread_mutex_init(&mutex, NULL);
	struct js_event e;
	int fd = open("/dev/input/js0", O_RDWR);
	// Init stuff in motor_ctl.h
	init_mot_ctl();
	while (true) {
		usleep(1000 * 10);
		if (ctl_listen(&fd, &e) == 0) {
			if (e.type == 1) 
				button(&e);
			else
				axis(&e);
		}
	}
	return 0;
}
