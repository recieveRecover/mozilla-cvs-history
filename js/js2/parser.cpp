// -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
//
// The contents of this file are subject to the Netscape Public
// License Version 1.1 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of
// the License at http://www.mozilla.org/NPL/
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
// The Original Code is the JavaScript 2 Prototype.
//
// The Initial Developer of the Original Code is Netscape
// Communications Corporation.  Portions created by Netscape are
// Copyright (C) 1998 Netscape Communications Corporation. All
// Rights Reserved.

#include "numerics.h"
#include "parser.h"
#include "world.h"

namespace JS = JavaScript;


//
// Reader
//


// Create a Reader reading characters from the source string.
// sourceLocation describes the origin of the source and may be used for error messages.
// initialLineNum is the line number of the first line of the source string.
JS::Reader::Reader(const String &source, const String &sourceLocation, uint32 initialLineNum):
	source(source), sourceLocation(sourceLocation), initialLineNum(initialLineNum)
{
	const char16 *b = Reader::source.c_str();
	begin = b;
	p = b;
	end = b + Reader::source.size();
  #ifdef DEBUG
	recordString = 0;
  #endif
  	beginLine();
}


// Mark the beginning of a line.  Call this after reading every line break to fill
// out the line start table.
void JS::Reader::beginLine()
{
	ASSERT(p <= end && (!linePositions.size() || p > linePositions.back()));
	linePositions.push_back(p);
}


// Return the number of the line containing the given character position.
// The line starts should have been recorded by calling beginLine.
uint32 JS::Reader::posToLineNum(uint32 pos) const
{
	ASSERT(pos <= getPos());
	std::vector<const char16 *>::const_iterator i = std::upper_bound(linePositions.begin(), linePositions.end(), begin + pos);
	ASSERT(i != linePositions.begin());
	return static_cast<uint32>(i-1 - linePositions.begin()) + initialLineNum;
}


// Return the character position as well as pointers to the beginning and end (not including
// the line terminator) of the nth line.  If lineNum is out of range, return 0 and two nulls.
// The line starts should have been recorded by calling beginLine().  If the nth line is the
// last one recorded, then getLine manually finds the line ending by searching for a line
// break; otherwise, getLine assumes that the line ends one character before the beginning
// of the next line.
uint32 JS::Reader::getLine(uint32 lineNum, const char16 *&lineBegin, const char16 *&lineEnd) const
{
	lineBegin = 0;
	lineEnd = 0;
	if (lineNum < initialLineNum)
		return 0;
	lineNum -= initialLineNum;
	if (lineNum >= linePositions.size())
		return 0;
	lineBegin = linePositions[lineNum];

	const char16 *e;
	++lineNum;
	if (lineNum < linePositions.size())
		e = linePositions[lineNum] - 1;
	else {
		e = lineBegin;
		const char16 *end = Reader::end;
		while (e != end && !isLineBreak(*e))
			++e;
	}
	lineEnd = e;
	return static_cast<uint32>(lineBegin - begin);
}


// Begin accumulating characters into the recordString, whose initial value is
// ignored and cleared.  Each character passed to recordChar() is added to the end
// of the recordString.  Recording ends when endRecord() or beginLine() is called.
// Recording is significantly optimized when the characters passed to readChar()
// are the same characters as read by get().  In this case the record String does
// not get allocated until endRecord() is called or a discrepancy appears between
// get() and recordChar().
void JS::Reader::beginRecording(String &recordString)
{
	Reader::recordString = &recordString;
	recordBase = p;
	recordPos = p;
}


// Append ch to the recordString.
void JS::Reader::recordChar(char16 ch)
{
	ASSERT(recordString);
	if (recordPos) {
		if (recordPos != end && *recordPos == ch) {
			recordPos++;
			return;
		} else {
			recordString->assign(recordBase, recordPos);
			recordPos = 0;
		}
	}
	*recordString += ch;
}


// Finish recording characters into the recordString that was last passed to beginRecording().
// Return that recordString.
JS::String &JS::Reader::endRecording()
{
	String *rs = recordString;
	ASSERT(rs);
	if (recordPos)
		rs->assign(recordBase, recordPos);
	recordString = 0;
	return *rs;
}


// Report an error at the given character position in the source code.
void JS::Reader::error(Exception::Kind kind, const String &message, uint32 pos)
{
	uint32 lineNum = posToLineNum(pos);
	const char16 *lineBegin;
	const char16 *lineEnd;
	uint32 linePos = getLine(lineNum, lineBegin, lineEnd);
	ASSERT(lineBegin && lineEnd && linePos <= pos);
	
	throw Exception(kind, message, sourceLocation, lineNum, pos - linePos, pos, lineBegin, lineEnd);
}


//
// Lexer
//


static const char controlCharNames[6] = {'b', 't', 'n', 'v', 'f', 'r'};

// Print the characters from begin to end, escaping them as necessary to make the resulting
// string be readable if placed between two quotes specified by quote (which should be either
// '\'' or '"').
void JS::escapeString(Formatter &f, const char16 *begin, const char16 *end, char16 quote)
{
	ASSERT(begin <= end);

	const char16 *chunk = begin;
	while (begin != end) {
		char16 ch = *begin++;
		CharInfo ci(ch);
		if (char16Value(ch) < 0x20 || isLineBreak(ci) || isFormat(ci) || ch == '\\' || ch == quote) {
			if (begin-1 != chunk)
				printString(f, chunk, begin-1);
			chunk = begin;

			f << '\\';
			switch (ch) {
			  case 0x0008:
			  case 0x0009:
			  case 0x000A:
			  case 0x000B:
			  case 0x000C:
			  case 0x000D:
				f << controlCharNames[ch - 0x0008];
				break;

			  case '\'':
			  case '"':
			  case '\\':
				f << ch;
				break;

			  case 0x0000:
				if (begin == end || char16Value(*begin) < '0' || char16Value(*begin) > '9') {
					f << '0';
					break;
				}
			  default:
				if (char16Value(ch) <= 0xFF) {
					f << 'x';
					printHex(f, static_cast<uint32>(char16Value(ch)), 2);
				} else {
					f << 'u';
					printHex(f, static_cast<uint32>(char16Value(ch)), 4);
				}
			}
		}
	}
	if (begin != chunk)
		printString(f, chunk, begin);
}


// Print s as a quoted string using the given quotes (which should be either '\'' or '"').
void JS::quoteString(Formatter &f, const String &s, char16 quote)
{
	f << quote;
	const char16 *begin = s.data();
	escapeString(f, begin, begin + s.size(), quote);
	f << quote;
}


