/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

For a C++ project simply rename the file to .cpp and run premake 

*/

#include "script_graph.h"

#include <chrono>

ScriptGraph Graph;

constexpr int LoopCount = 10000;

void SetupGraph()
{
	EntryNode* entry = new EntryNode();
	entry->Name = "Entry";
	entry->ID = 0;
	entry->OutputNodeRefs[0].ID = 1;

	Graph.Nodes.push_back(entry);
	Graph.EntryNodes[entry->Name] = entry;

	Loop* loop = new Loop();
	loop->ID = 1;
	loop->Itterations = LoopCount;
	loop->OutputNodeRefs[0].ID = 5;
	loop->OutputNodeRefs[1].ID = 2;
	Graph.Nodes.push_back(loop);

	PrintLog* log = new PrintLog();
	log->ID = 2;
	log->Arguments[0].ID = 3;
	log->Arguments[0].ValueId = 0;
	Graph.Nodes.push_back(log);

	StringLiteral* literal = new StringLiteral("Loop Cycle");
	literal->ID = 3;
	Graph.Nodes.push_back(literal);

	StringLiteral* endLiteral = new StringLiteral("Loop Complete");
	endLiteral->ID = 4;
	Graph.Nodes.push_back(endLiteral);

	log = new PrintLog();
	log->ID = 5;
	log->Arguments[0].ID = 4;
	log->Arguments[0].ValueId = 0;
	Graph.Nodes.push_back(log);
}

int main ()
{
	SetupGraph();

	ScriptInstance instance(Graph);

	std::chrono::high_resolution_clock clock;
	auto start = clock.now();
	auto val = instance.Run("Entry");
	auto end = clock.now();

	std::chrono::duration<double> scriptSeconds = end - start;

	printf("script time = %f", scriptSeconds.count());

	start = clock.now();
	for (int i = 0; i < LoopCount; i++)
	{
		printf("%s\n", "Loop Cycle");
	}
	printf("%s\n", "Loop Complete");
	end = clock.now();

	std::chrono::duration<double> nativeSeconds = end - start;

	printf("script time = %f %d iterations\n", scriptSeconds.count(), LoopCount);
	printf("native time = %f %d iterations\n", nativeSeconds.count(), LoopCount);

	double delta = scriptSeconds.count() - nativeSeconds.count();
	double percent = delta / scriptSeconds.count() * 100;
	printf("native faster by = %f %%\n", percent);
	return 0;
}