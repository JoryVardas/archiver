#ifndef ARCHIVER_CLONE_ARRAY_HPP
#define ARCHIVER_CLONE_ARRAY_HPP

#include <memory>

/* This function is only called to clone the parameters given to main so that
 * They can be passed to the cxxsubs library so if there is an error here ther
 * is nothing that can be done about it. */
template <typename OutputElementType>
auto cloneCharPtrArray(const char* const* const oldArray,
                       std::size_t length) noexcept
  -> std::unique_ptr<OutputElementType> {
  OutputElementType* newArray = new OutputElementType[length];
  std::fill(newArray, newArray + length, nullptr);

  try {
    std::transform(oldArray, oldArray + length, newArray,
                   [](const char* const cStr) {
                     /* Add 1 to the result of strlen to ensure that  our
                      * length includes the null character. */
                     const auto strLength = std::strlen(cStr) + 1;
                     char* newCStr = new char[strLength];
                     std::strncpy(newCStr, cStr, strLength);
                     return newCStr;
                   });
  } catch (...) {
    // Clean up anything we were able to allocate.
    for (int i = 0; i < length; ++i) {
      delete[] newArray[i];
    }
    delete[] newArray;

    /* This exception will terminate the application, since we are in a
     * noexcept function, but doing it lets us clean up the memory we were
     * able to allocate before we terminate.*/
    throw std::runtime_error(
      "Could not allocate memory to copy char pointer array");
  }
  return std::make_unique<OutputElementType>(newArray);
}
#endif
