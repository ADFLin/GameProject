#include "ConsolePanel.h"

#include "ConsoleSystem.h"
#include "EditorUtils.h"

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
	int numCommands = ConsoleSystem::Get().getAllCommandNames(commands, ARRAY_SIZE(commands));
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

void ConsolePanel::render(const char* title, bool* p_open)
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
	// So e.g. IsItemHovered() will return true when hovering the title bar.
	// Here we create a context menu only available from the title bar.
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Close Console"))
			*p_open = false;
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
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
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
	if (copy_to_clipboard)
		ImGui::LogToClipboard();

	ImGuiListClipper clipper;
	clipper.Begin(Items.size());
	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			auto& item = Items[i];
			const char* itemContent = Items[i].content.c_str();
			if (!Filter.PassFilter(itemContent))
				continue;

			ImGui::PushStyleColor(ImGuiCol_Text, FImGuiConv::To(item.color));
			ImGui::TextUnformatted(itemContent);
			ImGui::PopStyleColor();
		}
	}
	if (copy_to_clipboard)
		ImGui::LogFinish();

	if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		ImGui::SetScrollHereY(1.0f);
	ScrollToBottom = false;

	ImGui::PopStyleVar();
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

	ImGui::End();
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
	else if (!ConsoleSystem::Get().executeCommand(command_line))
	{
		AddLog("Unknown command: '%s'\n", command_line);
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;
}
