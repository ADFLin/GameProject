#include "LifeStage.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"

namespace Life
{
	REGISTER_STAGE_ENTRY("Game of Life", TestStage, EExecGroup::Dev4, "Game");

	TConsoleVariable< bool > CVarUseRenderer{ true, "Life.UseRenderer", CVF_TOGGLEABLE };


#if 0
	class FileReader
	{
	public:
		using bigint = int64;

#define LINESIZE 20000
#define CR 13
#define LF 10

		bool getedges = false;              // find pattern edges?
		bigint top, left, bottom, right;    // the pattern edges

#define BUFFSIZE 8192


#ifdef ZLIB
		gzFile zinstream;
#else
		FILE *pattfile;
#endif

		char filebuff[BUFFSIZE];
		int buffpos, bytesread, prevchar;

		long filesize;             // length of file in bytes

		// use buffered getchar instead of slow fgetc
		// don't override the "getchar" name which is likely to be a macro
		int mgetchar()
		{
			if (buffpos == BUFFSIZE) {
				double filepos;
#ifdef ZLIB
				bytesread = gzread(zinstream, filebuff, BUFFSIZE);
#if ZLIB_VERNUM >= 0x1240
				// gzoffset is only available in zlib 1.2.4 or later
				filepos = gzoffset(zinstream);
#else
				// use an approximation of file position if file is compressed
				filepos = gztell(zinstream);
				if (filepos > 0 && gzdirect(zinstream) == 0) filepos /= 4;
#endif
#else
				bytesread = fread(filebuff, 1, BUFFSIZE, pattfile);
				filepos = ftell(pattfile);
#endif
				buffpos = 0;
				lifeabortprogress(filepos / filesize, "");
			}
			if (buffpos >= bytesread) return EOF;
			return filebuff[buffpos++];
		}

