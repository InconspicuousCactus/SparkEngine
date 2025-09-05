#include "Spark/core/sstring.h"
#include "Spark/core/smemory.h"
#include "Spark/math/math_types.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>   // isspace


#ifndef _MSC_VER
#include <strings.h>
#endif

u32 
string_length(const char* str) {
    return strlen(str);
}
char* 
string_duplicate(const char* str) {
    u32 len = string_length(str) + 1;
    char* copy = sallocate(len, MEMORY_TAG_STRING);
    scopy_memory(copy, str, len);
    return copy;
}

b8 
string_equal(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

b8 
string_nequal(const char* a, const char* b, u32 n) {
    return strncmp(a, b, n) == 0;
}


// Case-insensitive string comparison. True if the same, otherwise false.
b8 string_equali(const char* str0, const char* str1) {
#if defined(__GNUC__)
    return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
    return _strcmpi(str0, str1) == 0;
#endif
}

s32 
string_format(char* dest, const char* format, ...) {
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        s32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

s32 
string_format_v(char* dest, const char* format, void* va_listp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        s32 written = vsnprintf(buffer, sizeof(buffer) - 1, format, va_listp);
        buffer[written] = 0;
        scopy_memory(dest, buffer, written + 1);

        return written;
    }
    return -1;
}


char* string_copy(char* dest, const char* source) {
    return strcpy(dest, source);
}

char* string_ncopy(char* dest, const char* source, s64 length) {
    return strncpy(dest, source, length);
}

char* string_trim(char* str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str) {
        char* p = str;
        while (*p) {
            p++;
        }
        while (isspace((unsigned char)*(--p)))
            ;

        p[1] = '\0';
    }

    return str;
}

void string_mid(char* dest, const char* source, s32 start, s32 length) {
    if (length == 0) {
        return;
    }
    u64 src_length = string_length(source);
    if (start >= src_length) {
        dest[0] = 0;
        return;
    }
    if (length > 0) {
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = 0;
    } else {
        // If a negative value is passed, proceed to the end of the string.
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + j] = 0;
    }
}

s32 string_index_of(char* str, char c) {
    if (!str) {
        return -1;
    }
    u32 length = string_length(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == c) {
                return i;
            }
        }
    }

    return -1;
}

b8 string_to_vec4(char* str, vec4* out_vector) {
    if (!str) {
        return false;
    }

    szero_memory(out_vector, sizeof(vec4));
    s32 result = sscanf(str, "%f %f %f %f", &out_vector->x, &out_vector->y, &out_vector->z, &out_vector->w);
    return result != -1;
}

b8 string_to_vec3(char* str, vec3* out_vector) {
    if (!str) {
        return false;
    }

    szero_memory(out_vector, sizeof(vec3));
    s32 result = sscanf(str, "%f %f %f", &out_vector->x, &out_vector->y, &out_vector->z);
    return result != -1;
}

b8 string_to_vec2(char* str, vec2* out_vector) {
    if (!str) {
        return false;
    }

    szero_memory(out_vector, sizeof(vec2));
    s32 result = sscanf(str, "%f %f", &out_vector->x, &out_vector->y);
    return result != -1;
}

b8 string_to_f32(char* str, f32* f) {
    if (!str) {
        return false;
    }

    *f = 0;
    s32 result = sscanf(str, "%f", f);
    return result != -1;
}

b8 string_to_f64(char* str, f64* f) {
    if (!str) {
        return false;
    }

    *f = 0;
    s32 result = sscanf(str, "%lf", f);
    return result != -1;
}

b8 string_to_s8(char* str, s8* i) {
    if (!str) {
        return false;
    }

    *i = 0;
    s32 result = sscanf(str, "%hhi", i);
    return result != -1;
}

b8 string_to_s16(char* str, s16* i) {
    if (!str) {
        return false;
    }

    *i = 0;
    s32 result = sscanf(str, "%hi", i);
    return result != -1;
}

b8 string_to_s32(char* str, s32* i) {
    if (!str) {
        return false;
    }

    *i = 0;
    s32 result = sscanf(str, "%i", i);
    return result != -1;
}

b8 string_to_s64(char* str, s64* i) {
    if (!str) {
        return false;
    }

    *i = 0;
    s32 result = sscanf(str, "%li", i);
    return result != -1;
}

b8 string_to_u8(char* str, u8* u) {
    if (!str) {
        return false;
    }

    *u = 0;
    s32 result = sscanf(str, "%hhu", u);
    return result != -1;
}

b8 string_to_u16(char* str, u16* u) {
    if (!str) {
        return false;
    }

    *u = 0;
    s32 result = sscanf(str, "%hu", u);
    return result != -1;
}

b8 string_to_u32(char* str, u32* u) {
    if (!str) {
        return false;
    }

    *u = 0;
    s32 result = sscanf(str, "%u", u);
    return result != -1;
}

b8 string_to_u64(char* str, u64* u) {
    if (!str) {
        return false;
    }

    *u = 0;
    s32 result = sscanf(str, "%lu", u);
    return result != -1;
}

b8 string_to_bool(char* str, b8* b) {
    if (!str) {
        return false;
    }

    return string_equal(str, "1") || string_equali(str, "true");
}

char* string_empty(char* str) {
    if (str) {
        str[0] = 0;
    }

    return str;
}

char* string_concat(char* dest, const char* append) {
    return strcat(dest, append);
}
char* string_nconcat(char* dest, const char* append, u32 max_size) {
    max_size -= string_length(dest);
    return strncat(dest, append, max_size);
}

char* string_start(const char* string, const char* substring) {
    return strstr(string, substring);
}
