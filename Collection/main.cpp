/*
MIT License

Copyright (c) 2018 Andrew J. Bean

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>     /* system, NULL, EXIT_FAILURE, rand */
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <cmath>
#include <pwd.h>
#include <unistd.h>
#include <algorithm>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <sys/time.h>
#include <iomanip>

using namespace std;

int const RowSize = 6;
#define NumPause 10
#define NumAttempts 5


double get_wall_time()
{
    struct timeval time;
    if (gettimeofday(&time,NULL))
    {
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}


// compute number of days since jan 1 2014
int DaysSince2014_01_01(int Year,int Month,int Day)
{
    struct std::tm a = {0,0,0,Day,Month,Year-1900};
    struct std::tm b = {0,0,0,1,0,2014-1900};
    std::time_t x = std::mktime(&a);
    std::time_t y = std::mktime(&b);
	double difference;
    if ( x != (std::time_t)(-1) && y != (std::time_t)(-1) )
    {
        difference = std::difftime(y, x) / (60 * 60 * 24);
    }
    return ( int )difference;
}

// read the entire contents of the file into a std::string
// "Fail" if the file doesn't open
string StringFromFile(string FileName)
{
	streampos size;
	ifstream myfile (FileName.c_str(), ios::in|ios::binary|ios::ate);

	string FileString;
	if (myfile.is_open())
	{
		myfile.seekg (0, ios::end);
		size = myfile.tellg();
		vector<char> raw_data((int)size+1);
		raw_data[size] = char(0);
		myfile.seekg (0, ios::beg);
		myfile.read (&raw_data[0], size);
		myfile.close();
		FileString = &raw_data[0];
	}
	else
	{
		return string("Fail");
	}
	return FileString;
}

// get a string with the last N characters of the file
// "Fail" if the file doesn't open
// undefined if file has less than N chars
string LastNChars(string FileName, int N)
{
	streampos size = N;
	ifstream myfile (FileName.c_str(), ios::in|ios::binary|ios::ate);

	string FileString;
	if (myfile.is_open())
	{
		myfile.seekg (-N, ios::end);
		vector<char> raw_data((int)size+1);
		raw_data[size] = char(0);
		myfile.read (&raw_data[0], size);
		myfile.close();
		FileString = &raw_data[0];
	}
	else
	{
		return string("Fail");
	}
	return FileString;
}

// get the last N double values from the file
// if file has at least N doubles, returns the N doubles
// else, return a single value of -1
vector<double> LastNDoubles(string FileName, int N)
{
	// open the file as binary input
	ifstream myfile (FileName.c_str(), ios::in|ios::binary);

	streampos size = 0;
	// allocate space for the data
	vector<double> FileData(N);

	// if successful open, determine size
	if (myfile.is_open())
	{
		myfile.seekg (0, ios::end);
		size = myfile.tellg();
	}
	// only give the data if it's all there
	if(size >= N*sizeof(double))
	{
		myfile.seekg ( (streampos)( (long)size - N*sizeof(double) ) );
		myfile.read ( (char *)(&FileData[0]) , N*sizeof(double));
	}
	else
	{
		FileData.push_back(-1);
	}

	if (myfile.is_open()) myfile.close();
	return FileData;
}

// get the all double values from the file
vector<double> ReadDoubles(string FileName)
{
	// open the file as binary input
	ifstream myfile (FileName.c_str(), ios::in|ios::binary);

	streampos size = 0;
	// allocate space for the data
	vector<double> FileData;

	// if successful open, determine size
	if (myfile.is_open())
	{
		myfile.seekg (0, ios::end);
		size = myfile.tellg();
		FileData.resize(size / sizeof(double));
		myfile.seekg ( 0 );
		myfile.read ( (char *)(&FileData[0]) , size ) ;
		myfile.close();
	}

	return FileData;
}

void FileCatDoubles(string FileName , vector<double> const & Data , int NDrop = 0)
{
	// open the file as binary, with ability to seek around
	fstream myfile (FileName.c_str() , ios::binary | ios::out | ios::in);
	if (!myfile.is_open())
		myfile.open(FileName.c_str() , ios::binary | ios::out );

	// determine file size
	streampos size = 0;
	if (myfile.is_open())
	{
		myfile.seekg (0, ios::end);
		size = myfile.tellg();
	}
	else
	{
		cout << "FileCatDoubles: File not open.\n" ;
	}

	// if there are enough doubles in the file to drop, then do so
	if(size >= NDrop * sizeof(double))
	{
		myfile.seekg ( (streampos)( (long)size - NDrop * sizeof(double) ) );
	}

	// write the data to the file at the correct position (overwriting existing data potentially)
	myfile.write( (char *)(&Data[0]) , Data.size() * sizeof(double) ) ;
	if (myfile.is_open()) myfile.close();
}

void FileWriteDoubles(string FileName , vector<double> const & Data )
{
	// open the file as binary, with ability to seek around
	fstream myfile (FileName.c_str() , ios::binary | ios::out );
	myfile.seekg ( (streampos)( 0 ) );

	// write the data to the file at the correct position (overwriting existing data potentially)
	myfile.write( (char *)(&Data[0]) , Data.size() * sizeof(double) ) ;
	if (myfile.is_open()) myfile.close();
}

int FileExists(string FileName)
{
	fstream myfile (FileName.c_str() , ios::binary | ios::out | ios::in);
	return myfile.is_open();
}




/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



int main ()
{
	cerr << (char)7 ;
	double StartTime = get_wall_time();
	// Check if processor is available
	if (!system(NULL)) exit (EXIT_FAILURE);

	// determine the home directory
	passwd* pw = getpwuid(getuid());
	string home_path(pw->pw_dir);

	long size = pathconf(".", _PC_PATH_MAX);
	string PWD;
	PWD.resize(size);
	if(!getcwd(&PWD[0], (size_t)size))
	{
		cerr << "Error getting PWD" << endl;
		return -1;
	}

	string BasePath = string(&PWD[0]);
	// string BasePath = home_path + string("/stocks");

	cout << "BasePath = " << BasePath << endl;

	string Email("ajbean@illinois.edu");
	// return 0;


	// Get Date String
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (buffer,80,"%F",timeinfo);
	string DateString(buffer);

	// List of Stocks
	string arr[] =
	{
		// small selection:
		"AAPL","AMZN","BABA","BRK.B","FB","GOOG","GOOGL","INTC","MSFT","NFLX","NVDA","TSLA"

		// // Large Cap Stock Symbols (over $10B)
		// ".DJI",".INX",".IXIC","A","AAPL","ABB","ABBV","ABC","ABT","ACN","ADBE","ADI","ADM","ADP","ADS","ADSK",
		// "AEE","AEP","AES","AET","AFL","AIG","AIV","AIZ","AKAM","ALB","ALL","ALLE","ALXN","AMAT","AMD","AME","AMG",
		// "AMGN","AMP","AMT","AMZN","AN","AON","APA","APC","APD","APH","ATI","ATVI","AVB","AVGO","AVP","AVY",
		// "AXP","AZO","BA","BABA","BAC","BAX","BB","BBBY","BBD","BBT","BBVA","BBY","BCS","BDX","BEN","BF.B",
		// "BIIB","BK","BLK","BLL","BMS","BMY","BOX","BRK.B","BSMX","BSX","BUD","BWA","BXP","C","CA","CAG","CAH",
		// "CAT","CB","CBG","CBS","CCE","CCI","CCL","CELG","CEO","CERN","CF","CHK","CHRW","CHU","CI","CINF",
		// "CL","CLF","CLX","CMA","CMCSA","CME","CMG","CMI","CMS","CNP","CNX","COF","COG","COH","COL","COP","CORE","COST",
		// "CPB","CRM","CSCO","CSLT","CSX","CTAS","CTL","CTSH","CTXS","CVS","CVX","D","DAL","DDD","DE","DFS",
		// "DG","DGX","DHI","DHR","DIS","DISCA","DLPH","DLTR","DNB","DNKN","DNR","DO","DOV","DPS","DPZ","DRI","DTE",
		// "DUK","DVA","DVN","DWDP","EA","EBAY","ECL","ED","EFX","EIX","EL","EMN","EMR","EOG","EQR","EQT","ESRX","ESV","ETFC",
		// "ETN","ETR","EW","EXC","EXPD","EXPE","F","FAST","FB","FCX","FDX","FE","FFIV","FIS","FISV","FITB","FLIR","FLR",
		// "FLS","FMC","FOSL","FOXA","FSLR","FTI","FTR","GD","GE","GGP","GHC","GILD","GIS","GLW","GM",
		// "GME","GNW","GOOG","GOOGL","GPC","GPS","GRA","GRMN","GRPN","GS","GSK","GT","GWW","HAL","HAS","HBAN","HCN",
		// "HCP","HD","HES","HIG","HOG","HON","HP","HPQ","HRB","HRL","HRS","HSBC","HST","HSY","HUM","IBM","ICE","IFF",
		// "IGT","ILMN","INTC","INTU","IP","IPG","IR","IRM","ISRG","ITW","IVZ","JBL","JCI","JCP","JD","JEC","JNJ","JNPR","JPM",
		// "JWN","K","KEY","KHC","KIM","KLAC","KMB","KMX","KO","KORS","KR","KSS","KSU","L","LB","LEG","LEN","LH",
		// "LLL","LLY","LM","LMT","LNC","LOW","LRCX","LUK","LUV","LYB","M","MA","MAC","MAR","MAS","MAT",
		// "MCD","MCHP","MCK","MCO","MDLZ","MDT","MEET","MET","MHK","MKC","MLM","MMC","MMM","MNST","MO","MON","MOS","MPC",
		// "MRK","MRO","MS","MSFT","MSI","MTB","MU","MUR","MYL","NATI","NBL","NBR","NDAQ","NDXT","NEE","NEM","NFG","NFLX","NFX",
		// "NGG","NI","NKE","NLSN","NOC","NOV","NRG","NSC","NTAP","NTRS","NUE","NVDA","NVS","NWL","NWSA","OI","OII","OKE",
		// "OMC","ORCL","ORLY","OXY","P","PAYX","PBCT","PBI","PBPB","PCAR","PCG","PCLN","PDCO","PEG","PEP",
		// "PFE","PFG","PG","PGR","PH","PHM","PKI","PLD","PM","PNC","PNR","PNW","PPG","PPL","PRGO","PRU",
		// "PSA","PSX","PTR","PUK","PVH","PWR","PX","PXD","PZZA","QCOM","QEP","QQQ","R","RDC","REGN","RF","RHI","RHT","RIG","RL",
		// "ROK","ROP","ROST","RRC","RSG","RST","RTN","S","SBUX","SCG","SCHW","SEE","SHLD","SHW",
		// "SIG","SINA","SJM","SLAB","SLB","SNA","SNI","SO","SP500-10","SP500-10102020","SP500-15","SP500-20","SP500-25",
		// "SP500-30","SP500-35","SP500-40","SP500-45","SP500-50","SP500-55","SPG","SPGI","SPWR","SPY","SRCL","SRE","STI","STM","STT",
		// "STX","STZ","SWK","SWN","SYK","SYMC","SYY","T","TAP","TDC","TEL","TGT","THC","TIF","TJX","TM",
		// "TMK","TMO","TMUS","TOT","TRIP","TROW","TRV","TSCO","TSLA","TSN","TSS","TWTR","TWX","TXN","TXT",
		// "UA","UL","UNFI","UNH","UNM","UNP","UPS","URBN","USB","UTX","V","VAR","VFC","VIAB","VLO","VMC","VMW",
		// "VNO","VOO","VRSN","VRTX","VTR","VZ","WBA","WAT","WDC","WEC","WFC","WHR","WM","WMB","WMT","WRB","WU","WY",
		// "WYN","WYNN","X","XEC","XEL","XL","XLNX","XOM","XRAY","XRX","XYL","YELP","YUM","Z","ZION","ZNGA","ZTS",

		// // Mid Cap Stock Symbols (around $2B to $10B)
		// "AAN","ABMD","ACHC","ACIW","ATGE","ACM","ACXM","ACC",
		// "AEO","AFG","AGCO","AHL","AKRX","ALEX","Y",
		// "AMCX","WTR","ARW","ARRS","ASB","ASH","ATO",
		// "ATR","CAR","AVT","BC","BCO","BDC","BID",
		// "BIG","BIO","BIVV","BKH","BLKB","BOH","BR",
		// "BRO","BWLD","BXS","CAA","CABO","CAKE","CARS","CASY",
		// "CATY","CBSH","CBT","CC","CDK","CFR","CGNX","CHDN",
		// "CHFC","CIEN","CLB","CLGX","CLH","CLI","CMC","CMP",
		// "CNK","CNO","COHR","CONE","COR","CPE","CPRT","CPT",
		// "CR","CREE","CRI","CRL","CRS","CRUS","CSL",
		// "CTB","CTLT","CUZ","CVG","CVLT","CXW","CW","CBRL",
		// "CY","DAN","DBD","DCI","DCT","DDS","DECK",
		// "DEI","DF","DKS","DLX","DNOW",
		// "DRQ","DST","DY","EAT","EDR","EGN","EHC",
		// "EME","ENDP","ENR","ENS","EPC","EPR","ESL",
		// "EV","EWBC","EXP","FAF","FDS","FHN","FICO","FII",
		// "FLO","FR","FNB","FTNT","FULT","GATX","GEF",
		// "GEO","GGG","GMED","GNTX","GPOR",
		// "GVA","GWR","GXP","HAIN","HBHC","HE","HELE","HFC",
		// "HII","HIW","HNI","HOMB","HPT","HR","HRC","HUBB",
		// "HYH","IBKR","IBOC","IDA","IDCC","IDTI","IEX","ILG",
		// "INCR","INGR","INT","IPGP","ISCA","ITT","JACK","JBGS",
		// "JHG","JBLU","JCOM","JKHY","JLL","JW.A","KBH",
		// "KBR","KEX","KEYS","KLXI","KMPR","KMT","KN","KNX",
		// "KRC","LAMR","LANC","LDOS","LECO","LFUS","LHO","LII",
		// "LIVN","LNCE","LOGM","LPNT","LPT","LPX","LSI",
		// "LSTR","LW","LYV","MAN","MANH","MASI","MBFI","MCY",
		// "MD","MDP","MDRX","MDSO","MDU","MIK","MKSI","MKTX",
		// "MLHR","MMS","MNK","MOH","MPW","MPWR","MSA","MSCC",
		// "MSCI","MSM","MTDR","MTX","MUSA",
		// "NCR","NDSN","NEU","NJR","NNN","NTCT","NUS",
		// "NUVA","NVR","NWE","NYCB","NYT","OA","ODFL","ODP",
		// "OFC","OGE","OGS","OHI","OLN","OMI","ORI",
		// "OSK","OZRK","PACW","PBF","PAY","PB","PBH",
		// "PCH","PII","PLT","PNFP","PNM","POL","POOL","POST",
		// "PRI","PTC","PTEN","QCP","RBC",
		// "RGA","RGLD","RNR","ROL","RPM","RS","RYN",
		// "SABR","SAFM","SAIC","SAM","SBH","SBNY","SBRA","SCI",
		// "SEIC","SF","SFM","SIVB","SIX","SKT","SKX",
		// "SLGN","SLM","SM","SMG","SNH","SNV","SNX","SON","SPN",
		// "STE","STL","STLD","SWX","SXT","SYNA","TCF",
		// "TCBI","TCO","TDS","TDY","TECD","TECH","TER",
		// "TEX","TFX","TGNA","THG","THO","THS","TKR",
		// "TOL","TPH","TPX","TR","TRMB","TRMK","TRN","TTC",
		// "TTWO","TUP","TXRH","TYL","UBSI","UE","UFS","UGI",
		// "ULTI","UMBF","UMPQ","UTHR","UNIT",
		// "VLY","VMI","VVV","VVC","VSM","VSAT","VSH",
		// "WAB","WAFD","WBS","WCG","WEN","WERN","WEX","WGL",
		// "WOR","WPG","WPX","WR","WRI","WSM","WSO","WST","WTFC",
		// "WWD","ZBRA",

		// // Small Cap Stock Symbols (up to about $2B)
		// "AAT","AAV","AB","ABG","ABM","ABR","AC","ACCO","ADC","ADSW","ADX",
		// "AGM","AGRO","AGX","AHH","AHT","AIN","AIR","AIT","AJRD","AKR","AKS",
		// "ALDW","ALG","ALX","AMC","AMID","AMN","AMRC","ANF","ANH","AOD",
		// "APRN","APTS","AQUA","ARA","ARCH","ARCO","ARI","AROC","ARR","ASIX",
		// "ATEN","ATKR","ATTO","ATU","AVD","AVX","AWF","AWP","AWR","AXE",
		// "AXL","AYR","AYX","AZRE","AZUL","AZZ","BANC","BAS","BBG","BBN",
		// "BBX","BCC","BCEI","BDJ","BEDU","BEL","BETR","BFK","BFS","BFZ",
		// "BGC","BGG","BGR","BGS","BGY","BH","BHB","BHE","BHK","BHLB","BHVN",
		// "BIF","BITA","BKD","BKE","BLD","BLW","BLX","BMI","BOE","BOOT",
		// "BPMP","BPT","BRC","BRS","BRSS","BTE","BTO","BXG","BXMX","BY",
		// "BZH","CADE","CAF","CAI","CAL","CAPL","CBB","CBI","CBL","CBM",
		// "CBPX","CBU","CBZ","CCC","CCF","CCO","CCR","CCS","CDE","CDR","CEL",
		// "CEQP","CET","CHCT","CHGG","CHS","CHSP","CIG","CII","CINR","CIO",
		// "CIR","CISN","CIVI","CJ","CKH","CLDR","CLDT","CLM","CLS","CLW",
		// "CMCM","CMO","CMRE","CNNE","CNS","CNXM","CO","CODI","CORR","COT",
		// "CPAC","CPF","CPK","CPS","CRC","CRCM","CRY","CSTM","CSV","CTS",
		// "CTT","CUB","CUBI","CUDA","CURO","CVA","CVEO","CVNA","CVRR","CWT",
		// "CXP","CYD","CYH","CYS","CZZ","DEA","DEL","DESP","DFIN","DHT",
		// "DIN","DK","DKL","DLNG","DOC","DOOR","DPLO","DQ","DRH","DSM","DSU",
		// "DSW","DYN","EBF","EBS","ECR","EDD","EDN","EE","EEQ","EEX","EFC",
		// "EFR","EFT","EGHT","EGL","EGO","EGP","EHI","EHIC","EIG","EIM",
		// "ELF","ELP","ELY","EMD","ENV","ENVA","EOI","EOS","EPE","ERC","ERF",
		// "ERN","EROS","ESE","ESTE","ETG","ETH","ETJ","ETM","ETV",
		// "ETW","ETY","EVA","EVC","EVH","EVRI","EVT","EVTC","EVV","EXG",
		// "EXPR","EXTN","FAX","FBC","FBK","FBM","FBP","FCB","FCF",
		// "FCN","FCPT","FDP","FELP","FEN","FENG","FET","FF","FFC","FFG","FG",
		// "FI","FIT","FIX","FLOW","FMO","FMSA","FN","FOE","FOR","FPH",
		// "FRAC","FRO","FSB","FSD","FSIC","FSP","FSS","FTAI","FUL","GAB",
		// "GAM","GBL","GBX","GCI","GCO","GCP","GES","GFF","GGN","GHL","GIM",
		// "GKOS","GLO","GLOB","GLOG","GLOP","GLP","GLT","GMS","GNK","GNL",
		// "GNRT","GOL","GOLF","GPI","GPMT","GPRK","GPX","GRC","GSAT","GTE",
		// "GTN","GTS","GTT","GTY","GWB","HASI","HCC","HCLP","HEP","HESM",
		// "HF","HI","HIFR","HIO","HIX","HK","HL","HLX","HMLP","HMN","HMY",
		// "HOME","HPF","HPI","HPS","HQH","HQL","HRI","HRTG","HSC","HT","HTD",
		// "HTH","HTZ","HVT","HY","HYT","HZO","IAG","IBP","IFN","IGD",
		// "IGR","IIF","IIM","IMAX","INN","INST","INSW","IPHI","IPI","IPOA",
		// "IQI","IRET","IRS","IRT","ITG","ITGR","IVC","IVR","JAG","JFR",
		// "JKS","JMEI","JMF","JOE","JP","JPC","JPS","JQC","JRO","KAI","KAMN",
		// "KEM","KFY","KL","KMG","KND","KNL","KNOP","KOP","KOS","KRA",
		// "KREF","KRG","KS","KTF","KW","KWR","KYN","LADR","LBRT","LC","LCI",
		// "LDL","LEO","LKSD","LL","LNN","LPG","LPI","LQ","LRN","LTC","LTS",
		// "LXFT","LXP","LZB","MAG","MAIN","MATX","MBI","MC","MCA","MCRN",
		// "MCS","MDC","MDR","MED","MEI","MFA","MFL","MG","MHO","MIN","MITT",
		// "MLI","MMI","MMT","MMU","MNR","MOD","MODN","MOV","MPO","MPX","MQY",
		// "MRC","MSGN","MTH","MTL","MTOR","MTRN","MTW","MUC","MUI","MUX",
		// "MVF","MWA","MX","MXL","MYD","MYE","MYI","MYN","MYOV","NAD","NAK",
		// "NBHC","NBLX","NCI","NCS","NCV","NCZ","NE","NEP","NEWM","NEXA",
		// "NFJ","NG","NGD","NGL","NHC","NHI","NIE","NKX","NL","NMFC","NMZ",
		// "NNI","NOAH","NOMD","NP","NPK","NPO","NQP","NR","NRE","NRP","NSA",
		// "NSH","NSM","NSP","NSU","NTB","NTP","NUV","NVGS","NVRO","NWN","NX",
		// "NXJ","NXRT","NYRT","NZF","OAS","OBE","OCIP","OCN","OEC","OFG",
		// "OIS","OLP","OMAM","OMN","OMP","OXM","OZM","PARR","PBFX",
		// "PBT","PCN","PDM","PDS","PDT","PEB","PEI","PEO","PFN","PFS","PFSI",
		// "PGEM","PGH","PGTI","PHK","PJC","PJT","PLOW","PML","PMO","PMT",
		// "PPDF","PPR","PPT","PQG","PRA","PRK","PRLB","PRO","PRTY","PTY",
		// "PUMP","PZE","PZN","QTS","QTWO","QUAD","QUOT","RAD","REI","REN",
		// "RENN","RESI","REV","REVG","REX","REXR","RFP","RGR","RGS","RH",
		// "RLI","RMAX","RMP","RNP","RPAI","RPT","RQI","RRD","RTEC","RVT",
		// "RWT","RXN","RYAM","RYB","RYI","SA","SAH","SAIL","SALT","SAVE",
		// "SBGL","SBR","SCL","SCS","SD","SEAS","SEM","SEMG","SEND","SENS",
		// "SFL","SFS","SFUN","SGU","SGY","SHAK","SITE","SJI","SJT",
		// "SJW","SLCA","SLD","SMLP","SMP","SN","SNR","SOI","SPH",
		// "SPXC","SRCI","SRG","SRI","SRLP","SSD","SSP","SSTK","SSW","STAG",
		// "STAR","STC","STNG","STRP","SUP","SUPV","SVU","SWM","SXC","SXCP",
		// "SXI","SYX","TAC","TBI","TCAP","TDF","TDOC","TDW","TEI","TG","TGH",
		// "TGI","TGP","TGS","THR","TIER","TIME","TISI","TK","TLP","TLRD",
		// "TLYS","TMP","TMST","TNC","TNET","TNH","TOO","TOWR","TPC","TPRE",
		// "TRC","TRK","TRNO","TROX","TRTX","TSLX","TTI","TVPT","TWI","TWLO",
		// "TWO","TY","TYG","UBA","UFI","UHT","UIS","UMH","UNT","USA",
		// "USAC","USM","USNA","USPH","UTF","UTG","UTL","UVE","UVV","VCO",
		// "VCRA","VG","VGM","VGR","VHI","VKI","VKQ","VLRS","VMO","VNTR",
		// "VRS","VRTV","VSTO","VTA","VVI","VVR","WAGE","WAIR","WBAI","WD",
		// "WDR","WGO","WHG","WIW","WK","WLH","WLKP","WLL","WMK","WMS","WNC",
		// "WNS","WOW","WRD","WRE","WSR","WTI","WTS","WTTR","WWE","XHR","XIN",
		// "XON","XOXO","XXII","YEXT","YRD",

		// // from Russell 3000, what's left after the above are already included
		// "AA","AAC","AAL","AAOI","AAON","AAP","AAWW","AAXN","ABAX","ABCB","ABCD","ABEO","ABTX","ACAD","ACBI","ACET",
		// "ACGL","ACHN","ACIA","ACLS","ACNB","ACOR","ACRE","ACRS","ACTA","ACTG","ADES","ADMS","ADNT","ADRO","ADTN","ADUS",
		// "ADXS","AE","AEGN","AEIS","AEL","AERI","AFAM","AFH","AFI","AFSI","AGEN","AGFS","AGII","AGIO","AGN","AGNC","AGO",
		// "AGR","AGYS","AHP","AI","AIMC","AIMT","AJG","AJX","AKAO","AKBA","AKTS","AL","ALCO","ALDR","ALE","ALGN","ALGT",
		// "ALK","ALKS","ALLY","ALNY","ALOG","ALRM","ALSN","AMAG","AMBA","AMBC","AMBR","AMED","AMH","AMKR","AMNB","AMOT",
		// "AMPH","AMSF","AMSWA","AMTD","AMWD","ANAB","ANAT","ANCX","ANDE","ANET","ANGI","ANGO","ANIK","ANIP","ANSS",
		// "ANTM","AOBC","AOS","AOSL","AP","APAM","APEI","APLE","APOG","APPF","APTI","AQMS","AR","ARAY","ARC","ARCB","ARD",
		// "ARDX","ARE","AREX","ARII","ARMK","ARNA","ARNC","AROW","ARRY","ARTNA","ASC","ASCMA","ASGN","ASMB","ASNA",
		// "ASPS","AST","ASTE","AT","ATH","ATHN","ATHX","ATLO","ATNI","ATRA","ATRC","ATRI","ATRO","ATRS","ATSG","AVA",
		// "AVAV","AVHI","AVID","AVXL","AVXS","AWI","AWK","AXAS","AXDX","AXGN","AXON","AXS","AXTA","AXTI","AYI","AZPN",
		// "B","BABY","BAH","BANF","BANR","BATRA","BATRK","BBGI","BBSI","BBW","BCBP","BCOR","BCOV","BCPC","BCRH","BCRX",
		// "BDGE","BDN","BEAT","BECN","BELFB","BERY","BF.A","BFAM","BFIN","BG","BGCP","BGFV","BGSF","BHBK","BIOS",
		// "BJRI","BKMU","BKS","BKU","BL","BLBD","BLCM","BLDR","BLMN","BLMT","BLUE","BMCH","BMRC","BMRN","BMTC","BNCL",
		// "BNED","BNFT","BOCH","BOFI","BOJA","BOKF","BOLD","BOOM","BPFH","BPI","BPMC","BPOP","BREW","BRG","BRKL",
		// "BRKR","BRKS","BRX","BSET","BSF","BSFT","BSRR","BSTC","BTU","BTX","BUFF","BURL","BUSE","BV","BW","BWFG","BWINB",
		// "BWXT","BYD","CAC","CACC","CACI","CALA","CALD","CALM","CALX","CAMP","CARA","CARB","CARO","CASH",
		// "CASS","CATM","CATO","CAVM","CBOE","CCBG","CCK","CCMP","CCNE","CCOI","CCRN","CCXI","CDEV","CDNS",
		// "CDW","CDXS","CDZI","CE","CECE","CECO","CENT","CENTA","CENX","CERS","CETV","CEVA","CFFI","CFFN","CFG",
		// "CFMS","CFX","CHCO","CHD","CHE","CHEF","CHFN","CHH","CHMG","CHMI","CHRS","CHTR","CHUBA","CHUBK","CHUY","CIA",
		// "CIM","CIT","CIVB","CLCT","CLD","CLDX","CLFD","CLNE","CLNS","CLPR","CLR","CLSD","CLVS","CMCO","CMD","CMPR",
		// "CMRX","CMT","CMTL","CNA","CNAT","CNBKA","CNC","CNCE","CNDT","CNMD","CNOB","CNSL","CNTY","CNXN","COBZ","COGT","COHU",
		// "COKE","COLB","COLL","COLM","COMM","CONN","COO","CORI","CORT","COTV","COTY","COUP","COWN","CPA","CPLA","CPN","CPRX",
		// "CPSI","CRAI","CRAY","CRBP","CRD.B","CRIS","CRMT","CROX","CRR","CRVL","CRVS","CRZO","CSBK","CSFL","CSGP","CSGS",
		// "CSII","CSOD","CSRA","CSS","CSTE","CSTR","CSU","CSWI","CTBI","CTMX","CTO","CTRE","CTRL","CTRN","CTWS","CUBE",
		// "CUTR","CVBF","CVCO","CVCY","CVGI","CVGW","CVI","CVLY","CVRS","CVTI","CWCO","CWH","CWST","CXO","CYBE",
		// "CYTK","CZNC","CZR","DAKT","DAR","DATA","DCO","DCOM","DDR","DENN","DEPO","DERM","DFRG",
		// "DGICA","DGII","DHIL","DHX","DIOD","DISCK","DISH","DLA","DLB","DLR","DLTH","DMRC","DORM","DOX",
		// "DRE","DRRX","DS","DSKE","DSPG","DVAX","DVMT","DX","DXC","DXCM","DXPE","EARN","EBIX","EBSB","EBTC","ECHO","ECOL",
		// "ECOM","ECPG","EDGE","EDIT","EEFT","EFII","EFSC","EGBN","EGLE","EGOV","EGRX","EHTH","EIGI","ELGX","ELLI","ELS",
		// "ELVT","EMCI","EMKR","EML","ENFC","ENSG","ENT","ENTA","ENTG","ENTL","ENZ","EPAM","EPAY","EPM","EPZM","EQBK",
		// "EQC","EQIX","ERA","ERI","ERIE","ERII","ES","ESCA","ESGR","ESIO","ESND","ESNT","ESPR","ESRT","ESS","ESSA","ESXB",
		// "ETSY","EVBG","EVBN","EVHC","EVI","EVR","EXAC","EXAS","EXEL","EXLS","EXPO","EXR","EXTR","EXXI","EZPW","FANG",
		// "FARM","FARO","FATE","FBHS","FBIO","FBIZ","FBMS","FBNC","FBNK","FC","FCBC","FCE.A","FCFS","FCNCA","FDC",
		// "FDEF","FELE","FEYE","FFBC","FFIC","FFIN","FFKT","FFNW","FFWM","FGBI","FGEN","FHB","FIBK","FINL","FISI","FIVE",
		// "FIVN","FIZZ","FL","FLDM","FLIC","FLT","FLWS","FLXN","FLXS","FMAO","FMBH","FMBI","FMI","FMNB","FNBG","FND","FNF",
		// "FNGN","FNHC","FNLC","FNSR","FNWB","FOGO","FOLD","FONR","FORM","FORR","FOX","FOXF","FPI","FPRX","FRAN",
		// "FRBK","FRC","FRED","FRGI","FRME","FRPH","FRPT","FRT","FRTA","FSAM","FSTR","FTD","FTK","FTV","FWONA",
		// "FWONK","FWRD","G","GABC","GAIA","GBCI","GBLI","GBNK","GBT","GCAP","GDDY","GDEN","GDI","GDOT","GEF.B","GEN",
		// "GENC","GEOS","GERN","GHDX","GHM","GIFI","GIII","GLDD","GLNG","GLPI","GLRE","GLUU","GMRE","GNBC","GNC",
		// "GNCA","GNCMA","GNE","GNMK","GNRC","GNTY","GOGO","GOOD","GORO","GOV","GPK","GPN","GPRE","GPRO","GPT","GRBK",
		// "GRUB","GSBC","GSIT","GST","GTLS","GWRE","GWRS","H","HA","HABT","HAE","HAFC","HALL","HALO","HAWK",
		// "HAYN","HBCP","HBI","HBMD","HBNC","HBP","HCA","HCCI","HCHC","HCI","HCKT","HCOM","HCSG","HDNG","HDP","HDS","HDSN",
		// "HEES","HEI","HEI.A","HFWA","HGV","HHC","HIBB","HIIQ","HIL","HIVE","HLF","HLI","HLIT","HLNE","HLT",
		// "HMHC","HMST","HMSY","HMTV","HNRG","HOFT","HOLX","HONE","HOPE","HOV","HPE","HPP","HQY","HRG","HRTX","HSIC",
		// "HSII","HSKA","HSTM","HTA","HTBI","HTBK","HTLD","HTLF","HUBG","HUBS","HUN","HURC","HURN","HWKN","HXL","HZN",
		// "HZNP","I","IAC","IART","IBCP","IBKC","IBTX","ICBK","ICD","ICFI","ICHR","ICON","ICPT","ICUI","IDRA","IDT","IDXX",
		// "IESC","IHC","III","IIIN","IIVI","IMDZ","IMGN","IMH","IMKTA","IMMR","IMMU","IMPV","INAP","INBK","INCY","INDB","INFN",
		// "INFO","INGN","INO","INOV","INSE","INSM","INSY","INTL","INVA","INVH","INWK","IONS","IOSP","IPAR","IPCC","IPHS",
		// "IPXL","IRBT","IRDM","IRTC","IRWD","ISBC","ISTR","IT","ITCI","ITI","ITRI","IVAC","JAX","JBHT",
		// "JBSS","JBT","JCAP","JELD","JILL","JJSF","JNCE","JONE","JOUT","JRVR","JUNO","KALU","KAR","KBAL","KE",
		// "KEG","KELYA","KERX","KEYW","KFRC","KIN","KINS","KIRK","KLDX","KMI","KNSL","KODK","KOPN","KPTI","KRNY","KRO",
		// "KTOS","KTWO","KURA","KVHI","LABL","LAD","LAUR","LAYN","LAZ","LBAI","LBRDA","LBRDK","LBY","LCII",
		// "LCNB","LCUT","LE","LEA","LEN.B","LEXEA","LFGR","LGF.A","LGF.B","LGIH","LGND","LHCG","LIND","LION","LITE",
		// "LJPC","LKFN","LKQ","LLEX","LLNW","LMAT","LMNR","LMNX","LNDC","LNG","LNT","LNTH","LOB","LOCO","LOPE",
		// "LORL","LOXO","LPLA","LPSN","LQDT","LSCC","LSXMA","LSXMK","LTRPA","LTXB","LULU","LVNTA","LVS","LWAY","LXRX",
		// "LXU","LYTS","MAA","MACK","MANT","MATW","MB","MBCN","MBTF","MBUU","MBWM","MCBC","MCF","MCFT","MCRB","MCRI","MDCA",
		// "MDCO","MDGL","MDLY","MDXG","MEDP","METC","MFSF","MGEE","MGEN","MGI","MGLN","MGM","MGNX","MGPI","MGRC","MHLD","MIC",
		// "MIDD","MINI","MITK","MJCO","MKL","MLAB","MLP","MLR","MLVF","MMSI","MNOV","MNRO","MNTA","MOBL","MOFG","MOG.A",
		// "MORN","MPAA","MRCY","MRLN","MRT","MRTN","MRVL","MSBI","MSEX","MSFG","MSG","MSL","MSTR","MTCH","MTD","MTG","MTGE",
		// "MTN","MTNB","MTRX","MTSC","MTSI","MTZ","MULE","MVIS","MXIM","MXWL","MYGN","MYOK","MYRG","NANO","NAT",
		// "NATH","NATR","NAV","NAVG","NAVI","NBIX","NBN","NBTB","NC","NCBS","NCLH","NCMI","NCOM","NCSM","NDLS",
		// "NEO","NEOG","NEOS","NERV","NEWR","NFBK","NGHC","NGS","NGVC","NGVT","NH","NHTC","NK","NKSH","NKTR","NLNK",
		// "NLS","NLY","NM","NMIH","NNA","NNBR","NODK","NOVT","NOW","NPTN","NRCIA","NRIM","NRZ","NSIT","NSSC","NSTG",
		// "NTGR","NTLA","NTNX","NTRA","NTRI","NUAN","NVAX","NVCR","NVEC","NVEE","NVLN","NVTA","NWBI","NWFL","NWHM",
		// "NWLI","NWPX","NWS","NXEO","NXPI","NXST","NXTM","NYLD","NYLD.A","NYMT","NYMX","NYNY","O","OBLN","OC","OCFC",
		// "OCLR","OCUL","OCX","ODC","OFIX","OFLX","OKTA","OLBK","OLED","OLLI","OMCL","OMER","OMF","OMNT",
		// "ON","ONB","ONCE","ONDK","ONVO","OOMA","OPB","OPK","OPOF","OPY","ORA","ORBC","ORC","ORIT","ORM","ORN","ORRF","OSBC",
		// "OSG","OSIS","OSTK","OSUR","OTIC","OTTR","OUT","OVBC","OVID","OXFD","PACB","PAG","PAH","PAHC","PANW","PATK","PAYC",
		// "PBIP","PBYI","PCMI","PCRX","PCSB","PCTY","PCYG","PCYO","PDCE","PDFS","PDLI","PDVW","PE","PEBK",
		// "PEBO","PEGA","PEGI","PEIX","PEN","PENN","PERY","PES","PETS","PETX","PF","PFBC","PFBI","PFGC","PFIS","PFPT","PGC",
		// "PGNX","PGRE","PHH","PHIIK","PHX","PI","PICO","PINC","PIR","PIRS","PK","PKBK","PKD","PKE","PKG","PKOH","PLAB",
		// "PLAY","PLCE","PLNT","PLPC","PLSE","PLUG","PLUS","PLXS","PMBC","PMTS","PNK","PODD","POR","POWI",
		// "POWL","PPBI","PPC","PRAA","PRAH","PRFT","PRGS","PRIM","PRMW","PROV","PRSC","PRTA","PRTK","PSB","PSDO","PSMT",
		// "PSTG","PTCT","PTGX","PTLA","PUB","PVAC","PWOD","PXLW","PYPL","Q","QADA","QCRH","QDEL","QGEN",
		// "QLYS","QNST","QRVO","QSII","QTM","QTNA","QTNT","QVCA","RAIL","RARE","RARX","RAS","RAVN","RBCAA","RCII",
		// "RCL","RCM","RDI","RDN","RDNT","RDUS","RE","RECN","REG","REGI","REIS","REPH","RES","RETA","RGC","RGCO","RGEN","RGNX",
		// "RHP","RICK","RIGL","RILY","RJF","RLGT","RLGY","RLH","RLJ","RM","RMBS","RMD","RMR","RMTI","RNET","RNG","RNST",
		// "RNWK","ROCK","ROG","ROIC","ROLL","ROSE","ROX","RP","RPD","RPXC","RRGB","RRTS","RSO","RSPP","RSYS","RTIX",
		// "RTRX","RUBI","RUN","RUSHA","RUSHB","RUTH","RVLT","RVNC","RVSB","RXDX","SAFT","SAGE","SAIA","SALM","SAMG","SANM",
		// "SASR","SATS","SB","SBAC","SBBP","SBCF","SBGI","SBOW","SBSI","SC","SCCO","SCHL","SCHN","SCMP","SCSC",
		// "SCVL","SCWX","SEB","SELB","SENEA","SERV","SFBS","SFE","SFLY","SFNC","SFST","SGA","SGC","SGEN",
		// "SGMO","SGMS","SGRY","SGYP","SHBI","SHEN","SHLM","SHLO","SHO","SHOO","SIEN","SIFI","SIGI","SIGM","SIR","SIRI",
		// "SKYW","SLG","SLP","SMBC","SMBK","SMCI","SMHI","SMMF","SMTC","SNBC","SNCR","SND","SNDR","SNDX","SNHY",
		// "SNPS","SONA","SONC","SP","SPA","SPAR","SPB","SPKE","SPLK","SPOK","SPPI","SPR","SPSC","SPTN",
		// "SPWH","SQ","SQBG","SR","SRC","SRCE","SRDX","SREV","SRPT","SRT","SSB","SSNC","SSYS","ST","STAA","STAY","STBA",
		// "STBZ","STFC","STML","STMP","STOR","STRA","STRL","STRS","STWD","SUI","SUM","SUPN","SWKS","SYBT","SYF",
		// "SYKE","SYNT","SYRS","TACO","TAHO","TAST","TAX","TBBK","TBK","TBNK","TBPH","TCBK","TCFC","TCI","TCMD","TCS","TCX",
		// "TDG","TEAM","TELL","TEN","TERP","TFSL","TGTX","THFF","THRM","TILE","TIPT","TIS","TITN","TIVO","TLGT","TMHC",
		// "TNAV","TNK","TOCA","TOWN","TPB","TPHS","TPIC","TRCB","TRCO","TREC","TREE","TREX","TRGP","TRHC","TRNC","TRS","TRST",
		// "TRTN","TRU","TRUE","TRUP","TRVN","TSBK","TSC","TSE","TSQ","TSRO","TTD","TTEC","TTEK","TTGT","TTMI","TTPH",
		// "TTS","TUSK","TVTY","TWIN","TWNK","TWOU","TXMD","TYPE","UAA","UAL","UBFO","UBNK","UBNT","UBSH","UCBI","UCFC",
		// "UCTT","UDR","UEC","UEIC","UFCS","UFPI","UFPT","UHAL","UHS","UIHC","ULH","ULTA","UNF","UNTY","UNVR","UPL",
		// "UPLD","URI","USAT","USCR","USFD","USG","USLM","UTMD","UVSP","VAC","VBIV","VBTX","VC","VCYT","VDSI","VEC",
		// "VECO","VEEV","VER","VERI","VHC","VIA","VIAV","VICR","VIRT","VIVE","VIVO","VLGEA","VNDA","VOXX","VOYA","VPG",
		// "VR","VRA","VRAY","VREX","VRNS","VRNT","VRSK","VRTS","VRTU","VSAR","VSEC","VSI","VSLR","VST","VTVT","VYGR","W",
		// "WAAS","WABC","WAL","WASH","WATT","WBC","WBT","WCC","WDAY","WDFC","WEB","WETF","WEYS","WFT","WG",
		// "WIFI","WIN","WINA","WING","WINS","WIRE","WLB","WLDN","WLFC","WLK","WLTW","WMC","WMGI","WMIH","WNEB",
		// "WPC","WRK","WRLD","WSBC","WSBF","WSFS","WTBA","WTM","WTW","WVE","WWW","XBIT","XCRA","XENT","XLRN",
		// "XNCR","XOG","XONE","XPER","XPO","YORW","YRCW","YUMC","ZAGG","ZAYO","ZBH","ZEN","ZEUS","ZG","ZGNX","ZIOP",
		// "ZIXI","ZOES","ZUMZ","ZYNE",
	};


	vector<string> stock_arg (arr, arr + sizeof(arr) / sizeof(arr[0]) );
	std::sort(stock_arg.begin(),stock_arg.end());

	cout << "\n\nNumber of stocks: " << stock_arg.size() << endl;
	cout << "DateString = " << DateString << endl;

	// Archive the existing composite files, just in case
	string FullCommand;
	FullCommand = string("/usr/bin/zip -r ");
	FullCommand = FullCommand + BasePath;
	FullCommand = FullCommand + string("/archive/archive_");
	FullCommand = FullCommand + DateString;
	FullCommand = FullCommand + string(".zip ");
	FullCommand = FullCommand + BasePath;
	FullCommand = FullCommand + string("/composite");
	// cout << " Command to Zip is:" << endl << FullCommand.c_str() << endl;
	system (FullCommand.c_str());

	// Loop through each stock, gathering data from google
	int WhichStock = 0;
	int Attempts = 0;
	while( WhichStock < stock_arg.size() )
	{
		#if NumPause
		if(rand()%5 == 0)
		{
			cout << "Pausing" ;
			for(int i=0;i<NumPause;i++)
			{
				cout << "." << flush;
				this_thread::sleep_for (chrono::milliseconds(500));
			}
			cout << "." << endl;
		}
		#endif
		// Save the downloaded HTML file from google, using wget
		// specify full path
		string FileName = BasePath + "/daily/" + DateString + "_" + stock_arg[WhichStock] + ".txt";

		// proceed with download

		/////////////////////////////////////////////////////////////////
		// typically Interval should be 60
		int const Interval = 60;

		if(WhichStock==0) cout << "Seconds Per Interval = " << Interval << endl;
		string INTERVAL = to_string(Interval);
		FullCommand = "wget -O " + FileName + " --timeout=30 'https://finance.google.com/finance/getprices?p=150d&i="+INTERVAL+"&f=d,o,h,l,c,v&q=" + stock_arg[WhichStock] + "'";
		FullCommand = "/usr/local/bin/" + FullCommand;
		FullCommand = FullCommand + " > /dev/null 2>&1" ;

		// execute the wget command
		{
			int result = system (FullCommand.c_str());
			if (result != 0)
			{
				Attempts++;
				cout << "non-zero wget result" << endl ;
				cout << "result = " << result << endl;
				if(Attempts>NumAttempts)
				{
					cout << "exiting now" << endl ;
					exit (EXIT_FAILURE);
				}
				else
				{
					cout << "WGET Attempt #" << Attempts << " failed." << endl;
					continue;
				}
			}
		}

		// retrieve the full HTML as a string
		string HTML_string;
		HTML_string = StringFromFile(FileName);

		// in case of error...
		// notify and possibly try again
		if (HTML_string.compare("Fail") == 0)
		{
			Attempts++;
			cout << "File Not Open Error" << endl ;
			if(Attempts>NumAttempts)
			{
				cout << "exiting now" << endl ;
				exit (EXIT_FAILURE);
			}
			else
			{
				cout << "File Open Attempt #" << Attempts << " failed." << endl;
				continue;
			}
		}


		// Trim the HTML down to just the 5 days intraday prices data
		size_t Position = HTML_string.find("TIMEZONE_OFFSET");
		if(Position == string::npos)
		{
			Attempts++;
			if(Attempts>NumAttempts)
			{
				Attempts=0;
				cout << "moving on now" << endl ;
				WhichStock++;
				continue;
			}
			else
			{
				cout << "TIMEZONE_OFFSET Attempt #" << Attempts << " failed.  --  " << stock_arg[WhichStock] << endl;
				continue;
			}
		}
		Position = HTML_string.find("a",Position);

		// this is just the chart data
		string Extracted = HTML_string.substr(Position);

		// may include extra TIMEZONE_OFFSET, delete these
		// have format TIMEZONE_OFFSET=[number] newline a#####
		// will leave alone the a####
		Position = Extracted.find("TIMEZONE_OFFSET");
		if(Position != string::npos)
		{
			size_t oldPosition = Position;
			Position = Extracted.find("a",Position+1);
			// delete from oldPosition to Position-1
			Extracted.erase( oldPosition , Position - oldPosition );
		}

		// Correct the time index indices that start with a#####
		Position = 0;
		Position = Extracted.find("a",Position);
		while(Position != string::npos)
		{
			Extracted.replace(Position,1,"0");
			Position = Extracted.find("a",Position+1);
		}

		// Replace all commas with spaces in preparation for stringstream input
		Position = 0;
		Position = Extracted.find(",",Position);
		while(Position != string::npos)
		{
			Extracted.replace(Position,1," ");
			Position = Extracted.find(",",Position+1);
		}
		if(Extracted.find("-") != string::npos)
		{
			cout << "Error: found a negative sign that shouldn't be there." << endl;
			Attempts++;
			if(Attempts>NumAttempts)
			{
				cout << "exiting now." << endl ;
				exit (EXIT_FAILURE);
			}
			else
			{
				cout << "found a bad negative sign, Attempt #" << Attempts << " failed." << endl;
				continue;
			}
		}
		// insert a negative to indicate end of data
		Extracted = Extracted + " -1.0";

		// extract all the data in the string to doubles, discard the additional -1 at the end
		vector<double> DataValues;
		stringstream ReadExtract(Extracted);
		do
		{
			double temp;
			ReadExtract >> temp;
			DataValues.push_back(temp);
		}while (DataValues.back() >=0);
		DataValues.pop_back();

		// Correct the intraday time indices
		double BaseVal = DataValues[0];
		double LastVal = DataValues[0];
		for(int i=0;i<DataValues.size();i+=RowSize)
		{
			if( DataValues[i] > 1000000 )
			{
				BaseVal = DataValues[i];
			}
			else
			{
				DataValues[i] = Interval*DataValues[i]+BaseVal;
			}
		}

		string CompositeFile = BasePath + "/composite/" + stock_arg[WhichStock] + ".dat" ;
		if(stock_arg[WhichStock].at(0) == '.')
			CompositeFile = BasePath + "/composite/" + stock_arg[WhichStock].substr(1,string::npos) + ".dat" ;

		// Figure out what to append to the composite files.
		vector<double> ExistingValues = ReadDoubles( CompositeFile );
		if(ExistingValues.size()%6 != 0)
		{
			cout << "wrong length of ExistingValues." << endl;
			return 1;
		}

		// values from file in ExistingValues
		// DiscardValues is empty now
		// full downloaded data in DataValues
		vector<double> DiscardValues ;
		int BeginWrite = 0;
		// keep all samples with time value strictly less than start of downloaded data
		while(BeginWrite < ExistingValues.size() && ExistingValues[BeginWrite] < DataValues[0]) BeginWrite += RowSize;
		int SizeOld = ExistingValues.size();
		// end segment of ExistingValues in DiscardValues
		// keep the only the rest (beginning part) in ExistingValues
		DiscardValues.insert( DiscardValues.end() , ExistingValues.begin() + BeginWrite , ExistingValues.end()) ;
		ExistingValues.erase( ExistingValues.begin() + BeginWrite , ExistingValues.end()) ;

		// Check DataValues against DiscardValues to detect stock split adjustments
		if( DiscardValues.size()>0 && ( DiscardValues[1]/DataValues[1] > 1.1 || DiscardValues[1]/DataValues[1] < 0.9 ) )
		{
			cerr << (char)7 ;
			cout << "DataValues[1] = " << DataValues[1] << "     DiscardValues[1] = " << DiscardValues[1] << "     Ratio = " << DiscardValues[1]/DataValues[1] << endl;
			cout << "stock split? (y/n)" << endl;
			fd_set fdset;
			struct timeval timeout;
			int  rc;
			int  val;

			timeout.tv_sec = 20;   /* wait for 20 seconds for data */
			timeout.tv_usec = 0;


			FD_ZERO(&fdset);

			FD_SET(0, &fdset);

			rc = select(1, &fdset, NULL, NULL, &timeout);
			if (rc == -1)  /* select failed */
			{
				cout << "ERROR path\n" ;
				val='E';
			}
			else if (rc == 0)  /* select timed out */
			{
				string ErrString = "echo \"Possible Stock Split, Manual Intervention Necessary, Ratio = "
					+ to_string(DiscardValues[1]/DataValues[1])
					+ "\" | mail -s \"" + stock_arg[WhichStock]
					+ " Possible Stock Split, Manual Intervention Necessary\" " + Email;
				system(ErrString.c_str());
				cout << "No reply, leaving the data untouched" << endl ;
				Attempts = 0;
				WhichStock++;
				continue;
			}
			else
			{
				if (FD_ISSET(0, &fdset))
				{
					val = getchar();
				}
				else
				{
					cout << "Some input error?" << endl ;
					Attempts++;
					if(Attempts>NumAttempts)
					{
						cout << "exiting now." << endl ;
						exit (EXIT_FAILURE);
					}
					else
					{
						cout << "Attempt #" << Attempts << " failed." << endl;
						continue;
					}
				}
				if(val == 'y' || val == 'Y')
				{
					cout << "New shares per old share =? " << endl ;
					double SplitCorrection;
					cin >> SplitCorrection;
					for(int i=0;i<ExistingValues.size();i += RowSize)
					{
						ExistingValues[i+1] /= SplitCorrection;
						ExistingValues[i+2] /= SplitCorrection;
						ExistingValues[i+3] /= SplitCorrection;
						ExistingValues[i+4] /= SplitCorrection;
						ExistingValues[i+5] *= SplitCorrection;
					}
				}
				else
				{
					string ErrString = string("echo \"Possible Stock Split, Manual ")
						+ "Intervention Necessary, Ratio = "
						+ to_string(DiscardValues[1]/DataValues[1])
						+ "\" | mail -s \"" + stock_arg[WhichStock]
						+ " Possible Stock Split, Manual Intervention Necessary\" " + Email;
					system(ErrString.c_str());
					cout << "Leaving the data untouched" << endl ;
					Attempts = 0;
					WhichStock++;
					continue;
				}
			}
		}

		// insert the new data
		ExistingValues.insert( ExistingValues.end() , DataValues.begin() , DataValues.end() );
		int SizeNew = ExistingValues.size();


		if(ExistingValues.size()%6 != 0)
		{
			cout << "data found not divisible by 6.\n";
		}
		else if((SizeNew - SizeOld)/6 < -50)
		{
			cout << setw(4) << WhichStock+1 << " - " << setw(4) << ((SizeNew - SizeOld)/6) << " - " << setw(14) << stock_arg[WhichStock] << "  not deleting data." << endl ;
		}
		else
		{
			cout << setw(4) << WhichStock+1 << " - " << setw(4) << ((SizeNew - SizeOld)/6) << " - " << setw(14) << stock_arg[WhichStock] << endl ;
			if((SizeNew - SizeOld)/6 == 0)
				cout << "--------------------------------------------------------------------" << endl;
			FileWriteDoubles( CompositeFile , ExistingValues );
		}

		Attempts = 0;
		WhichStock++;
	}

	cout << "total time: " << (get_wall_time() - StartTime)/60.0 << " min." << endl;

	return 0;
}


