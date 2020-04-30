//=========================================================================
// runMag.c
// 
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
// Derived in part from several sources:
//      https://github.com/miguelrasteiro/RM3100.X
//      Jeremiah Mattison / Rm3100: https://os.mbed.com/users/fwrawx/code/Rm3100/
//      https://github.com/shannon-jia/rm3100
//      Song Qiang <songqiang1304521@gmail.com (Linux driver):
//          Linux kernel driver: https://github.com/torvalds/linux/tree/v5.3/drivers/iio/magnetometer
// 
// Author:      David Witten, KD0EAG
// Date:        April 21, 2020
// License:     GPL 3.0
//=========================================================================
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <linux/i2c-dev.h>
#include "device_defs.h"
#include "i2c.h"

char version[] = SIMPLEI2C_VERSION;
    
int setup_mag(pList *p);
long currentTimeMillis();
void mag_get_sample_data(int * XYZ);
int readTemp(pList *p, int devAddr);
int readMag(pList *p, int devAddr, int32_t *XYZ);
void showSettings(pList *p);
int getCommandLine(int argc, char** argv, pList *p);
int main(int argc, char** argv);

//------------------------------------------
// Static variables 
//------------------------------------------
//static unsigned short int           mSampleRate;
//static SensorPowerMode              mSensorMode;
static char                         mSamples[9];

struct busDev
{
    const char *devPath;
    int         enumVal;
};

static struct busDev busDevs[] =
{
    /* Darice Path,     enum Value*/
    {RASPI_I2C_BUS,     eRASPI_I2C_BUS},
    {ODROIDC1_I2C_BUS,  eODROIDC1_I2C_BUS},
    {ODROIDN2_I2C_BUS,  eODROIDC2_I2C_BUS},
    {ODROIDN2_I2C_BUS,  eODROIDN2_I2C_BUS},
    {NV_XAVIER_I2C_BUS, eNV_XAVIER_I2C_BUS},
    {NV_NANO_I2C_BUS,   eNV_NANO_I2C_BUS},
    {NULL,              -1}
};

//------------------------------------------
// readTemp()
//------------------------------------------
int readTemp(pList *p, int devAddr)
{
    int temp = -99;
    char data[2] = {0};
    char reg[1] = {MCP9808_REG_AMBIENT_TEMP};

    i2c_SetAddress(p->i2c_fd, devAddr);
    write(p->i2c_fd, reg, 1);
    if(read(p->i2c_fd, data, 2) != 2)
    {
        printf("Error : Input/Output error \n");
    }
    else
    {
        // Convert the data to 13-bits
        temp = ((data[0] & 0x1F) * 256 + data[1]);
        if(temp > 4095)
        {
            temp -= 8192;
        }
    }
    return temp;
}

