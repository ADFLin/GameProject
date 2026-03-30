#include "ConsolePanel.h"

#include "ConsoleSystem.h"
#include "EditorUtils.h"
#include <algorithm>

REGISTER_EDITOR_PANEL(ConsolePanel, "Console", true, true);

ConsolePanel::ConsolePanel()
{
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPos = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	mCommands.push_back("#HELP");
	mCommands.push_back("#HISTORY");
	mCommands.push_back("#CLEAR");
	mCommands.push_back("#CLASSIFY");

	char const* commands[256];
	int numCommands = IConsoleSystem::Get().getAllCommandNames(commands, ARRAY_SIZE(commands));
	for (int i = 0; i < numCommands; ++i)
	{
		mCommands.push_back(commands[i]);
	}

	AutoScroll = true;
	ScrollToBottom = false;
	AddLog("Welcome to Dear ImGui!");

	addDefaultChannels();

}

ConsolePanel::~ConsolePanel()
{
	ClearLog();
}

void ConsolePanel::update()
{

	// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
	// So e.g. IsItemHovered() will return true when hovering the title bar.
	// Here we create a context menu only available from the title bar.
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Close Console"))
		{
			//*p_open = false;
		}
		ImGui::EndPopup();
	}


	// TODO: display items starting from the bottom
	if (ImGui::SmallButton("Add Debug Text"))
	{
		AddLog("%d some text", Items.size());
		AddLog("some more text");
		AddLog("display very important message here!");
	}

	ImGui::SameLine();
	if (ImGui::SmallButton("Add Debug Error"))
	{
		AddColorLog("[error] something went wrong", Color4f(1.0f, 0.4f, 0.4f, 1.0f));
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear")) { ClearLog(); }
	ImGui::SameLine();
	bool copy_to_clipboard = ImGui::SmallButton("Copy");
	//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

	ImGui::Separator();

	// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &AutoScroll);
		ImGui::EndPopup();
	}

	// Options, Filter
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
	ImGui::Separator();

	// Reserve enough left-over height for 1 separator + 1 input text
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
	{

		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets.
		// If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
		// to only process visible items. The clipper will automatically measure the height of your first item and then
		// "seek" to display only items in the visible area.
		// To use the clipper we can replace your standard loop:
		//      for (int i = 0; i < Items.Size; i++)
		//   With:
		//      ImGuiListClipper clipper;
		//      clipper.Begin(Items.Size);
		//      while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// - That your items are evenly spaced (same height)
		// - That you have cheap random access to your elements (you can access them given their index,
		//   without processing all the ones before)
		// You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
		// We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
		// or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage
		// to improve this example code!
		// If your items are of variable height:
		// - Split them into same height items would be simpler and facilitate random-seeking into your list.
		// - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

		static int selection_l_start = -1, selection_c_start = -1;
		static int selection_l_end = -1, selection_c_end = -1;
		static bool is_dragging = false;

		auto GetCharIdx = [](const char* text, float local_x) -> int
		{
			int len = (int)strlen(text);
			if (local_x <= 0) return 0;
			float x = 0;
			for (int i = 0; i < len; ++i)
			{
				float char_w = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, &text[i], &text[i + 1]).x;
				if (x + char_w * 0.5f > local_x) return i;
				x += char_w;
			}
			return len;
		};

		if (ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
		{
			if (selection_l_start != -1 && selection_l_end != -1)
			{
				int l_start = selection_l_start, c_start = selection_c_start;
				int l_end = selection_l_end, c_end = selection_c_end;
				if (l_start > l_end || (l_start == l_end && c_start > c_end))
				{
					std::swap(l_start, l_end);
					std::swap(c_start, c_end);
				}

				std::string selected_text;
				for (int i = l_start; i <= l_end; ++i)
				{
					if (Filter.PassFilter(Items[i].content.c_str()))
					{
						const char* line = Items[i].content.c_str();
						int start = (i == l_start) ? c_start : 0;
						int end = (i == l_end) ? c_end : (int)strlen(line);
						if (start < end)
						{
							selected_text.append(line + start, line + end);
						}
						if (i < l_end) selected_text += "\n";
					}
				}
				if (!selected_text.empty())
					ImGui::SetClipboardText(selected_text.c_str());
			}
		}

		if (copy_to_clipboard)
		{
			ImGui::LogToClipboard();
			for (auto const& item : Items)
			{
				if (Filter.PassFilter(item.content.c_str()))
				{
					ImGui::LogText("%s\n", item.content.c_str());
				}
			}
			ImGui::LogFinish();
		}

		if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			selection_l_start = selection_l_end = -1;
			selection_c_start = selection_c_end = -1;
			is_dragging = true;
		}

		if (is_dragging)
		{
			if (!ImGui::IsMouseDown(0))
			{
				is_dragging = false;
			}
			else
			{
				ImVec2 mouse_pos = ImGui::GetMousePos();
				ImVec2 win_pos = ImGui::GetWindowPos();
				float win_h = ImGui::GetWindowHeight();

				// Auto-scroll
				if (mouse_pos.y < win_pos.y + 20) ImGui::SetScrollY(ImGui::GetScrollY() - 200.0f * ImGui::GetIO().DeltaTime);
				if (mouse_pos.y > win_pos.y + win_h - 20) ImGui::SetScrollY(ImGui::GetScrollY() + 200.0f * ImGui::GetIO().DeltaTime);

				// Clamping end selection when mouse is out of window
				if (Items.size() > 0)
				{
					if (mouse_pos.y < win_pos.y && selection_l_start != -1)
					{
						selection_l_end = 0;
						selection_c_end = 0;
					}
					else if (mouse_pos.y > win_pos.y + win_h && selection_l_start != -1)
					{
						selection_l_end = Items.size() - 1;
						selection_c_end = (int)Items.back().content.size();
					}
				}
			}
		}

		ImGuiListClipper clipper;
		clipper.Begin(Items.size());
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				auto& item = Items[i];
				const char* itemContent = item.content.c_str();
				if (Filter.PassFilter(itemContent))
				{
					ImGui::PushStyleColor(ImGuiCol_Text, FImGuiConv::To(item.color));
					ImGui::PushID(i);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
					ImGui::Selectable(itemContent, false, ImGuiSelectableFlags_SpanAllColumns);
					ImGui::PopStyleColor(2);

					if (is_dragging)
					{
						ImVec2 mouse_pos = ImGui::GetMousePos();
						ImVec2 item_min = ImGui::GetItemRectMin();
						ImVec2 item_max = ImGui::GetItemRectMax();
						if (mouse_pos.y >= item_min.y && mouse_pos.y < item_max.y)
						{
							int char_idx = GetCharIdx(itemContent, mouse_pos.x - item_min.x);
							if (ImGui::IsMouseClicked(0))
							{
								selection_l_start = selection_l_end = i;
								selection_c_start = selection_c_end = char_idx;
							}
							else
							{
								selection_l_end = i;
								selection_c_end = char_idx;
							}
						}
					}

					if (selection_l_start != -1)
					{
						int l_start = selection_l_start, c_start = selection_c_start;
						int l_end = selection_l_end, c_end = selection_c_end;
						if (l_start > l_end || (l_start == l_end && c_start > c_end))
						{
							std::swap(l_start, l_end);
							std::swap(c_start, c_end);
						}

						if (i >= l_start && i <= l_end)
						{
							int start = (i == l_start) ? c_start : 0;
							int end = (i == l_end) ? c_end : (int)strlen(itemContent);
							if (start < end || (i > l_start && i < l_end))
							{
								ImVec2 min = ImGui::GetItemRectMin();
								float x_start = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, itemContent, itemContent + start).x;
								float x_end = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, itemContent, itemContent + end).x;
								ImGui::GetWindowDrawList()->AddRectFilled(
									ImVec2(min.x + x_start, min.y),
									ImVec2(min.x + x_end, ImGui::GetItemRectMax().y),
									ImGui::GetColorU32(ImGuiCol_TextSelectedBg, 0.5f)
								);
							}
						}
					}

					ImGui::PopStyleColor();

					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::Selectable("Copy"))
						{
							if (selection_l_start != -1)
							{
								int l_start = selection_l_start, c_start = selection_c_start;
								int l_end = selection_l_end, c_end = selection_c_end;
								if (l_start > l_end || (l_start == l_end && c_start > c_end)) { std::swap(l_start, l_end); std::swap(c_start, c_end); }

								std::string selected_text;
								for (int n = l_start; n <= l_end; ++n)
								{
									if (Filter.PassFilter(Items[n].content.c_str()))
									{
										const char* line = Items[n].content.c_str();
										int start = (n == l_start) ? c_start : 0;
										int end = (n == l_end) ? c_end : (int)strlen(line);
										if (start < end) selected_text.append(line + start, line + end);
										if (n < l_end) selected_text += "\n";
									}
								}
								if (!selected_text.empty()) ImGui::SetClipboardText(selected_text.c_str());
							}
							else
							{
								ImGui::SetClipboardText(itemContent);
							}
						}
						if (ImGui::Selectable("Clear")) { ClearLog(); }
						ImGui::EndPopup();
					}
					ImGui::PopID();
				}
			}
		}

		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

	ImGui::SetNextItemWidth(ImGui::GetContentRegionMax().x - 40);
	if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
	{
		char* s = InputBuf;
		Strtrim(s);
		if (s[0])
			ExecCommand(s);
		strcpy(s, "");
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

}

