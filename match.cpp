

#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <set>
#include <vector>
#include <sstream>
#include <memory>
#include <fstream>
#include <string.h>
#include <math.h>
#include <cstring>
#include <map>
#include <algorithm>
#include <ctime>
#include "dirent.h"

using namespace std;



const string ENV[ 24 ] = {                 
  "COMSPEC", "DOCUMENT_ROOT", "GATEWAY_INTERFACE",   
  "HTTP_ACCEPT", "HTTP_ACCEPT_ENCODING",             
  "HTTP_ACCEPT_LANGUAGE", "HTTP_CONNECTION",         
  "HTTP_HOST", "HTTP_USER_AGENT", "PATH",            
  "QUERY_STRING", "REMOTE_ADDR", "REMOTE_PORT",      
  "REQUEST_METHOD", "REQUEST_URI", "SCRIPT_FILENAME",
  "SCRIPT_NAME", "SERVER_ADDR", "SERVER_ADMIN",      
  "SERVER_NAME","SERVER_PORT","SERVER_PROTOCOL",     
  "SERVER_SIGNATURE","SERVER_SOFTWARE" };   

const char* DATADIR = "/home/nkersting/quantumrepoire.com/public/";

const int MAXSTRING = 200;

const string c_ContentHeader = "Content-type: text/html\n\n";

// ---- CONTENT LENGTH ----

///<Summary>
/// The content length environment variable
/// name
///</Summary>
const string c_ContentLengthVariableName = "CONTENT_LENGTH";

///<Summary>
/// Function to return the current requests
/// content length
///</Summary>
const int GetContentLength()
{
  int l_result = 0;
  char* l_ContentLengthVariable = getenv(c_ContentLengthVariableName.c_str());
  if ( l_ContentLengthVariable != NULL )
    {
      l_result = atoi(l_ContentLengthVariable);
    }
  return l_result;

}

// ---- END CONTENT LENGTH ----


// ---- GET CONTENT ----

///<Summary>
/// Function to return the content
///</Summary>
const list<string> GetContent()
{
  list<string> l_result;

  // Now seek the content
  int l_ContentLength = GetContentLength();
  if ( l_ContentLength > 0 )
    {
            try
	      {
                // Allocate a buffer for the information
                auto_ptr<char> l_Buffer (new char[l_ContentLength]);

                // Read the content sent into the buffer
                int l_bytesRead = fread (l_Buffer.get(), sizeof(char), l_ContentLength, stdin);

                // Check the data length
                if ( l_bytesRead == l_ContentLength )
		  {                                        
                    // Convert the buffer to a string
                    stringstream l_stream (l_Buffer.get());

                    // Push the content as a string into the buffer
                    while ( !l_stream.eof() )
		      {
                        string l_item;
                        l_stream >> l_item;

                        l_result.push_back(l_item);
		      }
		  }
	      }
            catch (bad_alloc l_badAllocationException)
	      {
                // TODO handle bad alloc
	      }
    }

  return l_result;
}

// ---- END GET CONTENT ----


unsigned int intConvert(string aInput, int i)
{
  char val1 = aInput.at(i+1);
  unsigned int myval1;
  std::stringstream ss1;
  ss1 << val1;
  ss1 >> std::hex >> myval1;

  char val2 = aInput.at(i+2);
  unsigned int myval2;
  std::stringstream ss2;
  ss2 << val2;
  ss2 >> std::hex >> myval2;

  return (16*myval1 + myval2);
  
}
//////////////////////
string cleanString(string aInput)
{
	char newstring[MAXSTRING];

	int pos = 0;
	for (int i = 0; i < aInput.length(); i++ )
	{
		if (aInput.at(i) == 37)    // %
		{
			newstring[pos] = intConvert(aInput, i);
			if (newstring[pos] == 10 || newstring[pos] == 13)
			{
	             // treat newline as end of word
				 newstring[pos] = '\0';
				 return newstring;
			}
			else
			{
				pos++;
			}		
			i += 2;
		}
		else
		{
			newstring[pos] = aInput.at(i);
			pos++;
		}
	}
	newstring[pos] = '\0';

	return newstring;
}



////////////////////////////////////////////////////////////////////////
// -- REPOIRE FUNCTIONS --