///**
// * @fn SensorMode mag_set_power_mode(SensorPowerMode mode);
// *
// * @brief If possible, sets the sensor to the requested power mode. 
// *  *
// * @param mode  The requested sensor mode.
// * @returns     The actual state the sensor was set to.
// */
//SensorPowerMode mag_set_power_mode(SensorPowerMode mode)
//{
//    switch(mode)
//    {
//        default:
//            return mSensorMode;
//            
//        case SensorPowerModePowerDown:
//        case SensorPowerModeSuspend:
//            mSensorMode = mode;
//            mag_disable_interrupts();
//            break;
//
//        case SensorPowerModeActive:
//            mSensorMode = SensorPowerModeActive;
//            mag_enable_interrupts();
//            break;
//    }
//
//    return mSensorMode;
//}
//
///**
// * @fn SensorStatus mag_initialize_sensor();
// *
// * @brief Initializes the sensor into a known state.
// *
// * @retval SensorOK                         The sensor has already been initialized
// * @retval SensorErrorUnexpectedDevice      The sensor did not return expected results.
// * @retval SensorUnknownError               An unknown error has occured.
// */
//SensorStatus mag_initialize_sensor()
//{
//    char i2cbuffer[2];
//    char settings[7];
//     
//    if(rm3100_i2c_read(RM3100_LROSCADJ_REG, i2cbuffer, 2) != SensorOK)
//    {
//        return SensorErrorNonExistant;
//    }
//
//    if (    (i2cbuffer[0] != RM3100_LROSCADJ_VALUE) ||
//            (i2cbuffer[1] != RM3100_SLPOSCADJ_VALUE))
//    {
//        return SensorErrorUnexpectedDevice;
//    }
//
//    /* Zero buffer content */
//    i2cbuffer[0]=0; 
//    i2cbuffer[1]=0;
//    
//    /* Clears MAG and BEACON register and any pending measurement */
//    rm3100_i2c_write(RM3100_MAG_REG, i2cbuffer, 2);
//    
//    /* Initialize settings */
//    settings[0]=CCP1; /* CCPX1 */
//    settings[1]=CCP0; /* CCPX0 */
//    settings[2]=CCP1; /* CCPY1 */
//    settings[3]=CCP0; /* CCPY0 */
//    settings[4]=CCP1; /* CCPZ1 */
//    settings[5]=CCP0; /* CCPZ0 */
//    settings[6]=NOS;
//    /* settings[7]=TMRC;  */
//    
//    /*  Write register settings */
//    rm3100_i2c_write(RM3100_CCPX1_REG, settings, 7);
//    
//    mag_set_power_mode(SensorPowerModePowerDown);
//        
//    return SensorOK;
//}
//
///**
// * @fn SensorMode mag_get_power_mode();
// *
// * @brief Used to determine the current power mode of the sensor.
// *
// * @returns The current power mode of the sensor.
// */
//SensorPowerMode mag_get_power_mode()
//{
//    return mSensorMode;
//}
//
///**
// * @fn unsigned short int mag_set_sample_rate(unsigned short int sample_rate);
// *
// * @brief Requests the hardware to perform sample conversions at the specified rate.
// *
// * @param sample_rate The requested sample rate of the sensor in Hz.
// *
// * @returns The actual sample rate of the sensor.
// */
//unsigned short mag_set_sample_rate(unsigned short sample_rate)
//{
//    int i;
//    static char i2cbuffer[1];
//    const unsigned short int supported_rates[][2] = \
//    {
//        /* [Hz], register value */
//        {   2, 0x0A},   // up to 2Hz
//        {   4, 0x09},   // up to 4Hz
//        {   8, 0x08},   // up to 8Hz
//        {  16, 0x07},   // up to 16Hz
//        {  31, 0x06},   // up to 31Hz
//        {  62, 0x05},   // up to 62Hz
//        {  125, 0x04},  // up to 125Hz
//        {  220, 0x03}   // up to 250Hz
//    };
//    for(i = 0; i < sizeof(supported_rates)/(sizeof(unsigned short int)*2) - 1; i++)
//    {
//        if(sample_rate <= supported_rates[i][0])
//        {
//            break;
//        }
//    }
//    if (mSensorMode == SensorPowerModeActive) 
//    {
//        mag_disable_interrupts();
//    }
//    mSampleRate = supported_rates[i][0];
//    i2cbuffer[0]= (char)supported_rates[i][1];
//    rm3100_i2c_write(RM3100_TMRC_REG, i2cbuffer, 1);
//    if (mSensorMode == SensorPowerModeActive) 
//    {
//        mag_enable_interrupts();
//    }
//    return mSampleRate;
//
//}
//
///**
// * @fn unsigned short int mag_get_sample_rate();
// *
// * @brief Retrieves the mset sample rate of the sensor.
// *
// * @returns The actual sample rate of the sensor.
// */
//unsigned short mag_get_sample_rate()
//{
//    return mSampleRate;
//}

