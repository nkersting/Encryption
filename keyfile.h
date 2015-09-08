
// keyfile.h
// structures defining encryption file and its contents



const int FREQUENCY_BINS = 81;    // binning of the spectrum
const int KEY_RANGE = 32000000;   // maximum expected key value


struct SpectrumInfo
{
   int numkeys;
   int r,g,b;
   vector<int> freq_bins; // number of keys by bin
SpectrumInfo(): freq_bins(FREQUENCY_BINS, 0){}
SpectrumInfo(int n, int rd, int gn, int bl): numkeys(n), r(rd), g(gn), b(bl), freq_bins(FREQUENCY_BINS, 0){}
};

struct KeyFile
{
   string name;
   time_t modtime; 
   KeyFile (string n, time_t c): name(n), modtime(c) {}

   set<int> keylist;  // may be empty; filled lazily as needed
   SpectrumInfo spectral_info;
   
   // for comparison of KeyFiles
   bool operator<(KeyFile other) const {

      if (modtime != other.modtime)
         return difftime(modtime, other.modtime) < 0;  

      return strcmp(name.c_str(), other.name.c_str()) > 0;
   }

};