const char *const JS::Token::kindNames[kindsEnd] = {
  // Special
	"end of input",		// end
	"number",			// number
	"string",			// string
	"unit",				// unit
	"regular expression",// regExp

  // Punctuators
	"(",				// openParenthesis
	")",				// closeParenthesis
	"[",				// openBracket
	"]",				// closeBracket
	"{",				// openBrace
	"}",				// closeBrace
	",",				// comma
	";",				// semicolon
	".",				// dot
	"..",				// doubleDot
	"...",				// tripleDot
	"->",				// arrow
	":",				// colon
	"::",				// doubleColon
	"#",				// pound
	"@",				// at
	"++",				// increment
	"--",				// decrement
	"~",				// complement
	"!",				// logicalNot
	"*",				// times
	"/",				// divide
	"%",				// modulo
	"+",				// plus
	"-",				// minus
	"<<",				// leftShift
	">>",				// rightShift
	">>>",				// logicalRightShift
	"&&",				// logicalAnd
	"^^",				// logicalXor
	"||",				// logicalOr
	"&",				// bitwiseAnd
	"^",				// bitwiseXor
	"|",				// bitwiseOr
	"=",				// assignment
	"*=",				// timesEquals
	"/=",				// divideEquals
	"%=",				// moduloEquals
	"+=",				// plusEquals
	"-=",				// minusEquals
	"<<=",				// leftShiftEquals
	">>=",				// rightShiftEquals
	">>>=",				// logicalRightShiftEquals
	"&&=",				// logicalAndEquals
	"^^=",				// logicalXorEquals
	"||=",				// logicalOrEquals
	"&=",				// bitwiseAndEquals
	"^=",				// bitwiseXorEquals
	"|=",				// bitwiseOrEquals
	"==",				// equal
	"!=",				// notEqual
	"<",				// lessThan
	"<=",				// lessThanOrEqual
	">",				// greaterThan
	">=",				// greaterThanOrEqual
	"===",				// identical
	"!==",				// notIdentical
	"?",				// question

  // Reserved words
	"abstract",			// Abstract
	"break",			// Break
	"case",				// Case
	"catch",			// Catch
	"class",			// Class
	"const",			// Const
	"continue",			// Continue
	"debugger",			// Debugger
	"default",			// Default
	"delete",			// Delete
	"do",				// Do
	"else",				// Else
	"enum",				// Enum
	"eval",				// Eval
	"export",			// Export
	"extends",			// Extends
	"false",			// False
	"final",			// Final
	"finally",			// Finally
	"for",				// For
	"function",			// Function
	"goto",				// Goto
	"if",				// If
	"implements",		// Implements
	"import",			// Import
	"in",				// In
	"instanceof",		// Instanceof
	"native",			// Native
	"new",				// New
	"null",				// Null
	"package",			// Package
	"private",			// Private
	"protected",		// Protected
	"public",			// Public
	"return",			// Return
	"static",			// Static
	"super",			// Super
	"switch",			// Switch
	"synchronized",		// Synchronized
	"this",				// This
	"throw",			// Throw
	"throws",			// Throws
	"transient",		// Transient
	"true",				// True
	"try",				// Try
	"typeof",			// Typeof
	"var",				// Var
	"volatile",			// Volatile
	"while",			// While
	"with",				// With

  // Non-reserved words
	"attribute",		// Attribute
	"constructor",		// Constructor
	"get",				// Get
	"language",			// Language
	"local",			// Local
	"namespace",		// Namespace
	"override",			// Override
	"set",				// Set
	"use",				// Use

	"identifier"		// identifier
};


// Initialize the keywords in the given world.
void JS::Token::initKeywords(World &world)
{
	const char *const*keywordName = kindNames + keywordsBegin;
	for (Kind kind = keywordsBegin; kind != keywordsEnd; kind = Kind(kind+1))
		world.identifiers[widenCString(*keywordName++)].tokenKind = kind;
}


// Print a description of the token to f.
void JS::Token::print(Formatter &f, bool debug) const
{
	switch (getKind()) {
	  case end:
		f << "[end]";
		break;

	  case number:
		if (debug)
			f << "[number " << getValue() << ']';
		f << getChars();
		break;

	  case unit:
		if (debug)
			f << "[unit]";
	  case string:
		quoteString(f, getChars(), '"');
		break;

	  case regExp:
		f << '/' << getIdentifier() << '/' << getChars();
		break;

	  case identifier:
		if (debug)
			f << "[identifier]";
		f << getIdentifier();
		break;

	  default:
		f << getKind();
	}
}


// Create a new Lexer for lexing the provided source code.  The Lexer will intern identifiers, keywords, and regular
// expressions in the designated world.
JS::Lexer::Lexer(World &world, const String &source, const String &sourceLocation, uint32 initialLineNum):
	world(world), reader(source, sourceLocation, initialLineNum)
{
	nextToken = tokens;
	nTokensFwd = 0;
  #ifdef DEBUG
	nTokensBack = 0;
  #endif
	lexingUnit = false;
}


// Get and return the next token.  The token remains valid until the next call to this Lexer.
// If the Reader reached the end of file, return a Token whose Kind is end.
// The caller may alter the value of this Token (in particular, take control over the
// auto_ptr's data), but if it does so, the caller is not allowed to unget this Token.
//
// If preferRegExp is true, a / will be preferentially interpreted as starting a regular
// expression; otherwise, a / will be preferentially interpreted as division or /=.
const JS::Token &JS::Lexer::get(bool preferRegExp)
{
	const Token &t = peek(preferRegExp);
	if (++nextToken == tokens + tokenBufferSize)
		nextToken = tokens;
	--nTokensFwd;
	DEBUG_ONLY(++nTokensBack);
	return t;
}


// Peek at the next token using the given preferRegExp setting.  If that token's kind matches
// the given kind, consume that token and return it.  Otherwise, do not consume that token and
// return nil.
const JS::Token *JS::Lexer::eat(bool preferRegExp, Token::Kind kind)
{
	const Token &t = peek(preferRegExp);
	if (t.kind != kind)
		return 0;
	if (++nextToken == tokens + tokenBufferSize)
		nextToken = tokens;
	--nTokensFwd;
	DEBUG_ONLY(++nTokensBack);
	return &t;
}


// Return the next token without consuming it.
//
// If preferRegExp is true, a / will be preferentially interpreted as starting a regular
// expression; otherwise, a / will be preferentially interpreted as division or /=.
// A subsequent call to peek or get will return the same token; that call must be presented
// with the same value for preferRegExp.
const JS::Token &JS::Lexer::peek(bool preferRegExp)
{
	// Use an already looked-up token if there is one.
	if (nTokensFwd) {
		ASSERT(savedPreferRegExp[nextToken - tokens] == preferRegExp);
	} else {
		lexToken(preferRegExp);
		nTokensFwd = 1;
	  #ifdef DEBUG
		savedPreferRegExp[nextToken - tokens] = preferRegExp;
		if (nTokensBack == tokenLookahead) {
			nTokensBack = tokenLookahead-1;
			if (tokenGuard)
				(nextToken >= tokens+tokenLookahead ? nextToken-tokenLookahead : nextToken+tokenBufferSize-tokenLookahead)->valid = false;
		}
	  #endif
	}
	return *nextToken;
}


#ifdef DEBUG
// Change the setting of preferRegExp for an already peeked token.  The token must not be one
// for which that setting mattered.
void JS::Lexer::redesignate(bool preferRegExp)
{
	ASSERT(nTokensFwd && !(nextToken->hasKind(Token::regExp) || nextToken->hasKind(Token::divide) ||
						   nextToken->hasKind(Token::divideEquals)));
	savedPreferRegExp[nextToken - tokens] = preferRegExp;
}
#endif


// Unread the last token.  This call may be called to unread at most tokenBufferSize tokens
// at a time (where a peek also counts as temporarily reading and unreading one token).
// When a token that has been unread is peeked or read again, the same value must be passed
// in preferRegExp as for the first time that token was read or peeked.
void JS::Lexer::unget()
{
	ASSERT(nTokensBack--);
	nTokensFwd++;
	if (nextToken == tokens)
		nextToken = tokens + tokenBufferSize;
	--nextToken;
}


// Report a syntax error at the backUp-th last character read by the Reader.
// In other words, if backUp is 0, the error is at the next character to be read by the Reader;
// if backUp is 1, the error is at the last character read by the Reader, and so forth.
void JS::Lexer::syntaxError(const char *message, uint backUp)
{
	reader.unget(backUp);
	reader.error(Exception::syntaxError, widenCString(message), reader.getPos());
}


// Get the next character from the reader, skipping any Unicode format-control (Cf) characters.
inline char16 JS::Lexer::getChar()
{
	char16 ch = reader.get();
	if (char16Value(ch) >= firstFormatChar)
		ch = internalGetChar(ch);
	return ch;
}

// Helper for getChar()
char16 JS::Lexer::internalGetChar(char16 ch)
{
	while (isFormat(ch))
		ch = reader.get();
	return ch;
}


// Peek the next character from the reader, skipping any Unicode format-control (Cf) characters,
// which are read and discarded.
inline char16 JS::Lexer::peekChar()
{
	char16 ch = reader.peek();
	if (char16Value(ch) >= firstFormatChar)
		ch = internalPeekChar(ch);
	return ch;
}

// Helper for peekChar()
char16 JS::Lexer::internalPeekChar(char16 ch)
{
	while (isFormat(ch)) {
		reader.get();
		ch = reader.peek();
	}
	return ch;
}


