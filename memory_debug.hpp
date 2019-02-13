#ifndef MEMORY_DEBUG_2019_02_12_1507
#define MEMORY_DEBUG_2019_02_12_1507

#include <cassert> // assert
#include <iostream> // cout, endl
#include <string> // string, to_string
#include <vector>
#include <iostream>

// A better way to find memory bugs.
//
// Usage: when requiring memory from the heap, instead of
// 
//   int* ary = new int[16];
//   ary[3] = 666;
//   cout << ary[3] << endl;
//   cout << *(ary + 3) << endl;
//   delete [] ary;
//
// Use this:
//
//   PTR(int) ary = NEW_ARRAY(int, 16);
//   ary[3] = 666;
//   cout << ary[3] << endl;
//   cout << *(ary + 3) << endl;
//   DELETE_ARRAY(ary)
//
// PTR(int) supports `+`, `-`, `*`, `++`, `--` operators

namespace mem {

struct Trace {
  Trace(const std::string& file, int line)
    : file(file), line(line) {}

  Trace(): Trace("", 0) {}

  Trace& operator=(const Trace& that) {
    file = that.file;
    line = that.line;
    return *this;
  }

  std::string file;
  int line;

  std::string to_s() const {
    return file + ":" + std::to_string(line);
  }
};

struct Block {
  virtual ~Block() {}
  virtual void free(const Trace&, bool single) = 0;
  virtual void* data() const = 0;
  virtual size_t size() const = 0;
  virtual size_t refcount() const = 0;
};

template <typename T>
class Trunk: public Block {
public:
  class Iter {
    friend class Trunk;
  public:
    T& operator*() { return _trunk[_idx]; }
    const T& operator*() const { return _trunk[_idx]; }

    Iter operator++() {
      Iter old = *this;
      ++_idx;
      return old;
    }

    Iter operator++(int) {
      ++_idx;
      return *this;
    }

    Iter operator--() {
      Iter old = *this;
      --_idx;
      return old;
    }

    Iter operator--(int) {
      --_idx;
      return *this;
    }

    Iter operator+(int offset) {
      return Iter(_trunk, _idx + offset);
    }

    Iter operator-(int offset) {
      return Iter(_trunk, _idx - offset);
    }
  private:
    Iter(Trunk* trunk, size_t idx = 0)
      : _trunk(trunk),
        _idx(idx) {}

    Iter(const Iter& that)
      : Iter(that._trunk, that._idx) {}

    Iter& operator=(const Iter& that) {
      _trunk = that._trunk;
      _idx = that._idx;
      return *this;
    }
    
    Trunk* _trunk;
    size_t _idx;
  };

  Trunk(
    const Trace& allocated,
    size_t count = 1,
    bool single = false
  ) : _allocated(allocated),
      _freed(Trace()),
      _single(single),
      _data(new T[count]),
      _size(count),
      _iter(Iter(this, 0)),
      _deleted(new bool(false)),
      _refcount(new size_t(1))
  {
    if (single) assert(count == 1);
    else assert(count >= 1);
  }

  Trunk& operator=(const Trunk& that) {
    if (this == &that) { return *this; }
    give_up_ref();

    _allocated = that._allocated;
    _freed = that._freed;
    _single = that._single;
    _data = that._data;
    _size = that._size;
    _iter = Iter(this, that._iter._idx);
    _deleted = that._deleted;
    _refcount = that._refcount;

    ++*_refcount;
    return *this;
  }

  virtual ~Trunk() {
    give_up_ref();
  }

