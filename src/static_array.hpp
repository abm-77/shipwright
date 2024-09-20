#pragma once
#include <array>
#include <assert.h>
#include <stdlib.h>
#include <utility>

template <typename DataType, size_t Capacity> class StaticArray {
public:
  void append(DataType item) {
    assert_has_space();
    data[len++] = item;
  }

  DataType pop() {
    assert_has_items();
    DataType res = data[--len];
    return res;
  }

  void swap_remove(size_t idx) {
    assert_has_items();
    assert_in_bounds(idx);
    std::swap(data[idx], data[len - 1]);
    len--;
  }

  void set(size_t idx, DataType value) {
    assert_has_items();
    assert_in_bounds(idx);
    data[idx] = value;
  }

  DataType get(size_t idx) {
    assert_has_items();
    assert_in_bounds(idx);
    return data[idx];
  }

  DataType *get_ptr(size_t idx) {
    assert_has_items();
    assert_in_bounds(idx);
    return &data[idx];
  }

  size_t size() { return len; }

private:
  inline void assert_has_items() { assert(len > 0); }
  inline void assert_has_space() { assert(len < Capacity); }
  inline void assert_in_bounds(size_t idx) {
    assert(idx >= 0 && idx < Capacity);
  }

private:
  std::array<DataType, Capacity> data;
  size_t len = 0;
};
