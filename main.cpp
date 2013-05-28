//==================================================
// Copyright (C) 2011 by Norwegian Radiation Protection Authority
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Authors: Dag Robøle,
//
//==================================================

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
using namespace std;

#include <windows.h>

const int PIVOT	= 80;

struct CHN_Header
{
	short signature;
	unsigned short mca_number;
	unsigned short segment;
	unsigned short seconds;
	long int realtime;
	long int livetime;
	char startdate[8];
	char starttime[4];
	unsigned short channel_offset;
	unsigned short channel_count;
};

int main(int argc, char* argv[])
{
	CHN_Header header;
	vector<string> files, error_messages;		
	vector<unsigned int> file_sizes;    
	int processed_files_failed = 0, processed_files_success = 0;
	bool fix;
	int year;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;		
	string dir = ".\\*.CHN";		
	
	hFind = FindFirstFile(dir.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 	
	{
		clog << "No .CHN files found in current directory. Exiting..." << endl;
	    return 0;	
	}
		
	files.push_back(FindFileData.cFileName);	
	file_sizes.push_back((unsigned int)FindFileData.nFileSizeLow);	
      	
	while (FindNextFile(hFind, &FindFileData) != 0) 	
	{
		files.push_back(FindFileData.cFileName);	
		file_sizes.push_back((unsigned int)FindFileData.nFileSizeLow);		
	}
    
	dwError = GetLastError();
	FindClose(hFind);
	if (dwError != ERROR_NO_MORE_FILES) 	
	{
		cerr << "Failed reading directory " + dir << endl;
		return 1;
	}

	for(unsigned int i=0; i<files.size(); ++i)
	{
		string fname = files[i];
		unsigned int fsize = file_sizes[i];		

		if(fsize < sizeof(CHN_Header))
		{
			error_messages.push_back("The file " + fname + " is too small to be a valid CHN file");			
			++processed_files_failed;
			continue;
		}		

		ifstream fin(fname.c_str(), fstream::binary);
		if(!fin.good())
		{
			error_messages.push_back("Failed to open file for reading: " + fname);			
			++processed_files_failed;
			continue;
		}

		memset((void*)&header, 0, sizeof(CHN_Header));

		if(!fin.read((char*)&header, sizeof(CHN_Header)))
		{
			fin.close();
			error_messages.push_back("Failed to read from file: " + fname);			
			++processed_files_failed;
			continue;
		}

		fix = false;		
		stringstream ss;
		ss << header.startdate[5] << header.startdate[6];
		ss >> year;

		if(year <= PIVOT && header.startdate[7] != '1')				
			fix = true;	

		fin.close();

		if(fix)
		{
			fstream fout(fname.c_str(), std::ios::binary | std::ios::in | std::ios::out);
			if(!fout.good())
			{
				error_messages.push_back("Failed to open file for writing: " + fname);				
				++processed_files_failed;
				continue;
			}
			char c = '1';
			fout.seekp(23);
			fout.write((char*)&c, sizeof(char));

			fout.close();
			++processed_files_success;
			clog << "File " << fname << " fixed successfully" << endl;
		}				
	}

	for(vector<string>::iterator it = error_messages.begin(); it != error_messages.end(); ++it)
		cerr << *it << endl;

	clog << "Of " << files.size() << " CHN files, " << processed_files_success << " was fixed and " << processed_files_failed << " failed" << endl;

	return 0;
}