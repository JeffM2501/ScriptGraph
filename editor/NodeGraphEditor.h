#pragma once

#include<string>
#include<functional>

class ScriptGraph;
class Node;
void ShowGraphEditor(ScriptGraph& graph, bool forcePositions);

void AddNodeBody(const std::string& name, std::function<void(Node*)> callback);
void AddNodeIcon(const std::string& name, const std::string& Icon);

const char* GetNodeIcon(const std::string& name);