// C library
#include "script_graph.h"


std::string NumberValueData::Text;
float StringValueData::ValueF = 0;


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

	int index = 0;
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

BooleanLiteral::BooleanLiteral(bool value)
	: ReturnValue(value)
{
	AllowInput = false;
}

const ValueData* BooleanLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

NumberLiteral::NumberLiteral(float value)
	: ReturnValue(value)
{
	AllowInput = false;
}

const ValueData* NumberLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

StringLiteral::StringLiteral(const std::string& value)
	: ReturnValue(value)
{
	AllowInput = false;
}

const ValueData* StringLiteral::GetValue(uint32_t id, ScriptInstance& state)
{
	return &ReturnValue;
}

PrintLog::PrintLog()
{
	OutputNodeRefs.emplace_back("Out");

	Arguments.emplace_back(ValueTypes::Number, "Text");
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