vector <int> hash_x(const char *str)
{
  vector <int> synsets;         // actually a vector of just one int
  unsigned long hash = 5381;
  int c;
  
  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  synsets.push_back((int)hash);
  return synsets;
}

vector <int> findSynsets(const std::string& word, std::map<std::string, vector <int> >& WordMap)
{
  std::map<std::string, vector <int> >::iterator iter = WordMap.find(word);
  if (iter != WordMap.end()) 
    { 
      return iter->second;  // matching set of synsets returned
    }
  else                     // try looking up without last character
    {                        // takes care of most plurals and final punctuation
      std::string short_word = word.substr(0,word.size() - 1);
      std::map<std::string, vector <int> >::iterator iter = WordMap.find(short_word);
      if (iter != WordMap.end())
	{
	  return iter->second;  // matching set of synsets returned
	}
      else
	{
	  // more plural handling:
	  // if last two letters are "es" try looking up without last two characters
	  if (word[word.size()-2] == 'e' && word[word.size()-1] == 's')
	    {
	      std::map<std::string, vector <int> >::iterator iter = WordMap.find(word.substr(0,word.size()-2));
	      if (iter != WordMap.end())
		{
		  return iter->second;  // matching set of synsets returned
		}
	      else
		{
		  // if last three letters are "ies" try replacing these with "y"
		  if (word[word.size()-3] == 'i')
		    {
		      string new_word = word.substr(0,word.size()-3);
		      new_word.append("y");
		      std::map<std::string, vector <int> >::iterator iter = WordMap.find(new_word);
		      if (iter != WordMap.end())
			{
			  return iter->second;  // matching set of synsets returned
			}
		    }
		}
	    }

	  return hash_x(word.c_str());     // if we get here then a hash is needed
	}                                  // e.g. some specialty words or proper nouns
    }
}

void populateKeyMap(std::map<int, vector <std::pair<string,string> > > &KeyMap,    // keys=keys in the encryption, values=word-pairs contributing to that key
			     std::map<std::string, vector <int> > &WordMap,   // the thesaurus
			     vector <std::string> &input_words)
{

  vector <int> keys1;
  vector <int> keys2;

  for (int i = 0; i < input_words.size(); i++)
    {
      keys1 = findSynsets(input_words.at(i), WordMap);
      for (int j = i+1; j < input_words.size(); j++)
	{
	  keys2 = findSynsets(input_words.at(j), WordMap);
	  for (vector<int>::iterator it1 = keys1.begin(); it1 != keys1.end(); it1++)
	    {
	      for (vector<int>::iterator it2 = keys2.begin(); it2 != keys2.end(); it2++)
		{
                   KeyMap[(*it1 + *it2)].push_back(std::pair<string, string>(input_words.at(i), input_words.at(j)));
		}  
	    }
	  
	}
    }

  return;

}  




/////////////////////////////////////////////




///////////////////////////////////////////////

void fill_dictionary( std::map<std::string, vector <int> >& WordMap)
{

  // read in the  "word-synsets.txt"
  ifstream word_synset_file;
  word_synset_file.open ("omegaO2.txt");



  string line;
  while (getline(word_synset_file, line))
    {
      istringstream read_ss(line);
      string read_s;
      int firstread = 1;
      string word = "";
      while (getline(read_ss, read_s, ' '))   
	{
	  if (firstread == 1 )
	    {
	      word = read_s;
	      firstread = 0;
	    }
	  else
	    {
	      WordMap[word].push_back(atoi(read_s.c_str()));
	    }
	}

    }




}

////////////////////////////////

