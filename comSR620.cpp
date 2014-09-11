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
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cerrno>

#define  DATA_DIR       "./TicData/"        
#define  SERIAL_DEVICE  "/dev/ttyS0"        

class sr620args{
    int last_day_number;        // saves last day number for rotation 
public:
    sr620args():device_name(SERIAL_DEVICE), data_path(DATA_DIR)
    {
	valid_tics.push_back("TravTIC");
	valid_tics.push_back("NICTTIC");
	valid_tics.push_back("LSUTIC");

	valid_locations.push_back("NU1");
	valid_locations.push_back("SK");
	valid_locations.push_back("RnHut");
	valid_locations.push_back("Kenkyuto");
	valid_locations.push_back("ND280");
	valid_locations.push_back("OTHER");
        
        query_only=0;
        last_day_number=-1;
        daily_rotate=0;
        verbose=0;
        calibrate=0;
        debug=0;
    }
// convenience lists
    std::vector<std::string> valid_tics;
    std::vector<std::string> valid_locations;
// actual parameters
    std::string device_name;
    std::string data_path;

    std::string tic_name;
    std::string loc_name;
    std::string channel_a;
    std::string channel_b;
    std::string user_suffix;
    std::string override_name;
    
    int query_only;
    int daily_rotate;
    int verbose;
    int calibrate;
    int debug;
    
    int needRotate(struct timespec *tm=NULL){
        struct timespec stNow;
	struct tm *stTm;
	time_t now;
	if (clock_gettime( CLOCK_REALTIME, &stNow )!=0)
	    perror("[Error] realtime clock not available!");
	now=stNow.tv_sec;
	stTm = gmtime( &now );
        if (tm) memcpy(tm,&stNow,sizeof(struct timespec));
        if (stTm->tm_mday!=last_day_number && ((daily_rotate!=0) || (last_day_number<0)))
            return 1;
        return 0;
    }
    
    std::string generateFileName(int sample=0){
	std::ostringstream os;
	struct timespec stNow;
	struct tm *stTm;
	time_t now;
	if (clock_gettime( CLOCK_REALTIME, &stNow )!=0)
	    perror("[Error] realtime clock not available!");
	now=stNow.tv_sec;
	stTm = gmtime( &now );
	if (!sample)
        {
                os << data_path;
                last_day_number=stTm->tm_mday;
                if (data_path.find_last_of("/")!=data_path.size()-1) os << "/";
                os << std::setw(4) << std::setfill('0') << (stTm->tm_year + 1900) <<
                    std::setw(2) << std::setfill('0') << ( stTm->tm_mon + 1 ) <<
                    std::setw(2) << std::setfill('0') << stTm->tm_mday << "T" <<
                    std::setw(2) << std::setfill('0') << stTm->tm_hour <<
                    std::setw(2) << std::setfill('0') << stTm->tm_min <<
                    std::setw(2) << std::setfill('0') << stTm->tm_sec << ".";
        }
        else
        {
            os << "YYYYMMDDTHHMMSS.";
        }
	if (!override_name.empty())
	os << override_name << ".dat";
	else
	{
		if (!loc_name.empty()) os << loc_name << ".";
		if (!channel_a.empty() && !channel_b.empty()) os << channel_a << "-" << channel_b << ".";
		if (!tic_name.empty()) os << tic_name;
		if (!user_suffix.empty()) os << "." << user_suffix;
		os << ".dat";
	}
	return os.str();
    }
    
    int parseOption(int argc,char* argv[],int &idx,std::string shortstr,std::string longstr,int has_par,std::string &result)
    {
        if (std::string(argv[idx])==shortstr || std::string(argv[idx])==longstr)
        {
            if (!has_par) return 1;
            if (idx+1>=argc)
            {
                std::cerr << "Parameter error at "<< argv[idx] << "(index: " << idx << ")" << std::endl;
                return -1;
            }
            result=argv[idx+1];
            idx++;
            return 2;
        }
        return 0;
    }
    
