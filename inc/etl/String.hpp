#pragma once

#include <cstddef>
#include <etl/cstring.h>

/**
 * A fixed-size string
 *
 * This class implements a string (i.e. an array of bytes) that has a maximum size (\p MAX_SIZE)
 * known at compile time. As such, we can predict how much space the string will take, and prevent
 * memory issues.
 *
 * Even though the maximum size (capacity) of the script should be specified beforehand, the size
 * of the actual content is variable, and can be accessed quickly using the `String::size()`
 * function.
 *
 * This class is a child of etl::string that provides some extra functionality that might be
 * useful to us. For the full documentation, look at https://www.etlcpp.com/string.html
 *
 * Strings defined using the String class <b>do NOT have a null-terminating byte</b>
 *
 * @tparam MAX_SIZE The maximum size of the strings
 */
template<const size_t MAX_SIZE>
class String : public etl::string<MAX_SIZE> {
public:
    /**
     * String constructor from a uint8_t array, with a length equal to \p MAX_SIZE
     *
     * The array does NOT need to be null-terminated.
     *
     * @param data The array of uint8_t data
     */
    explicit String(const uint8_t *data) // NOLINTNEXTLINE(google-explicit-constructor)
            : etl::string<MAX_SIZE>(reinterpret_cast<const char *>(data), MAX_SIZE) {}

    /**
     * An empty string constructor. An empty etl::string is allocated.
     */
    String() : etl::string<MAX_SIZE>() {}

    /**
     * String constructor from a uint8_t array
     *
     * The array does NOT need to be null-terminated.
     *
     * @param data The array of uint8_t data
     * @param count The number of bytes to include
     */
    String(const uint8_t *data, size_t count) : etl::string<MAX_SIZE>(reinterpret_cast<const char *>(data), count) {}

    /**
     * Declaration of the constructor from const char*s that calls the parent constructor
     *
     * This is required for some reason, so that C++ recognises and converts string literals
     * automatically
     *
     * @param text The pointer to a null terminated string
     *
     */
    explicit String(const char *text) // NOLINTNEXTLINE(google-explicit-constructor)
            : etl::string<MAX_SIZE>(text) {}
};
