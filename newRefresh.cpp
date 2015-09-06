// Compile this to make the refresh.cgi script that executes when the user clicks "Refresh Network"
// This appends to lines.txt and spectra.txt as needed (not at all if there are no new files),
// redoes to the repoire.json file, and redirects the web page


#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <set>
#include <vector>
#include <memory>
#include <fstream>
#include <string.h>
#include <math.h>
#include <cstring>
#include <map>
#include <algorithm>
#include <ctime>
#include "dirent.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


using namespace std;

const int FREQUENCY_BINS = 81;    // binning of the spectrum
const int KEY_RANGE = 32000000;   // maximum expected key value

const double MIN_OVERLAP = 41.5;   // percentage that sets must intersect in order to record a line 

const double SIZE_TOLERANCE = 10.0;  // factor within which sizes must agree to try connection
const double RGB_TOLERANCE = 2.0;   // factor within which RGB values must agree to try connection
const bool RGB_MATCH_ON = false;  // set true to use RGB as matching criterion
const bool SIZE_MATCH_ON = true; // set true to use size as matching criterion

const char* DATADIR = "/home/nkersting/quantumrepoire.com/public/";
const char* DATADIR_SPECTRUM = "/home/nkersting/quantumrepoire.com/spectra.txt";
const char* DATADIR_LINES = "/home/nkersting/quantumrepoire.com/lines.txt";
const char* REPOIRE_DATA = "/home/nkersting/quantumrepoire.com/data/repoire.json";

//////////////////////////////////////////////

/* A colour system is defined by the CIE x and y coordinates of
   its three primary illuminants and the x and y coordinates of
   the white point. */

struct colourSystem {
    string name;     	    	    /* Colour system name */
    double xRed, yRed,	    	    /* Red x, y */
           xGreen, yGreen,  	    /* Green x, y */
           xBlue, yBlue,    	    /* Blue x, y */
           xWhite, yWhite,  	    /* White point x, y */
	   gamma;   	    	    /* Gamma correction for system */
};

/* White point chromaticities. */

#define IlluminantC     0.3101, 0.3162	    	/* For NTSC television */
#define IlluminantD65   0.3127, 0.3291	    	/* For EBU and SMPTE */
#define IlluminantE 	0.33333333, 0.33333333  /* CIE equal-energy illuminant */

/*  Gamma of nonlinear correction.

    See Charles Poynton's ColorFAQ Item 45 and GammaFAQ Item 6 at:
    
       http://www.poynton.com/ColorFAQ.html
       http://www.poynton.com/GammaFAQ.html
 
*/

#define GAMMA_REC709	0		/* Rec. 709 */

static struct colourSystem
                  /* Name                  xRed    yRed    xGreen  yGreen  xBlue  yBlue    White point        Gamma   */
    NTSCsystem  =  { "NTSC",               0.67,   0.33,   0.21,   0.71,   0.14,   0.08,   IlluminantC,    GAMMA_REC709 },
    EBUsystem   =  { "EBU (PAL/SECAM)",    0.64,   0.33,   0.29,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 },
    SMPTEsystem =  { "SMPTE",              0.630,  0.340,  0.310,  0.595,  0.155,  0.070,  IlluminantD65,  GAMMA_REC709 },
    HDTVsystem  =  { "HDTV",               0.670,  0.330,  0.210,  0.710,  0.150,  0.060,  IlluminantD65,  GAMMA_REC709 },
    CIEsystem   =  { "CIE",                0.7355, 0.2645, 0.2658, 0.7243, 0.1669, 0.0085, IlluminantE,    GAMMA_REC709 },
    Rec709system = { "CIE REC 709",        0.64,   0.33,   0.30,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 };


