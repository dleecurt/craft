/*
 * Changed from the CRAFT 3.5 Standard Version (craftcc35.tar.Z),
 * as clause 3 of its license requires (see LICENSE):
 *   2018-2019  Quipyowert2/craft: fixes for modern compilers
 *   2026       dleecurt/craft: compiler, warning and bug fixes
 * Exact changes: git log -- xstring.h
 */

/*======================================================================*/
/*= CHANGES AND UPDATES                                                =*/
/*======================================================================*/
/*= date   person file       subject                                   =*/
/*=--------------------------------------------------------------------=*/
/*=                                                                    =*/
/*= 301092 hua    xstring.h  created                                   =*/
/*=                                                                    =*/
/*======================================================================*/

void compress   (char s []);
void lower      (char s []);
void changeall  (char s [], 
                 int  max_length_of_s,
                 const char tmpalte     [],
                 const char replacement []);
char *substring (const char s [], int from, int to);
char *substring (char s [], int from); 
int  submatch   (char s [], char p [], int &pos);
void strcat     (char s [], int len, char app []);
void delchar    (char s [], int pos);
void inschar    (char s [], int pos, char n);
int  strpos     (char s[], char p);
