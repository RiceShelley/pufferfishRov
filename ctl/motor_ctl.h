/*
 * Controls motors on ROV
 * via gpio pins that ctrl 
 * electro magnetic relays
 */

#include <wiringPi.h>

enum movements {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	H_LEFT,
	H_RIGHT,
	NIL 
};

/*
 * Set up gpio pins 
 * ignore the magic numbers
 * they only make sense when
 * looking at the wiring of 
 * the ROV 
 */
void init_mot_ctl() 
{
	wiringPiSetup();
	pinMode(3, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(15, OUTPUT);
	pinMode(16, OUTPUT);
	pinMode(1, OUTPUT);	
	pinMode(4, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(8, OUTPUT);
	pinMode(31, OUTPUT);
	pinMode(21, OUTPUT);
	pinMode(23, OUTPUT);
	pinMode(26, OUTPUT);

}

/*
 * ctl left motor 
 * dir 0 = forward
 * dir 1 = backward
 * dir -1 = mot off
 */
void left_mot(int dir)
{
	if (dir == 0) {
		digitalWrite(15, LOW);
		digitalWrite(16, LOW);
		digitalWrite(3, HIGH);
		digitalWrite(5, HIGH);
	} else if (dir == 1) {
		digitalWrite(3, LOW);
		digitalWrite(5, LOW);
		digitalWrite(15, HIGH);
		digitalWrite(16, HIGH);
	} else {
		digitalWrite(3, HIGH);
		digitalWrite(5, HIGH);
		digitalWrite(15, HIGH);
		digitalWrite(16, HIGH);
	}
}

/*
 * ctl right motor 
 * dir 0 = forward
 * dir 1 = backward
 * dir -1 = mot off
 */
void right_mot(int dir)
{
	if (dir == 0) {
		digitalWrite(1, LOW);
		digitalWrite(4, LOW);
		digitalWrite(2, HIGH);
		digitalWrite(8, HIGH);
	} else if (dir == 1) {
		digitalWrite(2, LOW);
		digitalWrite(8, LOW);
		digitalWrite(1, HIGH);
		digitalWrite(4, HIGH);
	} else {
		digitalWrite(1, HIGH);
		digitalWrite(4, HIGH);
		digitalWrite(2, HIGH);
		digitalWrite(8, HIGH);
	}
}

/*
 * ctl vertical motor 
 * dir 0 = forward
 * dir 1 = backward
 * dir -1 = mot off
 */
void vert_mot(int dir)
{
	if (dir == 0) {
		digitalWrite(31, LOW);
		digitalWrite(21, LOW);
		digitalWrite(23, HIGH);
		digitalWrite(26, HIGH);
	} else if (dir == 1) {
		digitalWrite(23, LOW);
		digitalWrite(26, LOW);
		digitalWrite(31, HIGH);
		digitalWrite(21, HIGH);
	} else {
		digitalWrite(31, HIGH);
		digitalWrite(21, HIGH);
		digitalWrite(23, HIGH);
		digitalWrite(26, HIGH);
	}
}

/*
 * func for making ROV ctl 
 * of horizontal movments easy 
 */
void horizontal_mov(enum movements mov) 
{
	switch(mov) {
	case FORWARD:
		left_mot(0);
		right_mot(0);
		break;
	case BACKWARD:
		left_mot(1);
		right_mot(1);
		break;
	case LEFT:
		left_mot(0);	
		right_mot(-1);
		break;
	case RIGHT:
		right_mot(0);
		left_mot(-1);
		break;
	case H_LEFT:
		left_mot(0);
		right_mot(1);
		break;
	case H_RIGHT:
		right_mot(0);
		left_mot(1);
		break;
	case NIL:
		right_mot(-1);
		left_mot(-1);
		break;
	}
}