///////////////////////////////
void    make_MatchMap(std::map<std::string, float >& MatchMap,
		      std::map<int, vector <std::pair<string,string> > >& KeyMap,
                      std::map<std::string, vector <int> > &WordMap,   // the thesaurus
		      vector <int>& keylist)
{

   std::map<std::pair<string,string>, int> pairScores;    // keys=word pairs from the message, values=number of matching keys responsible for

  for (vector<int>::iterator it = keylist.begin(); it != keylist.end(); it++)
  {
     if (KeyMap.find(*it) != KeyMap.end())
     {
        for (vector<std::pair<string,string> >::iterator it2 = (KeyMap[*it]).begin(); 
	       it2 != (KeyMap[*it]).end(); it2++)
           {
              if (pairScores.find(*it2) == pairScores.end())
              {
                 pairScores[*it2] = 1;
              }
	      else
              {
                 pairScores[*it2]+= 1;
              }
           }                               
     }
  }

  // here we assign scores to each word (from word pair) based on the number of keys it contributed to in the match, normalized by
  // the number of keys that word pair should have given if a perfect matchz
  for(std::map<std::pair<std::string, std::string>, int>::iterator it = pairScores.begin(); it != pairScores.end(); ++it)
  {
     int normalization = (findSynsets((it->first).first, WordMap).size()) * (findSynsets((it->first).second, WordMap).size());
     if (MatchMap.find((it->first).first) == MatchMap.end())
     {
        MatchMap[(it->first).first] = 1.0*(it->second)/normalization;
     }
     else
     {
        MatchMap[(it->first).first] = max((float)(1.0*(it->second)/normalization), MatchMap[(it->first).first]); 
     }
     if (MatchMap.find((it->first).second) == MatchMap.end())
     {
        MatchMap[(it->first).second] = 1.0*(it->second)/normalization;
     }
     else
     {
        MatchMap[(it->first).second] = max((float)(1.0*(it->second)/normalization), MatchMap[(it->first).second]);
     }
  }


}

/////////////////////////////


void  print_SortedMap(std::map<float, vector <std::string> >& SortedMap,
		      std::map<std::string, float >& MatchMap)
{

  for (std::map<std::string, float >::iterator iter = MatchMap.begin(); 
       iter != MatchMap.end(); iter++)
    {
      SortedMap[-1.0*(iter->second)].push_back(iter->first);   //trick to get biggest values first
    }



  for (std::map<float, vector <std::string> >::iterator iter = SortedMap.begin(); 
       iter != SortedMap.end(); iter++)
    {
      cout << "<tr>\r\n";
      cout << "<td>\r\n";
      cout << -1.0*(iter->first) << ": ";
      int count = 0;
      for (vector<std::string>::iterator iter2 = (iter->second).begin();
	   iter2 != (iter->second).end(); iter2++)
	{
	  cout << *iter2;
	  count++;
	  if (count < (iter->second).size())
	    {
	      cout << ", ";
	    }
	  else
	    {
	      cout << " ";
	    }
	    
	} 
      cout << "</td>\r\n";
      cout << "</tr>\r\n";
    }

}

//////////////////////////////



   


