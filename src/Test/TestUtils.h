#pragma once

#include <Precomp.h>

#ifdef UNIT_TEST_BUILD

namespace Vortex {

#define TestMethod(name) \
	static void name(); \
    static int name##_id = TestMethodRegister(name, #name, __FILE__, __LINE__); \
    static void name()

#define Check(condition) \
    TestMethodCheck(condition, #condition, __LINE__)
    
int TestMethodRegister(void (*method)(), const char* name, const char* file, int line);
void TestMethodCheck(bool conditionHolds, const char* description, int line);
int RunUnitTests();

}; // namespace Vortex

#endif // UNIT_TEST_BUILD