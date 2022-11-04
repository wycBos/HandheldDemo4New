#include <stdio.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdbool.h>
#include "UART_test.h"

bool GoGo = TRUE;

void serialread(int fd, int numbytes)
{
    unsigned char data[11]={0};
    for(int i=0;i<numbytes;i++)
            {
                data[i]=serialGetchar(fd);
                printf("%x ",data[i]);
            }
            printf("\n");
            unsigned char Check=0;
            for(int i=0;i<numbytes-1;i++)
            {
                Check=Check+data[i];
            }
            Check=~Check+1;
            printf("%x \n" ,Check);
            if(data[numbytes-1]==Check)
            {
                printf("CheckSum ok!\n");
            }
            else
            {
                printf("Invalid Data!\n");
            }
}

//int main()
float UART_main()
{
    int fd;
    int c;
    int numbytes;
    char contimeas[4]   ={0x80,0x06,0x03,0x77};
    char singlemeas[4]  ={0x80,0x06,0x02,0x78};
    char shutdown[4]    ={0x80,0x04,0x02,0x7A};
    char laseron[5]     ={0x80,0x06,0x05,0x01,0x74};
    char laseroff[5]    ={0x80,0x06,0x05,0x00,0x75};
    char setrange5[5]  ={0xFA,0x04,0x09,0x05,0xF4};
    unsigned char data[11]={0};
    float distance=0;

    if(wiringPiSetup() < 0)return 1;
    if((fd = serialOpen("/dev/serial0",9600)) < 0)return 1;
//    if((fd = serialOpen("/dev/ttyAMA0",115200)) < 0)return 1;
//    serialFlush(fd);
    //printf("serial test start ...\n");

    serialPrintf(fd,contimeas);
    
  
    int counter = 0, counterErr = 0; GoGo = TRUE;
    while(GoGo)
    {  
        
        if (counter > 2 || counterErr > 50) GoGo = FALSE;
        if (serialDataAvail(fd) > 0)
        {
            delay(50);
            counter++;
            for(int i=0;i<11;i++)
            {
                data[i]=serialGetchar(fd);
                //printf("%x ",data[i]);
            }
            //printf("\n");
            unsigned char Check=0;
            for(int i=0;i<10;i++)
            {
                Check=Check+data[i];
            }
            Check=~Check+1;
            //printf("%x \n" ,Check);
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
                //printf("Distance = ");
                //printf("%5.1f",distance);
                //printf(" m\n");
                }
            }
            else
            {
                printf("Invalid Data!\n");
            }
        }else{
            counterErr++;
        }
        delay(20);
    }
    //serialPrintf(fd,laseroff);
    //delay(500);
    //serialPrintf(fd,shutdown);     
    //delay(500);
    serialClose(fd);
    //printf("Received q for Quit \n"); 
    return distance; //0;
}

