#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <map>
#include <unordered_map>
#include <stack>
#include <functional>

class Node;
namespace NodeRegistry
{
	void RegisterNode(const char* typeName, std::function<Node* ()> newFactory, std::function<Node* (void*, size_t)> loadFactory);

	template<class T>
	inline void RegisterNode()
	{
		RegisterNode(T::GetTypeName(), T::Create, T::Load);
	}

	Node* CreateNode(const char* typeName);

	template<class T>
	inline Node* CreateNode()
	{
		return (T*)CreateNode(T::GetTypeName());
	}

	Node* LoadNode(const char* typeName, void* data, size_t size);

	template<class T>
	inline Node* LoadNode(void* data, size_t size)
	{
		return (T*)LoadNode(T::GetTypeName(), data, size);
	}

	void RegisterDefaultNodes();

	std::vector<std::string> GetNodeList();

	const std::string& GetNodeTypeFromIndex(size_t index);
}

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

class ValueDef : public NodeRef
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
	float NodePosX = 0;
	float NodePosY = 0;

	bool AllowInput = true;

	std::vector<NodeRef> OutputNodeRefs;

	std::vector<ValueRef> Arguments;

	std::vector<ValueDef> Values;

	virtual const NodeRef* Process(ScriptInstance& state) { return nullptr; }

	virtual const ValueData* GetValue(uint32_t id, ScriptInstance& state) { return nullptr; }

	virtual const char* TypeName() const = 0;

	virtual const char* Icon() const { return nullptr; }

	// IO
	virtual void Read(void* data, size_t size, size_t& offset);
	virtual size_t GetDataSize();
	virtual bool Write(void* data, size_t& offset);

protected:
	void WriteBool(bool value, void* data, size_t& offset);
	void WriteUInt(uint32_t value, void* data, size_t& offset);
	void WriteUInt(size_t value, void* data, size_t& offset);
	void WriteFloat(float value, void* data, size_t& offset);
	void WriteString(const std::string& value, void* data, size_t& offset);
	size_t GetStringDataSize(const std::string& value) { return 4 + value.size(); }

	bool ReadBool(void* data, size_t size, size_t& offset);
	uint32_t ReadUInt(void* data, size_t size, size_t& offset);
	float ReadFloat(void* data, size_t size, size_t& offset);
	std::string ReadString(void* data, size_t size, size_t& offset);
};

struct NodeResource
{
	static constexpr size_t MaxNodeName = 32;

	bool EntryPoint = false;
	uint32_t ID = uint32_t(-1);
	char TypeName[MaxNodeName] = { 0 };
	char Name[MaxNodeName] = { 0 };

	size_t DataSize = 0;
	void* Data = nullptr;
};

struct ScriptResource
{
	std::vector<NodeResource> Nodes;

	~ScriptResource()
	{
		for (auto& res : Nodes)
		{
			if (res.Data)
				free(res.Data);
		}
	}
};

