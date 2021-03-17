#include "SU.h"
#include "assert.h"



int strncmp_casei(const std::string & stra , const std::string & strb, int n)
{
	int aLen = stra.length();
	int bLen = strb.length();
	if(n&&n<=aLen&&n<=bLen) {
		aLen=bLen=n;
	}
	int iRes = 0 , iPos = 0;
	for (iPos = 0; iPos < aLen && iPos < bLen; ++iPos)
	{
		iRes = toupper(stra[iPos]) - toupper(strb[iPos]);
		if (!iRes)return iRes;
	}
	if (iPos == aLen && iPos == bLen)return 0;
	if (iPos < aLen) return 1;
	if (iPos < bLen) return -1;
}

int STR2LONGPTR(TCHAR* STR, LONG_PTR & LONGPTR)
{
	int len = lstrlen(STR);
	bool intOpened=false;
	int i=0;
	for(;i<len;i++) {
		int intVal = STR[i]-'0';
		int valval = intVal>=0&&intVal<=9||STR[i]=='-';
		if(!intOpened)
		{
			intOpened=valval;
		}
		else if(!valval)
		{
			break;
		}
		if(intOpened)
		{
			LONGPTR = LONGPTR*10+intVal;
		}
	}
	return i;
}

int STR2LONGPTRA(CHAR* STR, LONG_PTR & LONGPTR)
{
	int len = strlen(STR);
	bool intOpened=false;
	int i=0;
	for(;i<len;i++) {
		int intVal = STR[i]-'0';
		int valval = intVal>=0&&intVal<=9||STR[i]=='-';
		if(!intOpened)
		{
			intOpened=valval;
		}
		else if(!valval)
		{
			break;
		}
		if(intOpened)
		{
			LONGPTR = LONGPTR*10+intVal;
		}
	}
	return i;
}

#ifdef  UNICODE
int LONGPTR2STR(TCHAR* STR, LONG_PTR LONGPTR)
{
	TCHAR* start=STR;
	int cc=0;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
		cc++;
	}
	*STR='\0';
	wcsrev(start);
	return cc;
}
#endif

int LONGPTR2STR(CHAR* STR, LONG_PTR LONGPTR)
{
	CHAR* start=STR;
	int cc=0;
	while(LONGPTR)
	{
		*(STR++)='0'+(LONGPTR%10);
		LONGPTR/=10;
		cc++;
	}
	*STR='\0';
	strrev(start);
	return cc;
}

bool STRSTARTWITH(TCHAR* strA,TCHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = lstrlen(strB);
	int to = valen-pc;
	int po = 0;
	// Note: toffset might be near -1>>>1.
	if (to < 0) {
		return false;
	}
	while (--pc >= 0) {
		if (strA[po] != strB[po++]) {
			return false;
		}
	}
	return true;
}

bool STRENDWITH(TCHAR* strA,TCHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = lstrlen(strB);
	int to = valen-pc;
	int po = 0;
	// Note: toffset might be near -1>>>1.
	if (to < 0) {
		return false;
	}
	while (--pc >= 0) {
		if (strA[to++] != strB[po++]) {
			return false;
		}
	}
	return true;
}

#ifdef  UNICODE
bool STRSTARTWITH(TCHAR* strA,CHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = strlen(strB);
	int to = valen-pc;
	int po = 0;
	// Note: toffset might be near -1>>>1.
	if (to < 0) {
		return false;
	}
	while (--pc >= 0) {
		if (strA[po] != strB[po++]) {
			return false;
		}
	}
	return true;
}

bool STRENDWITH(TCHAR* strA,CHAR* strB)
{
	int valen = lstrlen(strA);
	int pc = strlen(strB);
	int to = valen-pc;
	int po = 0;
	// Note: toffset might be near -1>>>1.
	if (to < 0) {
		return false;
	}
	while (--pc >= 0) {
		if (strA[to++] != strB[po++]) {
			return false;
		}
	}
	return true;
}
#endif


unsigned char FromHex(unsigned char x)   
{   
	unsigned char y;  
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;  
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;  
	else if (x >= '0' && x <= '9') y = x - '0';  
	else y=0;
	return y;  
}  

void UrlDecode(char* dest, const char* str)  
{  
	size_t length = strlen(str);
	char* ddd=dest;
	for (size_t i = 0; i < length && ddd-dest+1<MAX_PATH; i++)  
	{  
		if (str[i] == '+') {
			*ddd++ = ' ';  
		}
		else if (str[i] == '%')  
		{  
			assert(i + 2 < length);  
			unsigned char high = FromHex(str[++i]);  
			unsigned char low = FromHex(str[++i]);  
			*ddd++ = high*16 + low;  
		}
		else if (str[i] == '?')  
		{  
			break;
		}
		else {
			*ddd++ = str[i];  
		}
	}
	*ddd='\0';
}  

void UrlDecode(char* dest, const TCHAR* str)  
{  
	size_t length = lstrlen(str);
	char* ddd=dest;
	for (size_t i = 0; i < length && ddd-dest+1<MAX_PATH; i++)  
	{  
		if (str[i] == '+') {
			*ddd++ = ' ';  
		}
		else if (str[i] == '%')  
		{  
			assert(i + 2 < length);  
			unsigned char high = FromHex(str[++i]);  
			unsigned char low = FromHex(str[++i]);  
			*ddd++ = high*16 + low;  
		}
		else if (str[i] == '?')  
		{  
			break;
		}
		else {
			*ddd++ = str[i];  
		}
	}
	*ddd='\0';
}  