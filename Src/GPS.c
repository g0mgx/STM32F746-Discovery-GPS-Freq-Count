#include "GPSConfig.h"
#include "GPS.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

extern int GPSData;

GPS_t GPS;

//##################################################################################################################
double convertDegMinToDecDeg (float degMin)
{
  double min = 0.0;
  double decDeg = 0.0;
 
  //get the minutes, fmod() requires double
  min = fmod((double)degMin, 100.0);
 
  //rebuild coordinates in decimal degrees
  degMin = (int) ( degMin / 100 );
  decDeg = degMin + ( min / 60 );
 
  return decDeg;
}
//##################################################################################################################
void	GPS_Init(void)
{
	GPS.rxIndex=0;
	HAL_UART_Receive_IT(&_GPS_USART,&GPS.rxTmp,1);	
}
//##################################################################################################################
void	GPS_CallBack(void)
{
	GPS.LastTime=HAL_GetTick();
	if(GPS.rxIndex < sizeof(GPS.rxBuffer)-2)
	{
		GPS.rxBuffer[GPS.rxIndex] = GPS.rxTmp;
		GPS.rxIndex++;
	}	
	HAL_UART_Receive_IT(&_GPS_USART,&GPS.rxTmp,1);
}
//##################################################################################################################
void	GPS_Process(void)
{
	uint32_t CurrentTime = HAL_GetTick();
	if( (CurrentTime -GPS.LastTime>50) && (GPS.rxIndex>0))
	{
		char	*str;
		#if (_GPS_DEBUG==1)
		printf("%s",GPS.rxBuffer);
		#endif
		str=strstr((char*)GPS.rxBuffer,"GPRMC,");
		GPSData = FALSE;
		if(str!=NULL)
		{

			memset(&GPS.GPRMC,0,sizeof(GPS.GPRMC));
			str = strtok_f(str,'$');
			char* Message_Header = strtok_f(str,',');
			char* Time = strtok_f(NULL,',');
			char* Data_Valid = strtok_f(NULL,',');
			char* Raw_Latitude = strtok_f(NULL,',');
			char* N_S = strtok_f(NULL,',');
			char* Raw_Longitude = strtok_f(NULL,',');
			char* E_W = strtok_f(NULL,',');
			char* Speed = strtok_f(NULL,',');
			char* COG = strtok_f(NULL,',');
			char* Date = strtok_f(NULL,',');
			char* Magnetic_Variation = strtok_f(NULL,',');
			char* M_E_W = strtok_f(NULL,',');
			char* Positioning_Mode = strtok_f(NULL,',');

			double Latitude = atof(Raw_Latitude);
			double Longitude = atof(Raw_Longitude);


			sscanf(Time,"%2d%2d%2d",&GPS.GPRMC.UTC_Hour,&GPS.GPRMC.UTC_Min,&GPS.GPRMC.UTC_Sec);

			sscanf(Data_Valid,"%c",&GPS.GPRMC.Validity);

			GPS.GPRMC.Latitude = Latitude;

			sscanf(N_S,"%c",&GPS.GPRMC.NS_Indicator);

			GPS.GPRMC.Longitude = Longitude;

			sscanf(E_W,"%c",&GPS.GPRMC.EW_Indicator);

			sscanf(Date,"%2d%2d%2d",&GPS.GPRMC.Date_Day,&GPS.GPRMC.Date_Month,&GPS.GPRMC.Date_Year);

			if(GPS.GPRMC.NS_Indicator==0)
				GPS.GPRMC.NS_Indicator='-';
			if(GPS.GPRMC.EW_Indicator==0)
				GPS.GPRMC.EW_Indicator='-';
			if(GPS.GPRMC.Variation_EW_Indicator==0)
				GPS.GPRMC.Variation_EW_Indicator='-';
			GPS.GPRMC.LatitudeDecimal=convertDegMinToDecDeg(GPS.GPRMC.Latitude);
			GPS.GPRMC.LongitudeDecimal=convertDegMinToDecDeg(GPS.GPRMC.Longitude);
			if ((GPS.GPRMC.Validity == 'A') && (&GPS.GPRMC.Date_Day != 0))
			{
				GPSData = TRUE;
			}

			// get rid of unused warnings
			if (Positioning_Mode)  Positioning_Mode = 0;
			if (M_E_W) M_E_W = 0;
			if (Magnetic_Variation) Magnetic_Variation = 0;
			if (COG) COG = 0;
			if (Speed) Speed = 0;
			if (Message_Header) Message_Header = 0;
		}

		memset(GPS.rxBuffer,0,sizeof(GPS.rxBuffer));
		GPS.rxIndex=0;
	}
	HAL_UART_Receive_IT(&_GPS_USART,&GPS.rxTmp,1);
}
//##################################################################################################################
