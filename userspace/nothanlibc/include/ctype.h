/* ============================================================
 * ctype.h — ASCII classification, hand-written for NothanOS
 * ============================================================ */

#ifndef _NOTHANLIBC_CTYPE_H
#define _NOTHANLIBC_CTYPE_H

int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);

#endif /* _NOTHANLIBC_CTYPE_H */
