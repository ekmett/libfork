/*
 * Copyright (c) Conor Williams, Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <catch2/catch_test_macros.hpp>
#include <libfork/core/invocable.hpp>

#include "libfork/core.hpp"

// NOLINTBEGIN No need to check the tests for style.

using namespace lf;

using namespace lf::impl;

namespace {

struct anything {
  template <typename T>
  operator T() const {
    return {};
  }
};

template <typename R, typename I>
struct coro {
  struct promise_type : return_result<R, I> {
    // clang-format off
    promise_type() : return_result<R, I>{anything{}} {}
    promise_type() requires std::is_void_v<R> = default;

    // clang-format on

    auto get_return_object() noexcept -> coro { return {}; }
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    auto final_suspend() noexcept -> std::suspend_always { return {}; }
    void unhandled_exception() noexcept {}
  };
};

// root/non-root, R = void/non-void, T = void/non-void

// T = void

auto test1() -> coro<void, discard_t> { co_return; }

// T = non-reference

#define trivial(name, type)                                                                                  \
  auto name##_name()->coro<int, type> {                                                                      \
    int x = 23;                                                                                              \
    co_return 23;                                                                                            \
    co_return x;                                                                                             \
    co_return 34.;                                                                                           \
    co_return anything{};                                                                                    \
    co_return {};                                                                                            \
  }

trivial(discard_t, discard_t);
trivial(int, int *);
trivial(double, double *);
trivial(int_o, std::optional<int> *);
trivial(int_e, eventually<int> *);

#define vector(name, type)                                                                                   \
  auto vector_##name()->coro<std::vector<int>, type> {                                                       \
    std::vector<int> x;                                                                                      \
    co_return x;                                                                                             \
    co_return std::vector<int>{};                                                                            \
    static std::vector<int> y;                                                                               \
    co_return y;                                                                                             \
    co_return {};                                                                                            \
    co_return {x.begin(), x.end()};                                                                          \
    co_return {1, 2, 3};                                                                                     \
  }

vector(void, discard_t);
vector(vec, std::vector<int> *);
vector(vec_o, std::optional<std::vector<int>> *);
vector(vec_e, eventually<std::vector<int>> *);

// ------------------------------------------- //

int x = 23;

#define reference(name, type)                                                                                \
  auto reference_##name()->coro<int &, type> { co_return x; }

#define rreference(name, type)                                                                               \
  auto rreference_##name()->coro<int &&, type> {                                                             \
    co_return 23;                                                                                            \
    co_return std::move(x);                                                                                  \
  }

reference(void, discard_t);
reference(int, eventually<int &> *);

rreference(void, discard_t);
rreference(int, eventually<int &&> *);

} // namespace

// NOLINTEND