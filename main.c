/*
* main.c
*
*  Created on: Mar 27, 2014
*      Author: se3910
*/


/*********************************************************************
* This program will allow on to turn a given port on and off at a user
* defined rate.  The code was initially developed by Dingo_aus, 7 January 2009
* email: dingo_aus [at] internode <dot> on /dot/ net
* From http://www.avrfreaks.net/wiki/index.php/Documentation:LinuxGPIO#gpio_framework (Note: This link is now broken.)
* Created in AVR32 Studio (version 2.0.2) running on Ubuntu 8.04
* Modified by Mark A. Yoder, 21-July-2011
* Refactored and further modified by Walter Schilling, Summer 2012 / Winter 2013-2014.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include <stdint.h>
#include "gpioInterface.h"

#define POLL_TIMEOUT (1 * 1000) /* 1 second */
#define MAX_BUF 64

// create a variable to keep track of the port that is to be opened as a string.
static int keepgoing = 1;	// Set to 0 when ctrl-c is pressed

/*******************************************************************************
* This method will read the given pin and perform processing on it.
*
* @param FILE *fp - This is a pointer to the io device driver which will be read to determine the state of the pin.
******************************************************************************/
int processPin(int gpio_fd, int outputPin, uint32_t gpioInputPort)
{
	struct pollfd fdset[2];
	int timeout, rc;
	int nfds = 2;
	char buf[MAX_BUF];
	int len;

	//create a variable to store whether we are sending a '1' or a '0'
	char set_value[5];
	char prevState;

	prevState = '?';
	timeout = POLL_TIMEOUT;

	while (keepgoing) {
		memset((void*)fdset, 0, sizeof(fdset));

		//fdset[0].fd = STDIN_FILENO;
		//fdset[0].events = POLLIN;

		fdset[1].fd = gpio_fd;
		fdset[1].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      // Sits and waits for an interrupt.

		if (rc < 0) {
			printf("\npoll() failed!  Something is going on.  Is the program exiting?\n");
			return -1;
		}

		if (rc == 0) {
			printf(".");
		}

		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n", gpioInputPort, buf[0], len);

			if (buf[0]!=prevState)
			{
				if (buf[0]=='0')
				{
					printf("The button is pressed.\n");

					//Write our value of "0" to the file
					gpio_set_value(outputPin, 0);
				}
				else
				{
					printf("The button is not pressed.\n");

					//Write our value of "1" to the file
					gpio_set_value(outputPin, 1);
				}
				prevState = buf[0];
			}
		}

		fflush(stdout);
	}
}

/****************************************************************
* signal_handler
****************************************************************/
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}



/****************************************************************
* Main
****************************************************************/
int main(int argc, char **argv)
{
	/* This variable is a file pointer to the file interface for the GPIO device driver
	  which controls the input pin. */
	uint32_t gpio_fd; // This is the file ID for the input file.

	uint8_t gpioSwitchP1 = 48; //Input Switch for Player 1 (GPIO1_16)
	uint8_t gpioSwitchP2 = 49; //Input Switch for Player 2 (GPIO0_26)

	uint8_t gpioLEDP1= 44; //Output LED light for Player 1 (GPIO1_12)
	uint8_t gpioLEDP2= 26; //Output LED light for Player 2 (GPIO0_26)

	char ioPort[56];   // This is the IO port that is to be referenced.

	if (argc < 3) {
		exit(-1);
	}

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	// Convert the input into the appropriate parameters.
	char player1[32];
	char player2[32];
	strcpy(player1, argv[1]);
	strcpy(player2, argv[2]);

	printf("Welcome to the game of Anticipation, %s and %s!", player1, player2);

	// Setup the input ports
	(void)gpio_export(gpioSwitchP1);
	(void)gpio_set_dir(gpioSwitchP1, 0);
	(void)gpio_set_edge(gpioSwitchP1, GPIO_BOTH_EDGES);  // Both indicates that an interrupt will fire on both a rising and falling edge.
	gpio_fd = gpio_fd_open(gpioSwitchP1);

	(void)gpio_export(gpioSwitchP2);
	(void)gpio_set_dir(gpioSwitchP2, 0);
	(void)gpio_set_edge(gpioSwitchP2, GPIO_BOTH_EDGES);  // Both indicates that an interrupt will fire on both a rising and falling edge.
	gpio_fd = gpio_fd_open(gpioSwitchP2);

	// Setup the output ports
	(void)gpio_export(gpioLEDP1);
	(void)gpio_set_dir(gpioLEDP1, 1);
	(void)gpio_fd_open(gpioLEDP1);

	(void)gpio_export(gpioLEDP2);
	(void)gpio_set_dir(gpioLEDP2, 1);
	(void)gpio_fd_open(gpioLEDP2);


	//GAME STUFF HERE



	// Start the process pin routine which will handle processing the pin.
	processPin(gpio_fd, gpioLEDP1, gpioSwitchP1);

	//***********************************************************************
	// cleanup the executing system
	// Close the pins
	gpio_fd_close(gpio_fd);

	// Unexport the pins.
	gpio_unexport(gpioSwitchP1);
	gpio_unexport(gpioSwitchP2);
	gpio_unexport(gpioLEDP1);
	gpio_unexport(gpioLEDP2);

	printf("Peace out girl scout\n");
	return 0;
}