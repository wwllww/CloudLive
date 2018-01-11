/********************************************************************************
 Copyright (C) 2001-2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#pragma once

 UINT   STDCALL   slen(const TCHAR *strSrc);
 UINT   STDCALL   ssize(const TCHAR *strSrc);
 void   STDCALL   scpy(TCHAR *strDest, const TCHAR *strSrc);
 void   STDCALL   scpy_n(TCHAR *strDest, const TCHAR *strSrc, unsigned int n);
 void   STDCALL   scat(TCHAR *strDest, const TCHAR *strSrc);
 void   STDCALL   scat_n(TCHAR *strDest, const TCHAR *strSrc, unsigned int n);
 void   STDCALL   slwr(TCHAR *str);
 void   STDCALL   supr(TCHAR *str);
 TCHAR* STDCALL   sdup(const TCHAR *str);
 int    STDCALL   scmp(const TCHAR *str1, const TCHAR *str2);
 int    STDCALL   scmpi(const TCHAR *str1, const TCHAR *str2);
 int    STDCALL   scmp_n(const TCHAR *str1, const TCHAR *str2, unsigned int num);
 int    STDCALL   scmpi_n(const TCHAR *str1, const TCHAR *str2, unsigned int num);
 TCHAR* STDCALL   srchr(const TCHAR *strSrc, TCHAR chr);
 TCHAR* STDCALL   schr(const TCHAR *strSrc, TCHAR chr);
 TCHAR* STDCALL   sstr(const TCHAR *strSrc, const TCHAR *strSearch);
 TCHAR* STDCALL   srchri(const TCHAR *strSrc, TCHAR chr);
 TCHAR* STDCALL   schri(const TCHAR *strSrc, TCHAR chr);
 TCHAR* STDCALL   sstri(const TCHAR *strSrc, const TCHAR *strSearch);
 TCHAR* STDCALL   sfix(TCHAR *str);

 BOOL STDCALL ValidFloatString(CTSTR lpStr);
 BOOL STDCALL ValidIntString(CTSTR lpStr);
 BOOL DefinitelyFloatString(CTSTR lpStr);

 BOOL            UsingUnicode();
 int             tstr_to_utf8(const TCHAR *strSrc, char *strDest, size_t destLen);
 int             tstr_to_wide(const TCHAR *strSrc, wchar_t *strDest, size_t destLen);
 int             tstr_to_utf8_datalen(const TCHAR *strSrc);
 int             tstr_to_wide_datalen(const TCHAR *strSrc);
 int             utf8_to_tchar_datalen(const char *strSrc);
 LPSTR           tstr_createUTF8(const TCHAR *strSrc);
 WSTR            tstr_createWide(const TCHAR *strSrc);
 TSTR            utf8_createTstr(const char *strSrc);

 int             vtscprintf(const TCHAR *format, va_list args);
 int             vtsprintf_s(TCHAR *pDest, size_t destLen, const TCHAR *format, va_list args);
 int             tsprintf_s(TCHAR *pDest, size_t destLen, const TCHAR *format, ...);

 int             itots_s(int val, TCHAR *buffer, size_t bufLen, int radix);
 int             uitots_s(unsigned int val, TCHAR *buffer, size_t bufLen, int radix);
 int             i64tots_s(INT64 val, TCHAR *buffer, size_t bufLen, int radix);
 int             ui64tots_s(UINT64 val, TCHAR *buffer, size_t bufLen, int radix);
 int             tstring_base_to_int(const TCHAR *nptr, TCHAR **endptr, int base);
 unsigned int    tstring_base_to_uint(const TCHAR *nptr, TCHAR **endptr, int base);
 INT64           tstring_base_to_int64(const TCHAR *nptr, TCHAR **endptr, int base);
 UINT64          tstring_base_to_uint64(const TCHAR *nptr, TCHAR **endptr, int base);
 double          tstof(TCHAR *lpFloat);
 int             tstoi(TCHAR *lpInt);
 unsigned int    tstoui(TCHAR *lpInt);
 INT64           tstoi64(TCHAR *lpInt);
 UINT64          tstoui64(TCHAR *lpInt);

class StringList;

class  String
{
    TSTR lpString;
    unsigned int curLength;

public:
    String();
    String(LPCSTR str);
    String(CWSTR str);
    String(const String &str);

    ~String();

    String& operator=(CTSTR str);
    String& operator+=(CTSTR str);
    String  operator+(CTSTR str) const;

    String& operator=(TCHAR ch);
    String& operator+=(TCHAR ch);
    String  operator+(TCHAR ch) const;

    String& operator=(int number);
    String& operator+=(int number);
    String  operator+(int number) const;

    String& operator=(unsigned int unumber);
    String& operator+=(unsigned int unumber);
    String  operator+(unsigned int unumber) const;

    String& operator=(const String &str);
    String& operator+=(const String &str);
    String  operator+(const String &str) const;

    String& operator<<(CTSTR str);
    String& operator<<(TCHAR ch);
    String& operator<<(int number);
    String& operator<<(unsigned int unumber);
    String& operator<<(const String &str);

    inline BOOL operator==(const String &str) const {return Compare(str);}
    inline BOOL operator!=(const String &str) const {return !Compare(str);}

    inline BOOL operator==(CTSTR lpStr) const {return Compare(lpStr);}
    inline BOOL operator!=(CTSTR lpStr) const {return !Compare(lpStr);}

    BOOL    Compare(CTSTR str)    const;
    BOOL    CompareI(CTSTR str)   const;

    LPSTR   CreateUTF8String();

    String& FindReplace(CTSTR strFind, CTSTR strReplace);

    void    InsertString(UINT dwPos, CTSTR str);

    String& AppendString(CTSTR str, UINT count=0);

    UINT    GetLinePos(UINT dwLine);

    void    RemoveRange(unsigned int from, unsigned int to);

    String& AppendChar(TCHAR chr);
    String& InsertChar(UINT pos, TCHAR chr);
    String& RemoveChar(UINT pos);

    String& Clear();

    String& GroupDigits();

    int     NumTokens(TCHAR token=' ') const;
    String  GetToken(int id, TCHAR token=' ') const;
    void    GetTokenList(StringList &strList, TCHAR token=' ', BOOL bIncludeEmpty=TRUE) const;
    CTSTR   GetTokenOffset(int token, TCHAR seperator=' ') const;

    String  Left(UINT iEnd);
    String  Mid(UINT iStart, UINT iEnd);
    String  Right(UINT iStart);

    inline int ToInt(int base=10) const
    {
        if(lpString && ValidIntString(lpString))
            return tstring_base_to_int(lpString, NULL, base);
        else
            return 0;
    }

    inline float ToFloat() const
    {
        if(lpString && ValidFloatString(lpString))
            return (float)tstof(lpString);
        else
            return 0.0f;
    }

    inline BOOL    IsEmpty() const              {return !lpString || !*lpString || curLength == 0;}
    inline BOOL    IsValid() const              {return !IsEmpty();}

    inline String& KillSpaces()                 {if(lpString) curLength = slen(sfix(lpString)); return *this;}

    inline TSTR Array() const                   {return lpString;}

    inline operator TSTR() const                {return lpString;}

    String& SetLength(UINT length);

    inline UINT    Length() const               {return curLength;}
    inline UINT    DataLength() const           {return curLength ? ssize(lpString) : 0;}

    inline String& MakeLower()                  {if(lpString) slwr(lpString); return *this;}
    inline String& MakeUpper()                  {if(lpString) supr(lpString); return *this;}

    inline String GetLower() const              {return String(*this).MakeLower();}
    inline String GetUpper() const              {return String(*this).MakeUpper();}

    static String RepresentationToString(String &stringToken);
    static String StringToRepresentation(String &string);
     friend Serializer& operator<<(Serializer &s, String &str);
};

 String FormattedStringva(CTSTR lpFormat, va_list arglist);
 String FormattedString(CTSTR lpFormat, ...);
WORD StringCRC16(CTSTR lpData);
WORD StringCRC16I(CTSTR lpData);

class  StringList : public List<String>
{
public:
    inline ~StringList()
    {
        for(unsigned int i=0; i<num; i++)
            array[i].Clear();
    }

    inline void Clear()
    {
        for(unsigned int i=0; i<num; i++)
            array[i].Clear();
        List<String>::Clear();
    }

    inline unsigned int Add(const String &val)
    {
        *CreateNew() = val;

        return num-1;
    }

    inline unsigned int Add(CTSTR lpStr)
    {
        *CreateNew() = lpStr;

        return num-1;
    }

    inline void Insert(unsigned int index, const String &val)
    {
        *InsertNew(index) = val;
    }

    inline void Insert(unsigned int index, CTSTR lpStr)
    {
        *InsertNew(index) = lpStr;
    }


    inline unsigned int AddSorted(CTSTR lpStr)
    {
        UINT i;
        for(i=0; i<num; i++)
        {
            if(scmpi(array[i], lpStr) >= 0)
                break;
        }

        Insert(i, lpStr);
        return i;
    }


    unsigned int SafeAdd(const String& val)
    {
        UINT i;
        for(i=0; i<num; i++)
        {
            if(array[i].Compare(val))
                break;
        }

        return (i != num) ? i : Add(val);
    }

    unsigned int SafeAdd(CTSTR lpStr)
    {
        UINT i;
        for(i=0; i<num; i++)
        {
            if(array[i].Compare(lpStr))
                break;
        }

        return (i != num) ? i : Add(lpStr);
    }

    unsigned int SafeAddI(const String& val)
    {
        UINT i;
        for(i=0; i<num; i++)
        {
            if(array[i].CompareI(val))
                break;
        }

        return (i != num) ? i : Add(val);
    }

    unsigned int SafeAddI(CTSTR lpStr)
    {
        UINT i;
        for(i=0; i<num; i++)
        {
            if(array[i].CompareI(lpStr))
                break;
        }

        return (i != num) ? i : Add(lpStr);
    }


    inline void Remove(unsigned int index)
    {
        array[index].Clear();
        List<String>::Remove(index);
    }

    inline void RemoveItem(const String& str)
    {
        for(UINT i=0; i<num; i++)
        {
            if(array[i].Compare(str))
            {
                Remove(i);
                break;
            }
        }
    }

    inline void RemoveItem(CTSTR lpStr)
    {
        for(UINT i=0; i<num; i++)
        {
            if(array[i].Compare(lpStr))
            {
                Remove(i);
                break;
            }
        }
    }

    inline void RemoveItemI(const String& str)
    {
        for(UINT i=0; i<num; i++)
        {
            if(array[i].CompareI(str))
            {
                Remove(i);
                break;
            }
        }
    }

    inline void RemoveItemI(CTSTR lpStr)
    {
        for(UINT i=0; i<num; i++)
        {
            if(array[i].CompareI(lpStr))
            {
                Remove(i);
                break;
            }
        }
    }

    inline void CopyArray(const String *new_array, unsigned int n)
    {
        if(!new_array && n)
        {
            AppWarning(TEXT("List::CopyArray:  NULL array with count above zero"));
            return;
        }

        Clear();

        if(n)
        {
            SetSize(n);

            for(unsigned int i=0; i<n; i++)
                array[i] = new_array[i];
        }
    }

    inline void InsertArray(unsigned int index, const String *new_array, unsigned int n)
    {
        assert(index <= num);

        if(index > num)
            return;

        if(!new_array && n)
        {
            AppWarning(TEXT("List::AppendArray:  NULL array with count above zero"));
            return;
        }

        if(!n)
            return;

        int oldnum = num;

        SetSize(n+num);

        mcpyrev(array+index+n, array+index, sizeof(String)*(oldnum-index));
        zero(array+index, sizeof(String)*n);

        for(unsigned int i=0; i<n; i++)
            array[index+i] = new_array[i];
    }

    inline void AppendArray(const String *new_array, unsigned int n)
    {
        if(!new_array && n)
        {
            AppWarning(TEXT("List::AppendArray:  NULL array with count above zero"));
            return;
        }

        if(!n)
            return;

        int oldnum = num;

        SetSize(n+num);

        for(unsigned int i=0; i<n; i++)
            array[oldnum+i] = new_array[i];
    }

    inline BOOL SetSize(unsigned int n)
    {
        if(n < num)
        {
            for(unsigned int i=n; i<num; i++)
                array[i].Clear();
        }

        return List<String>::SetSize(n);
    }

    inline void RemoveDupes()
    {
        for(UINT i=0; i<num; i++)
        {
            String val1 = array[i];
            for(UINT j=i+1; j<num; j++)
            {
                String val2 = array[j];
                if(val1.Compare(val2))
                {
                    Remove(j);
                    --j;
                }
            }
        }
    }

    inline void RemoveDupesI()
    {
        for(UINT i=0; i<num; i++)
        {
            String val1 = array[i];
            for(UINT j=i+1; j<num; j++)
            {
                String val2 = array[j];
                if(val1.CompareI(val2))
                {
                    Remove(j);
                    --j;
                }
            }
        }
    }

    inline void CopyList(const StringList& list)
    {
        CopyArray(list.Array(), list.Num());
    }

    inline void InsertList(unsigned int index, const StringList& list)
    {
        InsertArray(index, list.Array(), list.Num());
    }

    inline void AppendList(const StringList& list)
    {
        AppendArray(list.Array(), list.Num());
    }

    inline StringList& operator<<(const String& val)
    {
        Add(val);
        return *this;
    }

    inline StringList& operator<<(CTSTR lpStr)
    {
        Add(lpStr);
        return *this;
    }

    inline BOOL HasValueI(const String& val) const
    {
        for(UINT i=0; i<num; i++)
            if(array[i].CompareI(val)) return 1;

        return 0;
    }

    inline BOOL HasValueI(CTSTR lpStr) const
    {
        for(UINT i=0; i<num; i++)
            if(array[i].CompareI(lpStr)) return 1;

        return 0;
    }

    inline UINT FindValueIndexI(const String& val) const
    {
        for(UINT i=0; i<num; i++)
            if(array[i].CompareI(val)) return i;

        return INVALID;
    }

    inline UINT FindValueIndexI(CTSTR lpStr) const
    {
        for(UINT i=0; i<num; i++)
            if(array[i].CompareI(lpStr)) return i;

        return INVALID;
    }

    inline UINT FindNextValueIndexI(const String& val, UINT lastIndex=INVALID) const
    {
        for(UINT i=lastIndex+1; i<num; i++)
            if(array[i].CompareI(val)) return i;

        return INVALID;
    }

    inline UINT FindNextValueIndexI(CTSTR lpStr, UINT lastIndex=INVALID) const
    {
        for(UINT i=lastIndex+1; i<num; i++)
            if(array[i].CompareI(lpStr)) return i;

        return INVALID;
    }

    inline String Join(String separator)
    {
        if (!num)
            return {};

        if (num == 1)
            return array[0];

        String result = array[0];
        for (unsigned i = 1; i < num; i++)
            result << separator << array[i];

        return result;
    }

    inline friend Serializer& operator<<(Serializer &s, StringList &list)
    {
        if(s.IsLoading())
        {
            //FIXME: ???
            UINT num;
            s << num;
            list.SetSize(num);
            for(unsigned int i=0; i<num; i++)
                s << list[i];
        }
        else
        {
            UINT num = list.Num();
            s << num;
            for(unsigned int i=0; i<num; i++)
                s << list[i];
        }

        return s;
    }
};

 String UIntString(unsigned int ui, int radix=10);
 String IntString(int i, int radix=10);
 String UInt64String(UINT64 i, int radix=10);
 String Int64String(INT64 i, int radix=10);
 String FloatString(double f);

 int STDCALL GetStringLine(const TCHAR *lpStart, const TCHAR *lpOffset);

 String STDCALL GetStringSection(const TCHAR *lpStart, const TCHAR *lpOffset);
