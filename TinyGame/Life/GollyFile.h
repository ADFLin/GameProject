#pragma once
#ifndef GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD
#define GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD

#include "LifeCore.h"

#include <istream>

namespace Life
{
	class GollyFileReader
	{
	public:
		using bigint = int64;

		bool getedges = false;              // find pattern edges?
		bigint top, left, bottom, right;    // the pattern edges

		static constexpr int BUFFSIZE = 8192;

		GollyFileReader(std::istream& stream)
			:mStream(stream)
		{


		}


		const char *loadPattern(IAlgorithm& imp);

		std::istream& mStream;
		int   prevchar;

		// use buffered getchar instead of slow fgetc
		// don't override the "getchar" name which is likely to be a macro

		int imp_gridwd = 0;
		int imp_gridht = 0;

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

		// use getline instead of fgets so we can handle DOS/Mac/Unix line endings
		char *getline(char *line, int maxlinelen);


		// Read a text pattern like "...ooo$$$ooo" where '.', ',' and chars <= ' '
		// represent dead cells, '$' represents 10 dead cells, and all other chars
		// represent live cells.
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


		// This function guesses whether `line' is the start of a headerless Life RLE
		// pattern.  It is used to distinguish headerless RLE from plain text patterns.
		static bool isplainrle(const char *line);

		const char *build_err_str(const char *filename)
		{
			static char file_err_str[2048];
			sprintf(file_err_str, "Can't open pattern file:\n%s", filename);
			return file_err_str;
		}

		const char *readPattern(IAlgorithm& imp);

	};

}


#endif // GollyFIile_H_D7CB67EE_25F9_474F_BD06_0695544B25BD