// Peek the next character from the reader, skipping any Unicode format-control (Cf) characters,
// which are read and discarded.  If the peeked character matches ch, read that character and return true;
// otherwise return false.  ch must not be null.
bool JS::Lexer::testChar(char16 ch)
{
	ASSERT(ch);	// If ch were null, it could match the eof null.
	char16 ch2 = peekChar();
	if (ch == ch2) {
		reader.get();
		return true;
	}
	return false;
}


// A backslash has been read.  Read the rest of the escape code.
// Return the interpreted escaped character.  Throw an exception if the escape is not valid.
// If unicodeOnly is true, allow only \uxxxx escapes.
char16 JS::Lexer::lexEscape(bool unicodeOnly)
{
	char16 ch = getChar();
	int nDigits;

	if (!unicodeOnly || ch == 'u')
		switch (ch) {
		  case '0':
			// Make sure that the next character isn't a digit.
			ch = peekChar();
			if (!isASCIIDecimalDigit(ch))
				return 0x00;
			getChar();	// Point to the next character in the error message
		  case 'b':
			return 0x08;
		  case 'f':
			return 0x0C;
		  case 'n':
			return 0x0A;
		  case 'r':
			return 0x0D;
		  case 't':
			return 0x09;
		  case 'v':
			return 0x0B;
		  case 'x':
			nDigits = 2;
			goto lexHex;
		  case 'u':
			nDigits = 4;
		  lexHex:
			{
				uint32 n = 0;
				while (nDigits--) {
					ch = getChar();
					uint digit;
					if (!isASCIIHexDigit(ch, digit))
						goto error;
					n = (n << 4) | digit;
				}
				return static_cast<char16>(n);
			}
		default:
			if (!reader.getEof(ch)) {
				CharInfo chi(ch);
				if (!isAlphanumeric(chi) && !isLineBreak(chi))
					return ch;
			}
		}
  error:
	syntaxError("Bad escape code");
	return 0;
}


// Read an identifier into s.  The initial value of s is ignored and cleared.
// Return true if an escape code has been encountered.
// If allowLeadingDigit is true, allow the first character of s to be a digit, just like any
// continuing identifier character.
bool JS::Lexer::lexIdentifier(String &s, bool allowLeadingDigit)
{
	reader.beginRecording(s);
	bool hasEscape = false;

	while (true) {
		char16 ch = getChar();
		char16 ch2 = ch;
		if (ch == '\\') {
			ch2 = lexEscape(true);
			hasEscape = true;
		}
		CharInfo chi2(ch2);
		
		if (!(allowLeadingDigit ? isIdContinuing(chi2) : isIdLeading(chi2))) {
			if (ch == '\\')
				syntaxError("Identifier escape expands into non-identifier character");
			else
				reader.unget();
			break;
		}
		reader.recordChar(ch2);
		allowLeadingDigit = true;
	}
	reader.endRecording();
	return hasEscape;
}


// Read a numeric literal into nextToken->chars and nextToken->value.
// Return true if the numeric literal is followed by a unit, but don't read the unit yet.
bool JS::Lexer::lexNumeral()
{
	int hasDecimalPoint = 0;
	String &s = nextToken->chars;
	uint digit;

	reader.beginRecording(s);
	char16 ch = getChar();
	if (ch == '0') {
		reader.recordChar('0');
		ch = getChar();
		if ((ch&~0x20) == 'X') {
			uint32 pos = reader.getPos();
			char16 ch2 = getChar();
			if (isASCIIHexDigit(ch2, digit)) {
				reader.recordChar(ch);
				do {
					reader.recordChar(ch2);
					ch2 = getChar();
				} while (isASCIIHexDigit(ch2, digit));
				ch = ch2;
			} else
				reader.setPos(pos);
			goto done;
		} else if (isASCIIDecimalDigit(ch)) {
			syntaxError("Numeric constant syntax error");
		}
	}
	while (isASCIIDecimalDigit(ch) || ch == '.' && !hasDecimalPoint++) {
		reader.recordChar(ch);
		ch = getChar();
	}
	if ((ch&~0x20) == 'E') {
		uint32 pos = reader.getPos();
		char16 ch2 = getChar();
		char16 sign = 0;
		if (ch2 == '+' || ch2 == '-') {
			sign = ch2;
			ch2 = getChar();
		}
		if (isASCIIDecimalDigit(ch2)) {
			reader.recordChar(ch);
			if (sign)
				reader.recordChar(sign);
			do {
				reader.recordChar(ch2);
				ch2 = getChar();
			} while (isASCIIDecimalDigit(ch2));
			ch = ch2;
		} else
			reader.setPos(pos);
	}
	
  done:
	// At this point the reader is just past the character ch, which is the first non-formatting character
	// that is not part of the number.
	reader.endRecording();
	const char16 *sBegin = s.data();
	const char16 *sEnd = sBegin + s.size();
	const char16 *numEnd;
	nextToken->value = stringToDouble(sBegin, sEnd, numEnd);
	ASSERT(numEnd == sEnd);
	reader.unget();
	ASSERT(ch == reader.peek());
	return isIdContinuing(ch) || ch == '\\';
}


// Read a string literal into s.  The initial value of s is ignored and cleared.
// The opening quote has already been read into separator.
void JS::Lexer::lexString(String &s, char16 separator)
{
	char16 ch;

	reader.beginRecording(s);
	while ((ch = reader.get()) != separator) {
    	CharInfo chi(ch);
    	if (!isFormat(chi)) {
			if (ch == '\\')
				ch = lexEscape(false);
			else if (reader.getEof(ch) || isLineBreak(chi))
				syntaxError("Unterminated string literal");
			reader.recordChar(ch);
		}
	}
	reader.endRecording();
}


// Read a regular expression literal.  Store the regular expression in nextToken->id
// and the flags in nextToken->chars.
// The opening slash has already been read.
void JS::Lexer::lexRegExp()
{
	String s;
	char16 prevCh = 0;

	reader.beginRecording(s);
	while (true) {
		char16 ch = getChar();
    	CharInfo chi(ch);
		if (reader.getEof(ch) || isLineBreak(chi))
			syntaxError("Unterminated regular expression literal");
		if (prevCh == '\\') {
			reader.recordChar(ch);
			prevCh = 0;	// Ignore slashes and backslashes immediately after a backslash
		} else if (ch != '/') {
			reader.recordChar(ch);
			prevCh = ch;
		} else
			break;
	}
	reader.endRecording();
	nextToken->id = &world.identifiers[s];
	
	lexIdentifier(nextToken->chars, true);
}


