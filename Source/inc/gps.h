/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Generic driver for the GPS receiver UP501

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __GPS_H__
#define __GPS_H__

/* Structure to handle the GPS parsed data in ASCII */
typedef struct
{
    char NmeaDataType[12];
    char NmeaUtcTime[22];
    char NmeaDataStatus[4];
    char NmeaLatitude[20];
    char NmeaLatitudePole[4];
    char NmeaLongitude[22];
    char NmeaLongitudePole[4];
    char NmeaFixQuality[4];
    char NmeaSatelliteTracked[6];
    char NmeaHorizontalDilution[12];
    char NmeaAltitude[16];
    char NmeaAltitudeUnit[4];
    char NmeaHeightGeoid[16];
    char NmeaHeightGeoidUnit[4];
    char NmeaSpeed[16];
    char NmeaDetectionAngle[16];
    char NmeaDate[16];
}tNmeaGpsData;

extern tNmeaGpsData NmeaGpsData;

/*!
 * \brief Initializes the handling of the GPS receiver
 */
void GpsInit( void );

/*!
 * \brief Switch ON the GPS
 */
void GpsStart( void );

/*!
 * \brief Switch OFF the GPS
 */
void GpsStop( void );

/*!
 * Updates the GPS status
 */
void GpsProcess( void );

/*!
 * \brief PPS signal handling function
 */
void GpsPpsHandler( bool *parseData );

/*!
 * \brief PPS signal handling function
 *
 * \retval ppsDetected State of PPS signal.
 */
bool GpsGetPpsDetectedState( void );

/*!
 * \brief Indicates if GPS has fix
 *
 * \retval hasFix
 */
bool GpsHasFix( void );

/*!
 * \brief Converts the latest Position (latitude and longitude) into a binary
 *        number
 */
void GpsConvertPositionIntoBinary( void );

/*!
 * \brief Converts the latest Position (latitude and Longitude) from ASCII into
 *        DMS numerical format
 */
void GpsConvertPositionFromStringToNumerical( void );

/*!
 * \brief Gets the latest Position (latitude and Longitude) as two double values
 *        if available
 *
 * \param [OUT] lati Latitude value
 * \param [OUT] longi Longitude value
 *
 * \retval status [SUCCESS, FAIL]
 */
uint8_t GpsGetLatestGpsPositionDouble ( double *lati, double *longi );

/*!
 * \brief Gets the latest Position (latitude and Longitude) as two binary values
 *        if available
 *
 * \param [OUT] latiBin Latitude value
 * \param [OUT] longiBin Longitude value
 *
 * \retval status [SUCCESS, FAIL]
 */
uint8_t GpsGetLatestGpsPositionBinary ( int32_t *latiBin, int32_t *longiBin );

/*!
 * \brief Parses the NMEA sentence.
 *
 * \remark Only parses GPGGA and GPRMC sentences
 *
 * \param [IN] rxBuffer Data buffer to be parsed
 * \param [IN] rxBufferSize Size of data buffer
 *
 * \retval status [SUCCESS, FAIL]
 */
uint8_t GpsParseGpsData( int8_t *rxBuffer, int32_t rxBufferSize );

/*!
 * \brief Returns the latest altitude from the parsed NMEA sentence
 *
 * \retval altitude
 */
int16_t GpsGetLatestGpsAltitude( void );

/*!
 * \brief Format GPS data into numeric and binary formats
 */
void GpsFormatGpsData( void );
uint8_t GpsParseGpsData_2( int8_t *rxBuffer);
/*!
 * \brief Resets the GPS position variables
 */
void GpsResetPosition( void );

#endif  // __GPS_H__