#define DEFINE_NODE(TYPE) \
static const char* GetTypeName() { return #TYPE; } \
const char* TypeName() const override { return #TYPE; } \
static Node* Create() { return new TYPE(); } \
static Node* Load(void* data, size_t size) { Node* node = new TYPE(); size_t offset = 0; node->Read(data, size, offset); return node; } 

class ScriptGraph
{
public:
	std::map<uint32_t,Node*> Nodes;

	std::map<std::string, Node*> EntryNodes;

	void Write(ScriptResource& resource) const;

	bool Read(const ScriptResource& package);

	uint32_t AddNode(Node* node);
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

	ScriptInstance::Result Start(const std::string& entryPoint);
	ScriptInstance::Result Step();

	const ValueData* GetValue(const ValueRef& ref);

	void PushReturnNode();

	std::unordered_map<std::string, bool> BoolGlobals;
	std::unordered_map<std::string, float> NumGlobals;
	std::unordered_map<std::string, std::string> StringGlobals;
	std::unordered_map<uint32_t, int> NodeStateNums;

	std::stack<uint32_t> ReturnStack;
	uint32_t CurrentNode = 0;

	bool Running = false;

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
	DEFINE_NODE(EntryNode);

	EntryNode();
	const NodeRef* Process(ScriptInstance& state) override;
};

class Condition : public Node
{
public:
	DEFINE_NODE(Condition);

	Condition();
	const NodeRef* Process(ScriptInstance& state) override;
};

class Loop : public Node 
{
public:
	DEFINE_NODE(Loop);

	Loop();
	const NodeRef* Process(ScriptInstance& state) override;
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	uint32_t Itterations = 0;

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	NumberValueData IndexValue;
};

template<class T>
inline void DoForEachEnum(std::function<void(T)> func)
{
	if (!func)
		return;
	for (int i = 0; i < (int)T::LAST_OP; i++)
		func((T)i);
}

// Comparison
class BooleanComparison : public Node
{
public:
	enum class Operation
	{
		AND = 0,
		OR,
		LAST_OP
	};
	Operation Operator = Operation::AND;

	static const char* ToString(Operation op)
	{
		switch (op)
		{
			case BooleanComparison::Operation::AND:
				return "AND";
			case BooleanComparison::Operation::OR:
				return "OR";
			default:
				return "UNKNOWN";
		}
	}

	BooleanComparison(Operation op = Operation::AND);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	DEFINE_NODE(BooleanComparison);

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	BooleanValueData ReturnValue;
};

class NotComparison : public Node
{
public:
	NotComparison();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	DEFINE_NODE(NotComparison);

protected:
	BooleanValueData ReturnValue;
};

class NumberComparison : public Node
{
public:
	enum class Operation
	{
		GreaterThan = 0,
		GreaterThanEqual,
		LessThan,
		LessThanEqual,
		Equal,
		NotEqual,
		LAST_OP
	};
	Operation Operator = Operation::Equal;

	static const char* ToString(Operation op)
	{
		switch (op)
		{
			case NumberComparison::Operation::GreaterThan:
				return "Greater Than";
			case NumberComparison::Operation::GreaterThanEqual:
				return "Greater Than Or Equal";
			case NumberComparison::Operation::LessThan:
				return "Less Than";
			case NumberComparison::Operation::LessThanEqual:
				return "Less Than Or Equal";
			case NumberComparison::Operation::Equal:
				return "Equals";
			case NumberComparison::Operation::NotEqual:
				return "Not Equals";
			default:
				return "UNKNOWN";
		}
	}

	NumberComparison(Operation op = Operation::GreaterThan);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

	DEFINE_NODE(NumberComparison);

protected:
	BooleanValueData ReturnValue;
};

// Math
class Math : public Node
{
public:
	enum class Operation
	{
		Add = 0,
		Subtract,
		Multiply,
		Divide,
		Modulo,
		Pow,
		LAST_OP
	};
	Operation Operator = Operation::Add;

	static const char* ToString(Operation op)
	{
		switch (op)
		{
			case Math::Operation::Add:
				return "Add";
			case Math::Operation::Subtract:
				return "Subtract";
			case Math::Operation::Multiply:
				return "Multiply";
			case Math::Operation::Divide:
				return "Divide";
			case Math::Operation::Modulo:
				return "Modulo";
			case Math::Operation::Pow:
				return "Pow";
			default:
				return "UNKNOWN";
		}
	}

	DEFINE_NODE(Math);

	Math(Operation op = Operation::Add);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	NumberValueData ReturnValue;
};

// Literals
class BooleanLiteral : public Node
{
public:
	DEFINE_NODE(BooleanLiteral);
	BooleanLiteral(bool value = false);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	inline void SetValue(const bool& value) { ReturnValue.Value = value; };
	inline bool GetValue() const { return ReturnValue.Value; };

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	BooleanValueData ReturnValue;
};

class NumberLiteral : public Node
{
public:
	DEFINE_NODE(NumberLiteral);

	NumberLiteral(float value = 0);
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	inline void SetValue(const float& value) { ReturnValue.Value = value; };
	inline float GetValue() const { return ReturnValue.Value; };

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	NumberValueData ReturnValue;
};

class StringLiteral : public Node
{
public:
	StringLiteral(const std::string& value = "");
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

	inline void SetValue(const std::string& text) { ReturnValue.Value = text; };
	inline const char* GetValue() const { return ReturnValue.Value.c_str(); };

	DEFINE_NODE(StringLiteral);

	void Read(void* data, size_t size, size_t& offset) override;
	size_t GetDataSize() override;
	bool Write(void* data, size_t& offset) override;

protected:
	StringValueData ReturnValue;
};

// Debug
class PrintLog : public Node
{
public:
	DEFINE_NODE(PrintLog);

	PrintLog();
	const NodeRef* PrintLog::Process(ScriptInstance& state) override;

	static std::function<void(const std::string&)> LogFunction;
};

// Variable Access
class LoadBool : public Node
{
public:
	DEFINE_NODE(LoadBool);

	LoadBool();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	BooleanValueData ReturnValue;
};

class SaveBool : public Node
{
public:
	DEFINE_NODE(SaveBool);

	SaveBool();
	const NodeRef* Process(ScriptInstance& state) override;
};

class LoadNumber : public Node
{
public:
	DEFINE_NODE(LoadNumber);

	LoadNumber();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	NumberValueData ReturnValue;
};

class SaveNumber : public Node
{
public:
	DEFINE_NODE(SaveNumber);

	SaveNumber();
	const NodeRef* Process(ScriptInstance& state) override;
};

class LoadString : public Node
{
public:
	DEFINE_NODE(LoadString);

	LoadString();
	const ValueData* GetValue(uint32_t id, ScriptInstance& state) override;

protected:
	StringValueData ReturnValue;
};

class SaveString : public Node
{
public:
	DEFINE_NODE(SaveString);

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