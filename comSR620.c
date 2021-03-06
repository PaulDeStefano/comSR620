#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>



#define  DATA_DIR       "./TicData/"        // データ保存ディレクトリの指定
#define  SERIAL_DEVICE  "/dev/ttyS0"        // シリアル・デバイス・パスの指定
#define  C_ARRAYSIZE    256
#define  DEF_TIMEBASE   256



void sd_hook();
void parseOpts(int argc, char * const argv[], char *tty);


int   iSerialFD        =  -1;
char    sBuffer[ 256 ];
char   sISO8601[ C_ARRAYSIZE ];


int
main(int argc, char **argv) {

    int         iStatus,
              iGetCount,
                 retVal; // error checking
    char       cGetData,
               *cBufPtr,
               *cBufPtr2;
    char  sDataPath_old[ 256 ],
          sDataPath_now[ 256 ],
                    tty[ C_ARRAYSIZE ];

    FILE    *fWriteData = NULL;
    time_t      timeNow;
    long        nanosec;

    struct  tm         *stTm;
    struct  termio  stTermio;
    struct  timespec   stNow;


    memset( sDataPath_old, '\0', sizeof( sDataPath_old ) );
    memset( tty, '\0', sizeof tty  );

    signal( SIGINT, sd_hook );

    cBufPtr2 = tty;
    parseOpts(argc,argv,cBufPtr2);
    if ( 0 == strcmp(tty,"") ) {
        /* no tty specified */
        strncpy(tty,SERIAL_DEVICE,sizeof tty ) ;
        printf("[Notice] Serial device not specified, using default: %s\n",tty);
    }

    /* open serial port */
    sprintf( sBuffer, "Opening serial device: %s\n", tty );
    printf(sBuffer);
    iSerialFD = open( SERIAL_DEVICE, O_RDWR );
    if ( iSerialFD < 0 ) {
        sprintf( sBuffer, "[Error] open of %s failed... ", tty );
        perror( sBuffer );
        exit( 0 );
    }
    printf("Opening serial device: success\n");


    iStatus = ioctl( iSerialFD, TCGETA, &stTermio );
    if ( iStatus < 0 ) {
        perror( "[Error] ioctl (TCGETA) failed...");
        sd_hook();
    }

    stTermio.c_cflag |=  B9600;
    stTermio.c_lflag &= ~ECHO;
    stTermio.c_lflag &= ~ICANON;

    stTermio.c_cc[  VMIN ] = 1;
    stTermio.c_cc[ VTIME ] = 0;

    iStatus = ioctl( iSerialFD, TCSETA, &stTermio );
    if ( iStatus < 0 ) {
        perror( "[Error] ioctl (TCSETA) failed..." );
        sd_hook();
    } 
    printf("Setting port I/O parameters: success\n");

  /************************************************************/
  /*     SR620 パラメータ設定 （設定値の反映確認は省略）      */
  /*                                                          */

    /* リセット */
    strcpy( sBuffer, "*RST;*CLS\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* timebase = internal */
    strcpy( sBuffer, "CLCK0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* mode = time */
    strcpy( sBuffer, "MODE0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* trigger mode = +time (not +/- time) */
    strcpy( sBuffer, "ARMM1\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* display = mean */
    strcpy( sBuffer, "DISP0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the measurement source...  [ 0:A , 1:B ] */
    strcpy( sBuffer, "SRCE0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the number of samples... */
    strcpy( sBuffer, "SIZE1\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the trigger slope...  [ 0:positive , 1:negative ] */
    strcpy( sBuffer, "TSLP1,0;TSLP2,0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the trigger level... */
    strcpy( sBuffer, "LEVL1,1.2;LEVL2,1.2\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the trigger ac/dc coupling...  [ 0:DC , 1:AC ] */
    strcpy( sBuffer, "TCPL1,0;TCPL2,0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* Set the trigger impedance...  [ 0:50ohm , 1:1meg ] */
    strcpy( sBuffer, "TERM1,0;TERM2,0\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* local/remote = remote */
    strcpy( sBuffer, "LOCL1\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* auto measure */
    strcpy( sBuffer, "AUTM1\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

  /*                                                          */
  /*     SR620 パラメータ設定 （設定値の反映確認は省略）      */
  /************************************************************/


    while ( 1 ) {

        strcpy( sBuffer, "*WAI;XAVG?\n" );
        write( iSerialFD, sBuffer, strlen( sBuffer ) );

        cBufPtr = sBuffer;

        while ( 1 ) {

            // read results
            iGetCount = read( iSerialFD, &cGetData, 1 );
            if ( iGetCount < 0 ) {
                perror( "[Error] read failed..." );
                sd_hook();
            }

            *cBufPtr = cGetData;
            cBufPtr++;
            if (cBufPtr>=sBuffer+sizeof(sBuffer))
              {
              fprintf(stderr,"Answer exeeds bounds.\n\n");
              return 0;
              }

            if ( ( cGetData == '\n' ) || ( cGetData == '\0' ) ) {
                /* get new timestamp */
                retVal = clock_gettime( CLOCK_REALTIME, &stNow );
                if ( retVal != 0 ) {
                    perror("[Error] clock_gettime() failed..." );
                }

                tcflush( iSerialFD, TCIFLUSH );

                *cBufPtr = '\0';

                /* pull 'time_t' type (aka UNIXtime) from 'timespec' type variable 'stNow' */
                retVal = clock_gettime( CLOCK_REALTIME, &stNow );
                timeNow = stNow.tv_sec ;
                /* second part of struct 'timespec' contains nanosecond part */
                nanosec = stNow.tv_nsec ;
                stTm = gmtime( &timeNow );

                /* Pick log file */
                sprintf( sDataPath_now, "%s%d%02d%02d.dat"
                                      , DATA_DIR, ( stTm->tm_year + 1900 ), ( stTm->tm_mon + 1 ), stTm->tm_mday );
                if ( strcmp( sDataPath_now, sDataPath_old ) != 0 ) {
                    if ( fWriteData != NULL ) {
                        fclose( fWriteData );
                    }
                    fWriteData = fopen( sDataPath_now, "aw" );
                    strcpy( sDataPath_old, sDataPath_now );
                }

                /* write data to log */
                strftime(sISO8601, sizeof(sISO8601),"%Y-%m-%dT%H:%M:%S", stTm);
                fprintf( fWriteData, "%s %12.12lf %ju %09li\n"
                                   , sISO8601
                                   , atof( sBuffer )
                                   , (uintmax_t)timeNow
                                   , nanosec
                                       );
                fflush( fWriteData );

                break;

            }

        }

    }

}



void sd_hook()
{

    if ( iSerialFD != -1 ) {

        strcpy( sBuffer, "LOCL0\n" );
        write( iSerialFD, sBuffer, strlen( sBuffer ) );

        close( iSerialFD );
    
    }

    exit( 0 );

}


void parseOpts(int argc, char * const argv[], char *tty) {
    /* parse command line options */
    int         opt;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            if ( C_ARRAYSIZE <= strlen(optarg)) {
                fprintf(stderr,"[Error] string entry for option -d too long\n");
                exit(EXIT_FAILURE);
            } else {
                strncpy(tty,optarg,C_ARRAYSIZE);
            }
            break;
        default: /* '?' */
            fprintf(stderr, "Usage %s -d /dev/<serial device>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}
