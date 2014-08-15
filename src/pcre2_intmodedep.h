/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
         New API code Copyright (c) 2014 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/


/* This module contains mode-dependent macro and structure definitions. The 
file is #included by pcre2_internal.h if PCRE2_CODE_UNIT_WIDTH is defined.
These mode-dependent items are kept in a separate file so that they can also be
#included multiple times for different code unit widths by pcre2test in order 
to have access to the hidden structures at all supported widths. 

Some of the mode-dependent macros are required at different widths for
different parts of the pcre2test code (in particular, the included
pcre_printint.c file). We undefine them here so that they can be re-defined for
multiple inclusions. Not all of these are used in pcre2test, but it's easier 
just to undefine them all. */

#undef ACROSSCHAR
#undef BACKCHAR
#undef BYTES2CU
#undef CU2BYTES
#undef FORWARDCHAR
#undef GET
#undef GET2
#undef GETCHAR
#undef GETCHARINC
#undef GETCHARINCTEST
#undef GETCHARLEN
#undef GETCHARLENTEST
#undef GETCHARTEST
#undef GET_EXTRALEN
#undef HAS_EXTRALEN
#undef IMM2_SIZE
#undef MAX_255
#undef MAX_MARK
#undef MAX_PATTERN_SIZE
#undef MAX_UTF_SINGLE_CU
#undef NOT_FIRSTCHAR
#undef PUT
#undef PUT2
#undef PUT2INC
#undef PUTCHAR
#undef PUTINC
#undef TABLE_GET



/* -------------------------- MACROS ----------------------------- */

/* PCRE keeps offsets in its compiled code as at least 16-bit quantities
(always stored in big-endian order in 8-bit mode) by default. These are used,
for example, to link from the start of a subpattern to its alternatives and its
end. The use of 16 bits per offset limits the size of an 8-bit compiled regex
to around 64K, which is big enough for almost everybody. However, I received a
request for an even bigger limit. For this reason, and also to make the code
easier to maintain, the storing and loading of offsets from the compiled code
unit string is now handled by the macros that are defined here.

The macros are controlled by the value of LINK_SIZE. This defaults to 2, but 
values of 2 or 4 are also supported. */

/* ------------------- 8-bit support  ------------------ */

#if PCRE2_CODE_UNIT_WIDTH == 8

#if LINK_SIZE == 2
#define PUT(a,n,d)   \
  (a[n] = (d) >> 8), \
  (a[(n)+1] = (d) & 255)
#define GET(a,n) \
  (((a)[n] << 8) | (a)[(n)+1])
#define MAX_PATTERN_SIZE (1 << 16)

#elif LINK_SIZE == 3
#define PUT(a,n,d)       \
  (a[n] = (d) >> 16),    \
  (a[(n)+1] = (d) >> 8), \
  (a[(n)+2] = (d) & 255)
#define GET(a,n) \
  (((a)[n] << 16) | ((a)[(n)+1] << 8) | (a)[(n)+2])
#define MAX_PATTERN_SIZE (1 << 24)

#elif LINK_SIZE == 4
#define PUT(a,n,d)        \
  (a[n] = (d) >> 24),     \
  (a[(n)+1] = (d) >> 16), \
  (a[(n)+2] = (d) >> 8),  \
  (a[(n)+3] = (d) & 255)
#define GET(a,n) \
  (((a)[n] << 24) | ((a)[(n)+1] << 16) | ((a)[(n)+2] << 8) | (a)[(n)+3])
#define MAX_PATTERN_SIZE (1 << 30)   /* Keep it positive */

#else
#error LINK_SIZE must be 2, 3, or 4
#endif


/* ------------------- 16-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 16

#if LINK_SIZE == 2
#undef LINK_SIZE
#define LINK_SIZE 1
#define PUT(a,n,d)   \
  (a[n] = (d))
#define GET(a,n) \
  (a[n])
#define MAX_PATTERN_SIZE (1 << 16)

#elif LINK_SIZE == 3 || LINK_SIZE == 4
#undef LINK_SIZE
#define LINK_SIZE 2
#define PUT(a,n,d)   \
  (a[n] = (d) >> 16), \
  (a[(n)+1] = (d) & 65535)
#define GET(a,n) \
  (((a)[n] << 16) | (a)[(n)+1])
#define MAX_PATTERN_SIZE (1 << 30)  /* Keep it positive */