///**
// * @fn SensorStatus mag_get_sample_data(signed int *x, signed int *y, signed int *z);
// *
// * @brief Initiates an i2c read of the RM3100's sensor result registers.
// * @Each sensor reading consists of 3 bytes of data which are stored in 2’s
// * @complement format (range: -8388608 to 8388607) in the Results Registers
// *
// * @output: 3-axis Sensor data in Count 
// * 
// */
//void mag_get_sample_data(int * XYZ)
//{
//    // read out sensor data
//    rm3100_i2c_read(RM3100_QX2_REG, (char*)&mSamples, sizeof(mSamples)/sizeof(char));
//    
//    XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
//    XYZ[0] |= mSamples[1] * 256;
//    XYZ[0] |= mSamples[2];
//
//    XYZ[1] = ((signed char)mSamples[3]) * 256 * 256;
//    XYZ[1] |= mSamples[4] * 256;
//    XYZ[1] |= mSamples[5];
//
//    XYZ[2] = ((signed char)mSamples[6]) * 256 * 256;
//    XYZ[2] |= mSamples[7] * 256;
//    XYZ[2] |= mSamples[8];
//}

//------------------------------------------
// setup_mag()
//------------------------------------------
int setup_mag(pList *p)
{
    uint8_t ver = 0;
    int rv = SensorOK;

    // Set address of the RM3100
    i2c_SetAddress(p->i2c_fd,  p->magnetometerAddr);

    // Check Version
    if((ver = i2c_read(p->i2c_fd, RM3100I2C_REVID)) != (uint8_t)RM3100_VER_EXPECTED)
    {
        // Fail, exit...
        fprintf(stderr, "RM3100 REVID NOT CORRECT: ");
        fprintf(stderr, "RM3100 REVID: 0x%X <> EXPECTED: 0x%X.\n\n", ver, RM3100_VER_EXPECTED);
        fflush(stdout);
        exit(1);
    }
    else
    {
        uint8_t i2cbuffer[2];
        // Zero buffer content
        i2cbuffer[0] = 0;
        i2cbuffer[1] = 0;
        if(p->verboseFlag)
        {
             printf("RM3100 Detected Properly: ");
             printf("REVID: %x.\n", ver);
        }

        // Clears RM3100I2C_POLL and RM3100I2C_CMM register and any pending measurement
        i2c_writebuf(p->i2c_fd, RM3100I2C_POLL, i2cbuffer, 2);

        // Initialize settings
        uint8_t settings[7];
        settings[0] = CCP1;       // CCPX1 200 Cycle Count
        settings[1] = CCP0;       // CCPX0
        settings[2] = CCP1;       // CCPY1 200 Cycle Count
        settings[3] = CCP0;       // CCPY0
        settings[4] = CCP1;       // CCPZ1 200 Cycle Count
        settings[5] = CCP0;       // CCPZ0
        settings[6] = NOS;

        // // Linear equation: gain (LSB/uT) = (0.3671 * CycleCount + 1.5)
        // int CycleCount = CCP0 | (CCP1 << 8);
        // float gain = 0.3671 * CycleCount + 1.5;
        
        //  Write register settings
        i2c_writebuf(p->i2c_fd, RM3100I2C_CCX_1, settings, 7);
        //mag_set_power_mode(SensorPowerModePowerDown);
    }
    if(p->verboseFlag)
    {
        // poll the RM3100 for a three axis measurement
        printf("Polling I2C device: %x.\n\n", RM3100_I2C_ADDRESS);
        fflush(stdout);
    }
    // Sleep for 1 second
    usleep(100000);                           // delay to help monitor DRDY pin on eval board
    return rv;
}

