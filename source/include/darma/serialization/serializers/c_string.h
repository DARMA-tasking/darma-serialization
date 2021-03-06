/*
//@HEADER
// ************************************************************************
//
//                      c_string.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_SERIALIZATION_SERIALIZERS_C_STRING_H
#define DARMAFRONTEND_SERIALIZATION_SERIALIZERS_C_STRING_H

namespace darma {
namespace serialization {

#include <darma/serialization/nonintrusive.h>
#include <darma/serialization/serialization_traits.h>
#include <cstring>
#include <type_traits>

template <>
struct is_directly_serializable<char const*> : std::false_type { };

template <>
struct Serializer<char const*>
{
  template <typename SizingArchive>
  static void compute_size(char const* const& obj, SizingArchive& ar) {
    const auto len = std::strlen(obj);
    ar % len;
    ar.add_to_size_raw(len);
  }

  template <typename PackingArchive>
  static void pack(char const* const& obj, PackingArchive& ar) {
    const auto len = std::strlen(obj);
    ar << len;
    for(int i = 0; i < len; ++i) {
      ar << obj[i];
    }
  }

  // TODO unpack for char const* ?!??!?
  //template <typename UnpackingArchive>
  //static void unpack(void* allocated, UnpackingArchive& ar) {
  //  char* allocated_ptr = static_cast<char*>(allocated);
  //  for(int64_t i = 0; i < N; ++i, allocated_ptr += sizeof(T)) {
  //    ar.template unpack_next_item_at<T>(allocated_ptr);
  //  }
  //}
};

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_SERIALIZATION_SERIALIZERS_C_STRING_H