#else
#error LINK_SIZE must be 2, 3, or 4
#endif


/* ------------------- 32-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 32
#undef LINK_SIZE
#define LINK_SIZE 1
#define PUT(a,n,d)   \
  (a[n] = (d))
#define GET(a,n) \
  (a[n])
#define MAX_PATTERN_SIZE (1 << 30)  /* Keep it positive */

#else
#error Unsupported compiling mode
#endif 


/* --------------- Other mode-specific macros ----------------- */

/* PCRE uses some other (at least) 16-bit quantities that do not change when
the size of offsets changes. There are used for repeat counts and for other
things such as capturing parenthesis numbers in back references. 

Define the number of code units required to hold a 16-bit count/offset, and
macros to load and store such a value. For reasons that I do not understand,
the expression in the 8-bit GET2 macro is treated by gcc as a signed
expression, even when a is declared as unsigned. It seems that any kind of
arithmetic results in a signed value. Hence the cast. */

#if PCRE2_CODE_UNIT_WIDTH == 8
#define IMM2_SIZE 2
#define GET2(a,n) (unsigned int)(((a)[n] << 8) | (a)[(n)+1])
#define PUT2(a,n,d) a[n] = (d) >> 8, a[(n)+1] = (d) & 255

#else  /* Code units are 16 or 32 bits */
#define IMM2_SIZE 1
#define GET2(a,n) a[n]
#define PUT2(a,n,d) a[n] = d  
#endif

/* Other macros that are different for 8-bit mode. The MAX_255 macro checks
whether its argument is less than 256. The maximum length of a MARK name must
fit in one code unit; currently it is set to 255 or 65535. The TABLE_GET macro
is used to access elements of tables containing exactly 256 items. When code
points can be greater than 255, a check is needed before accessing these
tables. */

#if PCRE2_CODE_UNIT_WIDTH == 8
#define MAX_255(c) TRUE
#define MAX_MARK ((1u << 8) - 1)
#ifdef SUPPORT_UTF
#define SUPPORT_WIDE_CHARS
#endif  /* SUPPORT_UTF */
#define TABLE_GET(c, table, default) ((table)[c])

#else  /* Code units are 16 or 32 bits */
#define MAX_255(c) ((c) <= 255u)
#define MAX_MARK ((1u << 16) - 1)
#define SUPPORT_WIDE_CHARS
#define TABLE_GET(c, table, default) (MAX_255(c)? ((table)[c]):(default))
#endif



/* ----------------- Character-handling macros ----------------- */

/* There is a proposed future special "UTF-21" mode, in which only the lowest
21 bits of a 32-bit character are interpreted as UTF, with the remaining 11
high-order bits available to the application for other uses. In preparation for
the future implementation of this mode, there are macros that load a data item
and, if in this special mode, mask it to 21 bits. These macros all have names
starting with UCHAR21. In all other modes, including the normal 32-bit
library, the macros all have the same simple definitions. When the new mode is
implemented, it is expected that these definitions will be varied appropriately
using #ifdef when compiling the library that supports the special mode. */

#define UCHAR21(eptr)        (*(eptr))
#define UCHAR21TEST(eptr)    (*(eptr))
#define UCHAR21INC(eptr)     (*(eptr)++)
#define UCHAR21INCTEST(eptr) (*(eptr)++)

/* When UTF encoding is being used, a character is no longer just a single
byte in 8-bit mode or a single short in 16-bit mode. The macros for character
handling generate simple sequences when used in the basic mode, and more
complicated ones for UTF characters. GETCHARLENTEST and other macros are not
used when UTF is not supported. To make sure they can never even appear when
UTF support is omitted, we don't even define them. */

#ifndef SUPPORT_UTF

