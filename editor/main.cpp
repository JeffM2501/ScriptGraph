/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

For a C++ project simply rename the file to .cpp and run premake 

*/

#include "raylib.h"
#include "rlImGui.h"
#include "imnodes.h"
#include "imgui.h"
#include "script_graph.h"
#include "extras/IconsFontAwesome5.h"
#include "tinyfiledialogs.h"
#include "NodeGraphEditor.h"

#include <list>

bool Quit = false;
float MenuHeight = 0;
float SidebarSize = 200;
float OutputSize = 200;

std::vector<std::string> NodeRegistryCache;

std::list<std::string> LogLines;

void SaveScript(const ScriptGraph& graph, const std::string& filename)
{
	ScriptResource scriptRes;
	graph.Write(scriptRes);

	FILE* fp = fopen(filename.c_str(), "w+");
	if (!fp)
		return;

	uint32_t count = uint32_t(scriptRes.Nodes.size());

	fwrite(&count, 4, 1, fp);

	for (const auto& res : scriptRes.Nodes)
	{
		fwrite(&res.ID, 4, 1, fp);

		unsigned char isEntry = res.EntryPoint ? 1 : 0;
		fwrite(&isEntry, 1, 1, fp);

		fwrite(res.TypeName, NodeResource::MaxNodeName, 1, fp);
		fwrite(res.Name, NodeResource::MaxNodeName, 1, fp);

		uint32_t size = uint32_t(res.DataSize);
		fwrite(&size, 4, 1, fp);
		fwrite(res.Data, res.DataSize, 1, fp);
	}
	fclose(fp);
}

ScriptGraph LoadScript(const std::string& filename)
{
	ScriptGraph script;
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp)
		return script;

	ScriptResource res;
	uint32_t count = 0;
	fread(&count, 4, 1, fp);

	for (uint32_t i = 0; i < count; i++)
	{
		NodeResource nodeRes;

		fread(&nodeRes.ID, 4, 1, fp);
		unsigned char b = 0;
		fread(&b, 1, 1, fp);
		nodeRes.EntryPoint = b != 0;

		fread(nodeRes.TypeName, NodeResource::MaxNodeName, 1, fp);
		fread(nodeRes.Name, NodeResource::MaxNodeName, 1, fp);

		uint32_t size = 0;
		fread(&size, 4, 1, fp);
		nodeRes.DataSize = size_t(size);
		nodeRes.Data = malloc(size);
		if (nodeRes.Data)
			fread(nodeRes.Data, nodeRes.DataSize, 1, fp);
		else
			break;

		res.Nodes.emplace_back(std::move(nodeRes));
	}

	fclose(fp);

	script.Read(res);
	return script;
}

ScriptGraph TheGraph;
std::string GraphPath;
bool GraphNew = true;

ScriptInstance Instance(TheGraph);

