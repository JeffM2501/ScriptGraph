/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

For a C++ project simply rename the file to .cpp and run premake 

*/
#define _CRT_SECURE_NO_WARNINGS

#include "script_graph.h"

#include <chrono>

ScriptGraph Graph;

constexpr int LoopCount = 10000;

ScriptResource globalRes;

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

void SaveScript(const ScriptGraph& graph, const std::string& filename)
{
	ScriptResource scriptRes;
	graph.Write(scriptRes);

	FILE* fp = fopen(filename.c_str(), "w+");
	if (!fp)
		return;

	uint32_t count = uint32_t(scriptRes.Nodes.size());

	fwrite(&count, 4, 1, fp);

	for (const auto& res : scriptRes.Nodes)
	{
		fwrite(&res.ID, 4, 1, fp);

		unsigned char isEntry = res.EntryPoint ? 1 : 0;
		fwrite(&isEntry, 1, 1, fp);

		fwrite(res.TypeName, NodeResource::MaxNodeName, 1, fp);
		fwrite(res.Name, NodeResource::MaxNodeName, 1, fp);

		uint32_t size = uint32_t(res.DataSize);
		fwrite(&size, 4, 1, fp);
		fwrite(res.Data, res.DataSize, 1, fp);
	}
	fclose(fp);
}

ScriptGraph LoadScript(const std::string& filename)
{
	ScriptGraph script;
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp)
		return script;

	ScriptResource res;
	uint32_t count = 0;
	fread(&count, 4, 1, fp);

	for (uint32_t i = 0; i < count; i++)
	{
		NodeResource nodeRes;

		fread(&nodeRes.ID, 4, 1, fp);
		unsigned char b = 0;
		fread(&b, 1, 1, fp);
		nodeRes.EntryPoint = b != 0;

		fread(nodeRes.TypeName, NodeResource::MaxNodeName, 1, fp);
		fread(nodeRes.Name, NodeResource::MaxNodeName, 1, fp);

		uint32_t size = 0;
		fread(&size, 4, 1, fp);
		nodeRes.DataSize = size_t(size);
		nodeRes.Data = malloc(size);
		if (nodeRes.Data)
			fread(nodeRes.Data, nodeRes.DataSize, 1, fp);
		else
			break;

		res.Nodes.emplace_back(std::move(nodeRes));
	}

	fclose(fp);

	script.Read(res);
	return script;
}

int main ()
{
	NodeRegistry::RegisterDefaultNodes();

	SetupGraph();

	Graph.Write(globalRes);

	SaveScript(Graph, "test.script");
	ScriptGraph otherGraph = LoadScript("test.script");

	ScriptInstance instance(otherGraph);

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