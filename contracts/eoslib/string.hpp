#pragma once
#include <eoslib/types.hpp>
#include <eoslib/system.h>
#include <eoslib/memory.hpp>
#include <eoslib/print.hpp>

namespace eos {

  class String {

  private:
    uint32_t size;
    char* data;
    bool own_memory;
    uint32_t* refcount;

    // Release data if no more string reference to it
    void release_data_if_needed() {
      if (own_memory && refcount != nullptr) {
        (*refcount)--;
        if (*refcount == 0) {
          free(data);
        }
      }
    }

  public:
    /**
     * Default constructor
     */
    String() : size(0), data(nullptr), own_memory(false), refcount(nullptr) {
    }

    /**
     * Constructor to create string with reserved space
     * @param s size to be reserved (in number o)
     */
    String(uint32_t s) : size(s), own_memory(true) {
      data = (char *)malloc(s * sizeof(char));
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;
    }

    /**
     * Constructor to create string with given data and size
     * @param d    data
     * @param s    size of the string (in number of bytes)
     * @param copy true to have the data copied and owned by the object
     */
    String(char* d, uint32_t s, bool copy) {
      assign(d, s, copy);
    }

    // Copy constructor
    String(const String& obj) {
      if (this != &obj) {
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
    }

    // Destructor
    ~String() {
      release_data_if_needed();
    }

    // Get size of the string (in number of bytes)
    const uint32_t get_size() const {
      return size;
    }

    // Get the underlying data of the string
    const char* get_data() const {
      return data;
    }

    // Check if it owns memory
    const bool is_own_memory() const {
      return own_memory;
    }

    // Get the ref count
    const uint32_t get_refcount() const {
      return *refcount;
    }

    /**
     * Assign string with new data and size
     * @param  d    data
     * @param  s    size (in number of bytes)
     * @param  copy true to have the data copied and owned by the object
     * @return      the current string
     */
    String& assign(char* d, uint32_t s, bool copy) {
      release_data_if_needed();

      if (copy) {
        data = (char *)malloc(s * sizeof(char));
        memcpy(data, d, s * sizeof(char));
        own_memory = true;
        refcount = (uint32_t*)malloc(sizeof(uint32_t));
        *refcount = 1;
      } else {
        data = d;
        own_memory = false;
        refcount = nullptr;
      }
      size = s;

      return *this;
    }

    String substr(uint32_t offset, uint32_t substr_size, bool copy) {
      assert((offset < size) && (offset + substr_size < size), "out of bound");
      return String(data + offset, substr_size, copy);
    }

    // Count the number of characters in the string
    uint32_t strlen() {
      if (size > 0 && *(data + size - 1) == '\0') {
        return size/sizeof(char) - 1;
      } else {
        return size/sizeof(char);
      }
    }

    // Count the number of character in the UTF8 string
    uint32_t strlenUTF8() {
      uint32_t len = 0;

      char* iterator = data;
      char* end = data + size;

      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string
        end = end - 1;
      }

      while (iterator != end) {
        uint8_t byte = (uint8_t)(*iterator);
        // Valid UTF-8 are
        // 0xxxxxxx
        // 110xxxxx 10xxxxxx
        // 1110xxxx 10xxxxxx 10xxxxxx
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        // which means 10xxxxxx (0x80 - 0xBF) can't be independent
        if (!(byte >= 0x80 && byte <= 0xBF)) {
          len++;
        }
        iterator++;
      }
      return len;
    }


    char operator [] (const uint32_t index) {
      assert(index < size, "index out of bound");
      return *(data + index);
    }

    String& operator = (const String& obj) {
      if (this != &obj) {
        release_data_if_needed();
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
      return *this;
    }

    String& operator += (const String& str){
      assert((size + str.size > size) && (size + str.size > str.size), "overflow");

      char* new_data;
      uint32_t new_size;
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string, remove the \0 when concatenates
        new_size = size - 1 + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, (size - 1) * sizeof(char));
        memcpy(new_data + size - 1, str.data, str.size * sizeof(char));
      } else {
        new_size = size + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, size * sizeof(char));
        memcpy(new_data + size, str.data, str.size * sizeof(char));
      }

      // Release old data
      release_data_if_needed();
      // Assign new data
      data = new_data;

      size = new_size;
      own_memory = true;
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;

      return *this;
    }

    // Compare two strings
    // 1 if the first string is greater than the second string
    // 0 if both strings are equal
    // -1 if the first string is smaller than the second string
    int32_t compare(const String& str) const {
      const char* str1_iterator = data;
      const char* str2_iterator = str.data;

      const char* str1_end = data + size;
      const char* str2_end = str.data + str.size;

      while (str1_iterator != str1_end || str2_iterator != str2_end) {
        if (str1_iterator == str1_end) {
          return -1;
        } else if (str2_iterator == str2_end) {
          return 1;
        } else if (*str1_iterator > *str2_iterator) {
          return 1;
        } else if (*str1_iterator < *str2_iterator) {
          return -1;
        }
        ++str1_iterator;
        ++str2_iterator;
      }
      return 0;
    }

    friend bool operator < (const String& lhs, const String& rhs) {
      return lhs.compare(rhs) < 0;
    }

    friend bool operator > (const String& lhs, const String& rhs) {
      return lhs.compare(rhs) > 0;
    }

    friend bool operator == (const String& lhs, const String& rhs) {
      return lhs.compare(rhs) == 0;
    }

    friend bool operator != (const String& lhs, const String& rhs) {
      return lhs.compare(rhs) != 0;
    }

    friend String operator + (String lhs, const String& rhs) {
      return lhs += rhs;
    }

    void print() const {
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string
        prints(data);
      } else {
        // Non null terminated string
        // We need to specify the size of string so it knows where to stop
        prints_l(data, size);
      }
   }
  };
}
