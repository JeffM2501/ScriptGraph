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
	int VisualNodeId = -1;
	int EntryPlugId = -1;

	std::vector<NodeCachePinInfo> ExitPlugIds;
	std::vector<NodeCachePinInfo> ArgumentPlugIds;
	std::vector<NodeCachePinInfo> ValuePlugIds;

	int GetExitPlugIndex(int pinId)
	{
		for (int i = 0; i < int(ExitPlugIds.size()); i++)
		{
			if (ExitPlugIds[i].PinId == pinId)
				return i;
		}
		return -1;
	}

	int GetArgumentPlugIndex(int pinId)
	{
		for (int i = 0; i < int(ArgumentPlugIds.size()); i++)
		{
			if (ArgumentPlugIds[i].PinId == pinId)
				return i;
		}
		return -1;
	}

	int GetValuePlugIndex(int pinId)
	{
		for (int i = 0; i < int(ValuePlugIds.size()); i++)
		{
			if (ValuePlugIds[i].PinId == pinId)
				return i;
		}
		return -1;
	}
};

std::map<uint32_t, NodeCacheItem> NodeCache;

std::map<std::string, std::function<void(Node*)>> BodyCallbacks;
std::map<std::string, std::string> NodeIcons;

std::vector<std::pair<uint32_t, int>> PinCache;
std::vector<uint32_t> LinkNodeCache;

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

	const char* icon = ICON_FA_PUZZLE_PIECE;
	auto iconItr = NodeIcons.find(node->TypeName());
	if (iconItr != NodeIcons.end())
		icon = iconItr->second.c_str();

	char label[128] = { 0 };
	std::snprintf(label, 128, "%s %s", icon, node->Name.empty() ? node->TypeName() : node->Name.c_str());

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

void DrawLinks(ScriptGraph& graph)
{
	int linkID = 0;

	// draw the links
	for (auto& [i,node] : graph.Nodes)
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
				LinkNodeCache.push_back(node->ID);
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
				LinkNodeCache.push_back(node->ID);
			}
		}
	}
}

void ProcessNewLinks(ScriptGraph& graph)
{
	// add links
	int startPin = -1;
	int endPin = -1;
	bool fromSnap = false;

	if (ImNodes::IsLinkCreated(&startPin, &endPin, &fromSnap))
	{
		// figure out what the start pin is
		const auto& startPinInfo = PinCache[startPin];
		auto& startNodeInfo = NodeCache[startPinInfo.first];

		const auto& endPinInfo = PinCache[endPin];
		auto& endNodeInfo = NodeCache[endPinInfo.first];

		if (startPin == startNodeInfo.EntryPlugId)
		{
			// must go to an exit pin
			int destIndex = endNodeInfo.GetExitPlugIndex(endPin);
			if (destIndex >= 0)
			{
				graph.Nodes[endPinInfo.first]->OutputNodeRefs[destIndex].ID = startPinInfo.first;
			}
		}

		int exitPin = startNodeInfo.GetExitPlugIndex(startPin);
		if (exitPin >= 0)
		{
			// must go to a start pin
			if (endPin == endNodeInfo.EntryPlugId)
			{
				graph.Nodes[startPinInfo.first]->OutputNodeRefs[exitPin].ID = endPinInfo.first;
			}
		}

		int valuePin = startNodeInfo.GetValuePlugIndex(startPin);
		if (valuePin >= 0)
		{
			// must go to an argument pin
			int destIndex = endNodeInfo.GetArgumentPlugIndex(endPin);
			if (destIndex >= 0)
			{
				// verify compatible types
			//	if (graph.Nodes[endPinInfo.first]->Arguments[destIndex].RefType == graph.Nodes[startPinInfo.first]->Values[valuePin].Type)
				{
					graph.Nodes[endPinInfo.first]->Arguments[destIndex].ID = startPinInfo.first;
					graph.Nodes[endPinInfo.first]->Arguments[destIndex].ValueId = valuePin;
				}
			}
		}

		int argumentPin = startNodeInfo.GetArgumentPlugIndex(startPin);
		if (argumentPin >= 0)
		{
			// must go to a value pin
			int destIndex = endNodeInfo.GetValuePlugIndex(endPin);
			if (destIndex >= 0)
			{
				// verify compatible types
			//	if (graph.Nodes[endPinInfo.first]->Values[destIndex].Type == graph.Nodes[startPinInfo.first]->Arguments[argumentPin].RefType)
				{
					graph.Nodes[startPinInfo.first]->Arguments[argumentPin].ID = endPinInfo.first;
					graph.Nodes[startPinInfo.first]->Arguments[argumentPin].ValueId = destIndex;
				}
			}
		}
	}
}