// Read a token from the Reader and store it at *nextToken.
// If the Reader reached the end of file, store a Token whose Kind is end.
void JS::Lexer::lexToken(bool preferRegExp)
{
	Token &t = *nextToken;
	t.lineBreak = false;
	t.id = 0;
	//clear(t.chars);	// Don't really need to waste time clearing this string here
	Token::Kind kind;

	if (lexingUnit) {
		lexIdentifier(t.chars, false);
		ASSERT(t.chars.size());
		kind = Token::unit;				// unit
		lexingUnit = false;
	} else {
	  next:
		char16 ch = reader.get();
		if (reader.getEof(ch)) {
		  endOfInput:
			kind = Token::end;
		} else {
			char16 ch2;
			CharInfo chi(ch);

			switch (cGroup(chi)) {
		      case CharInfo::FormatGroup:
		      case CharInfo::WhiteGroup:
		    	goto next;

		      case CharInfo::IdGroup:
		    	t.pos = reader.getPos() - 1;
		      readIdentifier:
		    	{
			    	reader.unget();
			    	String s;
		    		bool hasEscape = lexIdentifier(s, false);
			    	t.id = &world.identifiers[s];
			    	kind = hasEscape ? Token::identifier : t.id->tokenKind;
		    	}
		    	break;

		      case CharInfo::NonIdGroup:
		      case CharInfo::IdContinueGroup:
		    	t.pos = reader.getPos() - 1;
		    	switch (ch) {
				  case '(':
					kind = Token::openParenthesis;	// (
					break;
				  case ')':
					kind = Token::closeParenthesis;	// )
					break;
				  case '[':
					kind = Token::openBracket;		// [
					break;
				  case ']':
					kind = Token::closeBracket;		// ]
					break;
				  case '{':
					kind = Token::openBrace;		// {
					break;
				  case '}':
					kind = Token::closeBrace;		// }
					break;
				  case ',':
					kind = Token::comma;			// ,
					break;
				  case ';':
					kind = Token::semicolon;		// ;
					break;
				  case '.':
					kind = Token::dot;				// .
					ch2 = getChar();
					if (isASCIIDecimalDigit(ch2)) {
						reader.setPos(t.pos);
						goto number;				// decimal point
					} else if (ch2 == '.') {
						kind = Token::doubleDot;	// ..
						if (testChar('.'))
							kind = Token::tripleDot; // ...
					} else
						reader.unget();
					break;
				  case ':':
					kind = Token::colon;			// :
					if (testChar(':'))
						kind = Token::doubleColon;	// ::
					break;
				  case '#':
					kind = Token::pound;			// #
					break;
				  case '@':
					kind = Token::at;				// @
					break;
				  case '?':
					kind = Token::question;			// ?
					break;

				  case '~':
					kind = Token::complement;		// ~
					break;
				  case '!':
					kind = Token::logicalNot;		// !
					if (testChar('=')) {
						kind = Token::notEqual;		// !=
						if (testChar('='))
							kind = Token::notIdentical; // !==
					}
					break;

				  case '*':
					kind = Token::times;			// * *=
				  tryAssignment:
					if (testChar('='))
						kind = Token::Kind(kind + Token::timesEquals - Token::times);
					break;

				  case '/':
					kind = Token::divide;			// /
					ch = getChar();
					if (ch == '/') {				// // comment
						do {
							ch = reader.get();
							if (reader.getEof(ch))
								goto endOfInput;
						} while (!isLineBreak(ch));
						goto endOfLine;
					} else if (ch == '*') {			// /* comment */
						ch = 0;
						do {
							ch2 = ch;
							ch = getChar();
							if (isLineBreak(ch)) {
								reader.beginLine();
								t.lineBreak = true;
							} else if (reader.getEof(ch))
								syntaxError("Unterminated /* comment");
						} while (ch != '/' || ch2 != '*');
						goto next;
					} else {
						reader.unget();
						if (preferRegExp) {			// Regular expression
							kind = Token::regExp;
							lexRegExp();
						} else
							 goto tryAssignment;	// /=
					}
					break;

				  case '%':
					kind = Token::modulo;			// %
					goto tryAssignment;				// %=

				  case '+':
					kind = Token::plus;				// +
					if (testChar('+'))
						kind = Token::increment;	// ++
					else
						goto tryAssignment;			// +=
					break;

				  case '-':
					kind = Token::minus;			// -
					ch = getChar();
					if (ch == '-')
						kind = Token::decrement;	// --
					else if (ch == '>')
						kind = Token::arrow;		// ->
					else {
						reader.unget();
						goto tryAssignment;			// -=
					}
					break;
			
				  case '&':
					kind = Token::bitwiseAnd;		// & && &= &&=
				  logical:
					if (testChar(ch))
						kind = Token::Kind(kind - Token::bitwiseAnd + Token::logicalAnd);
					goto tryAssignment;
				  case '^':
					kind = Token::bitwiseXor;		// ^ ^^ ^= ^^=
					goto logical;
				  case '|':
					kind = Token::bitwiseOr;		// | || |= ||=
					goto logical;

				  case '=':
					kind = Token::assignment;		// =
					if (testChar('=')) {
						kind = Token::equal;		// ==
						if (testChar('='))
							kind = Token::identical; // ===
					}
					break;

				  case '<':
					kind = Token::lessThan;			// <
					if (testChar('<')) {
						kind = Token::leftShift;	// <<
						goto tryAssignment;			// <<=
					}
				  comparison:
					if (testChar('='))				// <= >=
						kind = Token::Kind(kind + Token::lessThanOrEqual - Token::lessThan);
					break;
				  case '>':
					kind = Token::greaterThan;		// >
					if (testChar('>')) {
						kind = Token::rightShift;	// >>
						if (testChar('>'))
							kind = Token::logicalRightShift; // >>>
						goto tryAssignment;			// >>= >>>=
					}
					goto comparison;

				  case '\\':
					goto readIdentifier;			// An identifier that starts with an escape

				  case '\'':
				  case '"':
					kind = Token::string;			// 'string' "string"
					lexString(t.chars, ch);
					break;

				  case '0':
				  case '1':
				  case '2':
				  case '3':
				  case '4':
				  case '5':
				  case '6':
				  case '7':
				  case '8':
				  case '9':
					reader.unget();					// Number
				  number:
					kind = Token::number;
					lexingUnit = lexNumeral();
					break;

				  default:
					syntaxError("Bad character");
		    	}
		    	break;

		      case CharInfo::LineBreakGroup:
		      endOfLine:
				reader.beginLine();
				t.lineBreak = true;
				goto next;
			}
		}
	}
	t.kind = kind;
  #ifdef DEBUG
	t.valid = true;
  #endif
}


//
// Parser
//


const char *const JS::ExprNode::kindNames[kindsEnd] = {
	"NIL",			// none
	0,				// identifier
	0,				// number
	0,				// string
	0,				// regExp
	"null",			// Null
	"true",			// True
	"false",		// False
	"this",			// This
	"super",		// Super
	"public",		// Public
	"package",		// Package
	"private",		// Private

	0,				// parentheses
	0,				// numUnit
	0,				// exprUnit
	"::",			// qualify

	0,				// objectLiteral
	0,				// arrayLiteral
	0,				// functionLiteral

	0,				// call
	0,				// New
	0,				// index

	".",			// dot
	".(",			// dotParen
	"@",			// at

	"delete ",		// Delete
	"typeof ",		// Typeof
	"eval ",		// Eval
	"++ ",			// preIncrement
	"-- ",			// preDecrement
	" ++",			// postIncrement
	" --",			// postDecrement
	"+ ",			// plus
	"- ",			// minus
	"~ ",			// complement
	"! ",			// logicalNot

	"+",			// add
	"-",			// subtract
	"*",			// multiply
	"/",			// divide
	"%",			// modulo
	"<<",			// leftShift
	">>",			// rightShift
	">>>",			// logicalRightShift
	"&",			// bitwiseAnd
	"^",			// bitwiseXor
	"|",			// bitwiseOr
	"&&",			// logicalAnd
	"^^",			// logicalXor
	"||",			// logicalOr

	"==",			// equal
	"!=",			// notEqual
	"<",			// lessThan
	"<=",			// lessThanOrEqual
	">",			// greaterThan
	">=",			// greaterThanOrEqual
	"===",			// identical
	"!==",			// notIdentical
	"in",			// In
	"instanceof",	// Instanceof

	"=",			// assignment
	"+=",			// addEquals
	"-=",			// subtractEquals
	"*=",			// multiplyEquals
	"/=",			// divideEquals
	"%=",			// moduloEquals
	"<<=",			// leftShiftEquals
	">>=",			// rightShiftEquals
	">>>=",			// logicalRightShiftEquals
	"&=",			// bitwiseAndEquals
	"^=",			// bitwiseXorEquals
	"|=",			// bitwiseOrEquals
	"&&=",			// logicalAndEquals
	"^^=",			// logicalXorEquals
	"||=",			// logicalOrEquals

	"?",			// conditional
	","				// comma
};


const bool debugExprNodePrint = true;

// Print this onto f.
void JS::ExprNode::print(PrettyPrinter &f) const
{
	f << kindName(kind);
}

void JS::IdentifierExprNode::print(PrettyPrinter &f) const
{
	f << name;
}

void JS::NumberExprNode::print(PrettyPrinter &f) const
{
	f << value;
}

void JS::StringExprNode::print(PrettyPrinter &f) const
{
	quoteString(f, str, '"');
}

void JS::RegExpExprNode::print(PrettyPrinter &f) const
{
	f << '/' << regExp << '/' << flags;
}

void JS::NumUnitExprNode::print(PrettyPrinter &f) const
{
	f << numStr;
	StringExprNode::print(f);
}

