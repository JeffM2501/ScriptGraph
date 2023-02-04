#include "script_graph.h"


ScriptInstance::ScriptInstance(ScriptGraph& graph)
	: Graph(graph)
{

}

bool ScriptInstance::RunStep()
{
	const NodeRef* nextNode = Graph.Nodes[CurrentNode]->Process(*this);

	if (nextNode == nullptr || nextNode->ID >= Graph.Nodes.size())
	{
		if (ReturnStack.size() > 0)
		{
			CurrentNode = ReturnStack.top();
			ReturnStack.pop();
			return true;
		}

		CurrentNode = uint32_t(-1);
		return false;
	}
		
	CurrentNode = nextNode->ID;
	return true;
}

ScriptInstance::Result ScriptInstance::Run(const std::string& entryPoint)
{
	BoolGlobals.clear();
	NumGlobals.clear();
	StringGlobals.clear();

	auto itr = Graph.EntryNodes.find(entryPoint);
	if (itr == Graph.EntryNodes.end() || !itr->second)
		return Result::Error;

	CurrentNode = itr->second->ID;

	while (true)
	{
		if(!RunStep())
			break;
	}

	return Result::Complete;
}

const ValueData* ScriptInstance::GetValue(const ValueRef& ref)
{
	if (ref.ID > Graph.Nodes.size())
		return nullptr;

	return Graph.Nodes[ref.ID]->GetValue(ref.ValueId, *this);
}

void ScriptInstance::PushReturnNode()
{
	if (CurrentNode <= Graph.Nodes.size())
		ReturnStack.push(CurrentNode);
}