//////////////////////////////////
void SpectrumToXYZ(int spectrum[],
                     double& x, double& y, double& z)
{
    int i;
    double lambda, X = 0, Y = 0, Z = 0, XYZ;

    /* CIE colour matching functions xBar, yBar, and zBar for
       wavelengths from 380 through 780 nanometers, every 5
       nanometers.  For a wavelength lambda in this range:

            cie_colour_match[(lambda - 380) / 5][0] = xBar
            cie_colour_match[(lambda - 380) / 5][1] = yBar
            cie_colour_match[(lambda - 380) / 5][2] = zBar

	To save memory, this table can be declared as floats
	rather than doubles; (IEEE) float has enough 
	significant bits to represent the values. It's declared
	as a double here to avoid warnings about "conversion
	between floating-point types" from certain persnickety
	compilers. */

    static double cie_colour_match[81][3] = {
        {0.0014,0.0000,0.0065}, {0.0022,0.0001,0.0105}, {0.0042,0.0001,0.0201},
        {0.0076,0.0002,0.0362}, {0.0143,0.0004,0.0679}, {0.0232,0.0006,0.1102},
        {0.0435,0.0012,0.2074}, {0.0776,0.0022,0.3713}, {0.1344,0.0040,0.6456},
        {0.2148,0.0073,1.0391}, {0.2839,0.0116,1.3856}, {0.3285,0.0168,1.6230},
        {0.3483,0.0230,1.7471}, {0.3481,0.0298,1.7826}, {0.3362,0.0380,1.7721},
        {0.3187,0.0480,1.7441}, {0.2908,0.0600,1.6692}, {0.2511,0.0739,1.5281},
        {0.1954,0.0910,1.2876}, {0.1421,0.1126,1.0419}, {0.0956,0.1390,0.8130},
        {0.0580,0.1693,0.6162}, {0.0320,0.2080,0.4652}, {0.0147,0.2586,0.3533},
        {0.0049,0.3230,0.2720}, {0.0024,0.4073,0.2123}, {0.0093,0.5030,0.1582},
        {0.0291,0.6082,0.1117}, {0.0633,0.7100,0.0782}, {0.1096,0.7932,0.0573},
        {0.1655,0.8620,0.0422}, {0.2257,0.9149,0.0298}, {0.2904,0.9540,0.0203},
        {0.3597,0.9803,0.0134}, {0.4334,0.9950,0.0087}, {0.5121,1.0000,0.0057},
        {0.5945,0.9950,0.0039}, {0.6784,0.9786,0.0027}, {0.7621,0.9520,0.0021},
        {0.8425,0.9154,0.0018}, {0.9163,0.8700,0.0017}, {0.9786,0.8163,0.0014},
        {1.0263,0.7570,0.0011}, {1.0567,0.6949,0.0010}, {1.0622,0.6310,0.0008},
        {1.0456,0.5668,0.0006}, {1.0026,0.5030,0.0003}, {0.9384,0.4412,0.0002},
        {0.8544,0.3810,0.0002}, {0.7514,0.3210,0.0001}, {0.6424,0.2650,0.0000},
        {0.5419,0.2170,0.0000}, {0.4479,0.1750,0.0000}, {0.3608,0.1382,0.0000},
        {0.2835,0.1070,0.0000}, {0.2187,0.0816,0.0000}, {0.1649,0.0610,0.0000},
        {0.1212,0.0446,0.0000}, {0.0874,0.0320,0.0000}, {0.0636,0.0232,0.0000},
        {0.0468,0.0170,0.0000}, {0.0329,0.0119,0.0000}, {0.0227,0.0082,0.0000},
        {0.0158,0.0057,0.0000}, {0.0114,0.0041,0.0000}, {0.0081,0.0029,0.0000},
        {0.0058,0.0021,0.0000}, {0.0041,0.0015,0.0000}, {0.0029,0.0010,0.0000},
        {0.0020,0.0007,0.0000}, {0.0014,0.0005,0.0000}, {0.0010,0.0004,0.0000},
        {0.0007,0.0002,0.0000}, {0.0005,0.0002,0.0000}, {0.0003,0.0001,0.0000},
        {0.0002,0.0001,0.0000}, {0.0002,0.0001,0.0000}, {0.0001,0.0000,0.0000},
        {0.0001,0.0000,0.0000}, {0.0001,0.0000,0.0000}, {0.0000,0.0000,0.0000}
    };

    for (i = 0, lambda = 380; lambda < 780.1; i++, lambda += 5)
    {
        X += spectrum[i] * cie_colour_match[i][0];
        Y += spectrum[i] * cie_colour_match[i][1];
        Z += spectrum[i] * cie_colour_match[i][2];
    }
    XYZ = (X + Y + Z);
    x = X / XYZ;
    y = Y / XYZ;
    z = Z / XYZ;
}

///////////////////////////////////////////

