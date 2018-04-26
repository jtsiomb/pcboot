/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "ctype.h"

int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

int isalpha(int c)
{
	return isupper(c) || islower(c);
}

int isblank(int c)
{
	return c == ' ' || c == '\t';
}

int isdigit(int c)
{
	return c >= '0' && c <= '9';
}

int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}

int islower(int c)
{
	return c >= 'a' && c <= 'z';
}

int isgraph(int c)
{
	return c > ' ' && c <= '~';
}

int isprint(int c)
{
	return isgraph(c) || c == ' ';
}

int isspace(int c)
{
	return isblank(c) || c == '\f' || c == '\n' || c == '\r' || c == '\v';
}

int toupper(int c)
{
	return islower(c) ? (c + ('A' - 'a')) : c;
}

int tolower(int c)
{
	return isupper(c) ? (c + ('A' - 'a')) : c;
}
