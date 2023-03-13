#pragma once

#include<string>
#include<functional>

class ScriptGraph;
class Node;
void ShowGraphEditor(ScriptGraph& graph, bool forcePositions);

class NodeEditHandler
{
public:
	virtual const char* GetIcon(Node* node) const { return Icon.c_str(); }
	virtual void ShowNodeBody(Node* node) const {};

private:
	std::string Icon;
};

NodeEditHandler* AddNodeHandler(const std::string& name, NodeEditHandler* handler);
template<class T>
T* AddNodeHandler((const std::string& name)
{
	T* t = new T();
	return static_cast<T*>(AddNodeHandler(name, t));
}

void AddNodeBody(const std::string& name, std::function<void(Node*)> callback);
void AddNodeIcon(const std::string& name, const std::string& Icon);

const char* GetNodeIcon(const std::string& name);