  void free(const Trace& trace, bool single) {
    if (_single && !single) {
      std::cout <<
      "Allocated as NEW but freed as DELETE_ARRAY!"
      "\n-> Allocated here: (" + _allocated.to_s() + ")."
      "\n-> Freed here: (" + trace.to_s() + ")."
      << std::endl;
      cleanup_and_exit(21);
    } else if (!_single && single) {
      std::cout <<
      "Allocated as NEW_ARRAY but freed as DELETE!"
      "\n-> Allocated here: (" + _allocated.to_s() + ")."
      "\n-> Freed here: (" + trace.to_s() + ")."
      << std::endl;
      cleanup_and_exit(22);
    }
    if (*_deleted) {
      std::cout << "Double free detected!"
      "\n-> 1st free: (" + _freed.to_s() + ")."
      "\n-> 2nd free: (" + trace.to_s() + ")."
      << std::endl;
      exit(12);
    }
    delete [] _data;
    *_deleted = true;
    _freed = trace;
  }

  void* data() const { return (void*) _data; }
  size_t size() const { return _size; }
  size_t refcount() const { return *_refcount; }

  const T& operator[](size_t idx) const {
    validate(idx);
    return _data[idx];
  }

  T& operator[](size_t idx) {
    validate(idx);
    return _data[idx];
  }

  T& operator*() {
    check_access_after_freed();
    return _data[0];
  }

  const T& operator*() const {
    check_access_after_freed();
    return _data[0];
  }

  Iter operator++() {
    return _iter++;
  }

  Iter operator++(int) {
    return ++_iter;
  }

  Iter operator--() {
    return _iter--;
  }

  Iter operator--(int) {
    return --_iter;
  }

  Iter operator+(int offset) {
    return _iter + offset;
  }

  Iter operator-(int offset) {
    return _iter - offset;
  }

private:
  Trace _allocated;
  Trace _freed;

  bool _single;
  T* _data;
  size_t _size;

  Iter _iter;

  bool* _deleted;
  size_t* _refcount;

  void give_up_ref() {
    if (--*_refcount == 0 && !*_deleted) {
      std::cout <<
      "Memory leak detected!"
      "\n-> Allocated here: (" + _allocated.to_s() + ")."
      << std::endl;
      cleanup_and_exit(11);
    }
  }

  void cleanup_and_exit(int no) {
    delete [] _data;
    *_deleted = true;
    exit(no);
  }

  void validate(size_t idx) {
    if (idx >= _size) {
      std::cout << "Index out of boundary!"
      "\n-> Index: " + std::to_string(idx) +
      ", size: " + std::to_string(_size) + "."
      "\n-> Allocated here: (" + _allocated.to_s() + ")."
      << std::endl;
      cleanup_and_exit(13);
    }
    check_access_after_freed();
  }

  void check_access_after_freed() {
    if (*_deleted) {
      std::cout << "Access after freed!"
      "\n-> 1st free: (" + _freed.to_s() + ")." << std::endl;
      exit(14);
    }
  }
}; // Trunk

class Allocator {
public:
  virtual ~Allocator() {
    for (auto ptr : pool) delete ptr;
  }

  std::vector<Block*> pool;

  template <typename T>
  Trunk<T>& alloc(
    size_t count,
    const std::string& file,
    int line,
    bool single
  ) {
    Trunk<T>* p = new Trunk<T>(Trace(file, line), count, single);
    pool.push_back(p);
    return *p;
  }

  void overview() {
    for (Block* p : pool) {
      std::cout << p->data()
      << " with refcount = " << p->refcount()
      << "; size = " << p->size()
      << std::endl;
    }
  }
}; // Allocator

static Allocator _alloc;

} // namespace mem

#define NEW_ARRAY(T, count) \
  mem::_alloc.alloc<T>(count, __FILE__, __LINE__, false)

#define DELETE_ARRAY(trunk) \
  trunk.free(mem::Trace(__FILE__, __LINE__), false)

#define NEW(T) \
  mem::_alloc.alloc<T>(1, __FILE__, __LINE__, true)

#define DELETE(trunk) \
  trunk.free(mem::Trace(__FILE__, __LINE__), true)

#define PTR(T) mem::Trunk<T>&

void mem_overview() {
  mem::_alloc.overview();
}

#endif
