#ifndef DMLC_BUILD_CONFIG_H_
#define DMLC_BUILD_CONFIG_H_

/* #undef DMLC_FOPEN_64_PRESENT */

#if !defined(DMLC_FOPEN_64_PRESENT) && DMLC_USE_FOPEN64
  #define DMLC_EMIT_FOPEN64_REDEFINE_WARNING
  #define fopen64 std::fopen
#endif

/* #undef DMLC_CXXABI_H_PRESENT */
/* #undef DMLC_EXECINFO_H_PRESENT */

#if (defined DMLC_CXXABI_H_PRESENT) && (defined DMLC_EXECINFO_H_PRESENT)
  #define DMLC_LOG_STACK_TRACE 1
  #define DMLC_LOG_STACK_TRACE_SIZE 10
/* #undef DMLC_EXECINFO_H */
#endif

/* #undef DMLC_NANOSLEEP_PRESENT */

#define DMLC_CMAKE_LITTLE_ENDIAN 1

#endif  // DMLC_BUILD_CONFIG_DEFAULT_H_
