#include <stdio.h>
#include <stdlib.h>
#include <xil_io.h>
#include "xiicps.h"
#include <xparameters.h>
#include "xuartps.h"
#include "audio.h"
#include <sds_lib.h>

unsigned char IicConfig(unsigned int DeviceIdPS);
void AudioWriteToReg(unsigned char u8RegAddr, short u16Data);
void LineinLineoutConfig(void);
void audio_sample_write(int DataL, int DataR);
void audio_sample_read(int * DataL, int * DataR);
void audio_sample_wait(void);

XIicPs Iic;

#define N	58
#define M   59				// defining variable M for 1st while loop //			
typedef short	coef_t;
typedef short	data_t;
typedef long	acc_t;

void fir (
	  data_t *y,
	  data_t x
	  ) {
	  const data_t c[N+1]={
	 #include "fir_coef.dat"
		};

	  int i, total;			// defining int i for 2nd while loop and total to store answer of convolution each time //
	  total = 0;			// make total zero to erase the starting garbage value //

	  float j[M];			// defining float j with array of 59 to store the value of x in it //

	  i = 0;				// to start loop from 1 make i zero //
	  while (i < M)			// start of while loop //			
	  {
		  j[i] = 0.0;		// make j array zero to erase the starting garbage value //
		  i++;				// increament i by 1 //
	  }

	  i = 0;				// again make i zero for next loop operation //
	  while (i <= N)		// start of while loop for convolution or FIR operation //
	  {
		j[0] = x;			// transfering value of x input to j[0] //
		total += j[i] * c[i]; // equation for convolution operation and saving result to total for 58 times //

		if(i > 0)			// loop of if to save the value of j after each convolution operation //
		{
		 	j[i] = j[i - 1]; // equation to save the previouse value //
		}

		i++;				// increament i by 1 //
	  }

	 *y = total >> 16;		// shifting the final convolution answer by 16 digit to store it in short *y ouput //
	 // *y = x;
	  //Implement Filter here
	}


#define SAMPLES N+5
void filter(void)
{
	int i=0;
	data_t signal, output;

	int DataL, DataR;
	int FilOutDataL, FilOutDataR;

	Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR, 0b1);
	Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + 0x4, 0b1);
    for (i=0;i<SAMPLES;i++) {
	  if(i==0)
		  signal = 0x8000;
	  else
		  signal = 0;
	  fir(&output,signal);
	  printf("%i %d %d\n",i,(int)signal,(int)output);
    }

	while (1)
	{
		audio_sample_wait();
		audio_sample_read(&DataL, &DataR);
		fir(&FilOutDataL, (DataL>>8));
		fir(&FilOutDataR, (DataR>>8));
		FilOutDataL = (FilOutDataL & 0x0000ffff) << 8;
		FilOutDataR = (FilOutDataR & 0x0000ffff) << 8;
		audio_sample_write(FilOutDataL, FilOutDataR);
	}
}

int main(void)
{

	//Configure the IIC data structure
	IicConfig(XPAR_XIICPS_0_DEVICE_ID);

	//Configure the Line in and Line out ports.
	LineinLineoutConfig();

	Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR, 0b1);
	Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + 0x4, 0b1);

	filter();

}