/*                             XYZ_TO_RGB

    Given an additive tricolour system CS, defined by the CIE x
    and y chromaticities of its three primaries (z is derived
    trivially as 1-(x+y)), and a desired chromaticity (XC, YC,
    ZC) in CIE space, determine the contribution of each
    primary in a linear combination which sums to the desired
    chromaticity.  If the  requested chromaticity falls outside
    the Maxwell  triangle (colour gamut) formed by the three
    primaries, one of the r, g, or b weights will be negative. 

    Caller can use Constrain_rgb() to desaturate an
    outside-gamut colour to the closest representation within
    the available gamut and/or Norm_rgb to normalise the RGB
    components so the largest nonzero component has value 1.
    
*/

void XYZ_to_rgb(struct colourSystem *cs,
                double xc, double yc, double zc,
                double *r, double *g, double *b)
{
    double xr, yr, zr, xg, yg, zg, xb, yb, zb;
    double xw, yw, zw;
    double rx, ry, rz, gx, gy, gz, bx, by, bz;
    double rw, gw, bw;

    xr = cs->xRed;    yr = cs->yRed;    zr = 1 - (xr + yr);
    xg = cs->xGreen;  yg = cs->yGreen;  zg = 1 - (xg + yg);
    xb = cs->xBlue;   yb = cs->yBlue;   zb = 1 - (xb + yb);

    xw = cs->xWhite;  yw = cs->yWhite;  zw = 1 - (xw + yw);

    /* xyz -> rgb matrix, before scaling to white. */
    
    rx = (yg * zb) - (yb * zg);  ry = (xb * zg) - (xg * zb);  rz = (xg * yb) - (xb * yg);
    gx = (yb * zr) - (yr * zb);  gy = (xr * zb) - (xb * zr);  gz = (xb * yr) - (xr * yb);
    bx = (yr * zg) - (yg * zr);  by = (xg * zr) - (xr * zg);  bz = (xr * yg) - (xg * yr);

    /* White scaling factors.
       Dividing by yw scales the white luminance to unity, as conventional. */
       
    rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
    gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
    bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

    /* xyz -> rgb matrix, correctly scaled to white. */
    
    rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
    gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
    bx = bx / bw;  by = by / bw;  bz = bz / bw;

    /* rgb of the desired point */
    
    *r = (rx * xc) + (ry * yc) + (rz * zc);
    *g = (gx * xc) + (gy * yc) + (gz * zc);
    *b = (bx * xc) + (by * yc) + (bz * zc);
}
////////////////////////////////////////
/*  	    	    	    NORM_RGB

    Normalise RGB components so the most intense (unless all
    are zero) has a value of 1.
    
*/

void Norm_rgb(double *r, double *g, double *b)
{
#define Max(a, b)   (((a) > (b)) ? (a) : (b))
    double greatest = Max(*r, Max(*g, *b));
    
    if (greatest > 0) {
    	*r /= greatest;
	*g /= greatest;
	*b /= greatest;
    }
#undef Max
}
////////////////////////////////////

/*                          CONSTRAIN_RGB

    If the requested RGB shade contains a negative weight for
    one of the primaries, it lies outside the colour gamut 
    accessible from the given triple of primaries.  Desaturate
    it by adding white, equal quantities of R, G, and B, enough
    to make RGB all positive.  The function returns 1 if the
    components were modified, zero otherwise.
    
*/

int Constrain_rgb(double *r, double *g, double *b)
{
    double w;

    /* Amount of white needed is w = - min(0, *r, *g, *b) */
    
    w = (0 < *r) ? 0 : *r;
    w = (w < *g) ? w : *g;
    w = (w < *b) ? w : *b;
    w = -w;

    /* Add just enough white to make r, g, b all positive. */
    
    if (w > 0) {
        *r += w;  *g += w; *b += w;
        return 1;                     /* Colour modified to fit RGB gamut */
    }

    return 0;                         /* Colour within RGB gamut */
}
//////////////////////
struct SpectrumInfo
{
   int numkeys;
   int r,g,b;
   SpectrumInfo(){}
      SpectrumInfo(int n, int rd, int gn, int bl): numkeys(n), r(rd), g(gn), b(bl) 
      {
      }
};