/* #define MAX_UTF_SINGLE_CU */
/* #define HAS_EXTRALEN(c) */
/* #define GET_EXTRALEN(c) */
/* #define NOT_FIRSTCHAR(c) */
#define GETCHAR(c, eptr) c = *eptr;
#define GETCHARTEST(c, eptr) c = *eptr;
#define GETCHARINC(c, eptr) c = *eptr++;
#define GETCHARINCTEST(c, eptr) c = *eptr++;
#define GETCHARLEN(c, eptr, len) c = *eptr;
#define PUTCHAR(c, p) (*p = c, 1)
/* #define GETCHARLENTEST(c, eptr, len) */
/* #define BACKCHAR(eptr) */
/* #define FORWARDCHAR(eptr) */
/* #define ACROSSCHAR(condition, eptr, action) */

#else   /* SUPPORT_UTF */

/* ------------------- 8-bit support  ------------------ */

#if PCRE2_CODE_UNIT_WIDTH == 8
#define MAYBE_UTF_MULTI          /* UTF chars may use multiple code units */

/* The largest UTF code point that can be encoded as a single code unit. */

#define MAX_UTF_SINGLE_CU 127

/* Tests whether the code point needs extra characters to decode. */

#define HAS_EXTRALEN(c) HASUTF8EXTRALEN(c)

/* Returns with the additional number of characters if IS_MULTICHAR(c) is TRUE.
Otherwise it has an undefined behaviour. */

#define GET_EXTRALEN(c) (PRIV(utf8_table4)[(c) & 0x3f])

/* Returns TRUE, if the given character is not the first character
of a UTF sequence. */

#define NOT_FIRSTCHAR(c) (((c) & 0xc0) == 0x80)

/* Get the next UTF-8 character, not advancing the pointer. This is called when
we know we are in UTF-8 mode. */

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if (c >= 0xc0) GETUTF8(c, eptr);

/* Get the next UTF-8 character, testing for UTF-8 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && c >= 0xc0) GETUTF8(c, eptr);

/* Get the next UTF-8 character, advancing the pointer. This is called when we
know we are in UTF-8 mode. */

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if (c >= 0xc0) GETUTF8INC(c, eptr);

/* Get the next character, testing for UTF-8 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-8 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && c >= 0xc0) GETUTF8INC(c, eptr);

/* Get the next UTF-8 character, not advancing the pointer, incrementing length
if there are extra bytes. This is called when we know we are in UTF-8 mode. */

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if (c >= 0xc0) GETUTF8LEN(c, eptr, len);

/* Get the next UTF-8 character, testing for UTF-8 mode, not advancing the
pointer, incrementing length if there are extra bytes. This is called when we
do not know if we are in UTF-8 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && c >= 0xc0) GETUTF8LEN(c, eptr, len);

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-8 mode - we don't put a test within the macro
because almost all calls are already within a block of UTF-8 only code. */

#define BACKCHAR(eptr) while((*eptr & 0xc0) == 0x80) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) while((*eptr & 0xc0) == 0x80) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  while((condition) && ((eptr) & 0xc0) == 0x80) action
  
/* Deposit a character into memory, returning the number of code units. */

#define PUTCHAR(c, p) ((utf && c > MAX_UTF_SINGLE_CU)? \
  PRIV(ord2utf)(c,p) : (*p = c, 1))


/* ------------------- 16-bit support  ------------------ */

#elif PCRE2_CODE_UNIT_WIDTH == 16
#define MAYBE_UTF_MULTI          /* UTF chars may use multiple code units */

/* The largest UTF code point that can be encoded as a single code unit. */

#define MAX_UTF_SINGLE_CU 65535

/* Tests whether the code point needs extra characters to decode. */

#define HAS_EXTRALEN(c) (((c) & 0xfc00) == 0xd800)

/* Returns with the additional number of characters if IS_MULTICHAR(c) is TRUE.
Otherwise it has an undefined behaviour. */

#define GET_EXTRALEN(c) 1

/* Returns TRUE, if the given character is not the first character
of a UTF sequence. */

#define NOT_FIRSTCHAR(c) (((c) & 0xfc00) == 0xdc00)

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer. */

