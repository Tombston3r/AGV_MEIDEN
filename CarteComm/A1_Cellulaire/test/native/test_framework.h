// Micro-framework de tests natifs.
//
// PlatformIO/Unity est la cible officielle (brief §11) ; ce shim permet de faire
// tourner exactement les mêmes tests avec un simple g++, sans toolchain
// embarquée — utile en intégration continue et sur un poste sans PlatformIO.
#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace testing {

struct TestCase {
  const char* name;
  void (*fn)();
};

std::vector<TestCase>& registry();
extern int g_failures;
extern int g_checks;

struct Registrar {
  Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

void report_failure(const char* file, int line, const std::string& message);

}  // namespace testing

#define TEST(name)                                                    \
  static void name();                                                 \
  static testing::Registrar reg_##name(#name, name);                  \
  static void name()

#define CHECK(cond)                                                   \
  do {                                                                \
    ++testing::g_checks;                                              \
    if (!(cond)) testing::report_failure(__FILE__, __LINE__, #cond);  \
  } while (0)

#define CHECK_EQ(a, b)                                                            \
  do {                                                                            \
    ++testing::g_checks;                                                           \
    const auto va__ = (a);                                                         \
    const auto vb__ = (b);                                                         \
    if (!(va__ == vb__)) {                                                         \
      testing::report_failure(__FILE__, __LINE__,                                  \
                              std::string(#a " == " #b " (obtenu ") +              \
                                  std::to_string(static_cast<long long>(va__)) +   \
                                  ", attendu " +                                   \
                                  std::to_string(static_cast<long long>(vb__)) + ")"); \
    }                                                                              \
  } while (0)

// Comme CHECK, mais interrompt le test : à utiliser avant tout accès indexé,
// sinon un échec se transforme en segfault et masque les tests suivants.
#define REQUIRE(cond)                                                 \
  do {                                                                \
    ++testing::g_checks;                                              \
    if (!(cond)) {                                                    \
      testing::report_failure(__FILE__, __LINE__, "REQUIRE " #cond);  \
      return;                                                         \
    }                                                                 \
  } while (0)

#define CHECK_STR_EQ(a, b)                                                      \
  do {                                                                            \
    ++testing::g_checks;                                                           \
    const std::string va__ = (a);                                                  \
    const std::string vb__ = (b);                                                  \
    if (va__ != vb__) {                                                            \
      testing::report_failure(__FILE__, __LINE__,                                  \
                              "\"" + va__ + "\" != \"" + vb__ + "\"");             \
    }                                                                              \
  } while (0)