void JS::ExprUnitExprNode::print(PrettyPrinter &f) const
{
	f << op;
	StringExprNode::print(f);
}

void JS::FunctionExprNode::print(PrettyPrinter &) const
{
	NOT_REACHED("***** functions not implemented yet *****");
}

void JS::PairListExprNode::print(PrettyPrinter &f) const
{
	char beginBracket;
	char endBracket;
	
	switch (getKind()) {
	  case objectLiteral:
		beginBracket = '{';
		endBracket = '}';
		break;

	  case arrayLiteral:
	  case index:
		beginBracket = '[';
		endBracket = ']';
		break;

	  case call:
	  case New:
		beginBracket = '(';
		endBracket = ')';
		break;

	  default:
		NOT_REACHED("Bad kind");
		return;
	}

	f << beginBracket;
	PrettyPrinter::Block b(f);
	const ExprPairList *p = pairs;
	if (p) 
		while (true) {
			const ExprNode *field = p->field;
			if (field) {
				{
					PrettyPrinter::Block b2(f);
					f << field << ':';
				}
				f.fillBreak(0);
			}

			const ExprNode *value = p->value;
			if (value) {
				PrettyPrinter::Block b3(f);
				f << value;
			}
			
			p = p->next;
			if (!p) break;
			f << ',';
			f.linearBreak(static_cast<uint32>(field || value));
		}
	f << endBracket;
}

void JS::InvokeExprNode::print(PrettyPrinter &f) const
{
	ASSERT(hasKind(New) || hasKind(index));
	if (hasKind(New))
		f << "new ";
	{
		PrettyPrinter::Block b(f);
		f << op;
	}
	f.fillBreak(0);
	PairListExprNode::print(f);
}

void JS::UnaryExprNode::print(PrettyPrinter &f) const
{
	if (hasKind(parentheses)) {
		f << '(';
		{
			PrettyPrinter::Block b(f);
			f << op;
		}
		f << ')';
	} else {
		if (debugExprNodePrint)
			f << '(';
		const char *name = kindName(getKind());
		if (hasKind(postIncrement) || hasKind(postDecrement)) {
			f << op;
			f << name;
		} else {
			f << name;
			f << op;
		}
		if (debugExprNodePrint)
			f << ')';
	}
}

void JS::BinaryExprNode::print(PrettyPrinter &f) const
{
	if (debugExprNodePrint)
		f << '(';
	{
		PrettyPrinter::Block b(f);
		f << op1;
	}
	uint32 nSpaces = hasKind(dot) || hasKind(dotParen) || hasKind(at) || hasKind(qualify) ? (uint32)0 : (uint32)1;
	f.fillBreak(nSpaces);
	f << kindName(getKind());
	f.fillBreak(nSpaces);
	{
		PrettyPrinter::Block b(f);
		f << op2;
	}
	if (hasKind(dotParen))
		f << ')';
	if (debugExprNodePrint)
		f << ')';
}

void JS::TernaryExprNode::print(PrettyPrinter &f) const
{
	if (debugExprNodePrint)
		f << '(';
	{
		PrettyPrinter::Block b(f);
		f << op1;
	}
	f.fillBreak(1);
	f << '?';
	f.fillBreak(1);
	{
		PrettyPrinter::Block b(f);
		f << op2;
	}
	f.fillBreak(1);
	f << ':';
	f.fillBreak(1);
	{
		PrettyPrinter::Block b(f);
		f << op3;
	}
	if (debugExprNodePrint)
		f << ')';
}


// Create a new Parser for parsing the provided source code, interning identifiers, keywords, and regular
// expressions in the designated world, and allocating the parse tree in the designated arena.
JS::Parser::Parser(World &world, Arena &arena, const String &source, const String &sourceLocation, uint32 initialLineNum):
	lexer(world, source, sourceLocation, initialLineNum), arena(arena)
{
}


// Report a syntax error at the backUp-th last token read by the Lexer.
// In other words, if backUp is 0, the error is at the next token to be read by the Lexer (which
// must have been peeked already); if backUp is 1, the error is at the last token read by the Lexer,
// and so forth.
void JS::Parser::syntaxError(const char *message, uint backUp)
{
	syntaxError(widenCString(message), backUp);
}

// Same as above, but the error message is already a String.
void JS::Parser::syntaxError(const String &message, uint backUp)
{
	while (backUp--)
		lexer.unget();
	getReader().error(Exception::syntaxError, message, lexer.getPos());
}


// Get the next token using the given preferRegExp setting.  If that token's kind matches
// the given kind, consume that token and return it.  Otherwise throw a syntax error.
const JS::Token &JS::Parser::require(bool preferRegExp, Token::Kind kind)
{
	const Token &t = lexer.get(preferRegExp);
	if (!t.hasKind(kind)) {
		String message;
		bool special = Token::isSpecialKind(kind);

		if (special)
			message += '\'';
		message += Token::kindName(kind);
		if (special)
			message += '\'';
		message += " expected";
		syntaxError(message);
	}
	return t;
}


// Copy the Token's chars into the current arena and return the resulting copy.
inline JS::String &JS::Parser::copyTokenChars(const Token &t)
{
	return newArenaString(arena, t.getChars());
}


// An identifier or parenthesized expression has just been parsed into e.
// If it is followed by one or more ::'s followed by identifiers, construct the appropriate
// qualify parse node and return it and set foundQualifiers to true.  If no ::
// is found, return e and set foundQualifiers to false.
JS::ExprNode *JS::Parser::parseIdentifierQualifiers(ExprNode *e, bool &foundQualifiers)
{
	const Token *tDoubleColon = lexer.eat(false, Token::doubleColon);
	if (!tDoubleColon) {
		foundQualifiers = false;
		return e;
	}

	foundQualifiers = true;
	checkStackSize();
	return new(arena) BinaryExprNode(tDoubleColon->getPos(), ExprNode::qualify, e, parseQualifiedIdentifier(lexer.get(true)));
}


// An opening parenthesis has just been parsed into tParen.  Finish parsing a ParenthesizedExpression.
// If it is followed by one or more ::'s followed by identifiers, construct the appropriate
// qualify parse node and return it and set foundQualifiers to true.  If no ::
// is found, return the ParenthesizedExpression and set foundQualifiers to false.
JS::ExprNode *JS::Parser::parseParenthesesAndIdentifierQualifiers(const Token &tParen, bool &foundQualifiers)
{
	uint32 pos = tParen.getPos();
	ExprNode *e = new(arena) UnaryExprNode(pos, ExprNode::parentheses, parseExpression(false));
	require(false, Token::closeParenthesis);
	return parseIdentifierQualifiers(e, foundQualifiers);
}


// Parse and return a qualifiedIdentifier.  The first token has already been parsed and is in t.
// If the second token was peeked, it should be have been done with preferRegExp set to false.
JS::ExprNode *JS::Parser::parseQualifiedIdentifier(const Token &t)
{
	bool foundQualifiers;
	ExprNode::Kind eKind;
	ExprNode *e;

	if (Token::isIdentifierKind(t.getKind())) {
		IdentifierExprNode *id = new(arena) IdentifierExprNode(t.getPos(), ExprNode::identifier, t.getIdentifier());
		return parseIdentifierQualifiers(id, foundQualifiers);
	}
	if (t.hasKind(Token::openParenthesis)) {
		e = parseParenthesesAndIdentifierQualifiers(t, foundQualifiers);
		goto checkQualifiers;
	}

	if (t.hasKind(Token::Super)) {
		eKind = ExprNode::Super;
		goto keywordQualifier;
	}
	if (t.hasKind(Token::Public)) {
		eKind = ExprNode::Public;
		goto keywordQualifier;
	}
	if (t.hasKind(Token::Package)) {
		eKind = ExprNode::Package;
		goto keywordQualifier;
	}
	if (t.hasKind(Token::Private)) {
		eKind = ExprNode::Private;
	  keywordQualifier:
		e = parseIdentifierQualifiers(new(arena) ExprNode(t.getPos(), eKind), foundQualifiers);
	  checkQualifiers:
		if (!foundQualifiers)
			syntaxError(":: expected", 0);
		return e;
	}
	syntaxError("Identifier or '(' expected");
	return 0;	// Unreachable code here just to shut up compiler warnings
}