void SetupImGui()
{
	rlImGuiBeginInitImGui();
	ImGui::StyleColorsDark();
	rlImGuiAddFontAwesomeIconFonts(14);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
	ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_Right;
	rlImGuiEndInitImGui();

	ImNodes::CreateContext();
	ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f));
	ImNodes::StyleColorsDark();
	ImNodesIO& io = ImNodes::GetIO();
	io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void SetupScripting()
{
	NodeRegistry::RegisterDefaultNodes();

	NodeRegistryCache = NodeRegistry::GetNodeList();

	AddNodeBody(StringLiteral::GetTypeName(), [](Node* node)
		{
			float y = ImGui::GetCursorPosY();
			StringLiteral* literal = static_cast<StringLiteral*>(node);
			char temp[512];
			std::strncpy(temp, literal->GetValue(), 512);
			ImGui::SetNextItemWidth(125);
			if (ImGui::InputText("###Literal", temp, 512))
				literal->SetValue(temp);
			ImGui::SetCursorPosY(y);
		});
	AddNodeIcon(StringLiteral::GetTypeName(), ICON_FA_FONT);

	AddNodeBody(NumberLiteral::GetTypeName(), [](Node* node)
		{
			NumberLiteral* literal = static_cast<NumberLiteral*>(node);
			float value = literal->GetValue();
			ImGui::SetNextItemWidth(125);
			if (ImGui::InputFloat("###FloatLiteral", &value))
				literal->SetValue(value);
		});
	AddNodeIcon(NumberLiteral::GetTypeName(), ICON_FA_HASHTAG);

	AddNodeBody(BooleanLiteral::GetTypeName(), [](Node* node)
		{
			BooleanLiteral* literal = static_cast<BooleanLiteral*>(node);
			bool value = literal->GetValue();
			ImGui::SetNextItemWidth(125);
			if (ImGui::BeginCombo("###Value", value ? "TRUE" : "FALSE"))
			{
				if (ImGui::Selectable("TRUE", value))
					literal->SetValue(true);

				if (ImGui::Selectable("FALSE", !value))
					literal->SetValue(false);

				ImGui::EndCombo();
			}
		});

	AddNodeIcon(BooleanLiteral::GetTypeName(), ICON_FA_CHECK);

	AddNodeBody(Loop::GetTypeName(), [](Node* node)
		{
			Loop* loop = static_cast<Loop*>(node);
			ImGui::BeginDisabled(loop->Arguments[0].ID != uint32_t(-1));
			ImGui::SetNextItemWidth(50);
			ImGui::InputScalar("Itterations", ImGuiDataType_U32, &loop->Itterations);
			ImGui::EndDisabled();
		});

	AddNodeIcon(Loop::GetTypeName(), ICON_FA_RECYCLE);

	PrintLog::LogFunction = [](const std::string& text)
	{
		while (LogLines.size() > 100)
			LogLines.erase(LogLines.begin());

		LogLines.push_back(text);
	};
	AddNodeIcon(PrintLog::GetTypeName(), ICON_FA_TERMINAL);
	AddNodeIcon(EntryNode::GetTypeName(), ICON_FA_ARROW_RIGHT);

	AddNodeBody(BooleanComparison::GetTypeName(), [](Node* node)
	{
		BooleanComparison* comp = static_cast<BooleanComparison*>(node);
		const char* value = BooleanComparison::ToString(comp->Operator);
		ImGui::SetNextItemWidth(125);
		if (ImGui::BeginCombo("###Value", value))
		{
			DoForEachEnum<BooleanComparison::Operation>([comp](BooleanComparison::Operation op)
				{
					const char* value = BooleanComparison::ToString(op);
					if (ImGui::Selectable(value, op == comp->Operator))
						comp->Operator = op;
				});
			ImGui::EndCombo();
		}
	});

	AddNodeBody(NumberComparison::GetTypeName(), [](Node* node)
		{
			NumberComparison* comp = static_cast<NumberComparison*>(node);
			const char* value = NumberComparison::ToString(comp->Operator);
			ImGui::SetNextItemWidth(175);
			if (ImGui::BeginCombo("###Value", value))
			{
				DoForEachEnum<NumberComparison::Operation>([comp](NumberComparison::Operation op)
					{
						const char* value = NumberComparison::ToString(op);
						if (ImGui::Selectable(value, op == comp->Operator))
							comp->Operator = op;
					});
				ImGui::EndCombo();
			}
		});

	AddNodeIcon(BooleanComparison::GetTypeName(), ICON_FA_EQUALS);
	AddNodeIcon(NumberComparison::GetTypeName(), ICON_FA_EQUALS);
	AddNodeIcon(NotComparison::GetTypeName(), ICON_FA_EXCLAMATION);

	AddNodeIcon(Math::GetTypeName(), ICON_FA_PLUS);
	AddNodeIcon(Condition::GetTypeName(), ICON_FA_QUESTION);

	AddNodeIcon(LoadBool::GetTypeName(), ICON_FA_UPLOAD);
	AddNodeIcon(LoadNumber::GetTypeName(), ICON_FA_UPLOAD);
	AddNodeIcon(LoadString::GetTypeName(), ICON_FA_UPLOAD);

	AddNodeIcon(SaveBool::GetTypeName(), ICON_FA_DOWNLOAD);
	AddNodeIcon(SaveNumber::GetTypeName(), ICON_FA_DOWNLOAD);
	AddNodeIcon(SaveString::GetTypeName(), ICON_FA_DOWNLOAD);
}

