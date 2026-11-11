/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*======================================================================*/

#ifndef errorhandling_h
#define errorhandling_h

#include "stdlib.h"
#include "stdio.h"

[[noreturn]] void errorstop (int  error_no,
                const char c_class [],
                const char msg     []);

[[noreturn]] void errorstop (int  error_no,
                const char c_class [],
                const char msg     [],
                const char info    []);

[[noreturn]] void errorstop (int  error_no,
                const char c_class [],
                const char msg     [],
                const char info    [],
		char param1  []);

[[noreturn]] void errorstop (int  error_no,
                const char c_class [],
                const char msg     [],
                const char info    [],
		const char param1  [],
                const char param2  []);

/* Stop with an error unless a scanf-style read got all its fields. */
void read_check (int got, int expected, const char what []);

/* Run a shell command; report (but survive) a failure. */
void run_command (const char cmd []);

/* Delete a file if it exists. */
void remove_file (const char name []);

#endif


 