#define GETUTF16(c, eptr) \
   { c = (((c & 0x3ff) << 10) | (eptr[1] & 0x3ff)) + 0x10000; }

/* Get the next UTF-16 character, not advancing the pointer. This is called when
we know we are in UTF-16 mode. */

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if ((c & 0xfc00) == 0xd800) GETUTF16(c, eptr);

/* Get the next UTF-16 character, testing for UTF-16 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, advancing
the pointer. */

#define GETUTF16INC(c, eptr) \
   { c = (((c & 0x3ff) << 10) | (*eptr++ & 0x3ff)) + 0x10000; }

/* Get the next UTF-16 character, advancing the pointer. This is called when we
know we are in UTF-16 mode. */

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if ((c & 0xfc00) == 0xd800) GETUTF16INC(c, eptr);

/* Get the next character, testing for UTF-16 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-16 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16INC(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer, incrementing the length. */

#define GETUTF16LEN(c, eptr, len) \
   { c = (((c & 0x3ff) << 10) | (eptr[1] & 0x3ff)) + 0x10000; len++; }

/* Get the next UTF-16 character, not advancing the pointer, incrementing
length if there is a low surrogate. This is called when we know we are in
UTF-16 mode. */

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if ((c & 0xfc00) == 0xd800) GETUTF16LEN(c, eptr, len);

/* Get the next UTF-816character, testing for UTF-16 mode, not advancing the
pointer, incrementing length if there is a low surrogate. This is called when
we do not know if we are in UTF-16 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16LEN(c, eptr, len);

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-16 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-16 only
code. */

#define BACKCHAR(eptr) if ((*eptr & 0xfc00) == 0xdc00) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) if ((*eptr & 0xfc00) == 0xdc00) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  if ((condition) && ((eptr) & 0xfc00) == 0xdc00) action

/* Deposit a character into memory, returning the number of code units. */

#define PUTCHAR(c, p) ((utf && c > MAX_UTF_SINGLE_CU)? \
  PRIV(ord2utf)(c,p) : (*p = c, 1))


/* ------------------- 32-bit support  ------------------ */

#else

/* These are trivial for the 32-bit library, since all UTF-32 characters fit
into one PCRE2_UCHAR unit. */

#define MAX_UTF_SINGLE_CU (0x10ffffu)
#define HAS_EXTRALEN(c) (0)
#define GET_EXTRALEN(c) (0)
#define NOT_FIRSTCHAR(c) (0)

/* Get the next UTF-32 character, not advancing the pointer. This is called when
we know we are in UTF-32 mode. */

#define GETCHAR(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, testing for UTF-32 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, advancing the pointer. This is called when we
know we are in UTF-32 mode. */

#define GETCHARINC(c, eptr) \
  c = *((eptr)++);

/* Get the next character, testing for UTF-32 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-32 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *((eptr)++);

/* Get the next UTF-32 character, not advancing the pointer, not incrementing
length (since all UTF-32 is of length 1). This is called when we know we are in
UTF-32 mode. */

#define GETCHARLEN(c, eptr, len) \
  GETCHAR(c, eptr)

/* Get the next UTF-32character, testing for UTF-32 mode, not advancing the
pointer, not incrementing the length (since all UTF-32 is of length 1).
This is called when we do not know if we are in UTF-32 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  GETCHARTEST(c, eptr)

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-32 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-32 only
code.

These are all no-ops since all UTF-32 characters fit into one pcre_uchar. */

#define BACKCHAR(eptr) do { } while (0)

/* Same as above, just in the other direction. */

#define FORWARDCHAR(eptr) do { } while (0)

/* Same as above, but it allows a fully customizable form. */

#define ACROSSCHAR(condition, eptr, action) do { } while (0)

/* Deposit a character into memory, returning the number of code units. */

#define PUTCHAR(c, p) (*p = c, 1)

#endif  /* UTF-32 character handling */
#endif  /* SUPPORT_UTF */


/* Mode-dependent macros that have the same definition in all modes. */

