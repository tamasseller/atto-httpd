/*
 * UrlParser.h
 *
 *  Created on: 2017.02.11.
 *      Author: tooma
 */

#ifndef URLPARSER_H_
#define URLPARSER_H_

#include "http_parser.h"

// From nginx

/**
 * URL parser (CRTP)
 *
 * The client is requured to provide the following methods:
 *
 *  - void onQuery(const char* at, uint32_t length) {}
 *	- void queryDone() {}
 *	- void onPath(const char* at, uint32_t length) {}
 *	- void pathDone() {}
 */
template<class Child>
class UrlParser {
public:
	enum class State: uint8_t {
		SpacesBeforeUrl,
		Schema,
		SchemaSlash,
		SchemaSlashSlash,
		ServerStart,
		Server,
		ServerWithAt,
		ReqPath,
		ReqQueryStringStart,
		ReqQueryString,
		ReqFragmentStart,
		ReqFragment,
		Dead
	};

	inline void parse_url_char(const char ch);

	State state;

public:
	void reset() {
		state = State::SpacesBeforeUrl;
	}

	void parseUrl(const char* at, uint32_t length)
	{
		const char* start = at;
		while(length--) {
			State oldState = state;
			parse_url_char(*at);

			if( (oldState != State::ReqQueryString&& state == State::ReqQueryString) ||
				(oldState != State::ReqPath && state == State::ReqPath))
				start = at;
			else if(oldState == State::ReqQueryString && state != State::ReqQueryString) {
				((Child*)this)->onQuery(start, at-start);
				((Child*)this)->queryDone();
			} else if(oldState == State::ReqPath && state != State::ReqPath) {
				((Child*)this)->onPath(start, at-start);
				((Child*)this)->pathDone();
			}

			at++;
		}

		if(state == State::ReqPath)
			((Child*)this)->onPath(start, at-start);
		else if(state == State::ReqQueryString)
			((Child*)this)->onQuery(start, at-start);
	}

	void done()
	{
		if(state == State::ReqPath)
			((Child*)this)->pathDone();
		else if(state == State::ReqQueryString)
			((Child*)this)->queryDone();
	}
};

# define BIT_AT(a, i)                                                \
  (!!((unsigned int) (a)[(unsigned int) (i) >> 3] &                  \
   (1 << ((unsigned int) (i) & 7))))

#define LOWER(c)            (unsigned char)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_MARK(c)          ((c) == '-' || (c) == '_' || (c) == '.' || \
  (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '(' || \
  (c) == ')')
#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
  (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
  (c) == '$' || (c) == ',')

#define IS_URL_CHAR(c)      (BIT_AT(normal_url_char, (unsigned char)c))
#define T(v) 0

template<class Child>
inline void UrlParser<Child>::parse_url_char(const char ch)
{
	static constexpr const uint8_t normal_url_char[32] = {
	/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
	0 | T(2) | 0 | 0 | T(16) | 0 | 0 | 0,
	/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
	0 | 0 | 0 | 0 | 0 | 0 | 0 | 0,
	/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
	0 | 2 | 4 | 0 | 16 | 32 | 64 | 128,
	/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
	/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
	/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
	1 | 2 | 4 | 8 | 16 | 32 | 64 | 0, };

	if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t' || ch == '\f') {
		state = State::Dead;
		return;
	}

	switch (state) {
	case State::SpacesBeforeUrl:
		/* Proxied requests are followed by scheme of an absolute URI (alpha).
		 * All methods except CONNECT are followed by '/' or '*'.
		 */

		if (ch == '/' || ch == '*') {
			state = State::ReqPath;
		} else if (IS_ALPHA(ch)) {
			state = State::Schema;
		}

		break;

	case State::Schema:
		if (ch == ':') {
			state = State::SchemaSlash;
		}

		break;

	case State::SchemaSlash:
		if (ch == '/') {
			state = State::SchemaSlashSlash;
		}

		break;

	case State::SchemaSlashSlash:
		if (ch == '/') {
			state = State::ServerStart;
		}

		break;

	case State::ServerWithAt:
		if (ch == '@') {
			state = State::Dead;
			return;
		}
		/* no break */
	case State::ServerStart:
	case State::Server:
		if (ch == '/') {
			state = State::ReqPath;
		} else if (ch == '?') {
			state = State::ReqQueryStringStart;
		} else if (ch == '@') {
			state = State::ServerWithAt;
		} else if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
			state = State::Server;
		}

		break;

	case State::ReqPath:
		if (IS_URL_CHAR(ch)) {
			return;
		}

		switch (ch) {
		case '?':
			state = State::ReqQueryStringStart;
			break;

		case '#':
			state = State::ReqFragmentStart;
			break;
		}

		break;

	case State::ReqQueryStringStart:
	case State::ReqQueryString:
		if (IS_URL_CHAR(ch)) {
			state = State::ReqQueryString;
		} else {
			switch (ch) {
			case '?':
				/* allow extra '?' in query string */
				state = State::ReqQueryString;
				break;

			case '#':
				state = State::ReqFragmentStart;
				break;
			}
		}

		break;

	case State::ReqFragmentStart:
		if (IS_URL_CHAR(ch)) {
			state = State::ReqFragment;
		} else if (ch == '?') {
			state = State::ReqFragment;
		}

		break;
	default:
		break;
	}
}

#undef BIT_AT
#undef LOWER
#undef IS_ALPHA
#undef IS_NUM
#undef IS_ALPHANUM
#undef IS_MARK
#undef IS_USERINFO_CHAR
#undef IS_URL_CHAR
#undef T

#endif /* URLPARSER_H_ */
