#include <stdio.h>
#include <stdlib.h>
#include <xil_io.h>
#include <xparameters.h>
#include "xiicps.h"
#include "audio.h"

XIicPs Iic;

void audio_sample_wait(void)
{
	u32 u32Temp;

	do //wait for RX data to become available
	{
		u32Temp = Xil_In32(I2S_STATUS_REG);
	} while ( u32Temp == 0);

	Xil_Out32(I2S_STATUS_REG, 0x00000001); //Clear data rdy bit
}

void audio_sample_read(unsigned int * DataL, unsigned int * DataR)
{
	*DataL = Xil_In32(I2S_DATA_RX_L_REG);
	*DataR = Xil_In32(I2S_DATA_RX_R_REG);
}

void audio_sample_write(unsigned int u32DataL, unsigned int u32DataR)
{
	Xil_Out32(I2S_DATA_TX_L_REG, u32DataL);
	Xil_Out32(I2S_DATA_TX_R_REG, u32DataR);
}

void play(void)
{
	unsigned int u32DataL, u32DataR;
	while (1)
	{
		audio_sample_wait();
		audio_sample_read(&u32DataL, &u32DataR);
		audio_sample_write(u32DataL, u32DataR);
	}
}

unsigned char IicConfig(unsigned int DeviceIdPS)
{

	XIicPs_Config *Config;
	int Status;

	//Initialize the IIC driver so that it's ready to use
	//Look up the configuration in the config table, then initialize it.
	Config = XIicPs_LookupConfig(DeviceIdPS);
	if(NULL == Config) {
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//Set the IIC serial clock rate.
	XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);

	return XST_SUCCESS;
}


/******************************************************************************
 * Function to write 9-bits to one of the registers from the audio
 * controller.
 * @param	u8RegAddr is the register address.
 * @param	u16Data is the data word to write ( only least significant 9 bits).
  * @return	none.
 *****************************************************************************/
void AudioWriteToReg(unsigned char u8RegAddr, short u16Data) {

	unsigned char u8TxData[2];

	u8TxData[0] = (u8RegAddr << 1 ) | ((u16Data >> 8) & 0x01); // append msb of 9-bit data to the reg addr after shifting left
	u8TxData[1] = u16Data & 0xFF;

	XIicPs_MasterSendPolled(&Iic, u8TxData, 2, IIC_SLAVE_ADDR);
	while(XIicPs_BusIsBusy(&Iic));
}

/******************************************************************************
 * Configures Line-In input, ADC's, DAC's, Line-Out and HP-Out.
 * @param   none.
 * @return	none.
 *****************************************************************************/
void LineinLineoutConfig() {

	// software reset
	AudioWriteToReg(R15_SOFTWARE_RESET, 0x000);
	// power mgmt: 0_00110010=>0,Power up, power up, OSC dn, out off, DAC up, ADC up, MIC off, LineIn up
	AudioWriteToReg(R6_POWER_MANAGEMENT, 0x030);
	// left ADC Input: 0_01010111=>0,mute disable, Line volume 0 dB
	AudioWriteToReg(R0_LEFT_ADC_INPUT,0x017);
	// right ADC Input: 0_00010111=>0,mute disable, Line volume 0 dB
	AudioWriteToReg(R1_RIGHT_ADC_INPUT,0x017);
	AudioWriteToReg(R2_LEFT_DAC_VOLUME,0x079);
	AudioWriteToReg(R3_RIGHT_DAC_VOLUME,0x079);
	// analog audio path: 0_00010010=>0,-6 dB side attenuation, sidetone off, DAC selected, bypass disabled, line input, mic mute disabled, 0 dB mic
	AudioWriteToReg(R4_ANALOG_AUDIO_PATH, 0x012);
	// digital audio path: 0_00000000=>0_000, clear offset, no mute, no de-emphasize, adc high-pass filter enabled
	AudioWriteToReg(R5_DIGITAL_AUDIO_PATH, 0x000);
	// digital audio interface: 0_00001110=>0, BCLK not inverted, slave mode, no l-r swap, normal LRC and PBRC, 24-bit, I2S mode
	AudioWriteToReg(R7_DIGITAL_AUDIO_INTERFACE, 0x00A);
	// Digital core:0_00000001=>0_0000000, activate core
	AudioWriteToReg(R9_ACTIVE, 0x001);
	// power mgmt: 0_00100010 0_Power up, power up, OSC dn, out ON, DAC up, ADC up, MIC off, LineIn up
	AudioWriteToReg(R6_POWER_MANAGEMENT, 0x022); // power mgmt: 001100010 turn on OUT

}
