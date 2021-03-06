/*
//@HEADER
// ************************************************************************
//
//                      sizing_archive.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMAFRONTEND_SIZING_ARCHIVE_H
#define DARMAFRONTEND_SIZING_ARCHIVE_H

#include <darma/serialization/serialization_traits.h>
#include <darma/serialization/serialization_buffer.h>
#include <darma/serialization/simple_handler_fwd.h>

#include "simple_handler_fwd.h"
#include "pointer_reference_handler_fwd.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>

#ifndef DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX
#  define DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX 1024
#endif

namespace darma {
namespace serialization {

class SimpleSizingArchive {
  protected:

    std::size_t size_ = 0;

    SimpleSizingArchive() = default;

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_for_size(T const& obj) & {
      darma_compute_size(obj, *this);
      return *this;
    }

  public:

    // Concept "shortcut" tag
    using is_sizing_archive_t = std::true_type;
    using is_archive_t = std::true_type;

    static constexpr bool is_sizing() { return true; }
    static constexpr bool is_packing() { return false; }
    static constexpr bool is_unpacking() { return false; }

    void add_to_size_raw(size_t size) {
      size_ += size;
    }

    template <typename T>
    inline auto& operator|(T const& obj) & {
      return _ask_serializer_for_size(obj);
    }

    template <typename T>
    inline auto& operator%(T const& obj) & {
      return _ask_serializer_for_size(obj);
    }

};

template <typename SerializationBuffer=DynamicSerializationBuffer<std::allocator<char>>>
class SimplePackingArchive {
  protected:

    char* data_spot_ = nullptr;
    SerializationBuffer buffer_;

    template <typename BufferT>
    explicit SimplePackingArchive(BufferT&& buffer)
      : data_spot_(buffer.data()), buffer_(std::forward<BufferT>(buffer))
    { }

    char*& _data_spot() { return data_spot_; }

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_to_pack(T const& obj) & {
      darma_pack(obj, *this);
      return *this;
    }

  public:

    // Concept "shortcut" tag
    using is_packing_archive_t = std::true_type;
    using is_archive_t = std::true_type;

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return true; }
    static constexpr bool is_unpacking() { return false; }

    template <typename ContiguousIterator>
    void pack_data_raw(ContiguousIterator begin, ContiguousIterator end) {
      using value_type =
        std::remove_const_t<std::remove_reference_t<decltype(*begin)>>;
      // std::copy(begin, end, reinterpret_cast<value_type*>(data_spot_));
      // Use memcpy, since copy invokes the assignment operator, and "raw"
      // implies that this isn't necessary
      auto size = std::distance(begin, end) * sizeof(value_type);
      std::memcpy(data_spot_, static_cast<void const*>(begin), size);
      data_spot_ += size;
    }

    template <typename T>
    inline auto& operator|(T const& obj) & {
      return _ask_serializer_to_pack(obj);
    }

    template <typename T>
    inline auto& operator<<(T const& obj) & {
      return _ask_serializer_to_pack(obj);
    }

};

template <typename Allocator=std::allocator<char>>
class SimpleUnpackingArchive {
  public:

    // Concept "shortcut" tag
    using is_unpacking_archive_t = std::true_type;
    using is_archive_t = std::true_type;

    using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;

  protected:

    darma::utility::compressed_pair<char const*, allocator_type> data_spot_;

    template <typename BufferT>
    explicit SimpleUnpackingArchive(
      BufferT const& buffer, allocator_type const& alloc
    ) : data_spot_(
          std::piecewise_construct,
          std::forward_as_tuple(buffer.data()),
          std::forward_as_tuple(alloc)
        )
    { }

    char const*& _data_spot() { return data_spot_.first(); }

    template <typename>
    friend struct SimpleSerializationHandler;

  private:

    template <typename T>
    inline auto& _ask_serializer_to_unpack(
      T& obj,
      std::enable_if_t<
        not std::is_trivially_destructible<T>::value
          and not std::is_array<T>::value,
        darma::utility::_not_a_type
      > = { }
    ) & {
      // This seems slow (though the optimizer might get rid of it anyway,
      // but since these overloads are given an already-constructed object, and
      // the customization points take an allocated, uninitialized buffer
      auto* buffer = static_cast<void*>(&obj);
      using rebound_alloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
      rebound_alloc alloc{get_allocator()};
      std::allocator_traits<rebound_alloc>::destroy(alloc, &obj);
      darma_unpack(allocated_buffer_for<T>(buffer), *this);
      return *this;
    }

    template <typename T>
    inline auto& _ask_serializer_to_unpack(
      T& obj,
      std::enable_if_t<
        std::is_trivially_destructible<T>::value,
        darma::utility::_not_a_type
      > = { }
    ) & {
      // This seems slow (though the optimizer might get rid of it anyway,
      // but since these overloads are given an already-constructed object, and
      // the customization points take an allocated, uninitialized buffer
      auto* buffer = static_cast<void*>(&obj);
      darma_unpack(allocated_buffer_for<T>(buffer), *this);
      return *this;
    }

  public:


    // TODO Generate an iterator? (Maybe not for all types of archives)

    static constexpr bool is_sizing() { return false; }
    static constexpr bool is_packing() { return false; }
    static constexpr bool is_unpacking() { return true; }

    template <typename RawDataType>
    void unpack_data_raw(void* allocated_dest, size_t n_items = 1) {
      std::memcpy(
        allocated_dest,
        data_spot_.first(),
        n_items * sizeof(RawDataType)
      );
      data_spot_.first() += n_items * sizeof(RawDataType);
    }



    template <typename T>
    inline auto& operator|(T& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    template <typename T>
    inline auto& operator>>(T& obj) & {
      return _ask_serializer_to_unpack(obj);
    }

    // TODO move these to a mixin so code isn't duplicated between this and PointerReference*Archive?

    template <typename T>
    inline T unpack_next_item_as(
      std::enable_if_t<
        (sizeof(T) > DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX),
        darma::utility::_not_a_type_numbered<0>
      > = { }
    ) & {
      using allocator_t = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      auto alloc = allocator_t(get_allocator());
      auto* storage = std::allocator_traits<allocator_t>::allocate(
        alloc, 1
      );
      darma_unpack(allocated_buffer_for<T>(storage), *this);
      // Use a unique_ptr to delete the temporary storage after returning
      std::unique_ptr<T> raii_holder(*reinterpret_cast<T*>(storage));
      return *raii_holder.get();
    }

    template <typename T>
    inline T unpack_next_item_as(
      std::enable_if_t<
        (sizeof(T) <= DARMA_SERIALIZATION_SIMPLE_ARCHIVE_UNPACK_STACK_ALLOCATION_MAX),
        darma::utility::_not_a_type_numbered<1>
      > = { }
    ) & {
      char on_stack_buffer[sizeof(T)];
      darma_unpack(allocated_buffer_for<T>(on_stack_buffer), *this);
      // Use a unique_ptr to delete the temporary storage after returning
      auto destroy_but_not_delete = [this](auto* ptr) {
        using allocator_t = typename std::allocator_traits<allocator_type>::template rebind_alloc<T>;
        allocator_t alloc{this->get_allocator()};
        // Destroy but don't deallocate, since the data is allocated on the stack
        std::allocator_traits<allocator_t>::destroy(alloc, ptr);
      };
      std::unique_ptr<T, decltype(destroy_but_not_delete)> raii_destroy(
        reinterpret_cast<T*>(on_stack_buffer),
        destroy_but_not_delete
      );
      return *raii_destroy.get();
    }

    template <typename T>
    inline void unpack_next_item_at(void* allocated) & {
      darma_unpack(allocated_buffer_for<T>(allocated), *this);
    }

    auto const& get_allocator() const {
      return data_spot_.second();
    }
    auto& get_allocator() {
      return data_spot_.second();
    }

    template <typename NeededAllocatorT>
    NeededAllocatorT get_allocator_as() const {
      // Let's hope this is a compatible type
      return data_spot_.second();
    }
};

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_SIZING_ARCHIVE_H
