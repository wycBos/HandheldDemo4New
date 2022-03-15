#include <stdio.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdbool.h>
#include "UART_test.h"

bool GoGo = TRUE;

/* new code */
#define LASER_ADDRESS         0x80

#define LASER_REDPAR_CMD      1
#define LASER_REDNUM_CMD      2
#define LASER_SETADD_CMD      3
#define LASER_SETDSR_CMD      4
#define LASER_SETINL_CMD      5
#define LASER_SETSTA_CMD      6
#define LASER_SETRNG_CMD      7
#define LASER_SETFRQ_CMD      8
#define LASER_SETRES_CMD      9
#define LASER_ENAPOW_CMD      10
#define LASER_MESIGB_CMD      11
#define LASER_REDCAH_CMD      12
#define LASER_MEASIG_CMD      13
#define LASER_MEACON_CMD      14
#define LASER_CTLOFF_CMD      15
#define LASER_CTRLON_CMD      16
#define LASER_CTLSHT_CMD      17

/* Laser Commands */
/* global commands */
char read_laserParameter[4]    =   {0xFA,0x06,0x01,0xFF};      //Read Parameter following 0xFF. 
char read_laserNumber[4]       =   {0xFA,0x06,0x04,0xFC};      //Read machine number following 0xFC.
char set_laserAddr[5]          =   {0xFA,0x04,0x01,0x80,0xFF};         //Set address following a address & CS. default address 0x80
char set_laserDistRevise[6]    =   {0xFA,0x04,0x06,0x2B,0x01,0xFF};    //Revise distance, following symbol (0x2D or 0x2B), revised value, CS.
char set_laserInterval[5]      =   {0xFA,0x04,0x05,0x01,0xFF};         //Set datacontinuous interver, following intervaer value & CS.
char set_laserStart[5]         =   {0xFA,0x04,0x08,0x00,0xFF};         //Set distance starting and end point, following position value & CS
char set_laserRange[5]         =   {0xFA,0x04,0x09,0x50,0xFF};         //Set measuring range, following range value (0x05, 0x0A, 0x1E, 0x32, 0x50) & CS.
char set_laserFreq[5]          =   {0xFA,0x04,0x0A,0x0A,0xFF};         //Set frequency, following frequency value (0x05, 0x0A, 0x14)) & CS.
char set_laserResol[5]         =   {0xFA,0x04,0x0C,0x01,0xFF};         //Set resolution, following resolution value (1, 2) & CS.
char enable_laserPowerOn[5]    =   {0xFA,0x04,0x0D,0x00,0xFF};         //Set measurement starts when powered on (0, 1), following start flag & CS.
char measure_laserSingleB[4]   =   {0xFA,0x06,0x06,0xFA};      //Single measurement broadcast following 0xFA.
    /* module commands */
char read_laserCache[4]        =   {0x80,0x06,0x07,0xFF};      //Read cache following a CS. the first byte is laser's address. defaule address 0x80.
char measure_laserSingle[4]    =   {0x80,0x06,0x02,0xFF};      //Single measurement following a CS. the first byte is laser's address. defaule address 0x80.
char measure_laserContinu[4]   =   {0x80,0x06,0x03,0xFF};      //Continuous measurement following a CS. the first byte is laser's address. defaule address 0x80.
char control_laserOff[5]       =   {0x80,0x06,0x05,00,0xFF};   //Control laser off llowing a CS. the first byte is laser's address. defaule address 0x80.
char control_laserOn[5]        =   {0x80,0x06,0x05,01,0xFF};   //Control laser on llowing a CS. the first byte is laser's address. defaule address 0x80.
char control_laserShutdown[4]  =   {0x80,0x04,0x02,0x7A};      //Shut down llowing a CS. the first byte is laser's address. defaule address 0x80.

struct laserCmdRes
{
    int   cmdID;
    int   cmdLen;
    char  laserAddr;
    char  light;
    char  returned;
    char  temperature;
    float distance;
};

struct laserCmdRes cmdResult;

#if 1 //new code
/*****************//**
*  \brief Check Sum of the data.
*  @param Buffer the data buffer pointer
*  @param bytes the number of the data
*/
unsigned char checkSum(char *Buffer, int bytes)
{
	unsigned char Check = 0;
	for(int i = 0; i < bytes - 1; i++)
	{
		Check = Check + Buffer[i];
	}
	Check = ~Check + 1;
    return Check;
}