void ConsolePanel::ExecCommand(const char* command_line)
{
	AddColorLog("# %s\n", Color4f(1.0f, 0.8f, 0.6f, 1.0f), command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.size() - 1; i >= 0; i--)
	{
		if (FCString::CompareIgnoreCase(History[i].c_str(), command_line) == 0)
		{
			History.erase(History.begin() + i);
			break;
		}
	}
	History.push_back(command_line);
	if (command_line[0] == '#')
	{
		++command_line;
		switch (FEditor::Hash(command_line))
		{
		case FEditor::Hash("CLEAR"):
			{
				ClearLog();
			}
			break;
		case FEditor::Hash("HELP"):
			{
				AddLog("Commands:");
				for (int i = 0; i < mCommands.size(); i++)
					AddLog("- %s", mCommands[i]);
			}
			break;
		case FEditor::Hash("HISTORY"):
			{
				int first = History.size() - 10;
				for (int i = first > 0 ? first : 0; i < History.size(); i++)
					AddLog("%3d: %s\n", i, History[i]);
			}
			break;
		default:
			AddLog("Unknown command: '%s'\n", command_line);
		}
	}
	else if (!IConsoleSystem::Get().executeCommand(command_line))
	{
		AddLog("Unknown command: '%s'\n", command_line);
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;
}
