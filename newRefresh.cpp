// Compile this to make the refresh.cgi script that executes when the user clicks "Refresh Network"
// This appends to lines.txt and spectra.txt as needed (not at all if there are no new files),
// redoes to the repoire.json file, and redirects the web page

// New files are identified by timestamps later than the last 'registered file' (last line of spectra.txt)
// If files are found, they are compared to all registered files, and then registered themselves.

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
#include "color.h"  // contains functions for computing RGB values
#include "keyfile.h"  // class declarations

const char* DATADIR = "/home/nkersting/quantumrepoire.com/public/";  // user encryption files here
const char* DATADIR_SPECTRUM = "/home/nkersting/quantumrepoire.com/spectra.txt";  // list of registered files
const char* DATADIR_LINES = "/home/nkersting/quantumrepoire.com/lines.txt";  // list of connections thus far
const char* REPOIRE_DATA = "/home/nkersting/quantumrepoire.com/data/repoire.json"; // data for d3.js layout

const double MIN_OVERLAP = 10.0;   // minimum percentage of keys in keylist intersection to declare a 'connection'



/// Looks up keys for targetFile and stores them in the associated member.
/// \param targetFile intended file; keylist member is modified
/// \return nothing
void StoreFullKeys(KeyFile& targetFile)
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
      targetFile.keylist.insert(keyvalue);
   }
   keyfile.close();
}



/// Computes XYZ color coordinates from the keylist in inputfile
/// \param x populated with computed coord.
/// \param y populated with computed coord.
/// \param z populated with computed coord.
/// \param kfile inputfile; spectrum is populated 
/// \return nothing
void ComputeSpectrum(double& x, double& y, double& z, KeyFile& kfile)
{
   for (set<int>::iterator it = kfile.keylist.begin(); it != kfile.keylist.end(); ++it) 
   {
      int key = *it;
      if (key > 0 && key < KEY_RANGE)
      {
         kfile.spectral_info.freq_bins[(int)((1.0*key/KEY_RANGE)*FREQUENCY_BINS)]++;
      }
      else
      {
         kfile.spectral_info.freq_bins[(int)(log(abs(key)))]++;    // exotic keys are collected in the lowest bins
      }
   }

   SpectrumToXYZ(&kfile.spectral_info.freq_bins[0], x, y, z);
}


/// Inspect the public directory for files with timestamps later than the input param
/// \param newfiles container for any new files found
/// \param last_timestamp timestamp of the last registered file
/// \return nothing
void LookForNewFiles(set<KeyFile>& newfiles, time_t last_timestamp)    
{
   DIR *pDIR;
   struct stat buff;
   struct dirent *entry;

   // first sort all files in time-order
   if ( pDIR=opendir(DATADIR) )
   {
       while (entry = readdir(pDIR))
	{
	  if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
	    {
               string filepath = DATADIR;                  
               filepath.append(entry->d_name);  
               stat(filepath.c_str(), &buff);
               if (difftime(buff.st_mtime, last_timestamp) > 0) {    // must be a new file

                  KeyFile newfile(entry->d_name, buff.st_mtime);
                  StoreFullKeys(newfile);

                  struct colourSystem *cs = &SMPTEsystem;
                  double x1,y1,z1,r,g,b;

                  ComputeSpectrum(x1, y1, z1, newfile);
                  XYZ_to_rgb(cs, x1, y1, z1, &r, &g, &b);                     
                  Constrain_rgb(&r, &g, &b);
                  Norm_rgb(&r, &g, &b);      

                  newfile.spectral_info.r = r;
                  newfile.spectral_info.g = g;
                  newfile.spectral_info.b = b;

                  newfiles.insert(newfile);
               }
            }
        }
    }
  closedir(pDIR);
}


/// Reads the file containing registered file info and inserts into input container
/// \param registered input container; filled with registered file info
/// \return timestamp of latest registered file
time_t ReadRegisteredFiles(set<KeyFile>& registered)
{
   ifstream specfile;                               
   specfile.open(DATADIR_SPECTRUM);  

   string filename;
   time_t modtime, latest_time = 0;
   int numkeys, x, y, r, g, b;
   while (!specfile.eof())
   {
      specfile >> filename >> modtime >> numkeys >> r >> g >> b;   
      if (filename.size() > 0)        // don't treat empty lines as a file
      {
         KeyFile currFile(filename, modtime);
         currFile.spectral_info = SpectrumInfo(numkeys,r,g,b);
         for (int bin = 0; bin < FREQUENCY_BINS; ++bin) {
            specfile >> currFile.spectral_info.freq_bins[bin];
         }
         registered.insert(currFile);
         latest_time = modtime;    // lines in file are guaranteed time-ordered
      }
   }
   specfile.close();
   return latest_time;
}

