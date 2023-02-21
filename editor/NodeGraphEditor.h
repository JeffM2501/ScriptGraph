#pragma once

#include<string>
#include<functional>

class ScriptGraph;
class Node;
void ShowGraphEditor(ScriptGraph& graph, bool forcePositions);

void AddNodeBody(const std::string& name, std::function<void(Node*)> callback);