////////////////////////
struct File
{
   string name;
   time_t modtime;
   File (string n, time_t c): name(n), modtime(c)
      {
      }
};

struct FileCompare
{
   bool operator()(const File& f1, const File& f2)
      {
         if (f1.modtime != f2.modtime)
         {
            return difftime(f1.modtime, f2.modtime) < 0;  
         }
         return strcmp(f1.name.c_str(), f2.name.c_str()) > 0;
      }
};


//////////////////////////////////////////////
void ReadFileIntoMap(const File& targetFile, map<File, set<int>, FileCompare>& keyLists)
{
   ifstream keyfile;
   string filepath = DATADIR;
   filepath.append(targetFile.name);
   keyfile.open(filepath.c_str());
   set <int> keylist;
   int keyvalue;
   while (!keyfile.eof())
   {
      keyfile >> keyvalue;
      keylist.insert(keyvalue);
   }
   keyfile.close();
   keyLists.insert(pair<File, set<int> >(targetFile, keylist));
}



/////////////////////////////////////
// Here we read whatever files are in the public directory and order them by timestamp
// This is going to be compared to the files registered in spectrum.txt to determine what
// the 'new' files are that need to be registered.
void ReadFilesIntoSet(set<File, FileCompare >& fileSet)    
{
   DIR *pDIR;
   struct stat buff;
   struct dirent *entry;

   // first sort the files in time-order
   if ( pDIR=opendir(DATADIR) )
   {
       while (entry = readdir(pDIR))
	{
	  if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
	    {
               string filepath = DATADIR;                  
               filepath.append(entry->d_name);  
               stat(filepath.c_str(), &buff);
               fileSet.insert(File(entry->d_name, buff.st_mtime));
            }
        }
    }
  closedir(pDIR);
}

//////////////////////////////////////
// This adds files to the map which promise to have actual overlap with the input file
// Specifically, we compare the filesizes and RGB values; if within tolerance, add to map
void ReadRelevantFilesIntoMap(map<File, set<int> >& keyLists, int inputFilesize, int r, int g, int b)
{
   // read the spectra.txt file to get the relevant file info
}
/////////////////////////////
double FindMaxOverlap(const set<int>& keyListA, const set<int>& keyListB)
{

  set<int> commonKeys;
  commonKeys.clear();
  if (keyListA.size() <= 1 ||  keyListB.size() <= 1 ) {return 0;}   // don't compare empty files

  if (keyListA.size() <= keyListB.size())
  {
     for (set<int>::const_iterator it = keyListA.begin(); it!= keyListA.end(); it++)
     {
        if (keyListB.find(*it) !=  keyListB.end()) commonKeys.insert(*it);
     }
     return (double)(100.0*commonKeys.size()/keyListA.size());
  }
  else
  {
     for (set<int>::const_iterator it = keyListB.begin(); it!= keyListB.end(); it++)
     {
        if (keyListA.find(*it) !=  keyListA.end()) commonKeys.insert(*it);
     }
     return (double)(100.0*commonKeys.size()/keyListB.size());
  }
}

/////////////////////////////////////////////

