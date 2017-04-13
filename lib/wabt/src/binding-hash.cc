/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include "binding-hash.h"

#include <algorithm>
#include <vector>

namespace wabt {

void BindingHash::find_duplicates(DuplicateCallback callback,
                                  void* user_data) const {
  if (size() > 0) {
    ValueTypeVector duplicates;
    create_duplicates_vector(&duplicates);
    sort_duplicates_vector_by_location(&duplicates);
    call_callbacks(duplicates, callback, user_data);
  }
}

void BindingHash::create_duplicates_vector(
    ValueTypeVector* out_duplicates) const {
  // This relies on the fact that in an unordered_multimap, all values with the
  // same key are adjacent in iteration order.
  auto first = begin();
  bool is_first = true;
  for (auto iter = std::next(first); iter != end(); ++iter) {
    if (first->first == iter->first) {
      if (is_first)
        out_duplicates->push_back(&*first);
      out_duplicates->push_back(&*iter);
      is_first = false;
    } else {
      is_first = true;
      first = iter;
    }
  }
}

void BindingHash::sort_duplicates_vector_by_location(
    ValueTypeVector* duplicates) const {
  std::sort(
      duplicates->begin(), duplicates->end(),
      [](const value_type* lhs, const value_type* rhs) -> bool {
        return lhs->second.loc.line < rhs->second.loc.line ||
               (lhs->second.loc.line == rhs->second.loc.line &&
                lhs->second.loc.first_column < rhs->second.loc.first_column);
      });
}

void BindingHash::call_callbacks(const ValueTypeVector& duplicates,
                                 DuplicateCallback callback,
                                 void* user_data) const {
  // Loop through all duplicates in order, and call callback with first
  // occurrence.
  for (auto iter = duplicates.begin(), end = duplicates.end(); iter != end;
       ++iter) {
    auto first = std::find_if(duplicates.begin(), duplicates.end(),
                              [iter](const value_type* x) -> bool {
                                return x->first == (*iter)->first;
                              });
    if (first == iter)
      continue;
    assert(first != duplicates.end());
    callback(**first, **iter, user_data);
  }
}

}  // namespace wabt