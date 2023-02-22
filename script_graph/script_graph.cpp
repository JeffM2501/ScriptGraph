// C library
#define _CRT_SECURE_NO_WARNINGS

#include "script_graph.h"
#include <memory>

std::string NumberValueData::Text;
float StringValueData::ValueF = 0;

void Node::Read(void* data, size_t size, size_t& offset)
{
	AllowInput = ReadBool(data, size, offset);
	
	uint32_t outNodes = ReadUInt(data, size, offset);
	if (outNodes > OutputNodeRefs.size())
		outNodes = (uint32_t)OutputNodeRefs.size();
	for (uint32_t i = 0; i < outNodes; i++)
		OutputNodeRefs[i].ID = ReadUInt(data, size, offset);

	uint32_t args = ReadUInt(data, size, offset);
	if (args > Arguments.size())
		args = (uint32_t)Arguments.size();
	for (uint32_t i = 0; i < args; i++)
		Arguments[i].ID = ReadUInt(data, size, offset);

	NodePosX = ReadFloat(data, size, offset);
	NodePosY = ReadFloat(data, size, offset);
}

size_t Node::GetDataSize()
{
	size_t size = 1; // allow input
	size += 4; //  outputNode size;
	size += 4; // argument size
	size += 4; // values size;
	size += 8; // node pos

	size += OutputNodeRefs.size() * sizeof(uint32_t);
	size += Arguments.size() * sizeof(uint32_t);

	return size;
}

bool Node::Write(void* data, size_t& offset)
{
	WriteBool(AllowInput, data, offset);

	WriteUInt(OutputNodeRefs.size(), data, offset);
	for (const auto& value : OutputNodeRefs)
		WriteUInt(value.ID, data, offset);

	WriteUInt(Arguments.size(), data, offset);
	for (const auto& value : Arguments)
		WriteUInt(value.ID, data, offset);

	WriteFloat(NodePosX, data, offset);
	WriteFloat(NodePosY, data, offset);

	return true;
}

void Node::WriteBool(bool value, void* data, size_t& offset)
{
	unsigned char* buffer = static_cast<unsigned char*>(data) + offset;
	*buffer = value ? 1 : 0;
	offset++;
}

void Node::WriteUInt(uint32_t value, void* data, size_t& offset)
{
	uint32_t* buffer = (uint32_t*)((char*)data + offset);
	offset += 4;
	*buffer = value;
}

void Node::WriteUInt(size_t value, void* data, size_t& offset)
{
	WriteUInt(static_cast<uint32_t>(value), data, offset);
}

void Node::WriteFloat(float value, void* data, size_t& offset)
{
	float* buffer = (float*)((char*)data + offset);
	offset += 4;
	*buffer = value;
}

void Node::WriteString(const std::string& value, void* data, size_t& offset)
{
	WriteUInt(value.size(), data, offset);
	memcpy((char*)data + offset, value.c_str(), value.size());
	offset += value.size();
}

bool Node::ReadBool(void* data, size_t size, size_t& offset)
{
	if (offset >= size)
		return false;

	unsigned char* buffer = static_cast<unsigned char*>(data) + offset;
	offset++;
	return *buffer != 0;
}

uint32_t Node::ReadUInt(void* data, size_t size, size_t& offset)
{
	if (offset >= size)
		return 0;

	uint32_t* buffer = (uint32_t*)((char*)data + offset);
	offset += 4;
	return *buffer;
}

float Node::ReadFloat(void* data, size_t size, size_t& offset)
{
	if (offset >= size)
		return 0;

	float* buffer = (float*)((char*)data + offset);
	offset += 4;
	return *buffer;
}

std::string Node::ReadString(void* data, size_t size, size_t& offset)
{
	uint32_t len = ReadUInt(data, size, offset);
	std::string value((char*)data + offset, len);
	offset += len;
	return value;
}

EntryNode::EntryNode()
{
	AllowInput = false;
	OutputNodeRefs.emplace_back("Out");
}

const NodeRef* EntryNode::Process(ScriptInstance& state)
{
	return &OutputNodeRefs[0];
}

Condition::Condition()
{
	OutputNodeRefs.emplace_back("True");
	OutputNodeRefs.emplace_back("False");

	Arguments.emplace_back(ValueTypes::Boolean, "Condition");
}

