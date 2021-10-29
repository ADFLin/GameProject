#include "GollyFile.h"

namespace Life
{

	constexpr int LINESIZE = 20000;
	constexpr int CR = 13;
	constexpr int LF = 10;
	constexpr int MAXRULESIZE = 2000; // maximum number of characters in a rule

	const char *SETCELLERROR = "Impossible; set cell error for state 1";

	int GollyFileReader::mgetchar()
	{
		if (buffpos == BUFFSIZE)
		{
			double filepos;

			bytesread = fread(filebuff, 1, BUFFSIZE, pattfile);
			filepos = ftell(pattfile);
			buffpos = 0;
			lifeabortprogress(filepos / filesize, "");
		}
		if (buffpos >= bytesread) return EOF;
		return filebuff[buffpos++];
	}

	// use getline instead of fgets so we can handle DOS/Mac/Unix line endings
	char * GollyFileReader::getline(char *line, int maxlinelen)
	{
		int i = 0;
		while (i < maxlinelen)
		{
			int ch = mgetchar();
			if (isaborted())
				return NULL;
			switch (ch)
			{
			case CR:
				prevchar = CR;
				line[i] = 0;
				return line;
			case LF:
				if (prevchar != CR)
				{
					prevchar = LF;
					line[i] = 0;
					return line;
				}
				// if CR+LF (DOS) then ignore the LF
				break;
			case EOF:
				if (i == 0)
					return NULL;
				line[i] = 0;
				return line;
			default:
				prevchar = ch;
				line[i++] = (char)ch;
				break;
			}
		}
		line[i] = 0;      // silently truncate long line
		return line;
	}

	// Read a text pattern like "...ooo$$$ooo" where '.', ',' and chars <= ' '
	// represent dead cells, '$' represents 10 dead cells, and all other chars
	// represent live cells.
	const char * GollyFileReader::readtextpattern(IAlgorithm &imp, char *line)
	{
		int x = 0, y = 0;
		char *p;

		do {
			for (p = line; *p; p++) {
				if (*p == '.' || *p == ',' || *p <= ' ') {
					x++;
				}
				else if (*p == '$') {
					x += 10;
				}
				else {
					if (imp.setCell(x, y, 1) == false)
					{
						return SETCELLERROR;
					}
					x++;
				}
			}
			y++;
			if (getedges && right < x - 1) right = x - 1;
			x = 0;
		} while (getline(line, LINESIZE));

		if (getedges) bottom = y - 1;
		return 0;
	}

	void GollyFileReader::ParseXRLELine(char *line, int *xoff, int *yoff, bool *sawpos, bigint &gen)
	{
		char *key = line;
		while (key)
		{
			// set key to start of next key word
			while (*key && *key != ' ') key++;
			while (*key == ' ') key++;
			if (*key == 0) return;

			// set value to pos of char after next '='
			char *value = key;
			while (*value && *value != '=') value++;
			if (*value == 0) return;
			value++;

			if (strncmp(key, "Pos", 3) == 0)
			{
				// extract Pos=int,int
				sscanf(value, "%d,%d", xoff, yoff);
				*sawpos = true;

			}
			else if (strncmp(key, "Gen", 3) == 0)
			{
				// extract Gen=bigint
				char *p = value;
				while (*p >= '0' && *p <= '9') p++;
				char savech = *p;
				*p = 0;
				gen = bigint(value);
				*p = savech;
				value = p;
			}

			key = value;
		}
	}