/// Appends new line to the end of the spectra file and adds file to the input container
/// \param newfile file to add
/// \param registered_files container for files already registered; is added to
/// \return nothing
void RegisterNewFile(const KeyFile& newfile, set<KeyFile>& registered_files)
{
   fstream specfile;
   specfile.open (DATADIR_SPECTRUM, fstream::out | fstream::app);
   
   specfile << newfile.name << '\t' << newfile.modtime << '\t' << newfile.keylist.size() << '\t' 
            << (int)(255*newfile.spectral_info.r) << '\t' << (int)(255*newfile.spectral_info.g)
            << '\t' << (int)(255*newfile.spectral_info.b);

   for (int bin = 0; bin < FREQUENCY_BINS; ++bin) {
      specfile << '\t' << newfile.spectral_info.freq_bins[bin];
   }
   
   specfile << '\n';

   specfile.close();
   registered_files.insert(newfile);
}


/// Set intersection between input sets
/// \param s1 first set
/// \param s2 second set
/// \return smallest percent intersection, i.e. 100*(s1 & s2)/max(|s1|,|s2|)
double FindMinOverlap(const set<int>& s1, const set<int>& s2)
{
  if (0 == s1.size() || 0 == s2.size())
     return 0;

  set<int> commonKeys;

  // iterate over the smaller of the sets
  if (s1.size() <= s2.size()){
     for (set<int>::const_iterator it = s1.begin(); it!= s1.end(); it++){
        if (s2.find(*it) !=  s2.end()) commonKeys.insert(*it);
     }
     return (double)(100.0*commonKeys.size()/s2.size());
  }
  else {
     for (set<int>::const_iterator it = s2.begin(); it!= s2.end(); it++){
        if (s1.find(*it) !=  s1.end()) commonKeys.insert(*it);
     }
     return (double)(100.0*commonKeys.size()/s1.size());
  }
}


/// Determines upper bound on number of equal keys in two binned distributions.
/// This is done by simply comparing counts bin-wise.
/// \param b1 first binned distribution
/// \param b2 second binned distribution; assumed to be binned equivalently to first
/// \return upperbound on number of keys shared between distributions
int MaxBinIntersection(const vector<int>& b1, const vector<int>& b2) {
   int maxcount = 0;
   for (size_t i = 0; i < b1.size(); ++i) {
      maxcount += min(b1[i], b2[i]);
   }
   return maxcount;
}


/// Determines whether two keyfiles' spectra are too disparate to bother comparing in detail.
/// This is achieved by comparing sizes (a tiny file has no chance of mutual overlap with a huge one),
/// and counts in spectral bins (if the bins don't overlap enough, no need to look further).
/// \param s1 spectral info of first file
/// \param s2 spectral info of second file
/// \return true if the files are definitely not going to pass overlap threshold
bool FilesTooDissimilar(const SpectrumInfo& s1, const SpectrumInfo& s2)
{

   if (1.0*min(s1.numkeys, s2.numkeys)/max(s1.numkeys, s2.numkeys) < MIN_OVERLAP)    // sizes too dissimilar
      return true;

   int max_intersection = MaxBinIntersection(s1.freq_bins, s2.freq_bins);

   if (1.0*max_intersection/s1.numkeys < MIN_OVERLAP   // intersection too small
       ||
       1.0*max_intersection/s2.numkeys < MIN_OVERLAP) 
      return true;

   return false;
}

/// Reads the spectra and line files to get the contributions and their connections.
/// Writes this info to the .json file used in the d3.js script to draw force-directed layout.
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
     int binval;  // dummy variable to hold bin contents (we don't use them in the json)
     time_t modtime; 
     string contribution;

     while (count < number_of_files)
     {
        count++;
        specfile >> contribution >> modtime >> numkeys >> r >> g >> b;
        for (int bin = 0; bin < FREQUENCY_BINS; ++bin) {                                                          
           specfile >> binval;
        }
       
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

/// Trick to get the page to reload after computing the network
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


////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{

   set<KeyFile> new_files; 
   set<KeyFile> registered_files;

   time_t latest_timestamp = ReadRegisteredFiles(registered_files);
   LookForNewFiles(new_files, latest_timestamp);

   if (0 == new_files.size()) { 
      WriteRepoirePage();       // just prompt to redraw the page and exit
      return 0;
   }

   fstream line_file;
   line_file.open(DATADIR_LINES, fstream::out | fstream::app);

   // loop over new files, register them one by one and compare to previously-registered files
   for (set<KeyFile>::const_iterator itx = new_files.begin(); itx != new_files.end(); ++itx){
      KeyFile f1 = *itx;

      for (set<KeyFile>::const_iterator ity = registered_files.begin(); ity != registered_files.end(); ++ity){ 
         KeyFile f2 = *ity;
         if (FilesTooDissimilar(f1.spectral_info, f2.spectral_info))
             continue;   // don't even bother if match unlikely

         // lazy fill
         if (f2.keylist.size() == 0)
            StoreFullKeys(f2);

         double overlap = FindMinOverlap(f1.keylist, f2.keylist);
         
         if (overlap > MIN_OVERLAP)   
            line_file << f1.name << '\t' << f2.name << '\t' << overlap << '\n';
      
      }
      RegisterNewFile(f1, registered_files);
   }

   line_file.close();
   WriteRepoireJSON();
   WriteRepoirePage();

   return 0;
}


