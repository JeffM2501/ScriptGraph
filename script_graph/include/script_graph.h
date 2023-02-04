#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <map>
#include <unordered_map>
#include <stack>

class NodeRef
{
public:
	uint32_t ID = uint32_t(-1);
	std::string Name;

	NodeRef() = default;

	NodeRef(const std::string& name)
		: Name(name)
	{
	}

	virtual ~NodeRef() = default;
};

enum class ValueTypes
{
	Boolean,
	Number,
	String
};

class ValueRef : public NodeRef
{
public:
	ValueTypes RefType;
	uint32_t ValueId = uint32_t(-1);

	ValueRef(ValueTypes refType, const std::string& name)
		: NodeRef(name)
		, RefType(refType)
	{
	}
};

class ValueDef : NodeRef
{
public:
	
	ValueTypes Type;

	ValueDef(ValueTypes refType, const std::string& name, uint32_t id)
		: NodeRef(name)
		, Type(refType)
	{
		ID = id;
	}
};

class ValueData
{
public:
	ValueTypes Type;

	virtual const bool& Boolean() const = 0;
	virtual const float& Number() const = 0;
	virtual const std::string& String() const = 0;

	virtual ~ValueData() = default;
};

class BooleanValueData : public ValueData
{
public:
	bool Value = 0;
	BooleanValueData(bool val) : Value(val) { Type = ValueTypes::Boolean; }
	const bool& Boolean() const override { return Value; }
	const float& Number() const override { return Value ? TrueF : FalseF; }
	const std::string& String() const override { return Value ? TrueS : FalseS; }

protected:
	float TrueF = 1.0f;
	float FalseF = 0.0f;

	const std::string TrueS = "true";
	const std::string FalseS = "true";
};

class NumberValueData : public ValueData
{
public:
	float Value = 0;
	NumberValueData(float val) : Value(val) { Type = ValueTypes::Number; }
	const bool& Boolean() const override { return Value != 0 ? TrueB : FalseB; }
	const float& Number() const override { return Value; }
	const std::string& String() const override { Text = std::to_string(Value); return Text; }

protected:
	bool TrueB = true;
	bool FalseB = false;

	static std::string Text;
};

class StringValueData : public ValueData
{
public:
	std::string Value = 0;
	StringValueData(const std::string& val) : Value(val) { Type = ValueTypes::String; }
	const bool& Boolean() const override { return Value != "false" ? TrueB : FalseB; }
	const float& Number() const override { ValueF = float(atof(Value.c_str())); return ValueF; }
	const std::string& String() const override { return Value; }

protected:
	bool TrueB = true;
	bool FalseB = false;

	static float ValueF;
};

class ScriptInstance;

class Node : public NodeRef
{
public:
	bool AllowInput = true;

	std::vector<NodeRef> OutputNodeRefs;

	std::vector<ValueRef> Arguments;

	std::vector<ValueDef> Values;

	virtual const NodeRef* Process(ScriptInstance& state) { return nullptr; }

	virtual const ValueData* GetValue(uint32_t id, ScriptInstance& state) { return nullptr; }
};

class ScriptGraph
{
public:
	std::vector<Node*> Nodes;

	std::map<std::string, Node*> EntryNodes;
};

class ScriptInstance
{
public:
	ScriptInstance(ScriptGraph& graph);

	enum class Result
	{
		Error,
		Complete,
		Incomplete,
	};

	Result Run(const std::string& entryPoint);

	const ValueData* GetValue(const ValueRef& ref);

	void PushReturnNode();

	std::unordered_map<std::string, bool> BoolGlobals;
	std::unordered_map<std::string, float> NumGlobals;
	std::unordered_map<std::string, std::string> StringGlobals;
	std::unordered_map<uint32_t, int> NodeStateNums;

	std::stack<uint32_t> ReturnStack;
	uint32_t CurrentNode = 0;


protected:
	ScriptGraph& Graph;
	Result RunResult = Result::Error;

protected:
	bool RunStep();
	void Clear();
};

// Flow Control

class EntryNode : public Node
{
public:
	EntryNode();
	const NodeRef* Process(ScriptInstance& state) override;
};


class Condition : public Node
{
public:
	Condition();
	const NodeRef* Process(ScriptInstance& state) override;
};

class Loop : public Node 
{
public:
	Loop();
	const NodeRef* Process(ScriptInstance& state) override;
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	int Itterations = -1;
protected:
	NumberValueData IndexValue;
};

// Comparison
class BooleanComparison : public Node
{
public:
	enum class Operation
	{
		AND,
		OR,
	};
	Operation Operator;

	BooleanComparison(Operation op);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

class NotComparison : public Node
{
public:
	NotComparison();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

class NumberComparison : public Node
{
public:
	enum class Operation
	{
		GreaterThan,
		GreaterThanEqual,
		LessThan,
		LessThankEqual,
		Equal,
		NotEqual,
	};
	Operation Operator;

	NumberComparison(Operation op);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

// Math
class Math : public Node
{
public:
	enum class Operation
	{
		Add,
		Subtract,
		Multiply,
		Divide,
		Modulo,
		Pow,
	};
	Operation Operator;

	Math(Operation op);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	NumberValueData ReturnValue;
};

// Literals
class BooleanLiteral : public Node
{
public:
	BooleanLiteral(bool value);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

class NumberLiteral : public Node
{
public:
	NumberLiteral(float value);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	NumberValueData ReturnValue;
};

class StringLiteral : public Node
{
public:
	StringLiteral(const std::string& value);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	StringValueData ReturnValue;
};

// Debug
class PrintLog : public Node
{
public:
	PrintLog();
	const NodeRef* PrintLog::Process(ScriptInstance& state) override;
};

// Variable Access
class LoadBool : public Node
{
public:
	LoadBool();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

class SaveBool : public Node
{
public:
	SaveBool();
	const NodeRef* Process(ScriptInstance& state) override;
};

class LoadNumber : public Node
{
public:
	LoadNumber();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	NumberValueData ReturnValue;
};

class SaveNumber : public Node
{
public:
	SaveNumber();
	const NodeRef* Process(ScriptInstance& state) override;
};

class LoadString : public Node
{
public:
	LoadString();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	StringValueData ReturnValue;
};

class SaveString : public Node
{
public:
	SaveString();
	const NodeRef* Process(ScriptInstance& state) override;
};


/* Nodes Needed

// flow control
IF
For
While
Break

// Comparison
AND/OR
NOT
GT GTE EQ NEQ LT LTE

// MATH
ADD SUBTRACT MULTIPLY DIVIDE MODULATE

// Variables
LoadBool
SaveBool
LoadNumber
SaveNumber
LoadString
SaveString

// LOG
LOG

*/