/*****************//**
*  \brief read Laser data.
*  @param fd the serial port handler
*  @param RxBuffer the data buffer pointer
*  @param bytes the number of the data
*/
bool readLaserData(int fd, char *RxBuffer, int bytes)
{
	bool okflag = false;
	unsigned char sumChk;
	
	for(int i = 0; i < bytes; i++)
	{
		RxBuffer[i] = serialGetchar(fd);
        printf("%x ", RxBuffer[i]);
	}
    printf(" Rx\n");
	
	/* check sum */
    sumChk = checkSum(RxBuffer, bytes);
    if(sumChk == RxBuffer[bytes - 1])
	{
		okflag = true;
	}
	else
	{
		printf("Invalid Data(0)!\n");
	}
	return okflag;
}

/*****************//**
*  \brief prepare Laser command.
*  @param cmdBuffer the command buffer
*  @param addr laser module address
*  @param bytes the number of the command
*/
void prepLaserCmd(char *cmdBuffer, char addr, int bytes)
{
	unsigned char sumChk;
		
	/* set module address */
    if(addr) //if addr is 0, no addr change is needed.
    {
        cmdBuffer[0] = addr;
    }
    /* set check sum */
    sumChk = checkSum(cmdBuffer, bytes);
    cmdBuffer[bytes - 1] = sumChk;
}

bool LaserCmdProc(char *RxBuffer, struct laserCmdRes* cmdResult)
{
    bool okFlag = true;
    float dist = 0;
    char result = 0;
    int cmdId = cmdResult->cmdID;

    switch(cmdId)
    {
        case LASER_REDPAR_CMD:
            cmdResult->laserAddr = RxBuffer[3];
            cmdResult->light = RxBuffer[4];
            cmdResult->returned = RxBuffer[5];
            cmdResult->temperature = RxBuffer[6];
            break;
        case LASER_REDNUM_CMD:
            //cmdResult->laserAddr = RxBuffer[3];
            //cmdResult->light = RxBuffer[4];
            //cmdResult->returned = RxBuffer[5];
            //cmdResult->temperature = RxBuffer[6];
            break;
        case LASER_SETADD_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETDSR_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETINL_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETSTA_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETRNG_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETFRQ_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_SETRES_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_ENAPOW_CMD:
            result = RxBuffer[1];
            if(result == 0x04)
                okFlag = false;
            break;
        case LASER_MESIGB_CMD:
           break;
        case LASER_REDCAH_CMD:
        case LASER_MEASIG_CMD:
        case LASER_MEACON_CMD:
            result = RxBuffer[3];
            if(result == 0x45)
            {
                result = RxBuffer[4];
                if(result == 0x52)
                    okFlag = false;
                dist = 0;
            }
            else{
                dist = (RxBuffer[3] - 0x30)*100 + (RxBuffer[4] - 0x30) * 10 +
                    (RxBuffer[5] - 0x30) * 1 + (RxBuffer[7] - 0x30) * 0.1 + 
                    (RxBuffer[8] - 0x30) * 0.01 + (RxBuffer[9] - 0x30) * 0.001;
            }
            cmdResult->distance = dist;
            break;
        case LASER_CTLOFF_CMD:
        case LASER_CTRLON_CMD:
            result = RxBuffer[3];
            if(result == 0x0)
                okFlag = false;
            break;
        case LASER_CTLSHT_CMD:
            break;
    }
    return okFlag;
}
/*****************//**
*  \brief Laser command function.
*  @param fd the serial port handler
*  @param cmdBuffer the command buffer
*  @param RevNumber the number of the data received
*/
bool LaserCmdFunction(int fd, char *cmdBuffer, int RevNumber, struct laserCmdRes *cmdResult)
{
	const int maxTry = 20;
	bool  okFlag = false;
	int counter = 0;
	int numByt = 0;
	int bytes;
    char RxBuffer[10];

    bytes = cmdResult->cmdLen;
    prepLaserCmd(cmdBuffer, 0x80, bytes);
    
    /* check the command */
    for(int i = 0; i < bytes; i++)
        printf("%x ", cmdBuffer[i]);
    printf(" Tx\n");

    /* flush the receiver buffer */
	numByt = serialDataAvail(fd);
	while(numByt != 0 && counter < 5)
	{
		serialFlush(fd);
		delay(50);
		numByt = serialDataAvail(fd); // flush the buffer, then numByt should be zero.
		delay(50);
		counter ++;
	}
	if(numByt != 0)
	{
		printf("flush failure!\n");
        return false; //flush receiver buffer failure
	}
	
	/* send Laser command */
	serialPrintf(fd, cmdBuffer);
	delay(100);
	
	/* check command result */
    //okFlag = LaserReCode(fd, RxBuffer, RevNumber); //get the command result from laser module

	counter = 0;
	numByt = serialDataAvail(fd);
	do{
		if(numByt >= RevNumber) //the return message may be received
		{
            okFlag = readLaserData(fd, RxBuffer, RevNumber);
            if(!okFlag)
            {
               serialFlush(fd);
               numByt = serialDataAvail(fd);
            }           
		}
		else{ // the number of received is not enough, waiting for a while. 
			counter++;
			delay(50);
            numByt = serialDataAvail(fd);
		}
	}while(!okFlag && (counter < maxTry));

    /* process receive data */
    if(okFlag)
    {
        okFlag = LaserCmdProc(RxBuffer, cmdResult);
    }
	return okFlag; //out of the trying
}


