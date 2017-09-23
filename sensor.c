// Demo code for Grove - Temperature Sensor V1.1/1.2
// Loovee @ 2015-8-26

// #include <math.h> //for log
// #include <mraa/aio.h> //for the temp sensor
// #include <stdio.h> //for input output
// #include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h> 
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
           


const int R0 = 100000;
const int B = 4275;

/* FOR THE OPTION FLAGS */
char scale = 'F';
int period = 1; //global period is 1
int logFlag = 0; //log flag is initially zero
int logfd;
int stopped = 0;

char off[]      = "OFF";
char stop[]     = "STOP";
char start[]    = "START";
char scalef[]   = "SCALE=F";
char scalec[]   = "SCALE=C";

mraa_aio_context pinTempSensor; //grove temperature, attached ot A0
mraa_gpio_context button;

int end_flag = 0;


FILE* logFile;


void checkButton() //first thread to check for button
{
	button  = mraa_gpio_init(3);
	mraa_gpio_dir(button, MRAA_GPIO_IN);

	/* for time stamp */
	char buffer[9]; //e.g. 17:25:58\0 (plus the nullbyte)
	time_t curtime; //current time var
	struct tm *loctime; //time structure for local time


	while(1)
	{		

		curtime = time (NULL);

		/* converts the simple time pointed to by time to broken-down time representation */
		loctime = localtime(&curtime);

		strftime(buffer, 9, "%H:%M:%S", loctime); //now buffer has the specified time


		if (end_flag || mraa_gpio_read(button))
		{
			//fprintf(stdout, "%s SHUTDOWN\n", buffer);
			if ( logFlag == 1)
				fprintf(logFile,"%s SHUTDOWN\n", buffer);

			//fprintf(stdout,"%s SHUTDOWN\n", buffer);
			exit(0);
		}
	}
	pthread_exit(0);
}

void printTemp() //second thread to print the temp
{

	char buffer[9]; //e.g. 17:25:58\0 (plus the nullbyte)

	time_t curtime; //current time var
	struct tm *loctime; //time structure for local time

	pinTempSensor = mraa_aio_init(0);
	
	// mraa_gpio_context button = mraa_gpio_init(3);

	// mraa_gpio_dir(button, MRAA_GPIO_IN);

	while(1)
	{
		//put these dudes in here, so that time is constantly being updated

		/* get the current time */
		curtime = time (NULL);

		/* converts the simple time pointed to by time to broken-down time representation */
		loctime = localtime(&curtime);

		strftime(buffer, 9, "%H:%M:%S", loctime); //now buffer has the specified time

		int a = mraa_aio_read(pinTempSensor); //TEMPERATURE SENSOR CONNECTED ANALOG 0

		// if (mraa_gpio_read(button))
		// {
		// 	fprintf(stdout, "SHUTDOWN\n");
		// 	exit(0);
		// }

		double R = 1023.0/((double)a) -1.0;
	    R = R0*R;

	     double celsius = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
	     double farenheit = celsius * (9/5) + 32;

	     if ( stopped == 0)
	     {	
		     if ( logFlag == 1)
		     {
		     	if ( scale == 'C') //if scale is celsius
				{
			     	 fprintf(logFile,"%s %.1f\n",buffer, celsius);
			     	 fflush(logFile); //makes sure that the output is persistent
			    	// fprintf(logFile, "farenheit is :%.1f\n", farenheit);
		     	}
		     	else
		     	{
		     		 fprintf(logFile, "%s %.1f\n",buffer, farenheit);
		     		 fflush(logFile);
		     	}
		     }

		     if ( scale == 'C')
		     	fprintf(stdout,"%s %.1f\n",buffer, celsius);

		     if ( scale == 'F')
		     	fprintf(stdout, "%s %.1f\n", buffer, farenheit);

		   }


	     sleep(period); //leslie

	}	

}