const NodeRef* Condition::Process(ScriptInstance& state)
{
	const auto* val = state.GetValue(Arguments[0]);
	if (val == nullptr)
		return nullptr;

	if (val->Boolean())
		return &OutputNodeRefs[0];
	else
		return &OutputNodeRefs[1];
}

Loop::Loop()
	: IndexValue(0)
{
	OutputNodeRefs.emplace_back("Complete");
	OutputNodeRefs.emplace_back("Cycle");

	Arguments.emplace_back(ValueTypes::Boolean, "Condition");

	Values.emplace_back(ValueTypes::Number, "Index", 0);
}

const NodeRef* Loop::Process(ScriptInstance& state)
{
	auto indexItr = state.NodeStateNums.find(ID);

	uint32_t index = 0;
	if (indexItr != state.NodeStateNums.end())
		index = indexItr->second + 1;

	state.NodeStateNums[ID] = index;

	if (Itterations > 0 && index >= Itterations)
		return &OutputNodeRefs[0];

	auto* a = state.GetValue(Arguments[0]);
	if ((a && !a->Boolean()) || (!a && Itterations < 0))
		return &OutputNodeRefs[0];

	state.PushReturnNode();
	return &OutputNodeRefs[1];
}

const ValueData* Loop::GetValue(uint32_t id, ScriptInstance& state)
{
	auto indexItr = state.NodeStateNums.find(ID);

	if (indexItr != state.NodeStateNums.end())
		IndexValue.Value = float(indexItr->second);
	else
		IndexValue.Value = 0;

	return &IndexValue;
}

void Loop::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	Itterations = ReadUInt(data, size, offset);
}

size_t Loop::GetDataSize()
{
	return Node::GetDataSize() + 4;
}

bool Loop::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteUInt(Itterations, data, offset);
	return true;
}

BooleanComparison::BooleanComparison(Operation op)
	: Operator(op)
	, ReturnValue(false)
{
	AllowInput = false;

	Arguments.emplace_back(ValueTypes::Boolean, "A");
	Arguments.emplace_back(ValueTypes::Boolean, "B");

	Values.emplace_back(ValueTypes::Boolean, "Result", 0);
}

const ValueData* BooleanComparison::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* a = state.GetValue(Arguments[0]);
	auto* b = state.GetValue(Arguments[1]);

	if (a && b)
	{
		if (Operator == Operation::AND)
			ReturnValue.Value = a->Boolean() && b->Boolean();
		else
			ReturnValue.Value = a->Boolean() || b->Boolean();
	}

	return &ReturnValue;
}

void BooleanComparison::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	Operator = (Operation)ReadUInt(data, size, offset);
}

size_t BooleanComparison::GetDataSize()
{
	return Node::GetDataSize() + 4;
}

bool BooleanComparison::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteUInt((uint32_t)(Operator), data, offset);
	return true;
}

NotComparison::NotComparison()
	: ReturnValue(false)
{
	AllowInput = false;

	Arguments.emplace_back(ValueTypes::Boolean, "Input");

	Values.emplace_back(ValueTypes::Boolean, "Result", 0);
}

const ValueData* NotComparison::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* in = state.GetValue(Arguments[0]);

	ReturnValue.Value = in && !in->Boolean();
	return &ReturnValue;
}

NumberComparison::NumberComparison(Operation op) 
	: Operator(op)
	, ReturnValue(false)
{
	AllowInput = false;

	Arguments.emplace_back(ValueTypes::Number, "A");
	Arguments.emplace_back(ValueTypes::Number, "B");

	Values.emplace_back(ValueTypes::Boolean, "Result", 0);
}

const ValueData* NumberComparison::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* a = state.GetValue(Arguments[0]);
	auto* b = state.GetValue(Arguments[1]);

	ReturnValue.Value = false;

	if (a && b)
	{
		switch (Operator)
		{
			case NumberComparison::Operation::GreaterThan:
				ReturnValue.Value = a->Number() > b->Number();
				break;
			case NumberComparison::Operation::GreaterThanEqual:
				ReturnValue.Value = a->Number() >= b->Number();
				break;
			case NumberComparison::Operation::LessThan:
				ReturnValue.Value = a->Number() < b->Number();
				break;
			case NumberComparison::Operation::LessThankEqual:
				ReturnValue.Value = a->Number() <= b->Number();
				break;
			case NumberComparison::Operation::Equal:
				ReturnValue.Value = a->Number() == b->Number();
				break;
			case NumberComparison::Operation::NotEqual:
				ReturnValue.Value = a->Number() != b->Number();
				break;
		}
	}

	return &ReturnValue;
}

