// cc -o pwm-controller pwm-controller.c -lwiringPi
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#include <wiringPi.h>
#include <softPwm.h>

#include "controller.h"


struct ps3ctls {
	
	int fd;
	unsigned char nr_buttons;	// Max number of Buttons
	unsigned char nr_sticks;	// Max number of Sticks
	short *button;			// button[nr_buttons]
	short *stick;			// stick[nr_sticks]
};


int ps3c_test(struct ps3ctls *ps3dat) {

	unsigned char i;
	unsigned char nr_btn = ps3dat->nr_buttons;
	unsigned char nr_stk = ps3dat->nr_sticks;
	int y;

//	printf("%d %d\n",nr_btn,nr_stk);

  	printf(" 1=%2d ",ps3dat->button[PAD_KEY_LEFT]);
	printf(" 2=%2d ",ps3dat->button[PAD_KEY_RIGHT]);
	printf(" 3=%2d ",ps3dat->button[PAD_KEY_UP]);
	printf(" 4=%2d ",ps3dat->button[PAD_KEY_DOWN]);
	printf(" 5=%4d ",ps3dat->stick [PAD_LEFT_X]);
	printf(" 6=%4d ",ps3dat->stick [PAD_LEFT_Y]);
	printf(" 7=%4d ",ps3dat->stick [PAD_RIGHT_X]);
	printf(" 8=%4d ",ps3dat->stick [PAD_RIGHT_Y]);
	printf("\n"); 
	

	y = ps3dat->stick [PAD_LEFT_Y];
	if(abs(y) < 15) {
		softPwmWrite(23,0);		// GP13
		softPwmWrite(25,0);		// GP26
	} else if(y > 0) {
		softPwmWrite(23,abs(y));	// GP13
		softPwmWrite(25,0);		// GP26
	} else {
		softPwmWrite(23,0);		// GP13
		softPwmWrite(25,abs(y));	// GP26
	};
	
	if(ps3dat->button[PAD_KEY_CROSS]==1) return -1; // end of program

	return 0;
}


int ps3c_input(struct ps3ctls *ps3dat) {

	int rp;
	struct js_event ev;

	do {
		rp = read(ps3dat->fd, &ev, sizeof(struct js_event));
		if (rp != sizeof(struct js_event)) {
			return -1;
		}
	} while (ev.type & JS_EVENT_INIT);

	switch (ev.type) {
		case JS_EVENT_BUTTON:
			if (ev.number < ps3dat->nr_buttons) {
				ps3dat->button[ev.number] = ev.value;
			}
			break;
		case JS_EVENT_AXIS:
			if (ev.number < ps3dat->nr_sticks) {
				ps3dat->stick[ev.number] = ev.value / 327; // range -32767 ~ +32768 -> -100 ~ +100
			}
			break;
		default:
			break;
	}

	return 0;
}


int ps3c_getinfo(struct ps3ctls *ps3dat) {

	if(ioctl(ps3dat->fd , JSIOCGBUTTONS , &ps3dat->nr_buttons) < 0) return -1;
	if(ioctl(ps3dat->fd , JSIOCGAXES    , &ps3dat->nr_sticks ) < 0) return -2;

	return 0;
}


int ps3c_init(struct ps3ctls *ps3dat, const char *df) {

	unsigned char nr_btn;
	unsigned char nr_stk;
	unsigned char *p;
	int i;

	ps3dat->fd = open(df, O_RDONLY);
	if (ps3dat->fd < 0) return -1;

	if (ps3c_getinfo(ps3dat) < 0) {
		close(ps3dat->fd);
		return -2;
	}

	nr_btn = ps3dat->nr_buttons;
	nr_stk = ps3dat->nr_sticks;
//	printf("%d %d\n",nr_btn,nr_stk);

	p = calloc(nr_btn + nr_stk , sizeof(short));
	if (p == NULL) {
		close(ps3dat->fd);
		return -3;
	}
	ps3dat->button = (short *)p;
	ps3dat->stick  = (short *)&p[nr_btn * sizeof(short)];
	
//	for (i = 0; i<nr_btn; i++) ps3dat->button[i] = 0;
//	for (i = 0; i<nr_stk; i++) ps3dat->stick [i] = 0;
//	ps3dat->button[PAD_KEY_LEFT]=0;
//	ps3dat->button[PAD_KEY_RIGHT]=0;
//	ps3dat->button[PAD_KEY_UP]=0;;
//	ps3dat->button[PAD_KEY_DOWN]=0;
//	ps3dat->stick [PAD_LEFT_X]=0;
//	ps3dat->stick [PAD_LEFT_Y]=0;
//	ps3dat->stick [PAD_RIGHT_X]=0;
//	ps3dat->stick [PAD_RIGHT_Y]=0;

	return 0;
}

void ps3c_exit   (struct ps3ctls *ps3dat) {

	free (ps3dat->button);
	close(ps3dat->fd);
}


void main() {

	char *df = "/dev/input/js0";
	struct ps3ctls ps3dat;

	wiringPiSetup();
	softPwmCreate(23,0,100); // GP13 start-0 10ms
	softPwmCreate(25,0,100); // GP26 start-0 10ms

	if(!(ps3c_init(&ps3dat, df))) {

		do {
			if (ps3c_test(&ps3dat) < 0) break;
		} while (!(ps3c_input(&ps3dat)));
		
		ps3c_exit(&ps3dat);		
	}
}

