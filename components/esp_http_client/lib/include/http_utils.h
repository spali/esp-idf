/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _HTTP_UTILS_H_
#define _HTTP_UTILS_H_
#include <sys/time.h>

/**
 * @brief      Assign new_str to *str pointer, and realloc *str if it not NULL
 *
 * @param      str      pointer to string pointer
 * @param      new_str  assign this string to str
 * @param      len      length of string, less than 0 if new_str is zero terminated
 *
 * @return
 *  - new_str pointer
 *  - NULL
 */
char *http_utils_assign_string(char **str, const char *new_str, int len);

/**
 * @brief      Realloc *str and append new_str to it if new_str is not NULL; return *str pointer if new_str is NULL
 *
 * @param      str      pointer to string pointer
 * @param      new_str  append this string to str
 * @param      len      length of string, less than 0 if new_str is zero terminated
 *
 * @return
 *  - *str pointer
 */
char *http_utils_append_string(char **str, const char *new_str, int len);

/**
 * @brief      Remove white space at begin and end of string
 *
 * @param[in]  str   The string
 *
 * @return     New strings have been trimmed
 */
void http_utils_trim_whitespace(char **str);

/**
 * @brief      Gets the string between 2 string.
 *             It will allocate a new memory space for this string, so you need to free it when no longer use
 *
 * @param[in]  str    The source string
 * @param[in]  begin  The begin string
 * @param[in]  end    The end string
 *
 * @return     The string between begin and end
 */
char *http_utils_get_string_between(const char *str, const char *begin, const char *end) __attribute__((deprecated("Use http_utils_get_substring_between instead.")));;

/**
 * @brief      Returns a string that contains the part after the search string till the end of the source string.
 *             It will allocate a new memory space for this string, so you need to free it when no longer used
 *
 * @param[in]  str    The source string
 * @param[in]  begin  The search string
 *
 * @return     The string between begin and the end of str
 */
char *http_utils_get_string_after(const char *str, const char *begin) __attribute__((deprecated("Use http_utils_get_substring_after instead.")));;

/**
 * @brief      Extracts the substring between two specified delimiters.
 *             Allocates memory for the extracted substring and stores it in `out`.
 *             The caller must free the allocated memory when no longer needed.
 *
 * @param[in]  str    The source string to search.
 * @param[in]  begin  The starting delimiter string.
 * @param[in]  end    The ending delimiter string.
 * @param[out] out    Pointer to store the allocated substring. Set to NULL if the substring is not found.
 *
 * @return
 * - ESP_OK: Operation succeeded (even if no substring is found).
 * - ESP_ERR_NO_MEM: Memory allocation failed.
 */
esp_err_t http_utils_get_substring_between(const char *str, const char *begin, const char *end, char **out);

/**
 * @brief      Extracts the substring starting after a specified delimiter until the end of the source string.
 *             Allocates memory for the extracted substring and stores it in `out`.
 *             The caller must free the allocated memory when no longer needed.
 *
 * @param[in]  str    The source string to search.
 * @param[in]  begin  The delimiter string to search for.
 * @param[out] out    Pointer to store the allocated substring. Set to NULL if the substring is not found.
 *
 * @return
 * - ESP_OK: Operation succeeded (even if no substring is found).
 * - ESP_ERR_NO_MEM: Memory allocation failed.
 */
esp_err_t http_utils_get_substring_after(const char *str, const char *begin, char **out);

/**
 * @brief      Join 2 strings to one
 *             It will allocate a new memory space for this string, so you need to free it when no longer use
 *
 * @param[in]  first_str   The first string
 * @param[in]  len_first   The length first
 * @param[in]  second_str  The second string
 * @param[in]  len_second  The length second
 *
 * @return
 * - New string pointer
 * - NULL: Invalid input
 */
char *http_utils_join_string(const char *first_str, size_t len_first, const char *second_str, size_t len_second);

/**
 * @brief      Check if ``str`` is start with ``start``
 *
 * @param[in]  str    The string
 * @param[in]  start  The start
 *
 * @return
 *     - (-1) if length of ``start`` larger than length of ``str``
 *     - (1) if ``start`` NOT starts with ``start``
 *     - (0) if ``str`` starts with ``start``
 */
int http_utils_str_starts_with(const char *str, const char *start);

#endif
