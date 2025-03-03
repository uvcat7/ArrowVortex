#include <Precomp.h>

#ifdef UNIT_TEST_BUILD

#include <Core/Common/Singleton.h>
#include <Core/Common/Event.h>

namespace Vortex {

using namespace std;

struct TestMethodData
{
	void (*method)();
	const char* name;
	const char* file;
	int line;
};

static vector<TestMethodData>& GetTestMethods()
{
	static vector<TestMethodData> methods;
	return methods; // See: C++ magic statics.
}

static int failedChecks;

static const char* StripRoot(const char* path)
{
	static const char* root = "\\src\\";
	auto stripped = strstr(path, root);
	return stripped ? (stripped + strlen(root)) : path;
}

int TestMethodRegister(void (*method)(), const char* name, const char* file, int line)
{
	auto& methods = GetTestMethods();
	methods.push_back({ method, name, StripRoot(file), line });
	return (int)methods.size();
}

void TestMethodCheck(bool conditionHolds, const char* description, int line)
{
	if (conditionHolds)
		return;

	printf("\n  Check failed at L%i: %s", line, description);
	failedChecks++;
}

struct TestSingletonRegistry : public SingletonRegistry
{
	vector<deinitializeFunction> deinitializeFunctions;
	vector<eventHandlerFunction> eventHandlerFunctions;

	void add(deinitializeFunction deinitialize) override
	{
		deinitializeFunctions.push_back(deinitialize);
	}

	void add(eventHandlerFunction handleEvent) override
	{
		eventHandlerFunctions.push_back(handleEvent);
	}
};

int RunUnitTests()
{
	int failedTests = 0;

	TestSingletonRegistry registry;
	EventSystem::initialize(registry);

	printf(":: Running Arrow Vortex unit tests ::\n\n");
	for (auto& data : GetTestMethods())
	{
		printf("[%s:%i] %s", data.file, data.line, data.name);
		failedChecks = 0;
		data.method();
		if (failedChecks == 0)
		{
			printf(" :: PASS\n");
		}
		else
		{
			failedTests++;
			printf("\n");
		}
	}

	for (auto& deinit : registry.deinitializeFunctions)
		deinit();

	int passedTests = (int)GetTestMethods().size() - failedTests;
	if (failedTests == 0)
		printf("\n:: All tests passed (%i) ::\n", passedTests);
	else
		printf("\n:: Some tests failed (%i PASS, %i FAIL) ::\n", passedTests, failedTests);

	return 0;
}

}; // namespace Vortex

#endif // UNIT_TEST_BUILD