void  WriteMatchPage(std::map<std::string, float >& MatchMap, string input_file)
{


  cout << "Content-type:text/html\r\n\r\n";
  cout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n";
  cout << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n";
  cout << "<head>\r\n";
  cout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\r\n";
  cout << "<title>Matching</title>\r\n";
  cout << "</head>\r\n";
  cout << "<body>\r\n";
  
  cout << "<p>Results are rated from 1.0 (at least one exact word match) to 0.0 (no match)." <<
    "In general, any word with a matching value above 0.1 is probably reasonably close to a word in the target selection.</p>\r\n";
  cout << "<p>&nbsp;</p>\r\n";

  cout << "<table width=\"503\" border=\"0\">\r\n";
  cout << "<tr>\r\n";
  cout << "<form action=\"/cgi-bin/match.cgi\" method=\"post\" name=\"TestForm\" >\r\n";
  cout << "<td width=\"244\"><textarea name=\"TextField1\" cols=\"30\" rows=\"3\" columns=\"50\" type=\"text\" value=\"\" ></textarea></td>\r\n";
  cout << "<td width=\"244\"><input type=\"text\" name=\"TextField2\" value=\"" << input_file.substr(strlen(DATADIR),input_file.size() - strlen(DATADIR)) <<"\" /></td>\r\n";
  cout << "<td width=\"93\"><input type=\"submit\" name=\"submit\" value=\"Click to Submit\" /></td>\r\n";
  cout << "</form>\r\n";
  cout << "</tr>\r\n";
  cout << "<tr>\r\n";
  cout << "<td align=\"center\">Text to Match</td>\r\n";
  cout << "<td align=\"center\">Target File</td>\r\n";
  cout << "<td>&nbsp;</td>\r\n";
  cout << "</tr>\r\n";
  cout << "</table>\r\n";

  
  cout << "<p>&nbsp;</p>\r\n";
  cout << "<table width=\"505\" border=\"1\">\r\n";
  cout << "<tr>\r\n";
  cout << "<td width=\"495\" align=\"center\">Matching Results</td>\r\n";
  cout << "</tr>\r\n";

  // make sorted map to display results

  std::map<float, vector <std::string> > SortedMap;

  print_SortedMap(SortedMap, MatchMap);

  if (SortedMap.size() == 0)
    {
      cout << "<tr>\r\n";
      cout << "<td>No matches (even approximately) found! Please try again...</td>\r\n";
      cout << "</tr>\r\n";

    }

  cout << "</table>\r\n";

  cout << "</body>\r\n";
  cout << "</html>\r\n";

}
///////////////////////////////////////////////
void  WriteErrorMatchPage(string input_file)
{


  cout << "Content-type:text/html\r\n\r\n";
  cout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n";
  cout << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n";
  cout << "<head>\r\n";
  cout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\r\n";
  cout << "<title>Matching Error</title>\r\n";
  cout << "</head>\r\n";
  cout << "<body>\r\n";
  cout << "<p>Results are rated from \"N\" (at least N exact word matches) to \"0.0\" (no match)." <<
    "In general, any word with a matching value above 0.1 is probably reasonably close to a word in the target selection.</p>\r\n";
  cout << "<p>&nbsp;</p>\r\n";

  cout << "<table width=\"503\" border=\"0\">\r\n";
  cout << "<tr>\r\n";
  cout << "<form action=\"/cgi-bin/match.cgi\" method=\"post\" name=\"TestForm\" >\r\n";
  //   cout << "<td width=\"244\"><input type=\"text\" name=\"TextField1\" value=\"\" /></td>\r\n";
  cout << "<td width=\"244\"><textarea name=\"TextField1\" cols=\"30\" rows=\"3\" columns=\"50\" type=\"text\" value=\"\" ></textarea></td>\r\n";
  cout << "<td width=\"244\"><input type=\"text\" name=\"TextField2\" value=\"" << input_file.substr(strlen(DATADIR),input_file.size() - strlen(DATADIR)) <<"\" /></td>\r\n";
  cout << "<td width=\"93\"><input type=\"submit\" name=\"submit\" value=\"Click to Submit\" /></td>\r\n";
  cout << "</form>\r\n";
  cout << "</tr>\r\n";
  cout << "<tr>\r\n";
  cout << "<td align=\"center\">Text to Match</td>\r\n";
  cout << "<td align=\"center\">Target File</td>\r\n";
  cout << "<td>&nbsp;</td>\r\n";
  cout << "</tr>\r\n";
  cout << "</table>\r\n";

  
  cout << "<p>&nbsp;</p>\r\n";
  cout << "<table width=\"505\" border=\"1\">\r\n";
  cout << "<tr>\r\n";
  cout << "<td width=\"495\" align=\"center\">Matching Results</td>\r\n";
  cout << "</tr>\r\n";

  
  cout << "<tr>\r\n";
  cout << "<td>Submission Error: sentence must have 2 or more words. Please try again.</td>\r\n";
  cout << "</tr>\r\n";

  

  cout << "</table>\r\n";

  cout << "</body>\r\n";
  cout << "</html>\r\n";


   


}