void DoMainMenu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New"))
			{
				GraphNew = true;
				TheGraph.EntryNodes.clear();
				TheGraph.Nodes.clear();
				GraphPath.clear();
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Open"))
			{
				char const* lFilterPatterns[2] = { "*.script", "*.*" };

				const char* fileName = tinyfd_openFileDialog(
					"Load Script",
					"*.script",
					2,
					lFilterPatterns,
					"Script Files",
					0);

				if (fileName != nullptr)
				{
					GraphPath = fileName;
					TheGraph = LoadScript(GraphPath);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save"))
			{
				if (GraphPath.empty())
				{
					char const* lFilterPatterns[2] = { "*.script", "*.*" };

					const char* fileName = tinyfd_openFileDialog(
						"Save Script",
						GraphPath.c_str(),
						2,
						lFilterPatterns,
						"Script Files",
						0);

					if (fileName != nullptr)
					{
						GraphPath = fileName;
					}
				}

				if (!GraphPath.empty())
					SaveScript(TheGraph, GraphPath);
			}

			if (ImGui::MenuItem("Save As"))
			{
				char const* lFilterPatterns[2] = { "*.script", "*.*" };

				const char* fileName = tinyfd_openFileDialog(
					"Save Script",
					GraphPath.c_str(),
					2,
					lFilterPatterns,
					"Script Files",
					0);

				if (fileName != nullptr)
				{
					GraphPath = fileName;
					SaveScript(TheGraph, GraphPath);
				}
			}

			if (ImGui::MenuItem("Exit"))
				Quit = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Script"))
		{
			if (ImGui::MenuItem("Run"))
			{
				if (!Instance.Running)
					Instance.Start(TheGraph.EntryNodes.begin()->first);
			}
			ImGui::EndMenu();
		}

		MenuHeight = ImGui::GetWindowHeight();
		ImGui::EndMainMenuBar();
	}
}

void ShowNodes()
{
	std::map<uint64_t, std::pair<uint32_t, int>> EventNodeIdMap;

	auto& style = ImGui::GetStyle();

	ImGui::SetNextWindowPos(ImVec2(0, MenuHeight + style.FramePadding.y));
	ImGui::SetNextWindowSize(ImVec2(GetScreenWidth() - (SidebarSize + style.FramePadding.x), GetScreenHeight() - OutputSize));

	if (Instance.Running)
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4{ 1,0,0,1 });

	if (ImGui::Begin(ICON_FA_SITEMAP "  NodeGraph", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		if (Instance.Running)
			ImGui::PopStyleColor();

		ShowGraphEditor(TheGraph, GraphNew);
		GraphNew = false;
	}
	else
	{
		if (Instance.Running)
			ImGui::PopStyleColor();
	}

	ImGui::End();
}

void ShowSidebar()
{
	auto& style = ImGui::GetStyle();

	float width = (SidebarSize - 2);
	float height = GetScreenHeight() - MenuHeight - style.FramePadding.y - 2;

	ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() - SidebarSize - 1, MenuHeight + style.FramePadding.y));
	ImGui::SetNextWindowSize(ImVec2(width, height));

	if (ImGui::Begin(ICON_FA_PUZZLE_PIECE "  Nodes", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		if (ImGui::BeginChild("NodePalette"))
		{
			for (size_t i = 0; i < NodeRegistryCache.size(); ++i)
			{
				const auto& node = NodeRegistryCache[i];
				std::string label = GetNodeIcon(node);
				label += " " + node;

				bool selected = false;
				ImGui::Selectable(label.c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("NODE", node.c_str(), node.size() + 1 , ImGuiCond_Always);
					ImGui::TextUnformatted(label.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
			};

			ImGui::EndChild();
		}
	}

	ImGui::End();
}

void ShowOutput()
{
	auto& style = ImGui::GetStyle();

	float height = (OutputSize - MenuHeight - style.CellPadding.y - 2);

	ImGui::SetNextWindowPos(ImVec2(0, GetScreenHeight() - height));
	ImGui::SetNextWindowSize(ImVec2(GetScreenWidth() - (SidebarSize + style.FramePadding.x), height - 1));

	if (ImGui::Begin(ICON_FA_PRINT "  Output", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		for (auto itr = LogLines.rbegin(); itr != LogLines.rend(); itr++)
		{
			ImGui::TextUnformatted(itr->c_str());
		}
	}

	ImGui::End();
}

void ShowEditorUI()
{
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
	ShowNodes();
	ShowOutput();
	ShowSidebar();
	ImGui::PopStyleColor();
}

void SetupGraph()
{
	EntryNode* entry = new EntryNode();
	entry->Name = "Entry";
	entry->ID = 0;
	entry->OutputNodeRefs[0].ID = 1;
	entry->NodePosX = 10;
	entry->NodePosY = 10;

	TheGraph.Nodes[0] = (entry);
	TheGraph.EntryNodes[entry->Name] = entry;

	Loop* loop = new Loop();
	loop->ID = 1;
	loop->Itterations = 1000;
	loop->OutputNodeRefs[0].ID = 5;
	loop->OutputNodeRefs[1].ID = 2;

	loop->NodePosX = 300;
	loop->NodePosY = 10;

	TheGraph.Nodes[1] = (loop);

	PrintLog* log = new PrintLog();
	log->ID = 2;
	log->Arguments[0].ID = 3;
	log->Arguments[0].ValueId = 0;

	log->NodePosX = 530;
	log->NodePosY = 128;
	TheGraph.Nodes[2] = (log);

	StringLiteral* literal = new StringLiteral("Loop Cycle");
	literal->ID = 3;
	literal->NodePosX = 47;
	literal->NodePosY = 145;
	TheGraph.Nodes[3] = (literal);

	StringLiteral* endLiteral = new StringLiteral("Loop Complete");
	endLiteral->ID = 4;
	endLiteral->NodePosX = 517;
	endLiteral->NodePosY = 320;
	TheGraph.Nodes[4] = (endLiteral);

	log = new PrintLog();
	log->ID = 5;
	log->Arguments[0].ID = 4;
	log->Arguments[0].ValueId = 0;
	log->NodePosX = 825;
	log->NodePosY = 10;
	TheGraph.Nodes[5] = (log);
}


int main ()
{
	// set up the window
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
	InitWindow(1280, 800, "Graph Editor");
	SetTargetFPS(144);

	SetupScripting();

	// test graph
	SetupGraph();

	SetupImGui();
	
	// game loop
	while (!WindowShouldClose() && !Quit)
	{
		if (Instance.Running)
		{
			switch (Instance.Step())
			{
				case ScriptInstance::Result::Complete:
					LogLines.push_back("Script Complete");
					break;

				case ScriptInstance::Result::Error:
					LogLines.push_back("Script Error");
					break;
			}
		}

		// drawing
		BeginDrawing();
		ClearBackground(BLACK);

		rlImGuiBegin();
		DoMainMenu();
		ShowEditorUI();

		rlImGuiEnd();
		EndDrawing();
	}

	ImNodes::DestroyContext();
	rlImGuiShutdown();
	// cleanup
	CloseWindow();
	return 0;
}