void  WriteRepoireJSON()
{  

   //get current number of files recorded in "spectrum.txt"
    int number_of_files = 0;
    string line;
    ifstream myfile(DATADIR_SPECTRUM);

    while (getline(myfile, line)) { ++number_of_files; }
    myfile.close();


  // open output file
  ofstream outfile;
  outfile.open(REPOIRE_DATA);   

  outfile << "{ \"nodes\":[" << endl;

  ifstream specfile;
  specfile.open(DATADIR_SPECTRUM);
  if (specfile.is_open()) 
  {
     int count = 0;
     int r,g,b;
     int numkeys;
     string contribution;

     while (count < number_of_files)
     {
        count++;
        specfile >> contribution >> numkeys >> r >> g >> b;
        outfile << "{" <<
           "\"match\": \"" << 1.0  << "\"," << endl <<
           "\"name\": \"" << contribution.substr(0, contribution.size()-4)  << "\"," << endl <<
           "\"artist\": \"" << numkeys  << "\"," << endl <<
           "\"id\": \"" << contribution.substr(0, contribution.size()-4)  << "\"," << endl <<
           "\"playcount\":" << numkeys << "," << endl << 
           "\"CIEx\":" << 0 << "," << endl << 
           "\"CIEy\":" << 0 << "," << endl << 
           "\"red\":" << r << "," << endl << 
           "\"blue\":" << g << "," << endl << 
           "\"green\":" << b <<  "}" << endl ;         
        if (count < number_of_files) outfile << "," << endl;
     }
  }
  specfile.close();
  outfile << "], \"links\":["; 
  ifstream infile;
  infile.open(DATADIR_LINES);

  double overlap;
  string file1, file2;
  while (!infile.eof())
  {

     infile >> file1 >> file2 >> overlap;

     outfile << "{" << endl <<
        "\"source\": \"" << file1.substr(0, file1.size()-4) << "\"," << endl << 
        "\"target\": \"" << file2.substr(0, file2.size()-4) << "\"," << endl << 
        "\"overlap\": " << overlap/100.0 << endl << "}" << endl;
     if (!infile.eof()) outfile << "," << endl;
  }

  outfile << "]  }";
  outfile.close();
}
/////////////////////////
void ComputeSpectrum(double& x, double& y, double& z, set<int> key_set)
{
   int spectrum[FREQUENCY_BINS];
   for (int i = 0; i < FREQUENCY_BINS; i++)
   {
      spectrum[i] = 0;
   }

   for (set<int>::iterator it = key_set.begin();
        it != key_set.end(); ++it) 
   {
      int key = *it;
      if (key > 0 && key < KEY_RANGE)
      {
         spectrum[(int)((1.0*key/KEY_RANGE)*FREQUENCY_BINS)]++;
      }
      else
      {
         spectrum[(int)(log(abs(key)))]++;    // keys from non-dict words are collected in the lowest bins
      }
   }
   SpectrumToXYZ(spectrum, x, y, z);
}

void WriteRepoirePage()
{
  cout << "Content-type:text/html\r\n\r\n";
  cout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n";
  cout << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n";
   cout << "<html class=\"no-js\" lang=\"en\"> \r\n";
   cout << "<head>\r\n"; 
   cout << "  <meta charset=\"utf-8\"> \r\n";
   cout << "  <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge,chrome=1\">\r\n";

   cout << "   <title>Repoire Social Network</title>\r\n";
   cout << "  <meta name=\"description\" content=\"Ideas Colored by Meaning\">\r\n";
   cout << "  <meta name=\"author\" content=\"Nick Kersting\">\r\n";

   cout << "  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\r\n";
   cout << " <meta http-equiv=\"refresh\" content=\"1;url=http://www.quantumrepoire.com/results.html\">\r\n";  

   cout << "   <link rel=\"stylesheet\" href=\"../css/reset.css\">\r\n";
   cout << "  <link rel=\"stylesheet\" href=\"../css/bootstrap.min.css\">\r\n";
   cout << "  <link rel=\"stylesheet\" href=\"../css/style.css\">\r\n";

   cout << "</head>\r\n";

   cout << "<body>\r\n";
   cout << "   <p>&nbsp;</p>\r\n"; 
   cout << "   <p>&nbsp;</p>\r\n"; 
   cout << "   <p>&nbsp;</p>\r\n"; 
   cout << "   <p>&nbsp;</p>\r\n"; 
   cout << "<p align=\"center\">\r\n";
   cout << "If you are not redirected automatically, please click <a href=\"../results.html\">here</a>\r\n";
   cout << "</p>\r\n";
  cout << "</body>\r\n";
  cout << "</html>\r\n";


}
///////////////////////////////
void RegisterNewFile(File newFile, map<string, SpectrumInfo>& registeredMap, 
                     map<File, set<int>, FileCompare >& keyLists)
{
   fstream specFile;
   specFile.open (DATADIR_SPECTRUM, fstream::out | fstream::app);

   struct colourSystem *cs = &SMPTEsystem;
   double x1,y1,z1,r,g,b;
   ComputeSpectrum(x1, y1, z1, keyLists[newFile]);
   XYZ_to_rgb(cs, x1, y1, z1, &r, &g, &b);                     
   Constrain_rgb(&r, &g, &b);
   Norm_rgb(&r, &g, &b);      
   
   specFile << newFile.name << " " << keyLists[newFile].size() << " " 
               << (int)(255*r) << " " << (int)(255*g) << " " << (int)(255*b) << '\n';

   registeredMap.insert(pair<string, SpectrumInfo>(newFile.name, 
                                                      SpectrumInfo(keyLists[newFile].size(), (int)(255*r), (int)(255*g), (int)(255*b))));
   specFile.close();
}
////////////////////////////
// Files are dissimilar if the sizes or RGB spectrum are too different, for
// then one doesn't expect a close semantic match
bool FilesTooDissimilar(const SpectrumInfo s1, const SpectrumInfo s2)
{
   int size1 = s1.numkeys;
   int size2 = s2.numkeys;
   int r1 = s1.r;
   int g1 = s1.g;
   int b1 = s1.b;
   int r2 = s2.r;
   int g2 = s2.g;
   int b2 = s2.b;

   if (SIZE_MATCH_ON && (size1 >= SIZE_TOLERANCE * size2 || size1 <= size2 / SIZE_TOLERANCE) ||  // files must be within size of each other
       RGB_MATCH_ON && (r1 > RGB_TOLERANCE * r2 || r1 < r2 / RGB_TOLERANCE ||   // file color musn't be too different
       g1 > RGB_TOLERANCE * g2 || g1 < g2 / RGB_TOLERANCE ||
                                       b1 > RGB_TOLERANCE * b2 || b1 < b2 / RGB_TOLERANCE ))
   {
      return true;
   }
   return false;
}