////////////////////////////////////////////////
void  WriteErrorPage(string input_file)
{


  cout << "Content-type:text/html\r\n\r\n";
  cout << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n";
  cout << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n";
  cout << "<head>\r\n";
  cout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\r\n";
  cout << "<title>Matching Error</title>\r\n";
  cout << "</head>\r\n";
  cout << "<body>\r\n";
  

  cout << "<table width=\"503\" border=\"0\">\r\n";
  cout << "<tr>\r\n";
  cout << "<form action=\"/cgi-bin/match.cgi\" method=\"post\" name=\"TestForm\" >\r\n";
  cout << "<td width=\"244\"><input type=\"text\" name=\"TextField1\" value=\"\" /></td>\r\n";
  cout << "<td width=\"244\"><input type=\"text\" name=\"TextField2\" value=\"" <<   input_file.substr(strlen(DATADIR),input_file.size() - strlen(DATADIR))<< "\" /></td>\r\n";
  cout << "<td width=\"93\"><input type=\"submit\" name=\"submit\" value=\"Click to Submit\" /></td>\r\n";
  cout << "</form>\r\n";
  cout << "</tr>\r\n";
  cout << "<tr>\r\n";
  cout << "<td align=\"center\">Sentence</td>\r\n";
  cout << "<td align=\"center\">Target File</td>\r\n";
  cout << "<td>&nbsp;</td>\r\n";
  cout << "</tr>\r\n";
  cout << "</table>\r\n";

  
  cout << "<p>&nbsp;</p>\r\n";
  cout << "<table width=\"200\" border=\"1\">\r\n";
  cout << "<tr>\r\n";
  cout << "<td align=\"center\">Matching Results</td>\r\n";
  cout << "</tr>\r\n";

  
  cout << "<tr>\r\n";
  cout << "<td>File not found! Please try again...</td>\r\n";
  cout << "</tr>\r\n";



  cout << "</table>\r\n";

  cout << "</body>\r\n";
  cout << "</html>\r\n";


   


}

////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  

  //cout << c_ContentHeader;
  //cout << "<html><body>" << endl;
  //cout << "The content Length is: " << GetContentLength() << "<br>" << endl;

  //cout << "The Content is: <br><pre>" << endl;

  vector<string> input_words;
  input_words.clear();
  string userFILE(DATADIR);


  list<string> theContent = GetContent();
  for (list<string>::const_iterator itr = theContent.begin();
       itr != theContent.end();
       itr++)
    {
      // cout << (*itr) << endl;


      istringstream read_ss(*itr);
      string read_s;
      bool firstword;
      int count = 0;
      while (getline(read_ss, read_s, '&'))
	{
	  count++;
	  if (count == 1)   // this is for the sentence line
	    {
	      istringstream read_ss2(read_s);
	      string read_s2;
	      firstword = true;
	      while (getline(read_ss2, read_s2, '+'))
		{
		  if (firstword)
		    {
		      std::transform(read_s2.begin(), read_s2.end(), read_s2.begin(), ::tolower);  //convert to lowercase
		      input_words.push_back(read_s2.substr(11,read_s2.size() - 11));  // chop off the "TextField1="
		      firstword = false;
		    }
		  else
		    {
		      std::transform(read_s2.begin(), read_s2.end(), read_s2.begin(), ::tolower);  //convert to lowercase
		      input_words.push_back(read_s2);
		    }
		}
	    }
	  else if (count == 2)         // this is for the file name
	    {
	      if (read_s.length() == 11) 
		{
		  WriteErrorPage(userFILE);   
		  return 0;
		}
	      userFILE.append(read_s.substr(11,read_s.size() - 11));
	    }
	}




    }




  vector<string> new_input_words;
  for (vector<string>::iterator it = input_words.begin(); it != input_words.end(); it++)
  {
    new_input_words.push_back(cleanString(*it));   
  }

  string newUserFILE = cleanString(userFILE);


 
  ////////////////
  // Below we match
  ///////////////////
 
  // fill in the word-synset map
  //cout << "Loading dictionary, please wait ..." << endl;
  std::map<std::string, vector <int> > WordMap;
  fill_dictionary(WordMap);



  // open key file to match against
   
  ifstream keyfile;
  keyfile.open(newUserFILE.c_str());
  if (!keyfile.good())
    {
      WriteErrorPage(newUserFILE);   
      return 0;
    }
  vector <int> keylist;
  int keyvalue;
  while (!keyfile.eof())
    {
      keyfile >> keyvalue;
      keylist.push_back(keyvalue);
    }
  

   
  if (new_input_words.size() < 2)
    {
      WriteErrorMatchPage(newUserFILE);
      return 0;                        // exit the program since the input string is too short
    }
  else
    {

       std::map<int, vector <std::pair<string, string> > > KeyMap;   // lookup map from key to pairs of words giving that key
      populateKeyMap(KeyMap, WordMap, new_input_words); 

      // populate a match list with weights for each word
      std::map<std::string, float > MatchMap;
      make_MatchMap(MatchMap,KeyMap,WordMap,keylist);


      WriteMatchPage(MatchMap, newUserFILE);  
   
    }
  
  
  



  return 0;






}