////------------------------------------------
//// readMag() - Working Version
////------------------------------------------
//int readMag(pList *p, int devAddr, int32_t *XYZ)
//{
//    //printf("\nentering readMag()\n");
//    uint8_t mSamples[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
//    char data[2] = {0};
//    int bytes_read;
//    char reg[1] = { RM3100I2C_XYZ };
//
//    // set address of the RM3100.
//    i2c_SetAddress(p->i2c_fd, devAddr);
//    // Write command to  use polling.
//    i2c_regWrite((p->i2c_fd, RM3100I2C_POLL, RM3100I2C_POLLXYZ);
//
//    // Check if DRDY went high and wait unit high before reading results
//    while((p->i2c_fd, i2c_regRead(p->i2c_fd, RM3100I2C_STATUS) & RM3100I2C_READMASK) != RM3100I2C_READMASK) {}
//
//    // Read the XYZ registers
//    write(p->i2c_fd, reg, 1);
//    if((bytes_read = read(p->i2c_fd, mSamples, sizeof(mSamples))) != sizeof(mSamples))
//    {
//        perror("i2c transaction i2c_readbuf() failed.\n");
//    }
//
//    XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
//    XYZ[0] |= mSamples[1] * 256;
//    XYZ[0] |= mSamples[2];
//    XYZ[1] = ((signed char)mSamples[3]) * 256 * 256;
//    XYZ[1] |= mSamples[4] * 256;
//    XYZ[1] |= mSamples[5];
//    XYZ[2] = ((signed char)mSamples[6]) * 256 * 256;
//    XYZ[2] |= mSamples[7] * 256;
//    XYZ[2] |= mSamples[8];
//
//    //printf("exiting readMag()\n");
//    return bytes_read;
//}

//------------------------------------------
// readMag()
//------------------------------------------
int readMag(pList *p, int devAddr, int32_t *XYZ)
{
    // printf("\nentering readMag()\n");
    int bytes_read = 0;
    //char data[2] = {0};
    char reg[1] = { RM3100I2C_XYZ };

    // set address of the RM3100.
    i2c_SetAddress(p->i2c_fd, devAddr);
    // Write command to  use polling.
    i2c_regWrite(p->i2c_fd, RM3100I2C_POLL, RM3100I2C_POLLXYZ);

    // printf("Check if DRDY went high\n");

    // Check if DRDY went high and wait unit high before reading results
    while((p->i2c_fd, i2c_regRead(p->i2c_fd, RM3100I2C_STATUS) & RM3100I2C_READMASK) != RM3100I2C_READMASK) {}

    // printf("write(p->i2c_fd, reg, 1)h\n");
    // Read the XYZ registers
    write(p->i2c_fd, reg, 1);
    if((bytes_read = i2c_readbuf(p->i2c_fd, devAddr, RM3100I2C_XYZ, (unsigned char*) &mSamples, sizeof(mSamples)/sizeof(char))) != sizeof(mSamples)/sizeof(char))
    //if((bytes_read = read(p->i2c_fd, mSamples, sizeof(mSamples))) != sizeof(mSamples))
    {
        perror("i2c transaction i2c_readbuf() failed.\n");
    }

    XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
    XYZ[0] |= mSamples[1] * 256;
    XYZ[0] |= mSamples[2];
    XYZ[1] = ((signed char)mSamples[3]) * 256 * 256;
    XYZ[1] |= mSamples[4] * 256;
    XYZ[1] |= mSamples[5];
    XYZ[2] = ((signed char)mSamples[6]) * 256 * 256;
    XYZ[2] |= mSamples[7] * 256;
    XYZ[2] |= mSamples[8];

    // printf("exiting readMag()\n");
    return bytes_read;
}

    //// read out sensor data
    //rm3100_i2c_read(RM3100_QX2_REG, (char*)&mSamples, sizeof(mSamples)/sizeof(char));
    //
    //XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
    //XYZ[0] |= mSamples[1] * 256;
    //XYZ[0] |= mSamples[2];
    //
    // etc...


//------------------------------------------
// currentTimeMillis()
//------------------------------------------
long currentTimeMillis()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

#define UTC (0) 

//------------------------------------------
// getUTC()
//------------------------------------------
struct tm *getUTC()
{
    time_t now = time(&now);
    if (now == -1)
    {
        puts("The time() function failed");
    }
    struct tm *ptm = gmtime(&now);
    if (ptm == NULL)
    {
        puts("The gmtime() function failed");
    }    
    //printf("UTC time: %s", asctime(ptm));
    //return 0;
    return ptm;
}