// Parse and return an arrayLiteral. The opening bracket has already been read into initialToken.
JS::PairListExprNode *JS::Parser::parseArrayLiteral(const Token &initialToken)
{
	uint32 initialPos = initialToken.getPos();
	NodeQueue<ExprPairList> elements;

	while (true) {
		ExprNode *element = 0;
		const Token &t = lexer.peek(true);
		if (t.hasKind(Token::comma) || t.hasKind(Token::closeBracket))
			lexer.redesignate(false);
		else {
			lexer.get(true);
			element = parseAssignmentExpression(false);
		}
		elements += new(arena) ExprPairList(0, element);

		const Token &tSeparator = lexer.get(false);
		if (tSeparator.hasKind(Token::closeBracket))
			break;
		if (!tSeparator.hasKind(Token::comma))
			syntaxError("',' expected");
	}
	return new(arena) PairListExprNode(initialPos, ExprNode::arrayLiteral, elements.first);
}


// Parse and return an objectLiteral. The opening brace has already been read into initialToken.
JS::PairListExprNode *JS::Parser::parseObjectLiteral(const Token &initialToken)
{
	uint32 initialPos = initialToken.getPos();
	NodeQueue<ExprPairList> elements;

	if (!lexer.eat(true, Token::closeBrace))
		while (true) {
			const Token &t = lexer.get(true);
			ExprNode *field;
			if (Token::isIdentifierKind(t.getKind()) || t.hasKind(Token::openParenthesis) || t.hasKind(Token::Super) ||
				t.hasKind(Token::Public) || t.hasKind(Token::Package) || t.hasKind(Token::Private))
				field = parseQualifiedIdentifier(t);
			else if (t.hasKind(Token::string))
				field = new(arena) StringExprNode(t.getPos(), ExprNode::string, copyTokenChars(t));
			else if (t.hasKind(Token::number))
				field = new(arena) NumberExprNode(t.getPos(), t.getValue());
			else {
				syntaxError("Field name expected");
				field = 0;	// Unreachable code here just to shut up compiler warnings
			}
			require(false, Token::colon);
			elements += new(arena) ExprPairList(field, parseAssignmentExpression(false));

			const Token &tSeparator = lexer.get(false);
			if (tSeparator.hasKind(Token::closeBrace))
				break;
			if (!tSeparator.hasKind(Token::comma))
				syntaxError("',' expected");
		}
	return new(arena) PairListExprNode(initialPos, ExprNode::objectLiteral, elements.first);
}


// Parse and return a PrimaryExpression.
// If the first token was peeked, it should be have been done with preferRegExp set to true.
JS::ExprNode *JS::Parser::parsePrimaryExpression()
{
	ExprNode *e;
	ExprNode::Kind eKind;

	const Token &t = lexer.get(true);
	switch (t.getKind()) {
	  case Token::Null:
		eKind = ExprNode::Null;
		goto makeExprNode;
		
	  case Token::True:
		eKind = ExprNode::True;
		goto makeExprNode;
		
	  case Token::False:
		eKind = ExprNode::False;
		goto makeExprNode;
		
	  case Token::This:
		eKind = ExprNode::This;
		goto makeExprNode;
		
	  case Token::Super:
		eKind = ExprNode::Super;
		goto makeExprOrQualifierNode;
		
	  case Token::Public:
		eKind = ExprNode::Public;
	  makeExprOrQualifierNode:
		if (lexer.peek(false).hasKind(Token::doubleColon))
			goto makeQualifiedIdentifierNode;
	  makeExprNode:
		e = new(arena) ExprNode(t.getPos(), eKind);
		break;
		
	  case Token::number:
		{
			const Token &tUnit = lexer.peek(false);
			if (!tUnit.getLineBreak() && (tUnit.hasKind(Token::unit) || tUnit.hasKind(Token::string))) {
				lexer.get(false);
				e = new(arena) NumUnitExprNode(t.getPos(), ExprNode::numUnit, copyTokenChars(t), t.getValue(), copyTokenChars(tUnit));
			} else
				e = new(arena) NumberExprNode(t.getPos(), t.getValue());
		}
		break;
	
	  case Token::string:
		e = new(arena) StringExprNode(t.getPos(), ExprNode::string, copyTokenChars(t));
		break;
	
	  case Token::regExp:
		e = new(arena) RegExpExprNode(t.getPos(), ExprNode::regExp, t.getIdentifier(), copyTokenChars(t));
		break;
	
	  case Token::Package:
	  case Token::Private:
	  case CASE_TOKEN_NONRESERVED:
	  makeQualifiedIdentifierNode:
		e = parseQualifiedIdentifier(t);
		break;
	
	  case Token::openParenthesis:
		{
			bool foundQualifiers;
			e = parseParenthesesAndIdentifierQualifiers(t, foundQualifiers);
			if (!foundQualifiers) {
				const Token &tUnit = lexer.peek(false);
				if (!tUnit.getLineBreak() && tUnit.hasKind(Token::string)) {
					lexer.get(false);
					e = new(arena) ExprUnitExprNode(t.getPos(), ExprNode::exprUnit, e, copyTokenChars(tUnit));
				}
			}
		}
		break;
	
	  case Token::openBracket:
		e = parseArrayLiteral(t);
		break;
	
	  case Token::openBrace:
		e = parseObjectLiteral(t);
		break;
	
	  case Token::Function:
		syntaxError("***** functions not implemented yet *****");
		e = 0;	// Unreachable code here just to shut up compiler warnings
		break;

	  default:
		syntaxError("Expression expected");
		e = 0;	// Unreachable code here just to shut up compiler warnings
	}

	return e;
}


// Parse a . or @ followed by a QualifiedIdentifier or ParenthesizedExpression and return
// the resulting BinaryExprNode.  Use kind if a QualifiedIdentifier was found or parenKind
// if a ParenthesizedExpression was found.
// tOperator is the . or @ token.  target is the first operand.
JS::BinaryExprNode *JS::Parser::parseMember(ExprNode *target, const Token &tOperator, ExprNode::Kind kind, ExprNode::Kind parenKind)
{
	uint32 pos = tOperator.getPos();
	ExprNode *member;
	const Token &t2 = lexer.get(true);
	if (t2.hasKind(Token::openParenthesis)) {
		bool foundQualifiers;
		member = parseParenthesesAndIdentifierQualifiers(t2, foundQualifiers);
		if (!foundQualifiers)
			kind = parenKind;
	} else
		member = parseQualifiedIdentifier(t2);
	return new(arena) BinaryExprNode(pos, kind, target, member);
}


// Parse an ArgumentsList followed by a closing parenthesis or bracket and return
// the resulting InvokeExprNode.  The target function, indexed object, or created class
// is supplied.  The opening parenthesis or bracket has already been read.
// pos is the position to use for the InvokeExprNode.
JS::InvokeExprNode *JS::Parser::parseInvoke(ExprNode *target, uint32 pos, Token::Kind closingTokenKind, ExprNode::Kind invokeKind)
{
	NodeQueue<ExprPairList> arguments;
	bool hasNamedArgument = false;

	if (!lexer.eat(true, closingTokenKind))
		while (true) {
			ExprNode *field = 0;
			ExprNode *value = parseAssignmentExpression(false);
			if (lexer.eat(false, Token::colon)) {
				field = value;
				if (!ExprNode::isFieldKind(field->getKind()))
					syntaxError("Argument name must be an identifier, string, or number");
				hasNamedArgument = true;
				value = parseAssignmentExpression(false);
			} else if (hasNamedArgument)
				syntaxError("Unnamed argument cannot follow named argument", 0);
			arguments += new(arena) ExprPairList(field, value);

			const Token &tSeparator = lexer.get(false);
			if (tSeparator.hasKind(closingTokenKind))
				break;
			if (!tSeparator.hasKind(Token::comma))
				syntaxError("',' expected");
		}
	return new(arena) InvokeExprNode(pos, invokeKind, target, arguments.first);
}