#define CU2BYTES(x)     ((x)*((PCRE2_CODE_UNIT_WIDTH/8)))
#define BYTES2CU(x)     ((x)/((PCRE2_CODE_UNIT_WIDTH/8)))
#define PUTINC(a,n,d)   PUT(a,n,d), a += LINK_SIZE
#define PUT2INC(a,n,d)  PUT2(a,n,d), a += IMM2_SIZE


/* ----------------------- HIDDEN STRUCTURES ----------------------------- */

/* NOTE: All these structures *must* start with a pcre2_memctl structure. The 
code that uses them is simpler because it assumes this. */

/* The real general context structure. At present it holds only data for custom 
memory control. */

typedef struct pcre2_real_general_context {
  pcre2_memctl    memctl;
} pcre2_real_general_context;

/* The real compile context structure */

typedef struct pcre2_real_compile_context {
  pcre2_memctl    memctl;
  int       (*stack_guard)(uint32_t);
  const uint8_t *tables;
  uint16_t  bsr_convention;
  uint16_t  newline_convention;
  uint32_t  parens_nest_limit;
} pcre2_real_compile_context;

/* The real match context structure. */

typedef struct pcre2_real_match_context {
  pcre2_memctl  memctl;
#ifdef NO_RECURSE
  pcre2_memctl  stack_memctl;
#endif   
  int       (*callout)(pcre2_callout_block *);
  void      *callout_data; 
  uint16_t  bsr_convention;
  uint16_t  newline_convention;
  uint32_t  match_limit;
  uint32_t  recursion_limit;
} pcre2_real_match_context;

/* The real compiled code structure */

typedef struct pcre2_real_code {
  pcre2_memctl   memctl;          /* Memory control fields */
  const uint8_t *tables;          /* The character tables */
  void    *executable_jit;        /* Pointer to JIT code */  
  uint8_t  start_bitmap[32];      /* Bitmap for starting code unit < 256 */
  size_t   blocksize;             /* Total (bytes) that was malloc-ed */ 
  uint32_t magic_number;          /* Paranoid and endianness check */
  uint32_t compile_options;       /* Options passed to pcre2_compile() */
  uint32_t overall_options;       /* Options after processing the pattern */
  uint32_t flags;                 /* Various state flags */
  uint32_t limit_match;           /* Limit set in the pattern */
  uint32_t limit_recursion;       /* Limit set in the pattern */
  uint32_t first_codeunit;        /* Starting code unit */
  uint32_t last_codeunit;         /* This codeunit must be seen */
  uint16_t bsr_convention;        /* What \R matches */
  uint16_t newline_convention;    /* What is a newline? */  
  uint16_t max_lookbehind;        /* Longest lookbehind (characters) */
  uint16_t minlength;             /* Minimum length of match */ 
  uint16_t top_bracket;           /* Highest numbered group */ 
  uint16_t top_backref;           /* Highest numbered back reference */
  uint16_t name_entry_size;       /* Size (code units) of table entries */
  uint16_t name_count;            /* Number of name entries in the table */
} pcre2_real_code;

/* The real match data structure. */

typedef struct pcre2_real_match_data {
  pcre2_memctl     memctl;
  const pcre2_real_code *code;    /* The pattern used for the match */
  PCRE2_SPTR       subject;       /* The subject that was matched */
  int              rc;            /* The return code from the match */
  PCRE2_OFFSET     leftchar;      /* Offset to leftmost code unit */
  PCRE2_OFFSET     rightchar;     /* Offset to rightmost code unit */
  PCRE2_OFFSET     startchar;     /* Offset to starting code unit */  
  PCRE2_SPTR       mark;          /* Pointer to last mark */  
  uint16_t         oveccount;     /* Number of pairs */
  PCRE2_OFFSET     ovector[1];    /* The first field */ 
} pcre2_real_match_data;


/* ----------------------- PRIVATE STRUCTURES ----------------------------- */

/* These structures are not needed for pcre2test. */

#ifndef PCRE2_PCRE2TEST

/* Structure for maintaining a chain of pointers to the currently incomplete
branches, for testing for left recursion while compiling. */

typedef struct branch_chain {
  struct branch_chain *outer;
  PCRE2_UCHAR *current_branch;
} branch_chain;

/* Structure for building a list of named groups during the first pass of
compiling. */

