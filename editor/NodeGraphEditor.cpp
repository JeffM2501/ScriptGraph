#include "script_graph.h"
#include "NodeGraphEditor.h"
#include "imnodes.h"
#include "imgui.h"
#include "extras/IconsFontAwesome5.h"
#include <set>
#include <string>
#include <functional>

// the window origin so we can map the mouse cursor correctly
ImVec2 WindowOrigin(0, 0);

// nodes that were created last frame and need to be forced into position
std::set<uint32_t> NewNodes;

// a map of every graph node to what screen node/plugs it uses

struct NodeCachePinInfo
{
	int PinId;
	int LinkId;
};

struct NodeCacheItem
{
	std::string Icon = ICON_FA_PUZZLE_PIECE;
	int VisualNodeId = -1;
	int EntryPlugId = -1;

	std::vector<NodeCachePinInfo> ExitPlugIds;
	std::vector<NodeCachePinInfo> ArgumentPlugIds;
	std::vector<NodeCachePinInfo> ValuePlugIds;
};

std::map<uint32_t, NodeCacheItem> NodeCache;

std::map<std::string, std::function<void(Node*)>> BodyCallbacks;

std::vector<std::pair<uint32_t, int>> PinCache;

const char* GetValueTypeName(ValueTypes valType)
{
	switch (valType)
	{
		case ValueTypes::Boolean:
			return "BOOL";
		case ValueTypes::Number:
			return "NUM";
		case ValueTypes::String:
			return "STR";
		default:
			return "UNKNOWN";
	}
}