void NumberComparison::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	Operator = (Operation)ReadUInt(data, size, offset);
}

size_t NumberComparison::GetDataSize()
{
	return Node::GetDataSize() + 4;
}

bool NumberComparison::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteUInt((uint32_t)(Operator), data, offset);
	return true;
}

Math::Math(Operation op)
	: Operator(op)
	, ReturnValue(0)
{
	AllowInput = false;

	Arguments.emplace_back(ValueTypes::Number, "A");
	Arguments.emplace_back(ValueTypes::Number, "B");

	Values.emplace_back(ValueTypes::Number, "Result", 0);
}

const ValueData* Math::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* a = state.GetValue(Arguments[0]);
	auto* b = state.GetValue(Arguments[1]);

	ReturnValue.Value = 0;

	if (a && b)
	{
		switch (Operator)
		{
			case Math::Operation::Add:
				ReturnValue.Value = a->Number() + b->Number();
				break;
			case Math::Operation::Subtract:
				ReturnValue.Value = a->Number() - b->Number();
				break;
			case Math::Operation::Multiply:
				ReturnValue.Value = a->Number() * b->Number();
				break;
			case Math::Operation::Divide:
				ReturnValue.Value = a->Number() / b->Number();
				break;
			case Math::Operation::Modulo:
				ReturnValue.Value = float(int(a->Number()) % int(b->Number()));
				break;
			case Math::Operation::Pow:
				ReturnValue.Value = powf(a->Number(), b->Number());
				break;
		}
	}

	return &ReturnValue;
}

void Math::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	Operator = (Operation)ReadUInt(data, size, offset);
}

size_t Math::GetDataSize()
{
	return Node::GetDataSize() + 4;
}

bool Math::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteUInt((uint32_t)(Operator), data, offset);
	return true;
}

BooleanLiteral::BooleanLiteral(bool value)
	: ReturnValue(value)
{
	AllowInput = false;
	Values.emplace_back(ValueTypes::Boolean, "", 0);
}

const ValueData* BooleanLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

void BooleanLiteral::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	ReturnValue.Value = ReadBool(data, size, offset);
}

size_t BooleanLiteral::GetDataSize()
{
	return Node::GetDataSize() + 1;
}

bool BooleanLiteral::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteBool(ReturnValue.Value, data, offset);
	return true;
}

NumberLiteral::NumberLiteral(float value)
	: ReturnValue(value)
{
	AllowInput = false;
	Values.emplace_back(ValueTypes::Number, "", 0);
}

const ValueData* NumberLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

void NumberLiteral::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	ReturnValue.Value = ReadFloat(data, size, offset);
}

size_t NumberLiteral::GetDataSize()
{
	return Node::GetDataSize() + 4;
}

bool NumberLiteral::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteFloat(ReturnValue.Value, data, offset);
	return true;
}

StringLiteral::StringLiteral(const std::string& value)
	: ReturnValue(value)
{
	AllowInput = false;
	Values.emplace_back(ValueTypes::String, "", 0);
}

const ValueData* StringLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

void StringLiteral::Read(void* data, size_t size, size_t& offset)
{
	Node::Read(data, size, offset);
	ReturnValue.Value = ReadString(data, size, offset);
}

size_t StringLiteral::GetDataSize()
{
	return Node::GetDataSize() + GetStringDataSize(ReturnValue.Value);
}

bool StringLiteral::Write(void* data, size_t& offset)
{
	Node::Write(data, offset);
	WriteString(ReturnValue.Value, data, offset);
	return true;
}

PrintLog::PrintLog()
{
	OutputNodeRefs.emplace_back("Out");

	Arguments.emplace_back(ValueTypes::String, "Text");
}

const NodeRef* PrintLog::Process(ScriptInstance& state)
{
	auto* text = state.GetValue(Arguments[0]);

	if (text)
		printf("%s\n", text->String().c_str());

	return &OutputNodeRefs[0];
}

