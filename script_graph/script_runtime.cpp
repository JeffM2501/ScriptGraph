#include "script_graph.h"


namespace NodeRegistry
{
	struct NodeFactory
	{
		std::function<Node* ()> NewFactory;
		std::function<Node* (void*, size_t)> LoadFactory;

		std::string Name;
	};

	std::map<std::string, NodeFactory> NodeTypeDb;

	void NodeRegistry::RegisterNode(const char* typeName, std::function<Node* ()> newFactory, std::function<Node* (void*, size_t)> loadFactory)
	{
		NodeTypeDb[typeName] = NodeFactory{ newFactory, loadFactory, typeName };
	}

	Node* NodeRegistry::CreateNode(const char* typeName)
	{
		auto itr = NodeTypeDb.find(typeName);
		if (itr == NodeTypeDb.end())
			return nullptr;

		return itr->second.NewFactory();
	}

	Node* NodeRegistry::LoadNode(const char* typeName, void* data, size_t size)
	{
		auto itr = NodeTypeDb.find(typeName);
		if (itr == NodeTypeDb.end())
			return nullptr;

		return itr->second.LoadFactory(data, size);
	}

	void RegisterDefaultNodes()
	{
		RegisterNode<EntryNode>();
		RegisterNode<Condition>();
		RegisterNode<Loop>();
		RegisterNode<BooleanComparison>();
		RegisterNode<NotComparison>();
		RegisterNode<NumberComparison>();
		RegisterNode<Math>();
		RegisterNode<BooleanLiteral>();
		RegisterNode<NumberLiteral>();
		RegisterNode<StringLiteral>();
		RegisterNode<PrintLog>();
		RegisterNode<LoadBool>();
		RegisterNode<SaveBool>();
		RegisterNode<LoadNumber>();
		RegisterNode<SaveNumber>();
		RegisterNode<LoadString>();
		RegisterNode<SaveString>();
	}

	std::vector<std::string> GetNodeList()
	{
		std::vector<std::string> nodes;

		for (const auto& nodeInfo : NodeTypeDb)
		{
			nodes.push_back(nodeInfo.first);
		}	

		return nodes;
	}
}

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
	if (Running)
		return Result::Incomplete;

	Clear();
	Running = true;

	auto itr = Graph.EntryNodes.find(entryPoint);
	if (itr == Graph.EntryNodes.end() || !itr->second)
		return Result::Error;

	CurrentNode = itr->second->ID;

	while (true)
	{
		if(!RunStep())
			break;
	}

	Running = false;
	return Result::Complete;
}

ScriptInstance::Result ScriptInstance::Start(const std::string& entryPoint)
{
	if (Running)
		return Result::Error;

	Clear();
	Running = true;

	auto itr = Graph.EntryNodes.find(entryPoint);
	if (itr == Graph.EntryNodes.end() || !itr->second)
		return Result::Error;

	CurrentNode = itr->second->ID;

	return Step();
}

ScriptInstance::Result ScriptInstance::Step()
{
	if (!Running)
		return Result::Complete;

	if (RunStep())
		return Result::Incomplete;

	Running = false;
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


void ScriptInstance::Clear()
{
	BoolGlobals.clear();
	NumGlobals.clear();
	StringGlobals.clear();
	NodeStateNums.clear();
}
