#pragma once
#ifndef ConsolePanel_H_0DD392E0_8A9A_4858_96E4_9C6538691D1F
#define ConsolePanel_H_0DD392E0_8A9A_4858_96E4_9C6538691D1F

#include "EditorPanel.h"

#include "LogSystem.h"
#include "Core/Color.h"
#include "DataStructure/Array.h"

#include <string>

#include "ImGui/imgui.h"



class ConsolePanel : public IEditorPanel
				   , public LogOutput
{
public:
	char                  InputBuf[256];

	struct LogLine
	{
		LogLine( char const* str , Color4f const& color = Color4f(1,1,1,1))
		:content(str), color(color){}
		Color4f     color;
		std::string content;
	};
	TArray<LogLine>     Items;
	TArray<const char*> mCommands;
	TArray<std::string> History;
	int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
	ImGuiTextFilter       Filter;
	bool                  AutoScroll;
	bool                  ScrollToBottom;

	ConsolePanel();
	~ConsolePanel();

	static void  Strtrim(char* s) { char* str_end = s + FCString::Strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

	void    ClearLog()
	{
		Items.clear();
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.emplace_back(buf);
	}

	void    AddColorLog(const char* fmt, Color4f color , ...) IM_FMTARGS(3)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.emplace_back(buf,color);
	}

	void  render();

	void    ExecCommand(const char* command_line);

	// In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
	{
		ConsolePanel* console = (ConsolePanel*)data->UserData;
		return console->TextEditCallback(data);
	}

	int     TextEditCallback(ImGuiInputTextCallbackData* data)
	{
		//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackCompletion:
			{
				// Example of TEXT COMPLETION

				// Locate beginning of current word
				const char* word_end = data->Buf + data->CursorPos;
				const char* word_start = word_end;
				while (word_start > data->Buf)
				{
					const char c = word_start[-1];
					if (c == ' ' || c == '\t' || c == ',' || c == ';')
						break;
					word_start--;
				}

				// Build a list of candidates
				ImVector<const char*> candidates;
				for (int i = 0; i < mCommands.size(); i++)
				{
					if (FCString::CompareIgnoreCaseN(mCommands[i], word_start, (int)(word_end - word_start)) == 0)
						candidates.push_back(mCommands[i]);
				}

				if (candidates.Size == 0)
				{
					// No match
					AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
				}
				else if (candidates.Size == 1)
				{
					// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
					data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
					data->InsertChars(data->CursorPos, candidates[0]);
					data->InsertChars(data->CursorPos, " ");
				}
				else
				{
					// Multiple matches. Complete as much as we can..
					// So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
					int match_len = (int)(word_end - word_start);
					for (;;)
					{
						int c = 0;
						bool all_candidates_matches = true;
						for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
							if (i == 0)
								c = toupper(candidates[i][match_len]);
							else if (c == 0 || c != toupper(candidates[i][match_len]))
								all_candidates_matches = false;
						if (!all_candidates_matches)
							break;
						match_len++;
					}

					if (match_len > 0)
					{
						data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
						data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
					}

					// List matches
					AddLog("Possible matches:\n");
					for (int i = 0; i < candidates.Size; i++)
						AddLog("- %s\n", candidates[i]);
				}

				break;
			}
		case ImGuiInputTextFlags_CallbackHistory:
			{
				// Example of HISTORY
				const int prev_history_pos = HistoryPos;
				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (HistoryPos == -1)
						HistoryPos = History.size() - 1;
					else if (HistoryPos > 0)
						HistoryPos--;
				}
				else if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (HistoryPos != -1)
						if (++HistoryPos >= History.size())
							HistoryPos = -1;
				}

				// A better implementation would preserve the data on the current input line along with cursor position.
				if (prev_history_pos != HistoryPos)
				{
					const char* history_str = (HistoryPos >= 0) ? History[HistoryPos].c_str() : "";
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, history_str);
				}
			}
		}
		return 0;
	}

	//LogOutput
	virtual bool filterLog(LogChannel channel, int level) override
	{
		return true;
	}
	virtual void receiveLog(LogChannel channel, char const* str) override
	{

		switch (channel)
		{
		case LOG_MSG:
			AddColorLog(str, Color4f(1, 1, 1, 1));
			break;
		case LOG_DEV:
			AddColorLog(str, Color4f(0, 0, 1, 1));
			break;
		case LOG_WARNING:
			AddColorLog(str, Color4f(1, 1, 0, 1));
			break;
		case LOG_ERROR:
			AddColorLog(str, Color4f(1, 0, 0, 1));
			break;
		}

	}
	//~LogOutput
};


#endif // ConsolePanel_H_0DD392E0_8A9A_4858_96E4_9C6538691D1F