typedef struct named_group {
  PCRE2_SPTR   name;          /* Points to the name in the pattern */
  int          length;        /* Length of the name */
  uint32_t     number;        /* Group number */
} named_group;

/* Structure for passing "static" information around between the functions
doing the compiling, so that they are thread-safe. */

typedef struct compile_block {
  pcre2_real_compile_context *cx;  /* Points to the compile context */
  const uint8_t *lcc;              /* Points to lower casing table */
  const uint8_t *fcc;              /* Points to case-flipping table */
  const uint8_t *cbits;            /* Points to character type table */
  const uint8_t *ctypes;           /* Points to table of type maps */
  PCRE2_SPTR start_workspace;      /* The start of working space */
  PCRE2_SPTR start_code;           /* The start of the compiled code */
  PCRE2_SPTR start_pattern;        /* The start of the pattern */
  PCRE2_SPTR end_pattern;          /* The end of the pattern */
  PCRE2_UCHAR *hwm;                /* High watermark of workspace */
  PCRE2_UCHAR *name_table;         /* The name/number table */
  size_t workspace_size;           /* Size of workspace */
  uint16_t names_found;            /* Number of entries so far */
  uint16_t name_entry_size;        /* Size of each entry */
  open_capitem *open_caps;         /* Chain of open capture items */
  named_group *named_groups;       /* Points to vector in pre-compile */
  uint32_t named_group_list_size;  /* Number of entries in the list */
  uint32_t external_options;       /* External (initial) options */
  uint32_t external_flags;         /* External flag bits to be set */
  uint32_t bracount;               /* Count of capturing parens as we compile */
  uint32_t final_bracount;         /* Saved value after first pass */
  uint32_t top_backref;            /* Maximum back reference */
  uint32_t backref_map;            /* Bitmap of low back refs */
  uint32_t namedrefcount;          /* Number of backreferences by name */
  uint32_t nltype;                 /* Newline type */
  uint32_t nllen;                  /* Newline string length */
  PCRE2_UCHAR nl[4];               /* Newline string when fixed length */
  int  max_lookbehind;             /* Maximum lookbehind (characters) */
  int  parens_depth;               /* Depth of nested parentheses */
  int  assert_depth;               /* Depth of nested assertions */
  int  req_varyopt;                /* "After variable item" flag for reqbyte */
  BOOL had_accept;                 /* (*ACCEPT) encountered */
  BOOL had_pruneorskip;            /* (*PRUNE) or (*SKIP) encountered */
  BOOL check_lookbehind;           /* Lookbehinds need later checking */
  BOOL dupnames;                   /* Duplicate names exist */
} compile_block;

/* Structure for items in a linked list that represents an explicit recursive
call within the pattern; used by pcre_match(). */

typedef struct recursion_info {
  struct recursion_info *prevrec; /* Previous recursion record (or NULL) */
  unsigned int group_num;         /* Number of group that was called */
  PCRE2_OFFSET *offset_save;      /* Pointer to start of saved offsets */
  uint32_t saved_max;             /* Number of saved offsets */
  uint32_t saved_capture_last;    /* Last capture number */
  PCRE2_SPTR subject_position;    /* Position at start of recursion */
} recursion_info;

/* A similar structure for pcre_dfa_match(). */

typedef struct dfa_recursion_info {
  struct dfa_recursion_info *prevrec;
  PCRE2_SPTR subject_position;
  uint32_t group_num;
} dfa_recursion_info;

/* Structure for building a chain of data for holding the values of the subject
pointer at the start of each subpattern, so as to detect when an empty string
has been matched by a subpattern - to break infinite loops; used by
pcre2_match(). */

typedef struct eptrblock {
  struct eptrblock *epb_prev;
  PCRE2_SPTR epb_saved_eptr;
} eptrblock;

/* Structure for passing "static" information around between the functions
doing traditional NFA matching (pcre2_match() and friends). */

