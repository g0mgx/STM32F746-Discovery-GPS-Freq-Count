#ifndef _GPS_H_
#define _GPS_H_

#define TRUE 1
#define FALSE 0

#include <stdint.h>

//##################################################################################################################
typedef struct
{
	unsigned int	UTC_Hour;
	unsigned int	UTC_Min;
	unsigned int	UTC_Sec;
	unsigned int	UTC_MicroSec;
	
	float			Latitude;
	double			LatitudeDecimal;
	char			NS_Indicator;
	float			Longitude;
	double			LongitudeDecimal;
	char			EW_Indicator;
	
	unsigned int	PositionFixIndicator;
	unsigned int	SatellitesUsed;
	float			HDOP;
	float			MSL_Altitude;
	char			MSL_Units;
	float			Geoid_Separation;
	char			Geoid_Units;
	
	float			AgeofDiffCorr;
	char			DiffRefStationID[4];
	char			CheckSum[2];
	
}GPGGA_t;

typedef struct
{
	unsigned int	UTC_Hour;
	unsigned int	UTC_Min;
	unsigned int	UTC_Sec;
	unsigned int	UTC_MicroSec;

	char			Validity;

	float			Latitude;
	double			LatitudeDecimal;
	char			NS_Indicator;
	float			Longitude;
	double			LongitudeDecimal;
	char			EW_Indicator;

	float			SpeedKnots;
	float			TrueCourse;
	unsigned int	Date_Day;
	unsigned int	Date_Month;
	unsigned int	Date_Year;

	float			MagneticVariationDegrees;
	char			Variation_EW_Indicator;
	char			TBD;
	unsigned int	CheckSum;

}GPRMC_t;

typedef struct 
{
	uint8_t		rxBuffer[512];
	uint16_t	rxIndex;
	uint8_t		rxTmp;	
	uint32_t	LastTime;	
	
	GPRMC_t		GPRMC;
	
}GPS_t;

extern GPS_t GPS;
//##################################################################################################################
void	GPS_Init(void);
void	GPS_CallBack(void);
void	GPS_Process(void);
//##################################################################################################################

#endif