	const char * GollyFileReader::readrle(IAlgorithm& imp, char *line)
	{
		int n = 0, x = 0, y = 0;
		char *p;
		char *ruleptr;
		const char *errmsg = nullptr;
		int wd = 0, ht = 0, xoff = 0, yoff = 0;
		bigint gen = 0;
		bool sawpos = false;             // xoff and yoff set in ParseXRLELine?
		bool sawrule = false;            // saw explicit rule?

		   // parse any #CXRLE line(s) at start
		while (strncmp(line, "#CXRLE", 6) == 0)
		{
			ParseXRLELine(line, &xoff, &yoff, &sawpos, gen);
			//imp.setGeneration(gen);
			if (getline(line, LINESIZE) == NULL) return 0;
		}

		do
		{
			if (line[0] == '#')
			{
				if (line[1] == 'r')
				{
					ruleptr = line;
					ruleptr += 2;
					while (*ruleptr && *ruleptr <= ' ') ruleptr++;
					p = ruleptr;
					while (*p > ' ') p++;
					*p = 0;
					//errmsg = imp.setrule(ruleptr);
					if (errmsg) return errmsg;
					sawrule = true;
				}
				// there's a slight ambiguity here for extended RLE when a line
				// starts with 'x'; we only treat it as a dimension line if the
				// next char is whitespace or '=', since 'x' will only otherwise
				// occur as a two-char token followed by an upper case alphabetic.
			}
			else if (line[0] == 'x' && (line[1] <= ' ' || line[1] == '='))
			{
				// extract wd and ht
				p = line;
				while (*p && *p != '=') p++;
				p++;
				sscanf(p, "%d", &wd);
				while (*p && *p != '=') p++;
				p++;
				sscanf(p, "%d", &ht);

				while (*p && *p != 'r') p++;
				if (strncmp(p, "rule", 4) == 0)
				{
					p += 4;
					while (*p && (*p <= ' ' || *p == '=')) p++;
					ruleptr = p;
					while (*p > ' ') p++;
					// remove any comma at end of rule
					if (p[-1] == ',') p--;
					*p = 0;
					//errmsg = imp.setrule(ruleptr);
					if (errmsg) return errmsg;
					sawrule = true;
				}

				if (!sawrule)
				{
					// if no rule given then try Conway's Life; if it fails then
					// return error so Golly will look for matching algo
					//errmsg = imp.setrule("B3/S23");
					if (errmsg) return errmsg;
				}

				BoundBox bound = imp.getLimitBound();
				int imp_gridwd = 0;
				int imp_gridht = 0;
				if (bound.isValid())
				{
					imp_gridwd = bound.getSize().x;
					imp_gridht = bound.getSize().y;
				}



				// imp.setrule() has set imp.gridwd and imp.gridht
				if (!sawpos && (imp_gridwd > 0 || imp_gridht > 0))
				{
#if 1
					if (wd > 0 && (wd <= (int)imp_gridwd || imp_gridwd == 0) &&
						ht > 0 && (ht <= (int)imp_gridht || imp_gridht == 0)) {
						// pattern size is known and fits within the bounded grid
						// so position pattern in the middle of the grid
						xoff = int(wd / 2);
						yoff = int(ht / 2);
					}
					else
#endif
					{
						// position pattern at top left corner of bounded grid
						// to try and ensure the entire pattern will fit
						xoff = int(imp_gridwd / 2);
						yoff = int(imp_gridht / 2);
					}

				}

				if (getedges)
				{
					top = yoff;
					left = xoff;
					bottom = yoff + ht - 1;
					right = xoff + wd - 1;
				}
			}
			else
			{
				BoundBox bound = imp.getLimitBound();
				int imp_gridwd = 0;
				int imp_gridht = 0;
				if (bound.isValid())
				{
					imp_gridwd = bound.getSize().x;
					imp_gridht = bound.getSize().y;
				}

				int gwd = (int)imp_gridwd;
				int ght = (int)imp_gridht;
				for (p = line; *p; p++)
				{
					char c = *p;
					if ('0' <= c && c <= '9')
					{
						n = n * 10 + c - '0';
					}
					else
					{
						if (n == 0)
							n = 1;
						if (c == 'b' || c == '.')
						{
							x += n;
						}
						else if (c == '$')
						{
							x = 0;
							y += n;
						}
						else if (c == '!')
						{
							return 0;
						}
						else if (('o' <= c && c <= 'y') || ('A' <= c && c <= 'X'))
						{
							int state = -1;
							if (c == 'o')
							{
								state = 1;
							}
							else if (c < 'o')
							{
								state = c - 'A' + 1;
							}
							else
							{
								state = 24 * (c - 'p' + 1);
								p++;
								c = *p;
								if ('A' <= c && c <= 'X')
								{
									state = state + c - 'A' + 1;
								}
								else {
									// return "Illegal multi-char state" ;
									// be more forgiving so we can read non-standard rle files like
									// those at http://home.interserv.com/~mniemiec/lifepage.htm
									state = 1;
									p--;
								}
							}
							// write run of cells to grid checking cells are within any bounded grid
							if (ght == 0 || y < ght)
							{
								while (n-- > 0)
								{
									if (gwd == 0 || x < gwd)
									{
										if (imp.setCell(xoff + x, yoff + y, state) == false)
											return "Cell state out of range for this algorithm";
									}
									x++;
								}
							}
						}
						n = 0;
					}
				}
			}
		} while (getline(line, LINESIZE));

		return 0;
	}