//------------------------------------------
// showSettings()
//------------------------------------------
void showSettings(pList *p)
{
    fprintf(stdout, "\nVersion = %s\n", version);
    fprintf(stdout, "\nCurrent Parameters:\n\n");
    fprintf(stdout, "   I2C bus number as integer:                  %i\n", p->i2cBusNumber);
    fprintf(stdout, "   Built in self test (BIST) value:            0x%X\n", p->doBist);
    fprintf(stdout, "   Cycle count X as integer:                   %i\n", p->cc_x);
    fprintf(stdout, "   Cycle count Y as integer:                   %i\n", p->cc_y);
    fprintf(stdout, "   Cycle count Z as integer:                   %i\n", p->cc_z);
    fprintf(stdout, "   Hide raw measurements:                      %s\n", p->hideRaw ? "TRUE" : "FALSE" );
    fprintf(stdout, "   Format output as JSON:                      %s\n", p->jsonFlag ? "TRUE" : "FALSE" );
    fprintf(stdout, "   Read local temperature only:                %s\n", p->localTempOnly ? "TRUE" : "FALSE");
    fprintf(stdout, "   Local temperature address:                  %2X\n", p->localTempAddr);
    fprintf(stdout, "   Read magnetometer only:                     %s\n", p->magnetometerOnly ?  "TRUE" : "FALSE");
    fprintf(stdout, "   Magnetometer address:                       %2X\n", p->magnetometerAddr);
    fprintf(stdout, "   Show Parameters:                            %s\n", p->showParameters ? "TRUE" : "FALSE" );
    fprintf(stdout, "   Quiet mode:                                 %s\n", p->quietFlag ? "TRUE" : "FALSE" );
    fprintf(stdout, "   Read remote temperature only:               %s\n", p->remoteTempOnly ? "TRUE" : "FALSE");
    fprintf(stdout, "   Remote temperature address:                 %2X\n", p->remoteTempAddr);
    fprintf(stdout, "   Return single magnetometer reading:         %s\n", p->singleRead ? "TRUE" : "FALSE");
    fprintf(stdout, "   Read Simple Magnetometer Support Board:     %i\n", p->boardType);
    fprintf(stdout, "   Timestamp format:                           %s\n", p->tsMilliseconds ? "RAW" : "UTCSTRING");
    fprintf(stdout, "   Verbose output:                             %s\n", p->verboseFlag ? "TRUE" : "FALSE");
    fprintf(stdout, "   Read Board with Extender:                   %i\n", p->boardType);
    fprintf(stdout, "\n\n");
}

