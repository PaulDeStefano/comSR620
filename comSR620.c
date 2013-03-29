#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <termio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>



#define  DATA_DIR       "./TicData/"        // データ保存ディレクトリの指定
#define  SERIAL_DEVICE  "/dev/ttyS0"        // シリアル・デバイス・パスの指定



void sd_hook();



int   iSerialFD        =  -1;
char    sBuffer[ 256 ];



int main() {

    int         iStatus,
              iGetCount;
    char       cGetData,
               *cBufPtr;
    char  sDataPath_old[ 256 ],
          sDataPath_now[ 256 ];

    FILE    *fWriteData = NULL;
    time_t      timeNow;

    struct  tm         *stTm;
    struct  termio  stTermio;


    memset( sDataPath_old, '\0', sizeof( sDataPath_old ) );


    signal( SIGINT, sd_hook );


    iSerialFD = open( SERIAL_DEVICE, O_RDWR );
    if ( iSerialFD < 0 ) {
        sprintf( sBuffer, "[Error] open of %s failed... ", SERIAL_DEVICE );
        perror( sBuffer );
        exit( 0 );
    }


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


  /************************************************************/
  /*     SR620 パラメータ設定 （設定値の反映確認は省略）      */
  /*                                                          */

    /* リセット */
    strcpy( sBuffer, "*RST;*CLS\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* timebase = external */
    strcpy( sBuffer, "CLCK1\n" );
    write( iSerialFD, sBuffer, strlen( sBuffer ) );

    /* mode = time */
    strcpy( sBuffer, "MODE0\n" );
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
            // get timestamp
            time( &timeNow );
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

                tcflush( iSerialFD, TCIFLUSH );

                *cBufPtr = '\0';

                stTm = gmtime( &timeNow );

                sprintf( sDataPath_now, "%s%d%02d%02d.dat"
                                      , DATA_DIR, ( stTm->tm_year + 1900 ), ( stTm->tm_mon + 1 ), stTm->tm_mday );
                if ( strcmp( sDataPath_now, sDataPath_old ) != 0 ) {
                    if ( fWriteData != NULL ) {
                        fclose( fWriteData );
                    }
                    fWriteData = fopen( sDataPath_now, "aw" );
                    strcpy( sDataPath_old, sDataPath_now );
                }

                fprintf( fWriteData, "%d/%02d/%02d,%02d:%02d:%02d,%12.12lf,%i\n"
                                   , ( stTm->tm_year + 1900 ), ( stTm->tm_mon + 1 ), stTm->tm_mday
                                   , stTm->tm_hour, stTm->tm_min, stTm->tm_sec, atof( sBuffer )
                                   , (int)timeNow );
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