	const char * GollyFileReader::readpclife(IAlgorithm& imp, char *line)
	{
		int x = 0, y = 0;
		int leftx = x;
		char *p;
		char *ruleptr;
		const char *errmsg = nullptr;
		bool sawrule = false;            // saw explicit rule?

		do
		{
			if (line[0] == '#')
			{
				if (line[1] == 'P')
				{
					if (!sawrule)
					{
						// if no rule given then try Conway's Life; if it fails then
						// return error so Golly will look for matching algo
						//errmsg = imp.setrule("B3/S23");
						if (errmsg) return errmsg;
						sawrule = true;      // in case there are many #P lines
					}
					sscanf(line + 2, " %d %d", &x, &y);
					leftx = x;
				}
				else if (line[1] == 'N')
				{
					//errmsg = imp.setrule("B3/S23");
					if (errmsg) return errmsg;
					sawrule = true;
				}
				else if (line[1] == 'R')
				{
					ruleptr = line;
					ruleptr += 2;
					while (*ruleptr && *ruleptr <= ' ') ruleptr++;
					p = ruleptr;
					while (*p > ' ') p++;
					*p = 0;
					//errmsg = imp.setrule(ruleptr);
					if (errmsg) return errmsg;
					sawrule = true;
				}
			}
			else if (line[0] == '-' || ('0' <= line[0] && line[0] <= '9'))
			{
				sscanf(line, "%d %d", &x, &y);
				if (imp.setCell(x, y, 1) == false)
					return SETCELLERROR;
			}
			else if (line[0] == '.' || line[0] == '*')
			{
				for (p = line; *p; p++)
				{
					if (*p == '*')
					{
						if (imp.setCell(x, y, 1) == false)
							return SETCELLERROR;
					}
					x++;
				}
				x = leftx;
				y++;
			}
		} while (getline(line, LINESIZE));

		return 0;
	}

	const char * GollyFileReader::readdblife(IAlgorithm &imp, char *line)
	{
		int n = 0, x = 0, y = 0;
		char *p;

		while (getline(line, LINESIZE))
		{
			if (line[0] != '!')
			{
				// parse line like "23.O15.3O15.3O15.O4.4O"
				n = x = 0;
				for (p = line; *p; p++)
				{
					if ('0' <= *p && *p <= '9')
					{
						n = n * 10 + *p - '0';
					}
					else {
						if (n == 0) n = 1;
						if (*p == '.') {
							x += n;
						}
						else if (*p == 'O')
						{
							while (n-- > 0)
								if (imp.setCell(x++, y, 1) == false)
									return SETCELLERROR;
						}
						else {
							// ignore dblife commands like "5k10h@"
						}
						n = 0;
					}
				}
				y++;
			}
		}
		return 0;
	}