//------------------------------------------
// getCommandLine()
//------------------------------------------
int getCommandLine(int argc, char** argv, pList *p)
{
    int c;
    int digit_optind = 0;
    
    if(p != NULL)
    {
        memset(p, 0, sizeof(pList));
    }
    
    p->boardType        = 0;
    p->doBist           = FALSE;
    p->cc_x             = CC_200;
    p->cc_y             = CC_200;
    p->cc_z             = CC_200;

    p->hideRaw          = FALSE;
    p->i2cBusNumber     = 1;
    p->i2c_fd           = 0;
    p->jsonFlag         = FALSE;

    p->localTempOnly    = FALSE;
    p->localTempAddr    = 0x19;
    p->remoteTempOnly   = FALSE;
    p->remoteTempAddr   = 0x18;  
    p->magnetometerOnly = FALSE;
    p->magnetometerAddr = 0x20;
    p->outDelay         = 100000;
    p->quietFlag        = TRUE;
    p->showParameters   = FALSE;
    p->singleRead       = FALSE;
    p->tsMilliseconds   = FALSE;

    p->x_gain           = GAIN_75;
    p->y_gain           = GAIN_75;
    p->z_gain           = GAIN_75;
    
    p->verboseFlag      = FALSE;
    p->Version          = version;
   
    while((c = getopt(argc, argv, "?b:B:c:CHhjlL:mM:o:PqrR:sSTvXhqV")) != -1)
    {
        int this_option_optind = optind ? optind : 1;
        switch (c)
        {
            case 'b':
                p->i2cBusNumber = atoi(optarg);
                break;
            case 'B':
                p->doBist = atoi(optarg);
                break;
            case 'c':
                p->cc_x = p->cc_y = p->cc_z = atoi(optarg);
                break;
            case 'C':
                // fprintf(stdout, "CycleCount: '%s'\n", optarg);
                //p->cc_x = p->cc_y = p->cc_z = atoi(optarg);
                break;
            case 'H':
                p->hideRaw = TRUE;
                break;
            case 'j':
                p->jsonFlag = TRUE;
                break;
            case 'l':
                p->localTempOnly = TRUE;
                break;
            case 'L':
                p->localTempAddr = atoi(optarg);
                break;
            case 'm':
                p->magnetometerOnly = TRUE;
                break;
            case 'M':
                p->magnetometerAddr = atoi(optarg);
                break;
            case 'P':
                p->showParameters = TRUE;
                break;
            case 'o':
                p->outDelay = atoi(optarg) * 1000;
                break;
            case 'q':
                p->quietFlag = TRUE;
                p->verboseFlag = FALSE;
                break;
            case 'r':
                p->remoteTempOnly = TRUE;
                break;
            case 'R':
                p->remoteTempAddr = atoi(optarg);
                break;
            case 's':
                p->singleRead = TRUE;
                break;
            case 'S':
                p->boardType = 1;
                break;
            case 'T':
                p->tsMilliseconds = TRUE;
                break;
            case 'V':
                return 1;
                break;
            case 'v':
                p->verboseFlag = TRUE;
                p->quietFlag = FALSE;
                break;
            case 'X':
                p->boardType = 0;
                break;
            case 'h':
            case '?':
                fprintf(stdout, "\n%s Version = %s\n", argv[0], version);
                fprintf(stdout, "\nParameters:\n\n");
                fprintf(stdout, "   -b <bus as integer>    :  I2C bus number as integer.\n");
                fprintf(stdout, "   -B <reg mask>          :  Do built in self test (BIST). [Not really implemented].\n");
                fprintf(stdout, "   -c <count>             :  Set cycle counts as integer (default 200).\n");
                fprintf(stdout, "   -C                     :  Read cycle count registers.   [Not really implemented].\n");
                fprintf(stdout, "   -H                     :  Hide raw measurments.\n");
                fprintf(stdout, "   -j                     :  Format output as JSON.\n");
                fprintf(stdout, "   -l                     :  Read local temperature only.  [Not really implemented].\n");
                fprintf(stdout, "   -L [addr as integer]   :  Local temperature address (default 19 hex).\n");
                fprintf(stdout, "   -m                     :  Read magnetometer only.\n");
                fprintf(stdout, "   -M [addr as integer]   :  Magnetometer address (default 20 hex).\n");
                fprintf(stdout, "   -P                     :  Show Parameters.\n");
                fprintf(stdout, "   -q                     :  Quiet mode.                   [Not really implemented].\n");
                fprintf(stdout, "   -r                     :  Read remote temperature only.\n");
                fprintf(stdout, "   -R [addr as integer]   :  Remote temperature address (default 18 hex).\n");
                fprintf(stdout, "   -s                     :  Return single reading.\n");
                fprintf(stdout, "   -S                     :  Read Simple Magnetometer Support Board.\n");
                fprintf(stdout, "   -T                     :  Raw timestamp in milliseconds (default: UTC string).\n");
                fprintf(stdout, "   -v                     :  Verbose output.               [Not really implemented].\n");
                fprintf(stdout, "   -V                     :  Display software version and exit.\n");
                fprintf(stdout, "   -X                     :  Read board with extender (default).\n");
                fprintf(stdout, "   -h or -?               :  Display this help.\n\n");
                return 1;
                break;
            default:
                fprintf(stdout, "?? getopt returned character code 0x%2X ??\n", c);
                break;
        }
    }
    if(optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
        {
            printf ("%s ", argv[optind++]);
        }
        printf ("\n");
    }
    return 0;
}

