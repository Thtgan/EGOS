#include"basicPrint.h"
#include"vgaTextMode.h"

#include<stdbool.h>

#define STRING_BUFFER_SIZE 256

static char _stringBuffer[STRING_BUFFER_SIZE];
const static char* _hexPrefix = "0x";
const static char* _digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void print(const char* str)
{
	for(int i = 0; str[i] != '\0'; ++i)
		putchar(str[i]);
}

void printHex8(uint8_t val)
{
	print(_hexPrefix);
	for(int i = 1; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex16(uint16_t val)
{
	print(_hexPrefix);
	for(int i = 3; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex32(uint32_t val)
{
	print(_hexPrefix);
	for(int i = 7; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex64(uint64_t val)
{
	print(_hexPrefix);
	for(int i = 15; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printInt64(int64_t val, uint8_t base, uint8_t padding)
{
	if (base < 2 || 36 < base)
		return;
	_stringBuffer[STRING_BUFFER_SIZE - 1] = '\0';

	bool neg = false;

	int len = 0;
	if (val < 0)
	{
		if (base == 10)
		{
			neg = true;
			val = -val;
			len = 1;
		}
	}

	char * ptr = &_stringBuffer[STRING_BUFFER_SIZE - 2];
	for(uint64_t d = (uint64_t)val; d != 0; d /= base, --ptr, ++len)
		*ptr = _digits[d % base];
	
	if (len == 0)
	{
		*(ptr--) = '0';
		len = 1;
	}
	
	for(; len < padding; ++len, --ptr)
		*ptr = '0';

	if (neg)
		*(ptr--) = '-';
	
	print(ptr + 1);
}

void printUInt64(uint64_t val, uint8_t base, uint8_t padding)
{
	if (base < 2 || 36 < base)
		return;
	_stringBuffer[STRING_BUFFER_SIZE - 1] = '\0';

	char * ptr = &_stringBuffer[STRING_BUFFER_SIZE - 2];
	int len = 0;
	for(; val != 0; val /= base, --ptr, ++len)
		*ptr = _digits[val % base];
	
	if (len == 0)
	{
		*(ptr--) = '0';
		len = 1;
	}
	
	for(; len < padding; ++len, --ptr)
		*ptr = '0';

	print(ptr + 1);
}

void printDouble(double val, uint8_t precisions)
{
	if (val < 0)
	{
		val = -val;
		putchar('-');
	}

	printInt64((uint64_t)val, 10, 0);
	putchar('.');

	val -= (uint64_t)val;
	for(uint8_t i = 0; i < precisions; ++i)
		val *= 10.0;
	val += 0.5;

	printInt64((uint64_t)val, 10, precisions);
}

void printFloat(float val, uint8_t precisions)
{
	if (val < 0)
	{
		val = -val;
		putchar('-');
	}

	printInt64((uint64_t)val, 10, 0);
	putchar('.');

	val -= (uint64_t)val;
	for(uint8_t i = 0; i < precisions; ++i)
		val *= 10.0f;
	val += 0.5f;

	printInt64((uint64_t)val, 10, precisions);
}

void printPtr(const void* ptr)
{
	printHex64((uint64_t)ptr);
}