void ShowNode(Node* node, int& index, bool forcePositions)
{
	if (!node)
		return;
	int screeNodeId = node->ID;

	auto& nodeCacheItem = NodeCache[node->ID];

	nodeCacheItem.VisualNodeId = node->ID;

	std::string icon = ICON_FA_PUZZLE_PIECE;

	char label[128] = { 0 };
	std::snprintf(label, 128, "%s %s", icon.c_str(), node->Name.empty() ? node->TypeName() : node->Name.c_str());

	if (forcePositions)
		ImNodes::SetNodeEditorSpacePos(screeNodeId, ImVec2(node->NodePosX, node->NodePosY));

	ImNodes::BeginNode(screeNodeId);
	ImNodes::BeginNodeTitleBar();
	ImGui::Text(label);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Node Id %d\n", screeNodeId, node->TypeName());
	ImNodes::EndNodeTitleBar();

	float width = ImGui::CalcTextSize(label).x;
	if (width < 150)
		width = 150;

	int localPinIndex = 0;

	ImVec2 argTop = ImGui::GetCursorPos();

	if (node->AllowInput)
	{
		nodeCacheItem.EntryPlugId = index;

		ImNodes::BeginInputAttribute(index++, ImNodesPinShape_Triangle);
		ImGui::Text("In");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Pin Id %d", index - 1);
		PinCache.push_back(std::make_pair(node->ID, localPinIndex++));
		ImNodes::EndInputAttribute();
	}
	ImGui::SetCursorPos(argTop);
	nodeCacheItem.ExitPlugIds.clear();

	for (size_t exitPlug = 0; exitPlug < node->OutputNodeRefs.size(); exitPlug++)
	{
		NodeRef& ref = node->OutputNodeRefs[exitPlug];

		ImNodesPinShape shape = ImNodesPinShape_Triangle;
		if (ref.ID != uint32_t(-1))
			shape = ImNodesPinShape_TriangleFilled;

		nodeCacheItem.ExitPlugIds.emplace_back(NodeCachePinInfo{ index, -1});

		ImNodes::BeginOutputAttribute(index++, shape);
		float labelSize = ImGui::CalcTextSize(ref.Name.c_str()).x + 5;

		ImGui::Indent(width - labelSize);
		ImGui::Text("%s", ref.Name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Pin Id %d", index - 1);
		PinCache.push_back(std::make_pair(node->ID, localPinIndex++));
		ImNodes::EndOutputAttribute();
	}
	
	auto itr = BodyCallbacks.find(node->TypeName());
	if (itr != BodyCallbacks.end())
		itr->second(node);

	// save the pos so we can get back here
	nodeCacheItem.ArgumentPlugIds.clear();

	argTop = ImGui::GetCursorPos();

	for (size_t argumentPlug = 0; argumentPlug < node->Arguments.size(); argumentPlug++)
	{
		auto& argRef = node->Arguments[argumentPlug];

		ImNodesPinShape shape = ImNodesPinShape_Circle;
		if (argRef.ID != uint32_t(-1))
			shape = ImNodesPinShape_CircleFilled;

		nodeCacheItem.ArgumentPlugIds.emplace_back(NodeCachePinInfo{ index, -1 });
		ImNodes::BeginInputAttribute(index++, shape);
		ImGui::Text("%s", argRef.Name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Pin Id %d", index - 1);
		PinCache.push_back(std::make_pair(node->ID, localPinIndex++));
		ImNodes::EndInputAttribute();
	}

	ImGui::SetCursorPos(argTop);
	nodeCacheItem.ValuePlugIds.clear();
	for (size_t valuePlug = 0; valuePlug < node->Values.size(); valuePlug++)
	{
		auto& valueRef = node->Values[valuePlug];

		ImNodesPinShape shape = ImNodesPinShape_QuadFilled;

		nodeCacheItem.ValuePlugIds.emplace_back(NodeCachePinInfo{ index, -1 });

		ImNodes::BeginOutputAttribute(index++, shape);

		std::string  name = valueRef.Name;
		float labelSize = ImGui::CalcTextSize(name.c_str()).x;
		ImGui::Indent(width - labelSize);
		ImGui::Text("%s", name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s\nPin Id %d", GetValueTypeName(valueRef.Type), index - 1);
		PinCache.push_back(std::make_pair(node->ID, localPinIndex++));
		ImNodes::EndOutputAttribute();
	}
	ImNodes::EndNode();
	
	ImVec2 nodePos = ImNodes::GetNodeEditorSpacePos(screeNodeId);
	if (nodePos.x != node->NodePosX || nodePos.y != node->NodePosY)
	{
		// TODO flag dirty

		node->NodePosX = nodePos.x;
		node->NodePosY = nodePos.y;
	}
}

void ShowGraphEditor(ScriptGraph& graph, bool forcePositions)
{
	WindowOrigin = ImGui::GetWindowPos();

	ImNodes::BeginNodeEditor();

	PinCache.clear();

	int index = 0;
	for (auto& node : graph.Nodes)
	{
		ShowNode(node, index, forcePositions);
	}

	int linkID = 0;

	// draw the links
	for (auto& node : graph.Nodes)
	{
		auto& nodeCacheItem = NodeCache[node->ID];

		// exit plug links
		for (size_t exitPlug = 0; exitPlug < node->OutputNodeRefs.size(); exitPlug++)
		{
			NodeRef& ref = node->OutputNodeRefs[exitPlug];

			if (ref.ID != uint32_t(-1))
			{
				int sourcePlugId = nodeCacheItem.ExitPlugIds[exitPlug].PinId;

				int targetPlugId = NodeCache[ref.ID].EntryPlugId;
				
				nodeCacheItem.ExitPlugIds[exitPlug].LinkId = linkID;

				ImNodes::Link(linkID++, sourcePlugId, targetPlugId);
			}
		}

		// argument links
		for (size_t argumentPlug = 0; argumentPlug < node->Arguments.size(); argumentPlug++)
		{
			auto& ref = node->Arguments[argumentPlug];

			if (ref.ID != uint32_t(-1))
			{
				int sourcePlugId = nodeCacheItem.ArgumentPlugIds[argumentPlug].PinId;

				int targetPlugId = NodeCache[ref.ID].ValuePlugIds[ref.ValueId].PinId;

				nodeCacheItem.ArgumentPlugIds[argumentPlug].LinkId = linkID;

				ImNodes::Link(linkID++, sourcePlugId, targetPlugId);
			}
		}
	}
	ImNodes::EndNodeEditor();

	// add links
	int startPin = -1;
	int endPin = -1;
	bool fromSnap = false;

	if (ImNodes::IsLinkCreated(&startPin, &endPin, &fromSnap))
	{
// 		auto& startInfo = NodePlugMap.find(startPin);
// 		auto& endInfo = NodePlugMap.find(endPin);
// 
// 		// connection from exit to exit
// 		if (startInfo->second.second >= 0 && endInfo->second.second >= 0)
// 			return;
// 
// 		// connection from start to start
// 		if (startInfo->second.second < 0 && endInfo->second.second < 0)
// 			return;
// 
// 		uint32_t startNode = startInfo->second.first;
// 		int startPlugIndex = startInfo->second.second;
// 		uint32_t targetNode = endInfo->second.first;
// 
// 		// ensure that the link starts from an exit node
// 		if (startPlugIndex < 0)
// 		{
// 			startNode = endInfo->second.first;
// 			startPlugIndex = endInfo->second.second;
// 
// 			targetNode = startInfo->second.first;
// 		}
// 
// 		auto* sourceNode = NodeGraph.GetNode(startNode);
// 		sourceNode->SetPlug(startPlugIndex, targetNode);
	}


}

void AddNodeBody(const std::string& name, std::function<void(Node*)> callback)
{
	if (callback != nullptr)
		BodyCallbacks[name] = callback;
}