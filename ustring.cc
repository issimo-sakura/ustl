// This file is part of the ustl library, an STL implementation.
// Copyright (C) 2003 by Mike Sharov <msharov@talentg.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
// Boston, MA  02111-1307  USA.
//
// ustring.cpp
//
//	STL basic_string equivalent functionality.
//

#include "utypes.h"
#include "ustring.h"
#include "mistream.h"
#include "mostream.h"
#include "utf8.h"
#include <stdarg.h>	// for va_list, va_start, and va_end (in string::format)
#include <stdio.h>	// for vsnprintf (in string::format)

namespace ustl {

const char string::empty_string[string::size_Terminator] = "";

/// Creates an empty string.
string::string (void)
: memblock ()
{
    link (empty_string, sizeof(empty_string));
}

/// Assigns itself the value of string \p s
string::string (const cmemlink& s)
: memblock (s)
{
}

/// Assigns itself the value of string \p s
string::string (const string& s)
: memblock()
{
    if (s.is_linked())
	link (s.c_str(), s.length() + size_Terminator);
    else
	operator= (s);
}

/// Links to \p s
string::string (const_pointer s)
: memblock ()
{
    if (!s)
	s = empty_string;
    link (s, strlen(s) + size_Terminator);
}

/// Copies the value of \p s of length \p len into itself.
string::string (const_pointer s, size_t len)
: memblock ()
{
    assign (s, len);
}

/// Copies into itself the string data between \p s1 and \p s2
string::string (const_pointer s1, const_pointer s2)
: memblock ()
{
    assert (s1 <= s2 && "Negative ranges result in memory allocation errors.");
    assign (s1, s2);
}

/// Creates a string of length \p n filled with character \p c.
string::string (size_t n, value_type c)
: memblock (n + size_Terminator)
{
    fill_n (begin(), n, c);
}

/// Resize the string to \p n characters. New space contents is undefined.
void string::resize (size_t n)
{
    memblock::resize (n + size_Terminator);
    at(n) = c_Terminator;
}

/// Assigns itself the value of string \p s
void string::assign (const_pointer s)
{
    if (!s)
	s = empty_string;
    assign (s, strlen (s));
}

/// Assigns itself the value of string \p s of length \p len.
void string::assign (const_pointer s, size_t len)
{
    while (len && s[len - 1] == c_Terminator)
	-- len;
    resize (len);
    copy (s, len);
}

/// Appends to itself the value of string \p s of length \p len.
void string::append (const_pointer s)
{
    if (!s)
	s = empty_string;
    append (s, strlen (s));
}

/// Appends to itself the value of string \p s of length \p len.
void string::append (const_pointer s, size_t len)
{
    while (len && s[len - 1] == c_Terminator)
	-- len;
    memblock::insert (end(), len);
    copy_n (s, len, end() - len);
}

/// Appends to itself \p n characters of value \p c.
void string::append (size_t n, value_type c)
{
    memblock::insert (end(), n);
    fill_n (end() - n, n, c);
}

/// Copies into itself at offset \p start, the value of string \p p of length \p n.
size_t string::copyto (pointer p, size_t n, const_iterator start) const
{
    assert (p && n);
    if (!start)
	start = begin();
    const size_t btc = min(n - size_Terminator, length());
    copy_n (start, btc, p);
    p[btc] = c_Terminator;
    return (btc + size_Terminator);
}

/// Returns comparison value regarding string \p s.
/// The return value is:
/// \li 1 if this string is greater (by value, not length) than string \p s
/// \li 0 if this string is equal to string \p s
/// \li -1 if this string is less than string \p s
///
int string::compare (const_iterator first1, const_iterator last1, const_iterator first2, const_iterator last2) const
{
    assert (first1 <= last1 && first2 <= last2 && "Negative ranges result in memory allocation errors.");
    const size_t len1 = distance (first1, last1);
    const size_t len2 = last2 ? distance (first2, last2) : strlen(first2);
    int rv = memcmp (first1, first2, min (len1, len2));
    if (rv == 0) {
	if (len1 < len2)
	    rv = -1;
	else if (len2 < len1)
	    rv = 1;
    }
    return (rv);
}

/// Returns true if this string is equal to string \p s.
bool string::operator== (const string& s) const
{
    return (size() == s.size() && 0 == memcmp (c_str(), s.c_str(), size()));
}

/// Returns true if this string is equal to string \p s.
bool string::operator== (const_pointer s) const
{
    if (!s)
	s = empty_string;
    const size_t slen = strlen(s);
    return (size() == slen && 0 == memcmp (c_str(), s, size()));
}

/// Inserts character \p c into this string at \p start.
void string::insert (iterator start, const_reference c, size_t n)
{
    start = reinterpret_cast<iterator>(memblock::insert (start, n).base());
    fill_n (start, n, c);
}

/// Inserts \p count instances of string \p s at offset \p start.
void string::insert (iterator start, const_pointer s, size_t n)
{
    if (!s)
	s = empty_string;
    insert (start, s, s + strlen(s), n);
}

/// Inserts [first,last] \p n times.
void string::insert (iterator start, const_pointer first, const_pointer last, size_t n)
{
    assert (first <= last);
    assert (begin() <= start && end() >= start);
    start = reinterpret_cast<iterator>(memblock::insert (start, distance(first, last) * n).base());
    fill (start, first, distance(first, last), n);
}

/// Replaces range [\p start, \p start + \p len] with string \p s.
void string::replace (iterator first, iterator last, const_pointer s, size_t n)
{
    if (!s)
	s = empty_string;
    replace (first, last, s, s + strlen(s), n);
}

/// Replaces range [\p start, \p start + \p len] with \p count instances of string \p s.
void string::replace (iterator first, iterator last, const_pointer i1, const_pointer i2, size_t n)
{
    assert (first <= last);
    assert (n || distance(first, last));
    assert (first >= begin() && first <= end() && last >= first && last <= end());
    const size_t bte = distance(first, last);
    const size_t bti = distance(i1, i2) * n;
    if (bti < bte)
	first = reinterpret_cast<iterator>(memblock::erase (first, bte - bti).base());
    else if (bte < bti)
	first = reinterpret_cast<iterator>(memblock::insert (first, bti - bte).base());
    fill (first, i1, distance(i1, i2), n);
}

/// Returns the offset of the first occurence of \p c after \p pos.
string::const_iterator string::find (const_reference c, const_iterator pos) const
{
    if (!pos) pos = begin();
    assert (pos >= begin() && pos <= end());
    const_iterator found = reinterpret_cast<const_iterator>(memchr (pos, c, end() - pos));
    return (found ? found : end());
}

/// Returns the offset of the first occurence of substring \p s of length \p n after \p pos.
string::const_iterator string::find (const string& s, const_iterator pos) const
{
    if (!pos) pos = begin();
    assert (pos >= begin() && pos <= end());
    if (s.empty() || s.size() > distance (pos, end()))
	return (end());
    const uoff_t endi = s.size() - 1;
    const_reference endchar = s[endi];
    uoff_t lastPos = endi;
    while (lastPos-- && s[lastPos] != endchar);
    const size_t skip = endi - lastPos;
    const_iterator i = pos + s.size() - 1;
    while (i < end()) {
	i = reinterpret_cast<const_iterator>(memchr (i, endchar, distance(i, end())));
	if (!i)
	    break;
	if (memcmp (i - endi, s, s.size()) == 0)
	    return (i - endi);
	i += skip;
    }
    return (end());
}

/// Returns the offset of the last occurence of character \p c after \p pos.
string::const_iterator string::rfind (const_reference c, const_iterator pos) const
{
    if (!pos) pos = end() - 1;
    assert (pos >= begin() && pos <= end());
    const_pointer found = strrchr (pos, c);
    return (found ? found : begin());
}

/// Returns the offset of the last occurence of substring \p s of size \p n after \p pos.
string::const_iterator string::rfind (const string& s, const_iterator pos) const
{
    if (!pos) pos = end();
    assert (pos >= begin() && pos <= end());
    if (s.empty() || s.size() > distance (pos, end()))
	return (begin());
    // Match from the tail, iterating backwards.
    const_iterator d = --pos;
    const_iterator sp = begin() + s.size() - 1;
    const_iterator m = s.end() - 1;
    for (uoff_t i = 0; d > sp && i < s.size(); -- d)
	for (i = 0; i < s.size(); ++ i)
	    if (*(m - i) != *(d - i))
		break;
    return (d > sp ? d - s.size() : begin());
}

/// Returns the offset of the first occurence of one of characters in \p s of size \p n after \p pos.
string::const_iterator string::find_first_of (const string& s, const_iterator pos) const
{
    if (!pos) pos = begin();
    assert (pos >= begin() && pos <= end());
    while (pos < end())
	if (memchr(s, *pos++, s.length()))
	    return (--pos);
    return (end());
}

/// Returns the offset of the first occurence of one of characters not in \p s of size \p n after \p pos.
string::const_iterator string::find_first_not_of (const string& s, const_iterator pos) const
{
    if (!pos) pos = begin();
    assert (pos >= begin() && pos <= end());
    while (pos < end())
	if (!memchr(s, *pos++, s.length()))
	    return (--pos);
    return (end());
}

/// Returns the offset of the last occurence of one of characters in \p s of size \p n after \p pos.
string::const_iterator string::find_last_of (const string& s, const_iterator pos) const
{
    if (!pos) pos = end();
    assert (pos >= begin() && pos <= end());
    while (--pos < end())
	if (memchr(s, *pos, s.length()))
	    return (pos);
    return (end());
}

/// Returns the offset of the last occurence of one of characters not in \p s of size \p n after \p pos.
string::const_iterator string::find_last_not_of (const string& s, const_iterator pos) const
{
    if (!pos) pos = end();
    assert (pos >= begin() && pos <= end());
    while (--pos < end())
	if (!memchr(s, *pos, s.length()))
	    return (pos);
    return (end());
}

/// Equivalent to a sprintf on the string.
int string::format (const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    char nullbuf [size_Terminator] = ""; // On SunOS and Darwin vsnprintf can't handle NULL pointers, and this is faster than calling malloc.
    int rv = vsnprintf (capacity() ? data() : nullbuf, capacity() + size_Terminator, fmt, args);
    //
    // glibc 2.0.6 or later required for reallocation to work.
    // 		earlier versions will require the string to already
    // 		be allocated to the correct size.
    // 		vsnprintf should return the number of characters that
    // 		would have been written without truncation, as per C99 spec.
    // 		See vsnprintf man page for additional information.
    //
    if (rv >= 0) {
	if (rv > int(capacity())) {
	    reserve (rv);
	    rv = vsnprintf (data(), memblock::capacity(), fmt, args);
	}
	resize (rv);
    }
    va_end (args);
    return (rv);
}

/// Returns the number of bytes required to write this object to a stream.
size_t string::stream_size (void) const
{
    return (Utf8Bytes(length()) + length());
}

/// Reads the object from stream \p os
void string::read (istream& is)
{
    resize (*utf8in(is));
    is.read (data(), length());
}

/// Writes the object to stream \p os
void string::write (ostream& os) const
{
    *utf8out(os)++ = length();
    os.write (cdata(), length());
}

}; // namespace ustl