    int parse_args(int argc,char *argv[])
    {
        int p_arg=1;
        int res=0;
        std::string value;
        while (p_arg<argc)
        {
            if (parseOption(argc,argv,p_arg,"-h","--help",0,value)==1)
            {
                printHelp();
                return 1;
            }
            if (parseOption(argc,argv,p_arg,"-q","--query",0,value)==1)
            {
                query_only=1;
                p_arg++;
                continue;
            }
            if (parseOption(argc,argv,p_arg,"-r","--rotate",0,value)==1)
            {
                daily_rotate=1;
                p_arg++;
                continue;
            }
             if (parseOption(argc,argv,p_arg,"-v","--verbose",0,value)==1)
            {
                verbose=1;
                p_arg++;
                continue;
            }
             if (parseOption(argc,argv,p_arg,"-C","--calibrate",0,value)==1)
            {
                calibrate=1;
                p_arg++;
                continue;
            }
             if (parseOption(argc,argv,p_arg,"-D","--debug",0,value)==1)
            {
                debug=1;
                p_arg++;
                continue;
            }
            if ((res=parseOption(argc,argv,p_arg,"-d","--device",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    device_name = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-p","--datapath",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    data_path = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-t","--tic",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    if (std::find(valid_tics.begin(),valid_tics.end(),value)==valid_tics.end())
                    {
                        std::cerr << "Tic ID: " << value << " is invalid. Use -h to see the list "<< std::endl;
                        return 1;
                    }
                    tic_name = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-l","--location",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    if (std::find(valid_locations.begin(),valid_locations.end(),value)==valid_locations.end())
                    {
                        std::cerr << "Location ID: " << value << " is invalid. Use -h to see the list "<< std::endl;
                        return 1;
                    }
                    loc_name = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-a","--channel-a",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    channel_a = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-b","--channel-b",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    channel_b = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-u","--user",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    user_suffix = value;
                    p_arg++;
                    continue;
                }
            if ((res=parseOption(argc,argv,p_arg,"-o","--override",1,value))==-1)
                return 1;
                else
                if (res==2)
                {
                    override_name = value;
                    p_arg++;
                    continue;
                }

            p_arg++;
        }
        // valdate completness
        if (query_only) return 0;
        if (override_name.empty())
        {
            if (tic_name.empty() || 
                loc_name.empty() ||
                channel_a.empty() ||
                channel_b.empty())
            {
                std::cerr << "All of the required parameters must be set: TIC, Location, Channel A, Channel B" <<std::endl;
                return 1;
            }
            if (user_suffix.find_first_of("/ ")!=std::string::npos ||
                channel_a.find_first_of("/ ")!=std::string::npos ||
                channel_b.find_first_of("/ ")!=std::string::npos)
            {
                std::cerr << "You cannot use char '/' and spaces inside parameters! " << std::endl;
                return 1;
            }
        }
        else
        {
            if (override_name.find_first_of("/ ")!=std::string::npos)
            {
                std::cerr << "You cannot use char '/' and spaces inside parameters! " << std::endl;
                return 1;
            }
        }
        return 0;
    }

    void printHelp()
    {
        std::cout << "Output file naming convention: \n";
        std::cout << " YYYYMMDDTHHMMSS.<location>.<channel-A>-<channel B>.<tic id>[.user_suffix].dat \n";
        std::cout << "Usage parameters: \n";
        std::cout << "-d, --device - specify device to open, defaults to:  "<< SERIAL_DEVICE <<" \n";
        std::cout << "-p, --datapath - specify data path, deafults to: " << DATA_DIR << " \n";
        std::cout << "-h, --help - print this help \n";
        std::cout << "-t, --tic - specify valid tic name (from list): ";
        std::copy(valid_tics.begin(),valid_tics.end(),std::ostream_iterator<std::string>(std::cout,", "));
        std::cout << "\n";
        std::cout << "-a, --channel-a - source of the signal for channel a (free string) \n";
        std::cout << "-b, --channel-b - source of the signal for channel b (free string) \n";
        std::cout << "-l, --location - specify valid location (from list): ";
        std::copy(valid_locations.begin(),valid_locations.end(),std::ostream_iterator<std::string>(std::cout,", "));
        std::cout << "\n";
        std::cout << "-u, --user - user added suffix for measurement description (free string) \n";
        std::cout << "-o, --override - override naming convention with this string \n";
        std::cout << "-q, --query - query the tic \n";
        std::cout << "-r, --rotate - enable daily file rotation\n";
        std::cout << "-v, --verbose - print data to standard output too\n";
        std::cout << "-C, --calibrate - perform TIC auto-calibration at start\n";
        std::cout << "-D, --debug - enable extra debugging messages\n";
        std::cout << std::endl;
    }
    
};

int readLine(int fd, std::string &output)
{
    int iGetCount;
    char c;
    output.clear();
    while ( 1 ) {
        iGetCount = read( fd, &c, 1 );
        if ( iGetCount < 0 ) {
            perror( "[Error] read failed..." );
            return 1;
        }
        if ( ( c == '\n' ) || ( c == '\0' ) ) {
                tcflush( fd, TCIFLUSH );
                return 0;
        }
        output+=c;
    }
    return 1;
}

int writeLine(int fd,std::string str){
    if (write(fd,str.c_str(),str.size())!=(int)str.size()) return 1;
    return 0;
}

std::string timeToStr(struct timespec &tmNow)
{
    struct tm           *stTm;
    time_t              now;
    char                sISO8601[ 256 ];
    now=tmNow.tv_sec;
    stTm = gmtime( &now );
    strftime(sISO8601, sizeof(sISO8601),"%Y-%m-%dT%H:%M:%S", stTm);
    return std::string(sISO8601);
}

void sndStartCmds(int iSerialFD) {
/* initialize measurement */

    /* reset + clear screen */ 
    writeLine(iSerialFD,  "*RST;*CLS\n" );

    /* timebase = internal */
    writeLine(iSerialFD,  "CLCK0\n" );

    /* mode = time */
    writeLine(iSerialFD,  "MODE0\n" );

    /* arming mode = +time (not +/- time) */
    writeLine(iSerialFD,  "ARMM1\n" );

    /* display = mean */
    writeLine(iSerialFD,  "DISP0\n" );

    /* Set the measurement source...  [ 0:A , 1:B ] */
    writeLine(iSerialFD,  "SRCE0\n" );

    /* Set the number of samples... */
    writeLine(iSerialFD,  "SIZE1\n" );

    /* Set the trigger slope...  [ 0:positive , 1:negative ] */
    writeLine(iSerialFD,  "TSLP1,0;TSLP2,0\n" );

    /* Set the trigger level... */
    writeLine(iSerialFD,  "LEVL1,1.2;LEVL2,1.2\n" );

    /* Set the trigger ac/dc coupling...  [ 0:DC , 1:AC ] */
    writeLine(iSerialFD,  "TCPL1,0;TCPL2,0\n" );

    /* Set the trigger impedance...  [ 0:50ohm , 1:1meg ] */
    writeLine(iSerialFD,  "TERM1,0;TERM2,0\n" );

    /* local/remote = remote */
    writeLine(iSerialFD,  "LOCL1\n" );

    /* auto measure */
    writeLine(iSerialFD,  "AUTM1\n" );
}

void sd_hook(int signal);

int   iSerialFD        =  -1;

int main(int argc,char* argv[]) {
    sr620args           args;
    std::string         output;
    std::ofstream       outputFile;
    std::string         lastFilename;
    int                 iStatus;

    int cal_counter = 0; //counts number of calibrations
    int recal_minutes = 1000 *60 ;  //interval between calibrations (min)

    struct timespec     tmNow;
    struct timespec     startTime;

    struct  termio      stTermio;
    
    if (args.parse_args(argc,argv)) return 1;
    std::cout << "Starting using configuration: \n";
    std::cout << "Device            : " << args.device_name << "\n";
    std::cout << "Data path         : " << args.data_path << "\n";
    std::cout << "File name sample  : " << args.generateFileName(1) << "\n";
    std::cout << "Rotation          : " << (args.daily_rotate?"Enabled":"Disabled") << std::endl;
    std::cout << "Query mode        : " << (args.query_only?"YES":"No") << std::endl;
    
    signal( SIGIO, SIG_IGN);
    signal( SIGINT, sd_hook );
    signal( SIGTERM, sd_hook);


    iSerialFD = open( args.device_name.c_str(), O_RDWR );
    if ( iSerialFD < 0 ) {
        std::cerr << "[Error] open of " << args.device_name << " failed... :" << errno << std::endl;
        exit( 1 );
    }


    iStatus = ioctl( iSerialFD, TCGETA, &stTermio );
    if ( iStatus < 0 ) {
        perror( "[Error] ioctl (TCGETA) failed...");
        exit(1);
    }

    stTermio.c_cflag |=  B9600;
    stTermio.c_lflag &= ~ECHO;
    stTermio.c_lflag &= ~ICANON;

    stTermio.c_cc[  VMIN ] = 1;
    stTermio.c_cc[ VTIME ] = 0;

    iStatus = ioctl( iSerialFD, TCSETA, &stTermio );
    if ( iStatus < 0 ) {
        perror( "[Error] ioctl (TCSETA) failed..." );
        exit(1);
    } 

    // check if we are really talking to TIC......
    // not to reset all the equipment (like Cesium clock)
    writeLine(iSerialFD, "*IDN?\n" );
    
    if (readLine(iSerialFD,output))
        exit(1);
    
    std::transform(output.begin(),output.end(),output.begin(),::toupper);
    
    if (output.find("SR620")==std::string::npos){
        if (args.query_only)
            std::cout << "This is not a SR620 TIC!" << std::endl;
        else
            std::cerr << "[Error] This is not a SR620 TIC!" << std::endl;
        exit(1);
    }
    else
    {
        std::cout << "TIC presence verified OK." <<std::endl;
    }
    
    if (args.query_only)
        exit(0);

    sndStartCmds(iSerialFD) ; 

    /* use needRotate to prime this value */
    args.needRotate( & tmNow );
    memcpy(&startTime,&tmNow,sizeof(struct timespec));

    while ( 1 ) {

        /* check to see if elapsed time exceeds interval*calibrations done so far */
	if ( (tmNow.tv_sec - startTime.tv_sec) / 60 / recal_minutes  > cal_counter)
	{
		cal_counter++;
		std::cout << "Auto-calibration starting..." << std::endl;
                /*  DEBUG */
		//std::cout << "DEBUG:" << (tmNow.tv_sec - startTime.tv_sec) / 60  << std::endl;
                /*  stop current measurement */
	        writeLine( iSerialFD, "AUTM0;STOP;*CLS\n" );
                /*  DEBUG */
                //if ( cal_counter >= 2 ) { exit(1) ;}
                /*  do auto-calibration */
		writeLine( iSerialFD, "*CAL?\n");
		readLine(iSerialFD, output);
		std::cout << "Auto-calibration complete, result code: " << output << std::endl;
                /*  restart measurements */
                sndStartCmds(iSerialFD);
	}

	//std::cout << "DEBUG: Recalibration skipped, not time." << std::endl;
	//std::cout << "DEBUG:" << (tmNow.tv_sec - startTime.tv_sec) / 60  << std::endl;
        writeLine(iSerialFD,  "*WAI;XAVG?\n" );
        readLine(iSerialFD, output);
        
        if (args.needRotate(&tmNow)) {
            if ( outputFile.is_open() ) {
                outputFile.close();
                outputFile.clear();
            }
            lastFilename=args.generateFileName();
            std::cout << "Opening file: " << lastFilename << std::endl;
            outputFile.open(lastFilename.c_str(),std::ios::app);
            if (!outputFile.good())
            {
                std::cerr << "Could not open output file: '"<<lastFilename<<"'" << std::endl;
                outputFile.clear();
            }
        }
        
        std::ostringstream ofs;
        ofs << timeToStr(tmNow) << " " << std::fixed << std::setw(12) << std::setfill('0') << std::setprecision(12) << atof( output.c_str() ) << " " << (uintmax_t)tmNow.tv_sec << " " << tmNow.tv_nsec;
        outputFile << ofs.str() << std::endl;
        if (args.verbose)
            std::cout << ofs.str() << std::endl;
    }
}



void sd_hook(int signal)
{
    if ( iSerialFD != -1 ) {
	/* discontinue automeasure, STOP current measurement, clear, return control to front pannel */
        writeLine( iSerialFD, "AUTM0;STOP;*CLS;LOCL0\n" );
        close( iSerialFD );
    }
    
    exit( 0 );
}

