/* ============================================================
 * ctype.h — ASCII classification, hand-written for VinixOS
 * ============================================================ */

#ifndef _VINIXLIBC_CTYPE_H
#define _VINIXLIBC_CTYPE_H

int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);

#endif /* _VINIXLIBC_CTYPE_H */
