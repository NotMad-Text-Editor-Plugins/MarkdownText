#pragma once
#ifndef SU_H
#define SU_H
#include "stdafx.h"

int strncmp_casei(const std::string & stra , const std::string & strb, int n);

int STR2LONGPTR(TCHAR* STR, LONG_PTR & LONGPTR);

int STR2LONGPTRA(CHAR* STR, LONG_PTR & LONGPTR);

#ifdef  UNICODE
int LONGPTR2STR(TCHAR* STR, LONG_PTR LONGPTR);
#endif

int LONGPTR2STR(CHAR* STR, LONG_PTR LONGPTR);

bool STRSTARTWITH(TCHAR* strA,TCHAR* strB);

bool STRENDWITH(TCHAR* strA,TCHAR* strB);

#ifdef  UNICODE
bool STRSTARTWITH(TCHAR* strA,CHAR* strB);

bool STRENDWITH(TCHAR* strA,CHAR* strB);
#endif

#endif

unsigned char FromHex(unsigned char x);

void UrlDecode(char* dest, const char* str);

void UrlDecode(char* dest, const TCHAR* str);