		// use getline instead of fgets so we can handle DOS/Mac/Unix line endings
		char *getline(char *line, int maxlinelen)
		{
			int i = 0;
			while (i < maxlinelen) {
				int ch = mgetchar();
				if (isaborted()) return NULL;
				switch (ch) {
				case CR:
					prevchar = CR;
					line[i] = 0;
					return line;
				case LF:
					if (prevchar != CR) {
						prevchar = LF;
						line[i] = 0;
						return line;
					}
					// if CR+LF (DOS) then ignore the LF
					break;
				case EOF:
					if (i == 0) return NULL;
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

		const char *SETCELLERROR = "Impossible; set cell error for state 1";

		// Read a text pattern like "...ooo$$$ooo" where '.', ',' and chars <= ' '
		// represent dead cells, '$' represents 10 dead cells, and all other chars
		// represent live cells.
		const char *readtextpattern(IAlgorithm &imp, char *line)
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

		/*
		 *   Parse "#CXRLE key=value key=value ..." line and extract values.
		 */
		void ParseXRLELine(char *line, int *xoff, int *yoff, bool *sawpos, bigint &gen)
		{
			char *key = line;
			while (key) {
				// set key to start of next key word
				while (*key && *key != ' ') key++;
				while (*key == ' ') key++;
				if (*key == 0) return;

				// set value to pos of char after next '='
				char *value = key;
				while (*value && *value != '=') value++;
				if (*value == 0) return;
				value++;

				if (strncmp(key, "Pos", 3) == 0) {
					// extract Pos=int,int
					sscanf(value, "%d,%d", xoff, yoff);
					*sawpos = true;

				}
				else if (strncmp(key, "Gen", 3) == 0) {
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

		const char *readrle(IAlgorithm& imp, char *line)
		{
			int n = 0, x = 0, y = 0;
			char *p;
			char *ruleptr;
			const char *errmsg;
			int wd = 0, ht = 0, xoff = 0, yoff = 0;
			bigint gen = 0;
			bool sawpos = false;             // xoff and yoff set in ParseXRLELine?
			bool sawrule = false;            // saw explicit rule?

			   // parse any #CXRLE line(s) at start
			while (strncmp(line, "#CXRLE", 6) == 0) {
				ParseXRLELine(line, &xoff, &yoff, &sawpos, gen);
				//imp.setGeneration(gen);
				if (getline(line, LINESIZE) == NULL) return 0;
			}

			do {
				if (line[0] == '#') 
				{
					if (line[1] == 'r') {
						ruleptr = line;
						ruleptr += 2;
						while (*ruleptr && *ruleptr <= ' ') ruleptr++;
						p = ruleptr;
						while (*p > ' ') p++;
						*p = 0;
						errmsg = imp.setrule(ruleptr);
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
					if (strncmp(p, "rule", 4) == 0) {
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

					if (!sawrule) {
						// if no rule given then try Conway's Life; if it fails then
						// return error so Golly will look for matching algo
						//errmsg = imp.setrule("B3/S23");
						if (errmsg) return errmsg;
					}

					// imp.setrule() has set imp.gridwd and imp.gridht
					if (!sawpos && (imp.gridwd > 0 || imp.gridht > 0)) 
					{
						if (wd > 0 && (wd <= (int)imp.gridwd || imp.gridwd == 0) &&
							ht > 0 && (ht <= (int)imp.gridht || imp.gridht == 0)) {
							// pattern size is known and fits within the bounded grid
							// so position pattern in the middle of the grid
							xoff = -int(wd / 2);
							yoff = -int(ht / 2);
						}
						else 
						{
							// position pattern at top left corner of bounded grid
							// to try and ensure the entire pattern will fit
							xoff = -int(imp.gridwd / 2);
							yoff = -int(imp.gridht / 2);
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
					int gwd = (int)imp.gridwd;
					int ght = (int)imp.gridht;
					for (p = line; *p; p++) {
						char c = *p;
						if ('0' <= c && c <= '9') {
							n = n * 10 + c - '0';
						}
						else {
							if (n == 0)
								n = 1;
							if (c == 'b' || c == '.') {
								x += n;
							}
							else if (c == '$') {
								x = 0;
								y += n;
							}
							else if (c == '!') {
								return 0;
							}
							else if (('o' <= c && c <= 'y') || ('A' <= c && c <= 'X')) {
								int state = -1;
								if (c == 'o')
									state = 1;
								else if (c < 'o') {
									state = c - 'A' + 1;
								}
								else {
									state = 24 * (c - 'p' + 1);
									p++;
									c = *p;
									if ('A' <= c && c <= 'X') {
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
	};
#endif

#define LIFE_DIR "LifeGame"

	bool TestStage::savePattern(char const* name)
	{
		if (!FFileSystem::IsExist(LIFE_DIR))
		{
			FFileSystem::CreateDirectorySequence(LIFE_DIR);
		}

		OutputFileSerializer serializer;
		if (!serializer.open(InlineString<>::Make(LIFE_DIR"/%s.bin", name)))
		{
			LogWarning(0, "Can't Open Pattern File : %s", name);
			return false;
		}

		//serializer.registerVersion("LevelVersion", ELevelSaveVersion::LastVersion);

		PatternData data;
		mAlgorithm->getPattern(data.posList);
		serializer.write(data);
		return true;
	}

	bool TestStage::loadPattern(char const* name)
	{
		InputFileSerializer serializer;
		if (!serializer.open(InlineString<>::Make(LIFE_DIR"/%s.bin", name)))
		{
			LogWarning(0, "Can't Open Pattern File : %s", name);
			return false;
		}

		PatternData data;
		serializer.read(data);

		mAlgorithm->clearCell();
		for (auto const& pos : data.posList)
		{
			mAlgorithm->setCell(pos.x, pos.y, 1);
		}

		return true;
	}

	void TestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

		g.beginRender();

		RenderUtility::SetPen(g, EColor::Null);
		RenderUtility::SetBrush(g, EColor::Gray, COLOR_DEEP);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		Vector2 cellSize = Vector2(::Global::GetScreenSize()).div(mViewport.zoomOut * mViewport.size);
		float cellScale = cellSize.x;

		bool bDrawGridLine = true;
		float gridlineAlpha = 1.0f;
		float GridLineFadeStart = 10;
		if (cellScale < 3.0)
		{
			bDrawGridLine = false;
		}
		else if (cellScale < GridLineFadeStart)
		{
			gridlineAlpha = Remap(cellScale, 3.0, GridLineFadeStart, 0, 1);

		}


		g.pushXForm();
		g.transformXForm(mViewport.xform, true);


		BoundBox boundRender;
		Vector2 cellMinVP = mViewport.pos - 0.5 * (mViewport.zoomOut * mViewport.size);
		boundRender.min.x = Math::FloorToInt(cellMinVP.x);
		boundRender.min.y = Math::FloorToInt(cellMinVP.y);
		Vector2 cellMaxVP = mViewport.pos + 0.5 * (mViewport.zoomOut * mViewport.size);
		boundRender.max.x = Math::CeilToInt(cellMaxVP.x);
		boundRender.max.y = Math::CeilToInt(cellMaxVP.y);

		if (bDrawGridLine)
		{
			if (gridlineAlpha != 1.0)
			{
				g.beginBlend(gridlineAlpha);
			}
			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::White);
			Vector2 offsetH = Vector2(boundRender.getSize().x, 0);
			for (int j = boundRender.min.y; j <= boundRender.max.y; ++j)
			{
				Vector2 pos = Vector2(boundRender.min.x, j);
				g.drawLine(pos, pos + offsetH);
			}

			Vector2 offsetV = Vector2(0, boundRender.getSize().y);
			for (int i = boundRender.min.x; i <= boundRender.max.x; ++i)
			{
				Vector2 pos = Vector2(i, boundRender.min.y);
				g.drawLine(pos, pos + offsetV);
			}

			if (gridlineAlpha != 1.0)
			{
				g.endBlend();
			}


		}

		{
			RenderUtility::SetPen(g, EColor::Yellow);
			Vector2 offsetH = Vector2(boundRender.getSize().x, 0);
			for (int j = ChunkAlgo::ChunkLength * ( (boundRender.min.y + ChunkAlgo::ChunkLength - 1 ) / ChunkAlgo::ChunkLength ); j <= boundRender.max.y; j += ChunkAlgo::ChunkLength)
			{
				Vector2 pos = Vector2(boundRender.min.x, j);
				g.drawLine(pos, pos + offsetH);
			}

			Vector2 offsetV = Vector2(0, boundRender.getSize().y);
			for (int i = ChunkAlgo::ChunkLength * ( (boundRender.min.x + ChunkAlgo::ChunkLength - 1 ) / ChunkAlgo::ChunkLength ); i <= boundRender.max.x; i += ChunkAlgo::ChunkLength)
			{
				Vector2 pos = Vector2(i, boundRender.min.y);
				g.drawLine(pos, pos + offsetV);
			}
		}

		RenderUtility::SetPen(g, (cellScale > 3.0) ? EColor::Black : EColor::Null);
		RenderUtility::SetBrush(g, EColor::White);
		IRenderer* renderer = mAlgorithm->getRenderer();
		if (CVarUseRenderer && renderer)
		{
			renderer->draw(g, mViewport, boundRender);
		}
		else
		{

			BoundBox boundCellRender = boundRender.intersection(mAlgorithm->getBound());

			if (boundCellRender.isValid())
			{
				for (int j = boundCellRender.min.y; j < boundCellRender.max.y; ++j)
				{
					for (int i = boundCellRender.min.x; i < boundCellRender.max.x; ++i)
					{
						uint8 value = mAlgorithm->getCell(i, j);
						if (value)
						{
							g.drawRect(Vector2(i, j), Vector2(1, 1));
						}
					}
				}
			}
		}


		g.popXForm();

		g.endRender();
	}

}//namespace Life