#define OUTBUFLEN 60

//------------------------------------------
//  main()
//------------------------------------------
int main(int argc, char** argv)
{
    pList p;
    char outStr[OUTBUFLEN] = "";
    int32_t rXYZ[3];
    int32_t xyz[3];
    int temp = 0;
    float cTemp = 0.0;
    int rv = 0;
    struct tm *utcTime = getUTC();
    
    if(p.verboseFlag)
    {
        printf("\nUTC time: %s", asctime(utcTime));
    }
    if((rv = getCommandLine(argc, argv, &p)) != 0)
    {
        return rv;
    }
    if(p.showParameters)
    {
        showSettings(&p);
    }

    // Open I2C bus (only one at a time for now)    
    i2c_open(&p);

    // Setup the magnetometer.
    setup_mag(&p);
    
    // loop
    while(1)
    {
        //  Read temp sensor.
        if(!p.magnetometerOnly)
        {
            temp = readTemp(&p, MCP9808_I2CADDR_DEFAULT);
            cTemp = temp * 0.0625;
        }
 
        // Read Magnetometer.
        if(!p.localTempOnly)
        {
            readMag(&p, RM3100_I2C_ADDRESS, rXYZ);
            xyz[0] = (rXYZ[0] / p.x_gain);
            xyz[1] = (rXYZ[1] / p.y_gain);
            xyz[2] = (rXYZ[2] / p.z_gain);
        }
    
        // Output the results.
        if(!(p.jsonFlag))
        {
            if(p.tsMilliseconds)
            {
                fprintf(stdout, "Time Stamp: %ld, ", currentTimeMillis());
            }
            else
            {
                strftime(outStr, OUTBUFLEN, "%d %b %Y %T", utcTime);                
                fprintf(stdout, "Time: %s", outStr);
            }
            fprintf(stdout, ", Temp: %.2f C",    cTemp);
            fprintf(stdout, ", x: %i",           xyz[0]);
            fprintf(stdout, ", y: %i",           xyz[1]);
            fprintf(stdout, ", z: %i",           xyz[2]);
            if(!p.hideRaw)
            {
                fprintf(stdout, ", rx: %i",           rXYZ[0]);
                fprintf(stdout, ", ry: %i",           rXYZ[1]);
                fprintf(stdout, ", rz: %i",           rXYZ[2]);
            }
            fprintf(stdout, "\n");
        }
        else
        {
            fprintf(stdout, "{ ");
            if(p.tsMilliseconds)
            {
                fprintf(stdout, "\"%ld\", ",  currentTimeMillis());
            }
            else
            {
                strftime(outStr, OUTBUFLEN, "%d %b %Y %T", utcTime);        // RFC 2822: "%a, %d %b %Y %T %z"      RFC 822: "%a, %d %b %y %T %z"  
                fprintf(stdout, "ts:\"%s\", ", outStr);
            }
            fprintf(stdout, " lt:\"%.2f\", ",  cTemp);
            fprintf(stdout, " x:\"%i\", ",    xyz[0]);
            fprintf(stdout, " y:\"%i\", ",    xyz[1]);
            fprintf(stdout, " z:\"%i\",",     xyz[2]);
            if(!p.hideRaw)
            {
                fprintf(stdout, " rx:\"%i\",",    rXYZ[0]);
                fprintf(stdout, " ry:\"%i\",",    rXYZ[1]);
                fprintf(stdout, " rz:\"%i\"",     rXYZ[2]);
            }
            fprintf(stdout, " }\n");
        }
        fflush(stdout);
        if(p.singleRead)
        {
            break;
        }
        // wait p.outDelay (1000 ms default) for next poll.
        usleep(p.outDelay);
    }
    i2c_close(p.i2c_fd);
    return 0;
}
