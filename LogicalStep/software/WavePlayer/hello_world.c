/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */
/*=========================================================================*/
/*  Includes                                                               */
/*=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>
#include "altera_avalon_timer.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>

/*=========================================================================*/
/*  DEFINE: All Structures and Common Constants                            */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Macros                                                         */
/*=========================================================================*/

#define PSTR(_a)  _a

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/
static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer;   /* 1000Hz increment timer */

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/

/***************************************************************************/
/*  TimerFunction                                                          */
/*                                                                         */
/*  This timer function will provide a 10ms timer and                      */
/*  call ffs_DiskIOTimerproc.                                              */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static alt_u32 TimerFunction (void *context)
{
   static unsigned short wTimer10ms = 0;

   (void)context;

   Systick++;
   wTimer10ms++;
   Timer++; /* Performance counter for this module */

   if (wTimer10ms == 10)
   {
      wTimer10ms = 0;
      ffs_DiskIOTimerproc();  /* Drive timer procedure of low level disk I/O module */
   }

   return(1);
} /* TimerFunction */

/***************************************************************************/
/*  IoInit                                                                 */
/*                                                                         */
/*  Init the hardware like GPIO, UART, and more...                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void IoInit(void)
{
   uart0_init(115200);

   /* Init diskio interface */
   ffs_DiskIOInit();

   //SetHighSpeed();

   /* Init timer system */
   alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

/*=========================================================================*/
/*  DEFINE: All code exported                                              */
/*=========================================================================*/

uint32_t acc_size;                 /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif

char Line[256];                 /* Console input buffer */

FATFS Fatfs[_VOLUMES];          /* File system object for each logical drive */
FIL File1, File2;               /* File objects */
DIR Dir;                        /* Directory object */
uint8_t Buff[1024] __attribute__ ((aligned(4)));  /* Working buffer */

//push button ISR
int button_read;
static void button_ISR(void* context, alt_u32 id);
static void timer_ISR(void* context, alt_u32 id);
//useful variable
int state = 0; // 0:stop 1:play 2:pause
FILE* lcd_display; 				//LCD display
char wav_name[30][50]; //store wav file name
char play_mode_name[20];
unsigned long wav_size[30]; //store wav file size
int index1;
int current_index = 0;
int current_play = 0;

int is_wav(char* file){
	if(strstr(file, ".wav")!= NULL || strstr(file, ".WAV")!= NULL){
		return 0;
	}else{
		return 1;
	}
}

void lcd_display_song(int idx, char* name,char* mode, int stat){

	 if (lcd_display != NULL )
	 {
	   //command to clear lcd
	   fprintf(lcd_display, "%c%s", 27, "[2J");
	   //display
	   fprintf(lcd_display, "%d: %s\n", idx, name);
	   if(stat==0){
		   fprintf(lcd_display, "%s\n", "STOPPED");
	   }else if(stat==2){
		   fprintf(lcd_display, "%s\n", "PAUSED");
	   }else{
		   fprintf(lcd_display, "%s\n", mode);
	   }
	 }
}