	const char * GollyFileReader::readmcell(IAlgorithm &imp, char *line)
	{
		int x = 0, y = 0;
		int wd = 0, ht = 0;              // bounded if > 0
		int wrapped = 0;                 // plane if 0, torus if 1
		char *p;
		const char *errmsg = nullptr;
		bool sawrule = false;            // saw explicit rule?
		bool extendedHL = false;         // special-case rule translation for extended HistoricalLife rules
		bool useltl = false;             // using a Larger than Life rule?
		char ltlrule[MAXRULESIZE];       // the Larger than Life rule (without any suffix)
		int defwd = 0, defht = 0;        // default grid size for Larger than Life
		int Lcount = 0;                  // number of #L lines seen

		while (getline(line, LINESIZE))
		{
			if (line[0] == '#')
			{
				if (line[1] == 'L' && line[2] == ' ')
				{
					if (!sawrule)
					{
						// if no rule given then try Conway's Life; if it fails then
						// return error so Golly will look for matching algo
						//errmsg = imp.setrule("B3/S23");
						if (errmsg) return errmsg;
						sawrule = true;
					}

					Lcount++;
					if (Lcount == 1 && useltl)
					{
						// call setrule again with correct width and height so we don't need
						// yet another setrule call at the end
						char rule[MAXRULESIZE + 32];
						if (wd == 0 && ht == 0)
						{
							// no #BOARD line was seen so use default size saved earlier
							wd = defwd;
							ht = defht;
						}
						if (wrapped)
							sprintf(rule, "%s:T%d,%d", ltlrule, wd, ht);
						else
							sprintf(rule, "%s:P%d,%d", ltlrule, wd, ht);
						//errmsg = imp.setrule(rule);
						if (errmsg) return errmsg;
					}

					int n = 0;
					for (p = line + 3; *p; p++)
					{
						char c = *p;
						if ('0' <= c && c <= '9')
						{
							n = n * 10 + c - '0';
						}
						else if (c > ' ')
						{
							if (n == 0)
								n = 1;
							if (c == '.')
							{
								x += n;
							}
							else if (c == '$')
							{
								x = -(wd / 2);
								y += n;
							}
							else
							{
								int state = 0;
								if ('a' <= c && c <= 'j')
								{
									state = 24 * (c - 'a' + 1);
									p++;
									c = *p;
								}
								if ('A' <= c && c <= 'X')
								{
									state = state + c - 'A' + 1;
									if (extendedHL)
									{
										// adjust marked states for LifeHistory
										if (state == 8) state = 4;
										else if (state == 3) state = 5;
										else if (state == 5) state = 3;
									}
								}
								else
								{
									return "Illegal multi-char state";
								}
								while (n-- > 0)
								{
									if (imp.setCell(x, y, state) < 0)
									{
										// instead of returning an error message like "Cell state out of range"
										// it's nicer to convert the state to 1 (this is what MCell seems to do
										// for patterns like Bug1.mcl in its LtL collection)
										imp.setCell(x, y, 1);
									}
									x++;
								}
							}
							n = 0;
						}
					}

					// look for Larger than Life
				}
				else if (strncmp(line, "#GAME Larger than Life", 22) == 0)
				{
					useltl = true;

					// look for bounded universe
				}
				else if (strncmp(line, "#BOARD ", 7) == 0)
				{
					sscanf(line + 7, "%dx%d", &wd, &ht);
					// write pattern in top left corner initially
					// (pattern will be shifted to middle of grid below)
					x = -(wd / 2);
					y = -(ht / 2);

					// look for topology
				}
				else if (strncmp(line, "#WRAP ", 6) == 0)
				{
					sscanf(line + 6, "%d", &wrapped);

					// we allow lines like "#GOLLY WireWorld"
				}
				else if (!sawrule && (strncmp(line, "#GOLLY", 6) == 0 ||
					strncmp(line, "#RULE", 5) == 0))
				{
					if (strncmp(line, "#RULE 1,0,1,0,0,0,1,0,0,0,0,0,0,2,2,1,1,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2", 69) == 0) {
						// standard HistoricalLife rule -- all states transfer directly to LifeHistory
						if (strncmp(line, "#RULE 1,0,1,0,0,0,1,0,0,0,0,0,0,2,2,1,1,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,", 70) == 0) {
							// special case:  Brice Due's extended HistoricalLife rules have
							// non-contiguous states (State 8 but no State 4, 6, or 7)
							// that need translating to work in LifeHistory)
							extendedHL = true;
						}
						//errmsg = imp.setrule("LifeHistory");
						if (errmsg) return errmsg;
						sawrule = true;
					}
					else if (strncmp(line, "#RULE 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,1", 40) == 0) {
						//errmsg = imp.setrule("B3/S23");
						//if (errmsg) errmsg = imp.setrule("Life");
						if (errmsg) return errmsg;
						sawrule = true;
					}
					else
					{
						char *ruleptr = line;
						// skip "#GOLLY" or "#RULE"
						ruleptr += line[1] == 'G' ? 6 : 5;
						while (*ruleptr && *ruleptr <= ' ') ruleptr++;
						p = ruleptr;
						while (*p > ' ') p++;
						*p = 0;
						//errmsg = imp.setrule(ruleptr);
						if (errmsg) return errmsg;
						if (useltl)
						{
							// save suffix-less rule string for later use when we see first #L line
							sprintf(ltlrule, "%s", ruleptr);
							// also save default grid size in case there is no #BOARD line

							BoundBox bound = imp.getLimitBound();
							int imp_gridwd = 0;
							int imp_gridht = 0;
							if (bound.isValid())
							{
								imp_gridwd = bound.getSize().x;
								imp_gridht = bound.getSize().y;
							}

							defwd = imp_gridwd;
							defht = imp_gridht;
						}
						sawrule = true;
					}
				}
			}
		}

		if (wd > 0 || ht > 0)
		{
			if (useltl) {
				// setrule has already been called above
			}
			else {
				// grid is bounded, so append suitable suffix to rule
				char rule[MAXRULESIZE];
#if 0
				if (wrapped)
					sprintf(rule, "%s:T%d,%d", imp.getrule(), wd, ht);
				else
					sprintf(rule, "%s:P%d,%d", imp.getrule(), wd, ht);
				errmsg = imp.setrule(rule);
#endif
				if (errmsg) {
					// should never happen
					//lifewarning("Bug in readmcell code!");
					return errmsg;
				}
			}
			// shift pattern to middle of bounded grid
			//imp.endofpattern();
			//if (!imp.isEmpty()) 
			{
				findedges(imp);
				//imp.findedges(&top, &left, &bottom, &right);
				// pattern is currently in top left corner so shift down and right
				// (note that we add 1 to wd and ht to get same position as MCell)
				int shiftx = (wd + 1 - (right - left + 1)) / 2;
				int shifty = (ht + 1 - (bottom - top + 1)) / 2;
				if (shiftx > 0 || shifty > 0)
				{
					for (y = bottom; y >= top; y--)
					{
						for (x = right; x >= left; x--)
						{
							int state = imp.getCell(x, y);
							if (state > 0) {
								imp.setCell(x, y, 0);
								imp.setCell(x + shiftx, y + shifty, state);
							}
						}
					}
				}
			}
		}

		return 0;
	}

