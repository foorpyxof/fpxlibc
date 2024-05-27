#include "fpx_string.h"

int fpx_getstringlength(const char* stringToCheck) {
  /**
   * Returns the length from the start of the string
   * until the first occurence of a null-byte (\0).
  */

	int i=0;
	while (stringToCheck[i] != '\0') i++;
  
  return i;
}

int fpx_substringindex(const char* haystack, const char* needle) {
  /**
   * Will find the *first* occurence of a given substring
   * "needle" in the string "haystack".
   * 
   * If not found, returns -1.
  */

  const int haystackLen = fpx_getstringlength(haystack);
  const int needleLen = fpx_getstringlength(needle);
  const char* needleStart = needle;
  
  int foundSubstringIndex = 0;

	for (int i=0; i<haystackLen; i++) {
    if (*haystack == *needle) {
      needle++;
      if (*haystack == *needleStart && !(*(haystack-1) == *needleStart)) {
        foundSubstringIndex = i;
      }
    } else if (*haystack == *needleStart) {
      needle = needleStart+1;
    }else {
      needle = needleStart;
    }
    haystack++;
    if (needle-needleStart == needleLen) {
      return foundSubstringIndex;
    }
  }
  
  return -1;
}

char* fpx_substr_replace(const char* haystack, const char* needle, const char* replacement) {
  /**
   * Replaces the *first* occurence of the given "needle" string
   * within the "haystack" string with the given replacement string.
   * 
   * Returns new heap-allocated, null-terminated char[] when finished.
   * 
   * Otherwise, returns original haystack string if substring was not found.
  */

  const int needleLen = fpx_getstringlength(needle);
  const int replacementLen = fpx_getstringlength(replacement);
  const int haystackStartIndex = fpx_substringindex(haystack, needle);
  const int returnedHaystackLen = fpx_getstringlength(haystack) + fpx_getstringlength(replacement) - fpx_getstringlength(needle);

  // allocate string to return on the heap
  char* returnedHaystack = (char*)malloc(returnedHaystackLen + 1);

  // if haystack doesn't contain needle, return haystack back to the caller
  if (haystackStartIndex < 0) return haystack;

  /*
   * perform magic
   * (fill new string allocated earlier that contains the desired result)
   */ 
  for (int i=0; i<returnedHaystackLen+1; i++) {
    if (i < haystackStartIndex) {
      returnedHaystack[i] = haystack[i];
    } else if (i >= haystackStartIndex + fpx_getstringlength(replacement)) {
      returnedHaystack[i] = haystack[i + needleLen - replacementLen];
    } else {
      returnedHaystack[i] = replacement[i - haystackStartIndex];
    }
  }

  /*
   * add null-byte (\0) to the end of the char array, to denote the end of the string
   * (this null-byte was accounted for when allocating memory for the result string)
   */
  returnedHaystack[returnedHaystackLen] = '\0';
  return returnedHaystack;
}

char* fpx_string_to_upper(const char* input) {
  /**
   * Converts the whole string to its uppercase variant.
   * 
   * Returns new heap-allocated, null-terminated char[] when finished.
  */
  const int inputLength = fpx_getstringlength(input);
  char* inputCopy = (char*) malloc(inputLength + 1);

  for (int i=0; i<inputLength; i++) {
    inputCopy[i] = (input[i] >= 97 && input[i] <= 122) ? (input[i] - 32) : input[i];
  }

  inputCopy[inputLength] = '\0';
  return inputCopy;
}

char* fpx_string_to_lower(const char* input) {
  /**
   * Converts the whole string to its lowercase variant.
   * 
   * Returns new heap-allocated, null-terminated char[] when finished.
  */
  const int inputLength = fpx_getstringlength(input);
  // printf("%d", inputLength);
  char* inputCopy = (char*) malloc(inputLength + 1);

  for(int i=0; i<inputLength; i++) {
    inputCopy[i] = (input[i] >= 65 && input[i] <= 90) ? (input[i] + 32) : input[i];
  }

  inputCopy[inputLength] = '\0';
  return inputCopy;
}