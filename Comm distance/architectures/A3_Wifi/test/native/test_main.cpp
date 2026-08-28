#include "test_framework.h"

namespace testing {

std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

int g_failures = 0;
int g_checks = 0;

void report_failure(const char* file, int line, const std::string& message) {
  ++g_failures;
  std::printf("    ÉCHEC %s:%d : %s\n", file, line, message.c_str());
}

}  // namespace testing

int main(int argc, char** argv) {
  const char* filter = (argc > 1) ? argv[1] : nullptr;
  int run = 0;
  int failed_tests = 0;

  for (const auto& test : testing::registry()) {
    if (filter != nullptr && std::string(test.name).find(filter) == std::string::npos) continue;
    const int before = testing::g_failures;
    std::printf("[ RUN  ] %s\n", test.name);
    test.fn();
    ++run;
    if (testing::g_failures > before) {
      ++failed_tests;
      std::printf("[ FAIL ] %s\n", test.name);
    } else {
      std::printf("[  OK  ] %s\n", test.name);
    }
  }

  std::printf("\n%d tests, %d assertions, %d échecs\n", run, testing::g_checks,
              testing::g_failures);
  return failed_tests == 0 ? 0 : 1;
}