LoadBool::LoadBool()
	: ReturnValue(false)
{
	AllowInput = false;
	Arguments.emplace_back(ValueTypes::String, "VariableName");

	Values.emplace_back(ValueTypes::Boolean, "Value", 0);
}

const ValueData* LoadBool::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);

	if (name)
		ReturnValue.Value = state.BoolGlobals[name->String()];

	return &ReturnValue;
}

SaveBool::SaveBool()
{
	OutputNodeRefs.emplace_back("Out");

	Arguments.emplace_back(ValueTypes::String, "VariableName");
	Arguments.emplace_back(ValueTypes::Boolean, "Value");
}

const NodeRef* SaveBool::Process(ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);
	auto* value = state.GetValue(Arguments[1]);

	if (name)
		state.BoolGlobals[name->String()] = value->Boolean();

	return &OutputNodeRefs[0];
}

LoadNumber::LoadNumber()
	: ReturnValue(0)
{
	AllowInput = false;
	Arguments.emplace_back(ValueTypes::String, "VariableName");

	Values.emplace_back(ValueTypes::Number, "Value", 0);
}

const ValueData* LoadNumber::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);

	if (name)
		ReturnValue.Value = state.NumGlobals[name->String()];

	return &ReturnValue;
}

SaveNumber::SaveNumber()
{
	OutputNodeRefs.emplace_back("Out");

	Arguments.emplace_back(ValueTypes::String, "VariableName");
	Arguments.emplace_back(ValueTypes::Number, "Value");
}

const NodeRef* SaveNumber::Process(ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);
	auto* value = state.GetValue(Arguments[1]);

	if (name)
		state.NumGlobals[name->String()] = value->Number();

	return &OutputNodeRefs[0];
}

LoadString::LoadString()
	: ReturnValue("")
{
	AllowInput = false;
	Arguments.emplace_back(ValueTypes::String, "VariableName");

	Values.emplace_back(ValueTypes::String, "Value", 0);
}

const ValueData* LoadString::GetValue(uint32_t id, ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);

	if (name)
		ReturnValue.Value = state.StringGlobals[name->String()];

	return &ReturnValue;
}

SaveString::SaveString()
{
	OutputNodeRefs.emplace_back("Out");

	Arguments.emplace_back(ValueTypes::String, "VariableName");
	Arguments.emplace_back(ValueTypes::Number, "Value");
}

const NodeRef* SaveString::Process(ScriptInstance& state)
{
	auto* name = state.GetValue(Arguments[0]);
	auto* value = state.GetValue(Arguments[1]);

	if (name)
		state.StringGlobals[name->String()] = value->String();

	return &OutputNodeRefs[0];
}

void ScriptGraph::Write(ScriptResource& resource) const
{
	for (size_t i = 0; i < Nodes.size(); i++)
	{
		Node* node = Nodes[i];

		node->ID = uint32_t(i);

		NodeResource res;
		res.ID = uint32_t(i);

		auto itr = EntryNodes.find(node->Name);
		res.EntryPoint = itr != EntryNodes.end() && itr->second->ID == res.ID;

		strcpy(res.TypeName, node->TypeName());

		size_t nameLen = node->Name.size();
		if (nameLen >= NodeResource::MaxNodeName)
			nameLen = NodeResource::MaxNodeName-1;
		memcpy(res.Name, node->Name.c_str(), nameLen);

		res.DataSize = node->GetDataSize();
		res.Data = malloc(res.DataSize);
		size_t offset = 0;
		if (node->Write(res.Data, offset))
			resource.Nodes.emplace_back(std::move(res));
		else
			free(res.Data);
	}
}

bool ScriptGraph::Read(const ScriptResource& package)
{
	Nodes.resize(package.Nodes.size());
	EntryNodes.clear();

	for (const NodeResource& res : package.Nodes)
	{
		Nodes[res.ID] = NodeRegistry::LoadNode(res.TypeName, res.Data, res.DataSize);
		Nodes[res.ID]->ID = res.ID;
		Nodes[res.ID]->Name = res.Name;

		if (res.EntryPoint)
		{
			EntryNodes.insert_or_assign(Nodes[res.ID]->Name, Nodes[res.ID]);
		}
	}
	return true;
}