	bool GollyFileReader::isplainrle(const char *line)
	{
		// Find end of line, or terminating '!' character, whichever comes first:
		const char *end = line;
		while (*end && *end != '!') ++end;

		// Verify that '!' (if present) is the final printable character:
		if (*end == '!') {
			for (const char *p = end + 1; *p; ++p) {
				if ((unsigned)*p > ' ') {
					return false;
				}
			}
		}

		// Ensure line consists of valid tokens:
		bool prev_digit = false, have_digit = false;
		for (const char *p = line; p != end; ++p)
		{
			if ((unsigned)*p <= ' ')
			{
				if (prev_digit) return false;  // space inside token!
			}
			else if (*p >= '0' && *p <= '9')
			{
				prev_digit = have_digit = true;
			}
			else if (*p == 'b' || *p == 'o' || *p == '$') {
				prev_digit = false;
			}
			else {
				return false;  // unsupported printable character encountered!
			}
		}
		if (prev_digit) return false;  // end of line inside token!

		// Everything seems parseable; assume this is RLE if either we saw some
		// digits, or the pattern ends with a '!', both of which are unlikely to
		// occur in plain text patterns:
		return have_digit || *end == '!';
	}

	const char * GollyFileReader::loadpattern(IAlgorithm &imp)
	{
		char line[LINESIZE + 1];
		const char *errmsg = 0;

		// set rule to Conway's Life (default if explicit rule isn't supplied,
		// such as a text pattern like "...ooo$$$ooo")
#if 0
		const char *err = imp.setrule("B3/S23");
		if (err)
		{
			// try "Life" in case given algo is RuleLoader and a
			// Life.rule file exists (nicer for loading lexicon patterns)
			err = imp.setrule("Life");
			if (err)
			{
				// if given algo doesn't support B3/S23 or Life then the only sensible
				// choice left is to use the algo's default rule
				imp.setrule(imp.DefaultRule());
			}
		}
#endif

		if (getedges)
			lifebeginprogress("Reading from clipboard");
		else
			lifebeginprogress("Reading pattern file");

		// skip any blank lines at start to avoid problem when copying pattern
		// from Internet Explorer
		while (getline(line, LINESIZE) && line[0] == 0);


		auto postReadFile = [&]()
		{
			if (errmsg == 0)
			{
				//imp.endofpattern();
				if (getedges  /*&& !imp.isEmpty()*/)
				{
					findedges(imp);
				}
			}
		};

		// test for 'i' to cater for #LLAB comment in LifeLab file
		if (line[0] == '#' && line[1] == 'L' && line[2] == 'i')
		{
			errmsg = readpclife(imp, line);
			postReadFile();
		}
		else if (line[0] == '#' && line[1] == 'P' && line[2] == ' ')
		{
			// WinLifeSearch creates clipboard patterns similar to
			// Life 1.05 format but without the header line
			errmsg = readpclife(imp, line);
			postReadFile();

		}
		else if (line[0] == '#' && line[1] == 'M' && line[2] == 'C' &&
			line[3] == 'e' && line[4] == 'l' && line[5] == 'l')
		{
			errmsg = readmcell(imp, line);
			postReadFile();

		}
		else if (line[0] == '#' || line[0] == 'x')
		{
			errmsg = readrle(imp, line);
			if (errmsg == 0)
			{
				//imp.endofpattern();
				if (getedges  /* &&!imp.isEmpty()*/)
				{
					// readrle has set top,left,bottom,right based on the info given in
					// the header line and possibly a "#CXRLE Pos=..." line, but in case
					// that info is incorrect we find the true pattern edges and expand
					// top/left/bottom/right if necessary to avoid truncating the pattern
#if 0
					bigint t, l, b, r;
					imp.findedges(&t, &l, &b, &r);
					if (t < top) top = t;
					if (l < left) left = l;
					if (b > bottom) bottom = b;
					if (r > right) right = r;
#endif
				}
			}

		}
		else if (line[0] == '!')
		{
			errmsg = readdblife(imp, line);
			postReadFile();
		}
		else if (line[0] == '[')
		{
			//errmsg = imp.readmacrocell(line);
			postReadFile();

		}
		else if (isplainrle(line))
		{
			errmsg = readrle(imp, line);
			postReadFile();
		}
		else {
			// read a text pattern like "...ooo$$$ooo"
			errmsg = readtextpattern(imp, line);
			if (errmsg == 0)
			{
				//imp.endofpattern();
				// if getedges is true then readtextpattern has set top,left,bottom,right
			}
		}

		lifeendprogress();
		return errmsg;
	}

	const char * GollyFileReader::readpattern(const char *filename, IAlgorithm& imp)
	{
		filesize = getfilesize(filename);
		pattfile = fopen(filename, "r");
		if (pattfile == 0)
			return build_err_str(filename);

		buffpos = BUFFSIZE;                       // for 1st getchar call
		prevchar = 0;                             // for 1st getline call
		const char *errmsg = loadpattern(imp);
		fclose(pattfile);
		return errmsg;
	}

}//namespace Life