/////////////////////////////////////////////
void ReadRegisteredFilesIntoMap(map<string, SpectrumInfo>& registeredMap)
{
   ifstream specfile;                               
   specfile.open(DATADIR_SPECTRUM);  

   string filename;
   int numkeys, x, y, r, g, b;
   while (!specfile.eof())
   {
      specfile >> filename >> numkeys >> r >> g >> b;   
      if (filename.size() > 0)        // don't treat empty lines as a file
      {
         registeredMap.insert(pair<string, SpectrumInfo>(filename, SpectrumInfo(numkeys,r,g,b)));
      }
   }
   specfile.close();
}

////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{

   map<File, set<int>, FileCompare> keyLists;   // we only populate this with the files we really need for comparison
   set<File, FileCompare> fileSet;  // this includes every file in the public directory
   map<string, SpectrumInfo> registeredMap; // spectral information of registered files (keyed by filename)
   ReadFilesIntoSet(fileSet);   
   ReadRegisteredFilesIntoMap(registeredMap);


   fstream line_file;
   line_file.open(DATADIR_LINES, fstream::out | fstream::app);

   int spectrum[FREQUENCY_BINS];           
   int filecount = 0;
   int registeredcount = 0;
   int number_registered_files = registeredMap.size();

   // at this point we compare the number of registered files (number_registered_files) with
   // the actual number in the public directory (recorded in fileSet) to determine
   // what the 'new' ones are so we can register them

   for (set<File, FileCompare >::const_iterator itx = fileSet.begin();   // iterate over all files in directory
        itx != fileSet.end(); ++itx)
   {
      if (++filecount <= number_registered_files) { continue; }  // only process the new files
      File f1 = *itx;
      ReadFileIntoMap(f1, keyLists);
      RegisterNewFile(f1, registeredMap, keyLists);

      registeredcount = 0;
      for (set<File, FileCompare >::const_iterator ity = fileSet.begin(); ity != fileSet.end(); ++ity)                
      { 
         if (++registeredcount >= filecount) { break; }  //avoid double-counting new file comparisons

         File f2 = *ity;
         if (FilesTooDissimilar(registeredMap[f1.name], registeredMap[f2.name])) {continue; }  // don't even bother if match unlikely
         if (keyLists.find(f2) == keyLists.end())     // lazy addition to keyLists
         {
            ReadFileIntoMap(f2, keyLists);
         }

         // now compare the two files to get percent overlap
         set<int> setA = keyLists[f1];
         set<int> setB = keyLists[f2];
         double overlap = FindMaxOverlap(setA, setB);
         
         if (overlap > MIN_OVERLAP)    // arbitrary limit
         {
            line_file << f1.name << " " << f2.name << " " << overlap << endl;
         }

      }
   }

   line_file.close();
   if (filecount > number_registered_files) { WriteRepoireJSON(); }    // If any new file found, write the JSON file used for the force network

   WriteRepoirePage();

   return 0;
}