typedef struct match_block {
  pcre2_memctl memctl;            /* For general use */
#ifdef NO_RECURSE
  pcre2_memctl stack_memctl;      /* For "stack" frames */
#endif      
  uint32_t match_call_count;      /* As it says */
  uint32_t match_limit;           /* As it says */
  uint32_t match_limit_recursion; /* As it says */
  BOOL hitend;                    /* Hit the end of the subject at some point */
  BOOL hasthen;                   /* Pattern contains (*THEN) */
  const uint8_t *lcc;             /* Points to lower casing table */
  const uint8_t *fcc;             /* Points to case-flipping table */
  const uint8_t *ctypes;          /* Points to table of type maps */
  PCRE2_OFFSET *ovector;          /* Pointer to the offset vector */
  PCRE2_OFFSET offset_end;        /* One past the end */
  PCRE2_OFFSET offset_max;        /* The maximum usable for return data */
  PCRE2_OFFSET start_offset;      /* The start offset value */
  PCRE2_OFFSET end_offset_top;    /* Highwater mark at end of match */
  uint16_t partial;               /* PARTIAL options */
  uint16_t bsr_convention;        /* \R interpretation */
  uint16_t name_count;            /* Number of names in name table */
  uint16_t name_entry_size;       /* Size of entry in names table */
  PCRE2_SPTR name_table;          /* Table of group names */
  PCRE2_SPTR start_code;          /* For use when recursing */
  PCRE2_SPTR start_subject;       /* Start of the subject string */
  PCRE2_SPTR end_subject;         /* End of the subject string */
  PCRE2_SPTR start_match_ptr;     /* Start of matched string */
  PCRE2_SPTR end_match_ptr;       /* Subject position at end match */
  PCRE2_SPTR start_used_ptr;      /* Earliest consulted character */
  PCRE2_SPTR mark;                /* Mark pointer to pass back on success */
  PCRE2_SPTR nomatch_mark;        /* Mark pointer to pass back on failure */
  PCRE2_SPTR once_target;         /* Where to back up to for atomic groups */
  uint32_t moptions;              /* Match options */
  uint32_t poptions;              /* Pattern options */
  uint32_t capture_last;          /* Most recent capture number + overflow flag */
  uint32_t skip_arg_count;        /* For counting SKIP_ARGs */
  uint32_t ignore_skip_arg;       /* For re-run when SKIP arg name not found */
  uint32_t match_function_type;   /* Set for certain special calls of match() */
  uint32_t nltype;                /* Newline type */
  uint32_t nllen;                 /* Newline string length */
  PCRE2_UCHAR nl[4];              /* Newline string when fixed */
  eptrblock *eptrchain;           /* Chain of eptrblocks for tail recursions */
  recursion_info *recursive;      /* Linked list of recursion data */
  void  *callout_data;            /* To pass back to callouts */
  int (*callout)(pcre2_callout_block *);  /* Callout function or NULL */
#ifdef NO_RECURSE
  void  *match_frames_base;       /* For remembering malloc'd frames */
#endif
} match_block;

/* A similar structure is used for the same purpose by the DFA matching
functions. */

typedef struct dfa_match_block {
  pcre2_memctl memctl;              /* For general use */
  PCRE2_SPTR start_code;            /* Start of the compiled pattern */
  PCRE2_SPTR start_subject ;        /* Start of the subject string */
  PCRE2_SPTR end_subject;           /* End of subject string */
  PCRE2_SPTR start_used_ptr;        /* Earliest consulted character */
  const uint8_t *tables;            /* Character tables */
  PCRE2_OFFSET start_offset;        /* The start offset value */
  uint32_t moptions;                /* Match options */
  uint32_t poptions;                /* Pattern options */
  uint32_t nltype;                  /* Newline type */
  uint32_t nllen;                   /* Newline string length */
  PCRE2_UCHAR nl[4];                /* Newline string when fixed */
  uint16_t bsr_convention;          /* \R interpretation */
  void *callout_data;               /* To pass back to callouts */
  int (*callout)(pcre2_callout_block *);  /* Callout function or NULL */
  dfa_recursion_info *recursive;    /* Linked list of recursion data */
} dfa_match_block;

#endif  /* PCRE2_PCRE2TEST */

/* End of pcre2_intmodedep.h */