int main(int argc, char *argv[])
{

	
	static struct option long_options[] = {

		{"period",required_argument, 0, 'p'}, //for period
		{"scale", required_argument, 0, 's'}, //for scale
		{"log", required_argument, 0, 'l'},
		{0,0,0,0} //last one should be all 0's

	};

	int ret = 0;

	while(1)
	{
		ret = getopt_long(argc, argv, "", long_options, 0);
		if ( ret == -1) //swhile all command line args have not been parsed
			break;

		switch (ret)
		{
			case 'p':
			if ( !isdigit(*optarg))
			{
				fprintf(stderr, "Period should be an integer.\n");
				exit(1);
			}
			period = atoi(optarg);
			//fprintf(stderr, "period is %d\n", period);
			//fprintf(stderr, )
			if ( period < 0 )
			{
				fprintf(stderr, "Period should be greater than zero.\n");
				exit(1);
			}
			break;
			case 's':
			scale = optarg[0]; //assign the scale to scale
			if ( scale != 'F' && scale != 'C')
			{
				fprintf(stderr, "Only Farenheit and Celsius are allowed.\n");
				exit(1);
			}
			break;

			case 'l':
			// logfd = creat(optarg, S_IRUSR | S_IWUSR);

			// if ( logfd < 0)
			// {
			// 	fprintf(stderr, "log file descriptor is not valid.\n");
			// 	exit(1);
			// }
			logFile = fopen(optarg, "w" );

			if ( logFile == NULL)
			{
				fprintf(stderr, "Error occurred during creation of log file.\n");
				exit(1);
			}

			logFlag = 1;
			break;
			default:
			{
				fprintf(stderr, "Not the correct usage for long options.\n");
				exit(1); 
			}
		}
	}




	pthread_t buttonThread; //one thread for button
	if ( pthread_create(&buttonThread, NULL, (void *) checkButton, NULL) < 0 )
	{
		fprintf(stderr, "Error during button thread creation.\n");
		exit(1);
	}

	pthread_t tempThread; //one thread for temp sensor
	if ( pthread_create(&tempThread, NULL, (void *) printTemp, NULL) < 0)
	{
		fprintf(stderr, "Error during temperature thread creation.\n");
		exit(1);
	}

	int rv = 0; //return value is 0

	char buffer[9]; //e.g. 17:25:58\0 (plus the nullbyte)
	time_t curtime; //current time var
	struct tm *loctime; //time structure for local time

	struct pollfd fds[1]; //only one

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	sleep(1);

	char input_buffer[256] = {0};
	char command_buffer[256] = {0}; //constantly reset buffer to 0

	while(1) //following zhaoxing's pseudo code, main thread will process commands from stdin
	{
		
		curtime = time (NULL);
		/* converts the simple time pointed to by time to broken-down time representation */
		loctime = localtime(&curtime);

		strftime(buffer, 9, "%H:%M:%S", loctime); //now buffer has the specified time

		
		ret = poll(fds, 1, 0);

		if ( ret < 0)
		{
			fprintf(stderr, "Error occured during poll.\n");
			exit(1);
		}

			rv = read ( 0, input_buffer, 255);

			int b = 0;
			int c = 0;
			while ( b < rv && rv > 0 )
			{
				if ( input_buffer[b] == '\n')
				{


					if (strcmp(command_buffer, off) == 0) //if they're equal
					{
						if ( logFlag) //if logFlag is on
						{
							fprintf(logFile, "OFF\n");
							fprintf(logFile, "%s SHUTDOWN\n", buffer);
							//fflush(logFile);

						}

						//fprintf(stdout, "%s SHUTDOWN\n", buffer);

						//end_flag = 1;
						exit(0);	
					}
					else if ( strcmp(command_buffer, stop) == 0) //stop
					{
						stopped = 1; //stopped, check
						if ( logFlag) //if logFlag is on
						{
							fprintf(logFile, "STOP\n"); //write to log file
							//fflush(logFile); //flush it to make sure it's written
						}
					}
					else if ( strcmp(command_buffer, start) == 0) //start
					{
						stopped = 0; //started
						if ( logFlag) //if logFlag is on
						{
							fprintf(logFile, "START\n");
							//fflush(logFile);
						}
					}
					else if ( strcmp(command_buffer, scalef) == 0) //scale=f
					{
						scale = 'F';
						if (logFlag)
						{
							fprintf(logFile, "SCALE=F\n");
							//fflush(logFile);
						}
					}
					else if (strcmp(command_buffer, scalec) == 0) //scale=c
					{
						scale = 'C';
						if ( logFlag)
						{
							fprintf(logFile, "SCALE=C\n");
							//fflush(logFile);
						}
					}
					else if ( strlen(command_buffer) > 7)   //check the length
					{
						//fprintf(stdout, "here\n");
						char small_buffer[8];

						int return_value = strlen(command_buffer);
						//fprintf(stdout, "return value is %d.\n", return_value);

						memcpy(small_buffer, &command_buffer[0], 7);

						small_buffer[7] = '\0';

						if ( strcmp(small_buffer, "PERIOD=") == 0)
						{
							//fprintf(stdout, "here1\n");
							//need to somehow extract period #
							int interval = atoi(&command_buffer[7]); 
							if ( interval < 0 )
							{
								if (logFlag)
								{
									fprintf(logFile, "Period should be a number greater than 0\n");
									//fflush(logFile);
								}
								exit(1);
							}

							period = interval; //after I checked interval, assign it to period

							if (logFlag)
							{
								fprintf(logFile, "PERIOD=%d\n", period);
								//fflush(logFile);
							}

						} 
					}
					else //all other options has been parsed, should be error msg now
					{
						//any bad arguments
						if ( logFlag)
						{
							fprintf(logFile, "Error: bad options/arguments given while accepting commands from STDIN.\n");
							
							//fflush(logFile);
						}
						exit(1);
					}
				

					memset(command_buffer, 0, 256);
					c = 0;

				}
				else
				{
					command_buffer[c] = input_buffer[b];
					c++;
				}

				b++;

			}
			
			if ( rv == 0)
			{
				if ( logFlag)
				{
					fprintf(logFile, "EOF\n");
					fprintf(logFile, "%s SHUTDOWN\n", buffer);
					//fflush(logFile);
				}
				//fprintf(stdout, "%s SHUTDOWN\n", buffer);

				exit(0);
			}
		

	}

	mraa_aio_close(pinTempSensor);

}

