int main()
{
    int fd;
    int c;
    int numbytes;
    bool optFlag;
    unsigned char data[11] = {0};
    float distance = 0;
    char addr = 0x80; //default 0x80

    if(wiringPiSetup() < 0)return 1;
    if((fd = serialOpen("/dev/serial0",9600)) < 0)return 1;
    printf("serial test start ...\n");

    serialPrintf(fd,control_laserShutdown);     
    delay(100);

    /* start laser */
    cmdResult.cmdID = LASER_CTRLON_CMD;
    cmdResult.cmdLen = (int)sizeof(control_laserOn);
    optFlag = LaserCmdFunction(fd, control_laserOn, 5, &cmdResult);

    if(!optFlag)
    {
        printf("turn laser on is failure!\n");
    }
    delay(100);

    /* set continuous measurement */
    cmdResult.cmdID = LASER_MEACON_CMD;
    cmdResult.cmdLen = (int)sizeof(measure_laserContinu);
 //   optFlag = LaserCmdFunction(fd, measure_laserContinu, 11, &cmdResult);

    if(!optFlag)
    {
        printf("set continue measuring is failure!\n");
    }
    else{
        distance = cmdResult.distance;
    }

    int counter = 0;
    while(!GoGo)
    {  
        
        if (counter > 50) GoGo = FALSE;
        if (serialDataAvail(fd) > 0)
        {
            delay(50);
            counter++;
            for(int i=0;i<11;i++)
            {
                data[i]=serialGetchar(fd);
                printf("%x ",data[i]);
            }
            printf("\n");
            unsigned char Check=0;
            for(int i=0;i<10;i++)
            {
                Check=Check+data[i]; //check sum calculate
            }
            Check=~Check+1;
            printf("%x \n" ,Check);
            if(data[10]==Check)
            {
                if(data[3]=='E'&&data[4]=='R'&&data[5]=='R')
                {
                    printf("Out of range");
                }
                else
                {
                distance=0;
                distance=(data[3]-0x30)*100+(data[4]-0x30)*10+(data[5]-0x30)*1+(data[7]-0x30)*0.1+(data[8]-0x30)*0.01+(data[9]-0x30)*0.001;
                printf("Distance = ");
                printf("%5.1f",distance);
                printf(" m\n");
                }
            }
            else
            {
                optFlag = false;//abotionLaser(fd, 10, 1); //printf("Invalid Data!\n");
                if(optFlag)
                {
                    ;//serialPrintf(fd, contimeas);
                }
            }
        }
        else{
            //serialPrintf(fd,contimeas);
            counter++;
            delay(200);
        }
        delay(100);
    }

    for(int i = 0; i < 10; i++)
    {
        /* switch to single measurement */
        cmdResult.cmdID = LASER_MEASIG_CMD;
        optFlag = LaserCmdFunction(fd, measure_laserSingle, 11, &cmdResult);

        if(!optFlag)
        {
            printf("set to single measurement is failure!\n");
        }else{
            printf("Distance = %5.1f\n", cmdResult.distance);
            delay(500);
        }

    }

    /* turn laser off */
    cmdResult.cmdID = LASER_CTLOFF_CMD;
    optFlag = LaserCmdFunction(fd, control_laserOff, 5, &cmdResult);

    if(!optFlag)
    {
        printf("turn laser off is failure!\n");
    }

    //serialPrintf(fd,laseroff);
    //delay(100);
   
    /* shut down laser */
    //cmdResult.cmdID = LASER_CTLSHT_CMD;
    //optFlag = LaserCmdFunction(fd, control_laserShutdown, 0, &cmdResult);

    serialPrintf(fd,control_laserShutdown);     
    delay(100);
    /* check laser buffer */
    numbytes = serialDataAvail(fd);
    if(numbytes)
    {
        serialFlush(fd);
    }
    serialClose(fd);
    printf("Received q for Quit \n"); 
    //return distance; 
    return 0;
}
#endif