// Parse and return a PostfixExpression.
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// If newExpression is true, this expression is immediately preceded by 'new', so don't allow
// call, postincrement, or postdecrement operators on it.
JS::ExprNode *JS::Parser::parsePostfixExpression(bool newExpression)
{
	ExprNode *e;

	const Token *tNew = lexer.eat(true, Token::New);
	if (tNew) {
		checkStackSize();
		uint32 posNew = tNew->getPos();
		e = parsePostfixExpression(true);
		if (lexer.eat(false, Token::openParenthesis))
			e = parseInvoke(e, posNew, Token::closeParenthesis, ExprNode::New);
		else
			e = new(arena) InvokeExprNode(posNew, ExprNode::New, e, 0);
	} else
		e = parsePrimaryExpression();
	
	while (true) {
		ExprNode::Kind eKind;
		const Token &t = lexer.get(false);
		switch (t.getKind()) {
		  case Token::openParenthesis:
			if (newExpression)
				goto other;
			e = parseInvoke(e, t.getPos(), Token::closeParenthesis, ExprNode::call);
			break;
		
		  case Token::openBracket:
			e = parseInvoke(e, t.getPos(), Token::closeBracket, ExprNode::index);
			break;
		
		  case Token::dot:
			e = parseMember(e, t, ExprNode::dot, ExprNode::dotParen);
			break;
		
		  case Token::at:
			e = parseMember(e, t, ExprNode::at, ExprNode::at);
			break;
		
		  case Token::increment:
			eKind = ExprNode::postIncrement;
		  incDec:
			if (newExpression)
				goto other;
			e = new(arena) UnaryExprNode(t.getPos(), eKind, e);
			break;
		
		  case Token::decrement:
			eKind = ExprNode::postDecrement;
			goto incDec;
		
		  default:
		  other:
			lexer.unget();
			return e;
		}
	}
}


// Parse and return a UnaryExpression.
// If the first token was peeked, it should be have been done with preferRegExp
// set to true.
JS::ExprNode *JS::Parser::parseUnaryExpression()
{
	ExprNode::Kind eKind;
	ExprNode *e;

	const Token &t = lexer.peek(true);
	uint32 pos = t.getPos();
	switch (t.getKind()) {
	  case Token::Delete:
		eKind = ExprNode::Delete;
		goto getPostfixExpression;

	  case Token::increment:
		eKind = ExprNode::preIncrement;
		goto getPostfixExpression;

	  case Token::decrement:
		eKind = ExprNode::preDecrement;
	  getPostfixExpression:
		e = parsePostfixExpression();
		break;

	  case Token::Typeof:
		eKind = ExprNode::Typeof;
		goto getUnaryExpression;

	  case Token::Eval:
		eKind = ExprNode::Eval;
		goto getUnaryExpression;

	  case Token::plus:
		eKind = ExprNode::plus;
		goto getUnaryExpression;

	  case Token::minus:
		eKind = ExprNode::minus;
		goto getUnaryExpression;

	  case Token::complement:
		eKind = ExprNode::complement;
		goto getUnaryExpression;

	  case Token::logicalNot:
		eKind = ExprNode::logicalNot;
	  getUnaryExpression:
		checkStackSize();
		e = parseUnaryExpression();
		break;

	  default:
		return parsePostfixExpression();
	}
	return new(arena) UnaryExprNode(pos, eKind, e);
}