int main()
{
	alt_up_audio_dev * audio_dev;
	uint32_t s1, s2, cnt, blen = sizeof(Buff);
	uint8_t res = 0;
	long p1;
	uint32_t ofs = 0;

    //open Audio port
    audio_dev = alt_up_audio_open_dev ("/dev/Audio");
    //if ( audio_dev == NULL)
    	//alt_printf ("Error: could not open audio device \n");
    //else
    	//alt_printf ("Opened audio device \n");

    //open LCD
    lcd_display = fopen("/dev/lcd_display", "w");

    (uint16_t) disk_initialize((uint8_t) 0); //di 0
    f_mount((uint8_t) 0, &Fatfs[0]); //fi 0
    //fl, also generate song index
	char* ptr = "";
	res = f_opendir(&Dir, ptr);
	p1 = s1 = s2 = 0; // initialize
	index1 = 0;
	while (1)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++;
			p1 += Finfo.fsize;
		}

		if(is_wav(&(Finfo.fname[0])) == 0){

			strcpy(wav_name[index1], &(Finfo.fname[0]));
			wav_size[index1] = Finfo.fsize;
			index1++;
		}
	}
	//end of generate song index

	//display the first song name
	strcpy(play_mode_name, "Normal Speed");
	lcd_display_song(current_play+1, wav_name[current_play],play_mode_name,state);
	//REGISTE IRQ
	alt_irq_register( BUTTON_PIO_IRQ, 0, button_ISR);
	alt_irq_register( TIMER_0_IRQ, 0, timer_ISR);
	// clear timer_0
	IOWR(TIMER_0_BASE, 1, 0);
	// enable button_IRQ /mask
	IOWR(BUTTON_PIO_BASE, 2, 0x0F);
    while(1){// play song in the while loop
        ofs = File1.fptr;
        int i = 0;
        s2 = 0;
        int speed =4; //normal speed
        int mono=1;
        int half = 0;
    	unsigned int l_buf;
    	unsigned int r_buf;
    	//decide the play mode by switches
    	unsigned int play_mode = IORD(SWITCH_PIO_BASE, 0) & 0x03;
    	        if(play_mode==0x00){
    	        	strcpy(play_mode_name, "PBACK-NORM SPD");
    	        }else if(play_mode==0x01){
    	        	strcpy(play_mode_name, "PBACK–HALF SPD");
    	                speed = 2;
    	        		//half=1;
    	        }else if(play_mode==0x02){
    	        	strcpy(play_mode_name, " PBACK–DBL SPD");
    	                speed = 8;
    	        }else if(play_mode==0x03){
    	        	strcpy(play_mode_name, " PBACK–MONO");
    	            	mono = 0;
    	        }
    	 current_play = current_index;
    	 f_open(&File1, wav_name[current_play], (uint8_t) 1);
    	 p1 = wav_size[current_play];
     while (p1>0 && state != 0)
		{
      	 	 if(current_play != current_index){
      	 		break;
      	 	 }
              	s2=0; //reset s2
              	//decide how many byte left to read
              	if(p1>blen){
              		cnt = blen;
              		p1 = p1-blen;
              	}else{
              		cnt = p1;
              		p1 = 0;
              	}
                  /* read and echo audio data */
              	res = f_read(&File1,Buff,cnt,&s2);
              	i=0;
              	for(i = 0; i < s2; i += speed){
              		l_buf = 0;
              		r_buf = 0;
                  	l_buf = (Buff[i+1] << 8) | Buff[i];
                  	r_buf = (Buff[i+3] << 8) | Buff[i+2];

              		if(mono==1){ //stereo
            			while( alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_LEFT) == 0);
            		   	alt_up_audio_write_fifo(audio_dev, &(r_buf), 1, ALT_UP_AUDIO_LEFT);
            			while( alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT) == 0);
            			alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_RIGHT);
              		}
              		else{ //mono
          				while( alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_RIGHT) == 0);
          				alt_up_audio_write_fifo(audio_dev,&(l_buf), 1, ALT_UP_AUDIO_RIGHT);
          				while( alt_up_audio_write_fifo_space(audio_dev, ALT_UP_AUDIO_LEFT) == 0);
          				alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
              		}
              		while(state==2); //pause when necessary
              	}
              	continue;


		}
     	if(current_play == current_index){
     		if(state != 0){
         		state = 0;
         		lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
     		}
     	}

    };

  return 0;
}

// ISRs
static void button_ISR(void* context, alt_u32 id){
	// Disable PB Interrupts
	IOWR(BUTTON_PIO_BASE, 2, 0x0);
	 //delay 30 ms, we need 1500000 clock cycles 0x0016E360
	// lower bits
	IOWR(TIMER_0_BASE, 0x02, 0xE360);
   // higher bits
   IOWR(TIMER_0_BASE, 0x03, 0x0016);
	// Enable Timer
   IOWR(TIMER_0_BASE, 1, 0x05);
}

static void timer_ISR(void* context, alt_u32 id){
	// read all 4 pb values. PBs are active low. For PB = 0, LED should be 1.
	button_read = IORD(BUTTON_PIO_BASE, 0x0);
	if(button_read==0x0e){
		//printf("next\n");
		current_index = (current_index+1)%index1;
		//state = 0;
		if(state != 1){
			state=0;
		}
		lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
	}else if(button_read==0x07){
		//printf("previous\n");
		if(current_index == 0){
			current_index = index1-1;
		}else{
			current_index = (current_index-1);
		}
		if(state != 1){
			state=0;
		}
		//state = 0;
		lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
	}else if(button_read==0x0b){
		//printf("stop\n");
		state = 0;
		lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
	}else if(button_read==0x0d){
		//printf("pause\n");
		if(state == 1){
			state = 2;
			lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
		}else if(state == 2 || state == 0){
			state = 1;
			lcd_display_song(current_index+1, wav_name[current_index],play_mode_name,state);
		}
	}else{
		//return to initial earlier
		// Clear Status for Timer IRQ (set status to 0- both RUN and TO)
		IOWR(TIMER_0_BASE, 0, 0);
		// Clear timer control
		IOWR(TIMER_0_BASE, 1, 0);
		// Clear the button_ISR
		IOWR(BUTTON_PIO_BASE, 0x03, 0);
		// Enable button interrupt
		IOWR(BUTTON_PIO_BASE, 0x02, 0x0F);
	}
	// Clear Status for Timer IRQ (set status to 0- both RUN and TO)
	IOWR(TIMER_0_BASE, 0, 0);
	// Clear timer control
	IOWR(TIMER_0_BASE, 1, 0);
	// Clear the button_ISR
	IOWR(BUTTON_PIO_BASE, 0x03, 0);
	// Enable button interrupt
	IOWR(BUTTON_PIO_BASE, 0x02, 0x0F);
}
