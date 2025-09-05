#pragma once

#include "Spark/defines.h"
#include "Spark/math/math_types.h"

/**
 * @brief Returns the number of characters in a string, including the null terminating character. (i.e. "test" returns 5)
 */
SAPI u32 string_length(const char* str);
SAPI char* string_duplicate(const char* str);

// Case-sensitive string comparison. True if the same, otherwise false.
SAPI b8 string_nequal(const char* a, const char* b, u32 n);
SAPI b8 string_equal(const char* a, const char* b);


// Case-insensitive string comparison. True if the same, otherwise false.
SAPI b8 string_equali(const char* str0, const char* str1);


// Performs string formatting to dest given format string and parameters.
SAPI s32 string_format(char* dest, const char* format, ...);

//
/**
 * Performs variadic string formatting to dest given format string and va_list.
 * @param dest The destination for the formatted string.
 * @param format The string to be formatted.
 * @param va_list The variadic argument list.
 * @returns The size of the data written.
 */
SAPI s32 string_format_v(char* dest, const char* format, void* va_list);

SAPI char* string_copy(char* dest, const char* source);

SAPI char* string_ncopy(char* dest, const char* source, s64 length);

SAPI char* string_trim(char* str);

SAPI void string_mid(char* dest, const char* source, s32 start, s32 length);

/**
 * @brief Returns the index of the first occurance of c in str; otherwise -1.
 * 
 * @param str The string to be scanned.
 * @param c The character to search for.
 * @return The index of the first occurance of c; otherwise -1 if not found. 
 */
SAPI s32 string_index_of(char* str, char c);

/**
 * @brief Attempts to parse a vector from the provided string.
 * 
 * @param str The string to parse from. Should be space-delimited. (i.e. "1.0 2.0 3.0 4.0")
 * @param out_vector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_vec4(char* str, vec4* out_vector);

/**
 * @brief Attempts to parse a vector from the provided string.
 * 
 * @param str The string to parse from. Should be space-delimited. (i.e. "1.0 2.0 3.0")
 * @param out_vector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_vec3(char* str, vec3* out_vector);

/**
 * @brief Attempts to parse a vector from the provided string.
 * 
 * @param str The string to parse from. Should be space-delimited. (i.e. "1.0 2.0")
 * @param out_vector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_vec2(char* str, vec2* out_vector);

/**
 * @brief Attempts to parse a 32-bit floating-point number from the provided string.
 * 
 * @param str The string to parse from. Should *not* be postfixed with 'f'.
 * @param f A pointer to the float to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_f32(char* str, f32* f);

/**
 * @brief Attempts to parse a 64-bit floating-point number from the provided string.
 * 
 * @param str The string to parse from.
 * @param f A pointer to the float to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_f64(char* str, f64* f);

/**
 * @brief Attempts to parse an 8-bit signed integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param i A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_s8(char* str, s8* i);

/**
 * @brief Attempts to parse a 16-bit signed integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param i A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_s16(char* str, s16* i);

/**
 * @brief Attempts to parse a 32-bit signed integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param i A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_s32(char* str, s32* i);

/**
 * @brief Attempts to parse a 64-bit signed integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param i A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_s64(char* str, s64* i);

/**
 * @brief Attempts to parse an 8-bit unsigned integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param u A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_u8(char* str, u8* u);

/**
 * @brief Attempts to parse a 16-bit unsigned integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param u A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_u16(char* str, u16* u);

/**
 * @brief Attempts to parse a 32-bit unsigned integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param u A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_u32(char* str, u32* u);

/**
 * @brief Attempts to parse a 64-bit unsigned integer from the provided string.
 * 
 * @param str The string to parse from.
 * @param u A pointer to the int to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_u64(char* str, u64* u);

/**
 * @brief Attempts to parse a boolean from the provided string.
 * "true" or "1" are considered true; anything else is false.
 * 
 * @param str The string to parse from. "true" or "1" are considered true; anything else is false.
 * @param b A pointer to the boolean to write to.
 * @return True if parsed successfully; otherwise false.
 */
SAPI b8 string_to_bool(char* str, b8* b);


/**
 * @brief Empties the provided string by setting the first character to 0.
 * 
 * @param str The string to be emptied.
 * @return A pointer to str. 
 */
SAPI char* string_empty(char* str);

SAPI char* string_concat(char* dest, const char* append);
SAPI char* string_nconcat(char* dest, const char* append, u32 max_size);

SAPI char* string_start(const char* string, const char* substring);