const JS::Parser::BinaryOperatorInfo JS::Parser::tokenBinaryOperatorInfos[Token::kindsEnd] = {
  // Special
	{ExprNode::none, pExpression, pNone},						// Token::end
	{ExprNode::none, pExpression, pNone},						// Token::number
	{ExprNode::none, pExpression, pNone},						// Token::string
	{ExprNode::none, pExpression, pNone},						// Token::unit
	{ExprNode::none, pExpression, pNone},						// Token::regExp

  // Punctuators
	{ExprNode::none, pExpression, pNone},						// Token::openParenthesis
	{ExprNode::none, pExpression, pNone},						// Token::closeParenthesis
	{ExprNode::none, pExpression, pNone},						// Token::openBracket
	{ExprNode::none, pExpression, pNone},						// Token::closeBracket
	{ExprNode::none, pExpression, pNone},						// Token::openBrace
	{ExprNode::none, pExpression, pNone},						// Token::closeBrace
	{ExprNode::comma, pExpression, pExpression},				// Token::comma
	{ExprNode::none, pExpression, pNone},						// Token::semicolon
	{ExprNode::none, pExpression, pNone},						// Token::dot
	{ExprNode::none, pExpression, pNone},						// Token::doubleDot
	{ExprNode::none, pExpression, pNone},						// Token::tripleDot
	{ExprNode::none, pExpression, pNone},						// Token::arrow
	{ExprNode::none, pExpression, pNone},						// Token::colon
	{ExprNode::none, pExpression, pNone},						// Token::doubleColon
	{ExprNode::none, pExpression, pNone},						// Token::pound
	{ExprNode::none, pExpression, pNone},						// Token::at
	{ExprNode::none, pExpression, pNone},						// Token::increment
	{ExprNode::none, pExpression, pNone},						// Token::decrement
	{ExprNode::none, pExpression, pNone},						// Token::complement
	{ExprNode::none, pExpression, pNone},						// Token::logicalNot
	{ExprNode::multiply, pMultiplicative, pMultiplicative},		// Token::times
	{ExprNode::divide, pMultiplicative, pMultiplicative},		// Token::divide
	{ExprNode::modulo, pMultiplicative, pMultiplicative},		// Token::modulo
	{ExprNode::add, pAdditive, pAdditive},						// Token::plus
	{ExprNode::subtract, pAdditive, pAdditive},					// Token::minus
	{ExprNode::leftShift, pShift, pShift},						// Token::leftShift
	{ExprNode::rightShift, pShift, pShift},						// Token::rightShift
	{ExprNode::logicalRightShift, pShift, pShift},				// Token::logicalRightShift
	{ExprNode::logicalAnd, pBitwiseOr, pLogicalAnd},			// Token::logicalAnd	(right-associative for efficiency)
	{ExprNode::logicalXor, pLogicalAnd, pLogicalXor},			// Token::logicalXor	(right-associative for efficiency)
	{ExprNode::logicalOr, pLogicalXor, pLogicalOr},				// Token::logicalOr		(right-associative for efficiency)
	{ExprNode::bitwiseAnd, pBitwiseAnd, pBitwiseAnd},			// Token::bitwiseAnd
	{ExprNode::bitwiseXor, pBitwiseXor, pBitwiseXor},			// Token::bitwiseXor
	{ExprNode::bitwiseOr, pBitwiseOr, pBitwiseOr},				// Token::bitwiseOr
	{ExprNode::assignment, pPostfix, pAssignment},				// Token::assignment
	{ExprNode::multiplyEquals, pPostfix, pAssignment},			// Token::timesEquals
	{ExprNode::divideEquals, pPostfix, pAssignment},			// Token::divideEquals
	{ExprNode::moduloEquals, pPostfix, pAssignment},			// Token::moduloEquals
	{ExprNode::addEquals, pPostfix, pAssignment},				// Token::plusEquals
	{ExprNode::subtractEquals, pPostfix, pAssignment},			// Token::minusEquals
	{ExprNode::leftShiftEquals, pPostfix, pAssignment},			// Token::leftShiftEquals
	{ExprNode::rightShiftEquals, pPostfix, pAssignment},		// Token::rightShiftEquals
	{ExprNode::logicalRightShiftEquals, pPostfix, pAssignment},	// Token::logicalRightShiftEquals
	{ExprNode::logicalAndEquals, pPostfix, pAssignment},		// Token::logicalAndEquals
	{ExprNode::logicalXorEquals, pPostfix, pAssignment},		// Token::logicalXorEquals
	{ExprNode::logicalOrEquals, pPostfix, pAssignment},			// Token::logicalOrEquals
	{ExprNode::bitwiseAndEquals, pPostfix, pAssignment},		// Token::bitwiseAndEquals
	{ExprNode::bitwiseXorEquals, pPostfix, pAssignment},		// Token::bitwiseXorEquals
	{ExprNode::bitwiseOrEquals, pPostfix, pAssignment},			// Token::bitwiseOrEquals
	{ExprNode::equal, pEquality, pEquality},					// Token::equal
	{ExprNode::notEqual, pEquality, pEquality},					// Token::notEqual
	{ExprNode::lessThan, pRelational, pRelational},				// Token::lessThan
	{ExprNode::lessThanOrEqual, pRelational, pRelational},		// Token::lessThanOrEqual
	{ExprNode::greaterThan, pRelational, pRelational},			// Token::greaterThan
	{ExprNode::greaterThanOrEqual, pRelational, pRelational},	// Token::greaterThanOrEqual
	{ExprNode::identical, pEquality, pEquality},				// Token::identical
	{ExprNode::notIdentical, pEquality, pEquality},				// Token::notIdentical
	{ExprNode::conditional, pLogicalOr, pConditional},			// Token::question

  // Reserved words
	{ExprNode::none, pExpression, pNone},						// Token::Abstract
	{ExprNode::none, pExpression, pNone},						// Token::Break
	{ExprNode::none, pExpression, pNone},						// Token::Case
	{ExprNode::none, pExpression, pNone},						// Token::Catch
	{ExprNode::none, pExpression, pNone},						// Token::Class
	{ExprNode::none, pExpression, pNone},						// Token::Const
	{ExprNode::none, pExpression, pNone},						// Token::Continue
	{ExprNode::none, pExpression, pNone},						// Token::Debugger
	{ExprNode::none, pExpression, pNone},						// Token::Default
	{ExprNode::none, pExpression, pNone},						// Token::Delete
	{ExprNode::none, pExpression, pNone},						// Token::Do
	{ExprNode::none, pExpression, pNone},						// Token::Else
	{ExprNode::none, pExpression, pNone},						// Token::Enum
	{ExprNode::none, pExpression, pNone},						// Token::Eval
	{ExprNode::none, pExpression, pNone},						// Token::Export
	{ExprNode::none, pExpression, pNone},						// Token::Extends
	{ExprNode::none, pExpression, pNone},						// Token::False
	{ExprNode::none, pExpression, pNone},						// Token::Final
	{ExprNode::none, pExpression, pNone},						// Token::Finally
	{ExprNode::none, pExpression, pNone},						// Token::For
	{ExprNode::none, pExpression, pNone},						// Token::Function
	{ExprNode::none, pExpression, pNone},						// Token::Goto
	{ExprNode::none, pExpression, pNone},						// Token::If
	{ExprNode::none, pExpression, pNone},						// Token::Implements
	{ExprNode::none, pExpression, pNone},						// Token::Import
	{ExprNode::In, pRelational, pRelational},					// Token::In
	{ExprNode::Instanceof, pRelational, pRelational},			// Token::Instanceof
	{ExprNode::none, pExpression, pNone},						// Token::Native
	{ExprNode::none, pExpression, pNone},						// Token::New
	{ExprNode::none, pExpression, pNone},						// Token::Null
	{ExprNode::none, pExpression, pNone},						// Token::Package
	{ExprNode::none, pExpression, pNone},						// Token::Private
	{ExprNode::none, pExpression, pNone},						// Token::Protected
	{ExprNode::none, pExpression, pNone},						// Token::Public
	{ExprNode::none, pExpression, pNone},						// Token::Return
	{ExprNode::none, pExpression, pNone},						// Token::Static
	{ExprNode::none, pExpression, pNone},						// Token::Super
	{ExprNode::none, pExpression, pNone},						// Token::Switch
	{ExprNode::none, pExpression, pNone},						// Token::Synchronized
	{ExprNode::none, pExpression, pNone},						// Token::This
	{ExprNode::none, pExpression, pNone},						// Token::Throw
	{ExprNode::none, pExpression, pNone},						// Token::Throws
	{ExprNode::none, pExpression, pNone},						// Token::Transient
	{ExprNode::none, pExpression, pNone},						// Token::True
	{ExprNode::none, pExpression, pNone},						// Token::Try
	{ExprNode::none, pExpression, pNone},						// Token::Typeof
	{ExprNode::none, pExpression, pNone},						// Token::Var
	{ExprNode::none, pExpression, pNone},						// Token::Volatile
	{ExprNode::none, pExpression, pNone},						// Token::While
	{ExprNode::none, pExpression, pNone},						// Token::With

  // Non-reserved words
	{ExprNode::none, pExpression, pNone},						// Token::Attribute
	{ExprNode::none, pExpression, pNone},						// Token::Constructor
	{ExprNode::none, pExpression, pNone},						// Token::Get
	{ExprNode::none, pExpression, pNone},						// Token::Language
	{ExprNode::none, pExpression, pNone},						// Token::Local
	{ExprNode::none, pExpression, pNone},						// Token::Namespace
	{ExprNode::none, pExpression, pNone},						// Token::Override
	{ExprNode::none, pExpression, pNone},						// Token::Set
	{ExprNode::none, pExpression, pNone},						// Token::Use

	{ExprNode::none, pExpression, pNone}						// Token::identifier
};


struct JS::Parser::StackedSubexpression {
	ExprNode::Kind kind;				// The kind of BinaryExprNode the subexpression should generate
	uchar precedence;					// Precedence of an operator with respect to operators on its right
	uint32 pos;							// The operator token's position
	ExprNode *op1;						// First operand of the operator
	ExprNode *op2;						// Second operand of the operator (used for ?: only)
};



// Parse and return an Expression.  If noIn is false, allow the in operator.  If noAssignment is
// false, allow the = and op= operators.  If noComma is false, allow the comma operator.
// If the first token was peeked, it should be have been done with preferRegExp
// set to true.
JS::ExprNode *JS::Parser::parseExpression(bool noIn, bool noAssignment, bool noComma)
{
	ArrayBuffer<StackedSubexpression, 10> subexpressionStack;

	checkStackSize();
	// Push a limiter onto subexpressionStack.
	subexpressionStack.reserve_advance_back()->precedence = pNone;

	while (true) {
	  foundColon:
		ExprNode *e = parseUnaryExpression();

		const Token &t = lexer.peek(false);
		const BinaryOperatorInfo &binOpInfo = tokenBinaryOperatorInfos[t.getKind()];
		Precedence precedence = binOpInfo.precedenceLeft;
		ExprNode::Kind kind = binOpInfo.kind;
		ASSERT(precedence > pNone);
		
		// Disqualify assignments, 'in', and comma if the flags indicate that these should end the expression.
		if (precedence == pPostfix && noAssignment || kind == ExprNode::In && noIn || kind == ExprNode::comma && noComma) {
			kind = ExprNode::none;
			precedence = pExpression;
		}

		if (precedence == pPostfix) {
			// Ensure that the target of an assignment is a postfix subexpression.
			if (ExprNode::isUnaryKind(e->getKind()))
				syntaxError("Cannot assign to the result of this unary expression", 0);
		} else
			// Reduce already stacked operators with precedenceLeft or higher precedence
			while (subexpressionStack.back().precedence >= precedence) {
				StackedSubexpression &s = subexpressionStack.pop_back();
				if (s.kind == ExprNode::conditional) {
					if (s.op2)
						e = new(arena) TernaryExprNode(s.pos, s.kind, s.op1, s.op2, e);
					else {
						if (!t.hasKind(Token::colon))
							syntaxError("':' expected", 0);
						lexer.get(false);
						s.op2 = e;
						goto foundColon;
					}
				} else
					e = new(arena) BinaryExprNode(s.pos, s.kind, s.op1, e);
			}
		
		if (kind == ExprNode::none) {
			ASSERT(subexpressionStack.size() == 1);
			return e;
		}
		
		// Push the current operator onto the subexpressionStack.
		lexer.get(false);
		StackedSubexpression &s = *subexpressionStack.reserve_advance_back();
		s.kind = kind;
		s.precedence = binOpInfo.precedenceRight;
		s.pos = t.getPos();
		s.op1 = e;
		s.op2 = 0;
	}
}