void ProcessRemovedLinks(ScriptGraph& graph)
{
	int deadLink = -1;
	if (!ImNodes::IsLinkDestroyed(&deadLink) || (deadLink < 0 || deadLink >= LinkNodeCache.size()))
		return;

	Node* node = graph.Nodes[LinkNodeCache[deadLink]];
	auto& nodeCacheItem = NodeCache[node->ID];

	// see what kind of link it is, exit or argument
	for (size_t i = 0; i < nodeCacheItem.ExitPlugIds.size(); i++)
	{
		if (nodeCacheItem.ExitPlugIds[i].LinkId == deadLink)
		{
			node->OutputNodeRefs[i].ID = uint32_t(-1);
			return;
		}
	}

	for (size_t i = 0; i < nodeCacheItem.ArgumentPlugIds.size(); i++)
	{
		if (nodeCacheItem.ArgumentPlugIds[i].LinkId == deadLink)
		{
			node->Arguments[i].ID = uint32_t(-1);
			return;
		}
	}
}

void AddNodeAtCursorPos(const std::string& nodeName, ScriptGraph& graph)
{
	ImVec2 mousePos = ImGui::GetMousePos();

	Node* node = NodeRegistry::CreateNode(nodeName.c_str());

	node->NodePosX = mousePos.x - WindowOrigin.x - 50;
	node->NodePosY = mousePos.y - WindowOrigin.y - 25;

	uint32_t id = graph.AddNode(node);

	NewNodes.insert(id);
}

void HandleNodeDrag(ScriptGraph& graph)
{
	ImVec2 mousePos = ImGui::GetMousePos();

	if (ImGui::BeginDragDropTarget())
	{
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE");
		if (payload != nullptr && payload->IsDelivery())
		{
			const char* name = (const char*)payload->Data;

			AddNodeAtCursorPos(name, graph);
		}
		ImGui::EndDragDropTarget();
	}
}

void ShowGraphEditor(ScriptGraph& graph, bool forcePositions)
{
	WindowOrigin = ImGui::GetWindowPos();

	ImNodes::BeginNodeEditor();

	PinCache.clear();
	LinkNodeCache.clear();

	int index = 0;
	for (auto& [i,node] : graph.Nodes)
		ShowNode(node, index, forcePositions || NewNodes.find(i) != NewNodes.end());

	DrawLinks(graph);
	
	ImNodes::EndNodeEditor();

	ProcessNewLinks(graph);

	ProcessRemovedLinks(graph);

	NewNodes.clear();
	HandleNodeDrag(graph);
}

void AddNodeBody(const std::string& name, std::function<void(Node*)> callback)
{
	if (callback != nullptr)
		BodyCallbacks[name] = callback;
}

void AddNodeIcon(const std::string& name, const std::string& Icon)
{
	NodeIcons[name] = Icon;
}

const char* GetNodeIcon(const std::string& name)
{
	auto itr = NodeIcons.find(name);
	if (itr == NodeIcons.end())
		return ICON_FA_PUZZLE_PIECE;

	return itr->second.c_str();
}
