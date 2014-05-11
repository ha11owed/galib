#include "test.h"

#include <iostream>
#include <crtdbg.h>

#pragma comment(lib, "gtest.lib")

using namespace std;
using namespace testing;

namespace testing
{
	class MemoryLeakDetector : public EmptyTestEventListener {
	public:
		virtual void OnTestStart(const TestInfo&) {
			_CrtMemCheckpoint(&memState_);
		}
		virtual void OnTestEnd(const TestInfo& test_info) {
			// Ignore some tests
			if(strstr(test_info.name(), "NoLeakCheck") != NULL) {
				return;
			}
			// Fail passed tests if they have memory leaks
			if(test_info.result()->Passed())
			{
				_CrtMemState stateNow, stateDiff;
				_CrtMemCheckpoint(&stateNow);
				int diffResult = _CrtMemDifference(&stateDiff, &memState_, &stateNow);
				if (diffResult)
				{
					FAIL() << "Memory leak of " << stateDiff.lSizes[1] << " byte(s) detected.";
				}
			}
		}

	private:
		_CrtMemState memState_;
	};
}

int main(int argc, char** argv) {
	InitGoogleTest(&argc, argv);
	UnitTest::GetInstance()->listeners().Append(new MemoryLeakDetector());

	RUN_ALL_TESTS();
	std::cout << "All tests executed" << std::endl;
	char c;
	std::cin >> c;
}
