/*
  Copyright (C) 2010-2012 Birunthan Mohanathas <http://poiru.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"
#include "Lexer.h"

ILexer* RainLexer::LexerFactory()
{
	try
	{
		return new RainLexer();
	}
	catch (...)
	{
		// Should not throw into caller as may be compiled with different compiler or options
		return nullptr;
	}
}

void SCI_METHOD RainLexer::Lex(unsigned int startPos, int length, int initStyle, IDocument* pAccess)
{
	try
	{
		Accessor astyler(pAccess, &props);
		Lexer(startPos, length, initStyle, keyWordLists, astyler);
		astyler.Flush();
	}
	catch (...)
	{
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

void SCI_METHOD RainLexer::Fold(unsigned int startPos, int length, int initStyle, IDocument* pAccess)
{
	try
	{
		Accessor astyler(pAccess, &props);
		Folder(startPos, length, initStyle, astyler);
		astyler.Flush();
	}
	catch (...)
	{
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

void RainLexer::Lexer(unsigned int startPos, int length, int initStyle, WordList* keywordlists[], Accessor& styler)
{
	char ch;
	char buffer[33];
	char* name = nullptr;
	int count, digits, chEOL;
	length += startPos;

	WordList& keywords = *keywordlists[0];
	WordList& numwords = *keywordlists[1];
	WordList& optwords = *keywordlists[2];
	WordList& options = *keywordlists[3];
	WordList& bangs = *keywordlists[4];
	WordList& variables = *keywordlists[5];

	int state = TS_DEFAULT;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);

	for (int i = startPos; i < length; ++i)
	{
		// Make ch 0 if at EOF
		ch = (i == length - 1) ? '\0' : styler[i];

		// Amount of EOL chars is 2 (\r\n) with the Windows format and 1 (\n) with Unix format.
		chEOL = (styler[i] == '\n' && styler[i - 1] == '\r') ? 2 : 1;

		switch(state)
		{
		case TS_DEFAULT:
			switch (ch)
			{
			case '\0':
			case '\n':
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '[':
				state = TS_SECTION;
				break;

			case ';':
				state = TS_COMMENT;
				break;

			case '\t':
			case ' ':
				break;

			default:
				if (isalpha(ch) || ch == '@')
				{
					count = 0;
					digits = 0;
					buffer[count++] = tolower(ch);
					state = TS_KEYWORD;
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_COMMENT:
			// Style as comment when EOL (or EOF) is reached
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i - chEOL, TC_COMMENT);
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_SECTION:
			// Style as section when EOL (or EOF) is reached unless section name has a space
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i - chEOL, TC_SECTION);
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_KEYWORD:
			// Read max. 32 chars into buffer until the equals sign (or EOF/EOL) is met.
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '=':
				// Ignore trailing whitespace
				while (isspacechar(buffer[count - 1]))
				{
					--count;
				}

				buffer[count] = '\0';

				if (keywords.InList(buffer))
				{
					state = TS_VALUE;
				}
				else if (optwords.InList(buffer))
				{
					buffer[count++] = '=';
					state = TS_OPTION;

					// Ignore leading whitepsace
					while (isspacechar(styler[i + 1]))
					{
						++i;
					}
				}
				else if (digits)
				{
					// Try removing chars from the end to check for words like ScaleN
					buffer[count - digits] = '\0';
					digits = 0;
					state = TS_VALUE;

					if (!numwords.InList(buffer))
					{
						break;
					}
				}
				else
				{
					state = TS_VALUE;
					break;
				}

				styler.ColourTo(i - 1, TC_KEYWORD);
				styler.ColourTo(i, TC_EQUALS);
				break;

			default:
				if (count < 32)
				{
					if (isdigit(ch))
					{
						++digits;
					}
					buffer[count++] = tolower(ch);
				}
				else
				{
					state = TS_LINEEND;
				}
			}
			break;

		case TS_OPTION:
			// Read value into buffer and check if it's valid for cases like StringAlign=RIGHT
			switch (ch)
			{
			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			case '\0':
				// Read the last character if at EOF
				buffer[count++] = tolower(styler[i++]);

			case '\r':
			case '\n':
				while (isspacechar(buffer[count - 1]))
				{
					--count;
				}

				buffer[count] = '\0';
				count = 0;
				state = TS_DEFAULT;

				if (options.InList(buffer))
				{
					styler.ColourTo(i - chEOL, TC_VALID);
				}
				else
				{
					styler.ColourTo(i - chEOL, TC_INVALID);
				}
				styler.ColourTo(i, TC_DEFAULT);
				break;

			default:
				if (count < 32)
				{
					buffer[count++] = tolower(ch);
				}
				else
				{
					state = TS_LINEEND;
				}
			}
			break;

		case TS_VALUE:
			// Read values to highlight variables and bangs
			switch (ch)
			{
			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			case '!':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_BANG;
				break;

			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;

		case TS_BANG:
			// Highlight bangs
			switch (ch)
			{
			case '\0':
				buffer[count++] = tolower(styler[i++]);

			case '\n':
			case ' ':
			case '[':
			case ']':
				buffer[count] = '\0';
				count = 0;
				state = (ch == '\n') ? TS_DEFAULT : TS_VALUE;

				// Skip rainmeter before comparing the bang
				if (bangs.InList(&buffer[(strncmp(buffer, "rainmeter", 9) == 0) ? 9 : 0]))
				{
					styler.ColourTo(i - chEOL, TC_BANG);
				}
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '\r':
				break;

			case '#':
				count = 0;
				styler.ColourTo(i - 1, TC_DEFAULT);
				state = TS_VARIABLE;
				break;

			default:
				if (count < 32)
				{
					buffer[count++] = tolower(ch);
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_VARIABLE:
			// Highlight variables
			switch (ch)
			{
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
				break;

			case '\0':
			case '#':
				if (count)
				{
					buffer[count] = '\0';

					if (variables.InList(buffer))
					{
						styler.ColourTo(i, TC_INTVARIABLE);
					}
					else
					{
						if (buffer[0] == '*' && buffer[count - 1] == '*')
						{
							// Escaped variable, don't highlight
							styler.ColourTo(i, TC_DEFAULT);
						}
						else
						{
							styler.ColourTo(i, TC_EXTVARIABLE);
						}
					}

					count = 0;
				}
				state = TS_VALUE;
				break;

			case ' ':
			case '[':
			case ']':
				state = TS_VALUE;
				break;

			default:
				if (count < 32)
				{
					buffer[count++] = tolower(ch);
				}
				else
				{
					state = TS_VALUE;
				}
			}
			break;

		case TS_LINEEND:
			// Apply default style when EOL (or EOF) is reached
			switch (ch)
			{
			case '\0':
			case '\n':
				state = TS_DEFAULT;
				styler.ColourTo(i, TC_DEFAULT);
			}
			break;
		}
	}
}

void RainLexer::Folder(unsigned int startPos, int length, int, Accessor& styler)
{
	length += startPos;

	int line = styler.GetLine(startPos);

	for (unsigned int i = startPos, isize = (unsigned int)length; i < isize; ++i, ++line)
	{
		if ((styler[i] == '\n') || (i == length - 1))
		{
			int lev = (styler.StyleAt(i - 2) == TS_SECTION)
				? SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG
				: SC_FOLDLEVELBASE + 1;

			if (lev != styler.LevelAt(line))
			{
				styler.SetLevel(line, lev);
			}
		}
	}
}

// Scintilla exports

int SCI_METHOD GetLexerCount()
{
	return 1;
}

void SCI_METHOD GetLexerName(unsigned int index, char* name, int buflength)
{
	strncpy_s(name, buflength, "Rainmeter", _TRUNCATE);
}

void SCI_METHOD GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength)
{
	wcsncpy_s(desc, buflength, L"Rainmeter skin file", _TRUNCATE);
}

LexerFactoryFunction SCI_METHOD GetLexerFactory(unsigned int index)
{
	return (index == 0) ? RainLexer::LexerFactory : 0;
}
