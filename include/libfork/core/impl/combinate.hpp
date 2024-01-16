#ifndef AD9A2908_3043_4CEC_9A2A_A57DE168DF19
#define AD9A2908_3043_4CEC_9A2A_A57DE168DF19

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <utility> // for as_const, forward

#include "libfork/core/first_arg.hpp" // for quasi_pointer, async_function_...
#include "libfork/core/impl/awaitables.hpp"
#include "libfork/core/invocable.hpp" // for invoke_result_t, return_address...
#include "libfork/core/tag.hpp"       // for tag
#include "libfork/core/task.hpp"      // for returnable, task

/**
 * @file combinate.hpp
 *
 * @brief Utility for building a coroutine's first argument and invoking it.
 */

namespace lf::impl {

// ---------------------------- //

template <returnable R, return_address_for<R> I, tag Tag>
struct promise;

// -------------------------------------------------------- //

/**
 * @brief Awaitable in the context of an `lf::task` coroutine.
 *
 * This will be transformed by an `await_transform` and trigger a fork or call.
 */
template <returnable R, return_address_for<R> I, tag Tag>
struct [[nodiscard("A quasi_awaitable MUST be immediately co_awaited!")]] quasi_awaitable {
  promise<R, I, Tag> *prom; ///< The parent/semaphore needs to be set!
};

// ---------------------------- //

/**
 * @brief Call an async function with a synthesized first argument.
 *
 * The first argument will contain a copy of the function hence, this is a fixed-point combinator.
 */
template <quasi_pointer I, tag Tag, async_function_object F>
struct [[nodiscard("A bound function SHOULD be immediately invoked!")]] y_combinate {

  [[no_unique_address]] I ret; ///< The return address.
  [[no_unique_address]] F fun; ///< The asynchronous function.

  /**
   * @brief Invoke the coroutine, set's the return pointer.
   */
  template <typename... Args>
    requires async_invocable<I, Tag, F, Args...>
  auto operator()(Args &&...args) && -> quasi_awaitable<invoke_result_t<F, Args...>, I, Tag> {

    task task = std::move(fun)(                                 //
        first_arg_t<I, Tag, F, Args &&...>(std::as_const(fun)), //
        std::forward<Args>(args)...                             //
    );

    using R = invoke_result_t<F, Args...>;
    using P = promise<R, I, Tag>;

    auto *prom = static_cast<P *>(task.prom);

    if constexpr (!std::is_void_v<R>) {
      prom->set_return(std::move(ret));
    }

    return {prom};
  }
};

// ---------------------------- //

/**
 * @brief Build a combinator for `ret` and `fun`.
 */
template <tag Tag, quasi_pointer I, async_function_object F>
  requires std::is_rvalue_reference_v<I &&>
auto combinate(I &&ret, F fun) -> y_combinate<I, Tag, F> {
  return {std::move(ret), std::move(fun)};
}

/**
 * @brief Build a combinator for `ret` and `fun`.
 *
 * This specialization prevents each layer wrapping the function in another `first_arg_t`.
 */
template <tag Tag,
          tag OtherTag,
          quasi_pointer I,
          quasi_pointer OtherI,
          async_function_object F,
          typename... Args>
  requires std::is_rvalue_reference_v<I &&>
auto combinate(I &&ret, first_arg_t<OtherI, OtherTag, F, Args...> arg) -> y_combinate<I, Tag, F> {
  return {std::move(ret), unwrap(std::move(arg))};
}

} // namespace lf::impl

#endif /* AD9A2908_3043_4CEC_9A2A_A57DE168DF19 */
