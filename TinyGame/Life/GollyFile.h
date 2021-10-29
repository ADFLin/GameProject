#pragma once
#ifndef GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD
#define GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD

#include "LifeCore.h"

namespace Life
{
	class GollyFileReader
	{
	public:
		using bigint = int64;

		bool getedges = false;              // find pattern edges?
		bigint top, left, bottom, right;    // the pattern edges

		static constexpr int BUFFSIZE = 8192;

		FILE *pattfile;
		char filebuff[BUFFSIZE];
		int buffpos, bytesread, prevchar;

		long filesize;             // length of file in bytes

		// use buffered getchar instead of slow fgetc
		// don't override the "getchar" name which is likely to be a macro

		void findedges(IAlgorithm& imp)
		{
			BoundBox bound = imp.getBound();
			bottom = bound.min.y;
			top = bound.max.y;
			left = bound.min.x;
			right = bound.max.x;
		}

		void lifebeginprogress(char const*)
		{

		}
		void lifeendprogress()
		{

		}
		void lifeabortprogress(double, char const*)
		{

		}
		bool isaborted() { return false; }

		int mgetchar();

		
		char *getline(char *line, int maxlinelen);


		const char *readtextpattern(IAlgorithm &imp, char *line);

		/*
		 *   Parse "#CXRLE key=value key=value ..." line and extract values.
		 */
		void ParseXRLELine(char *line, int *xoff, int *yoff, bool *sawpos, bigint &gen);

		const char *readrle(IAlgorithm& imp, char *line);


		/*
		 *   This ugly bit of code will go undocumented.  It reads Alan Hensel's
		 *   PC Life format, either 1.05 or 1.06.
		 */
		const char *readpclife(IAlgorithm& imp, char *line);

		/*
		 *   This routine reads David Bell's dblife format.
		 */
		const char *readdblife(IAlgorithm &imp, char *line);

		//
		// Read Mirek Wojtowicz's MCell format.
		// See http://psoup.math.wisc.edu/mcell/ca_files_formats.html for details.
		//
		const char *readmcell(IAlgorithm &imp, char *line);

		long getfilesize(const char *filename)
		{
			long flen = 0;
			FILE *f = fopen(filename, "r");
			if (f != 0) {
				fseek(f, 0L, SEEK_END);
				flen = ftell(f);
				fclose(f);
			}
			return flen;
		}

		// This function guesses whether `line' is the start of a headerless Life RLE
		// pattern.  It is used to distinguish headerless RLE from plain text patterns.
		static bool isplainrle(const char *line);

		const char *build_err_str(const char *filename)
		{
			static char file_err_str[2048];
			sprintf(file_err_str, "Can't open pattern file:\n%s", filename);
			return file_err_str;
		}

		const char *loadpattern(IAlgorithm &imp);

		const char *readpattern(const char *filename, IAlgorithm& imp);
	};

}


#endif // GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD
