/*
Generic web-server engine from the FLSB application.
Copyright 1998 Fred Rothganger
Merged into FL.

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/server.h"
#include "fl/string.h"

#include <iostream>
#include <stdio.h>


using namespace fl;
using namespace std;


// class Server ---------------------------------------------------------------

const char * Server::wkday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char * Server::month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

Server::~Server ()
{
  for (int i = 0; i < responders.size (); i++) delete responders[i];
}

void
Server::respond (Request & request, Response & response)
{
  try
  {
	bool found = false;
	vector<Responder *>::iterator i;
	for (i = responders.begin (); i < responders.end ()  &&  ! found; i++)
	{
	  found = (*i)->respond (request, response);
	}
	if (! found) response.error (404);
  }
  catch (const char * message)
  {
	wstring wmessage;
	widenCast (message, wmessage);
	response.error (500, wmessage);
  }
  catch (...)
  {
	response.error (500);
  }
}

time_t
Server::parseTime (const string & time)
{
  return 0;
}

void
Server::encodeTime (time_t time, string & result)
{
  struct tm * gmt = gmtime (&time);
  char buffer[40];
  sprintf
  (
    buffer,
	"%s, %.2i %s %i %.2i:%.2i:%.2i GMT",
	wkday[gmt->tm_wday],
	gmt->tm_mday,
	month[gmt->tm_mon],
	gmt->tm_year + 1900,
	gmt->tm_hour,
	gmt->tm_min,
	gmt->tm_sec
  );
  result = buffer;
}


// class Responder ------------------------------------------------------------

Responder::~Responder ()
{
}


// class Header ---------------------------------------------------------------

Header::Header (const string & name)
: name (name)
{
  headerSent = false;
  valuesSent = 0;
}

void
Header::addValue (const string & value, bool caseSensitive)
{
  string v = value;
  trim (v);

  // Check if we already have the value.  If so, move to back and make "unsent".
  vector<string>::iterator i;
  for (i = values.begin (); i < values.end (); i++)
  {
	if (caseSensitive ? *i == v : strcasecmp (i->c_str (), v.c_str ()) == 0)
	{
	  if (i - values.begin () < valuesSent)
	  {
		valuesSent--;
		values.erase (i);
		values.push_back (v);
	  }
	  return;
	}
  }

  values.push_back (v);
}

void
Header::addValues (const string & values, bool caseSensitive)
{
  string v = values;  // function parameter hides member with same name
  int p;
  while ((p = v.find (',')) != string::npos)
  {
	string value = v.substr (0, p);
	v.erase (0, p + 1);
	addValue (value, caseSensitive);
  }
  addValue (v, caseSensitive);
}

void
Header::removeValue (const string & value, bool caseSensitive)
{
  vector<string>::iterator i;
  for (i = values.begin (); i < values.end (); i++)
  {
	if (caseSensitive ? *i == value : strcasecmp (i->c_str (), value.c_str ()) == 0)
	{
	  if (i - values.begin () < valuesSent)
	  {
		valuesSent--;
	  }
	  values.erase (i);
	  return;
	}
  }
}

bool
Header::hasValue (const string & value, bool caseSensitive)
{
  vector<string>::iterator i;
  for (i = values.begin (); i < values.end (); i++)
  {
	if (caseSensitive ? *i == value : strcasecmp (i->c_str (), value.c_str ()) == 0)
	{
	  return true;
	}
  }

  return false;
}

bool
Header::sent () const
{
  return headerSent  &&  valuesSent >= values.size ();
}

void
Header::unsentValues (string & result)
{
  vector<string>::iterator i;
  for (i = values.begin () + valuesSent; i < values.end (); i++)
  {
	result += *i;
	if (i < values.end () - 1) result += ", ";
	valuesSent++;
  }
  headerSent = true;
}

ostream &
fl::operator << (ostream & stream, const Header & header)
{
  stream << header.name << ": ";
  vector<string>::const_iterator i;
  for (i = header.values.begin (); i < header.values.end (); i++)
  {
	stream << *i;
	if (i < header.values.end () - 1) stream << ", ";
  }
  return stream;
}


// class Message --------------------------------------------------------------

typedef struct pair<set<Header>::iterator, bool> HeaderInsertResult;

Message::Message (int versionMajor, int verionMinor)
: versionMajor (versionMajor),
  versionMinor (versionMinor)
{
}

Header *
Message::addHeader (const string & name, const string & value, bool caseSensitive)
{
  HeaderInsertResult result = headers.insert (Header (name));

  // Determine if commas are part of the format of the data, or if they are multi-item delimiters.
  bool nonDelimited = false;
  const char ** ndh = nonDelimitedHeaders;
  while (*ndh)
  {
	if (strcasecmp (*ndh, name.c_str ()) == 0)
	{
	  nonDelimited = true;
	  break;
	}
	ndh++;
  }
  Header & header = const_cast<Header &> (*result.first);
  if (nonDelimited) header.addValue  (value, caseSensitive);
  else              header.addValues (value, caseSensitive);

  return &header;
}

Header *
Message::getHeader (const string & name)
{
  Header header (name);
  set<Header>::iterator i = headers.find (header);
  if (i == headers.end ()) return 0;
  else                     return const_cast<Header *> (&(*i));
}

const char *
Message::getHeader (const string & name, string & values)
{
  Header * header = getHeader (name);
  if (! header) return values.c_str ();
  if (header->values.size () == 0)
  {
	values.clear ();  // this Header is empty
	return values.c_str ();
  }
  values = header->values[0];
  for (int i = 1; i < header->values.size (); i++)
  {
	values = values + "," + values[i];
  }
  return values.c_str ();
}

void
Message::removeHeader (const string & name)
{
  headers.erase (Header (name));
}

void
Message::removeHeader (const string & name, const string & value, bool caseSensitive)
{
  Header * header = getHeader (name);
  if (header != 0) header->removeValue (value, caseSensitive);
}

const char * Message::nonDelimitedHeaders[] =
{
  "Date",
  "Expires",
  "If-Modified-Since",
  "If-Range",
  "If-Unmodified-Since",
  "Last-Modified",
  //"Range",  // Range would be a candidate for this list. However, it has to be parsed at the commas anyway, so no harm in treating as multiple items.
  "Retry-After",
  "Warning",  // Warnings are comma delimited, but they can also contain HTTP-Dates. Since Warnings are part of a response, we can assume that our software generates them and that only one item is inserted per call to addHeader ().
  0  // Marks end of list
};

Message::characterEntity
Message::characterEntities[] =
{
  {34,   "quot"},
  {38,   "amp"},
  {39,   "apos"},
  {60,   "lt"},
  {62,   "gt"},
  {160,  "nbsp"},
  {161,  "iexcl"},
  {162,  "cent"},
  {163,  "pound"},
  {164,  "curren"},
  {165,  "yen"},
  {166,  "brvbar"},
  {167,  "sect"},
  {168,  "uml"},
  {169,  "copy"},
  {170,  "ordf"},
  {171,  "laquo"},
  {172,  "not"},
  {173,  "shy"},
  {174,  "reg"},
  {175,  "macr"},
  {176,  "deg"},
  {177,  "plusmn"},
  {178,  "sup2"},
  {179,  "sup3"},
  {180,  "acute"},
  {181,  "micro"},
  {182,  "para"},
  {183,  "middot"},
  {184,  "cedil"},
  {185,  "sup1"},
  {186,  "ordm"},
  {187,  "raquo"},
  {188,  "frac14"},
  {189,  "frac12"},
  {190,  "frac34"},
  {191,  "iquest"},
  {192,  "Agrave"},
  {193,  "Aacute"},
  {194,  "Acirc"},
  {195,  "Atilde"},
  {196,  "Auml"},
  {197,  "Aring"},
  {198,  "AElig"},
  {199,  "Ccedil"},
  {200,  "Egrave"},
  {201,  "Eacute"},
  {202,  "Ecirc"},
  {203,  "Euml"},
  {204,  "Igrave"},
  {205,  "Iacute"},
  {206,  "Icirc"},
  {207,  "Iuml"},
  {208,  "ETH"},
  {209,  "Ntilde"},
  {210,  "Ograve"},
  {211,  "Oacute"},
  {212,  "Ocirc"},
  {213,  "Otilde"},
  {214,  "Ouml"},
  {215,  "times"},
  {216,  "Oslash"},
  {217,  "Ugrave"},
  {218,  "Uacute"},
  {219,  "Ucirc"},
  {220,  "Uuml"},
  {221,  "Yacute"},
  {222,  "THORN"},
  {223,  "szlig"},
  {224,  "agrave"},
  {225,  "aacute"},
  {226,  "acirc"},
  {227,  "atilde"},
  {228,  "auml"},
  {229,  "aring"},
  {230,  "aelig"},
  {231,  "ccedil"},
  {232,  "egrave"},
  {233,  "eacute"},
  {234,  "ecirc"},
  {235,  "euml"},
  {236,  "igrave"},
  {237,  "iacute"},
  {238,  "icirc"},
  {239,  "iuml"},
  {240,  "eth"},
  {241,  "ntilde"},
  {242,  "ograve"},
  {243,  "oacute"},
  {244,  "ocirc"},
  {245,  "otilde"},
  {246,  "ouml"},
  {247,  "divide"},
  {248,  "oslash"},
  {249,  "ugrave"},
  {250,  "uacute"},
  {251,  "ucirc"},
  {252,  "uuml"},
  {253,  "yacute"},
  {254,  "thorn"},
  {255,  "yuml"},
  {338,  "OElig"},
  {339,  "oelig"},
  {352,  "Scaron"},
  {353,  "scaron"},
  {376,  "Yuml"},
  {402,  "fnof"},
  {710,  "circ"},
  {732,  "tilde"},
  {913,  "Alpha"},
  {914,  "Beta"},
  {915,  "Gamma"},
  {916,  "Delta"},
  {917,  "Epsilon"},
  {918,  "Zeta"},
  {919,  "Eta"},
  {920,  "Theta"},
  {921,  "Iota"},
  {922,  "Kappa"},
  {923,  "Lambda"},
  {924,  "Mu"},
  {925,  "Nu"},
  {926,  "Xi"},
  {927,  "Omicron"},
  {928,  "Pi"},
  {929,  "Rho"},
  {931,  "Sigma"},
  {932,  "Tau"},
  {933,  "Upsilon"},
  {934,  "Phi"},
  {935,  "Chi"},
  {936,  "Psi"},
  {937,  "Omega"},
  {945,  "alpha"},
  {946,  "beta"},
  {947,  "gamma"},
  {948,  "delta"},
  {949,  "epsilon"},
  {950,  "zeta"},
  {951,  "eta"},
  {952,  "theta"},
  {953,  "iota"},
  {954,  "kappa"},
  {955,  "lambda"},
  {956,  "mu"},
  {957,  "nu"},
  {958,  "xi"},
  {959,  "omicron"},
  {960,  "pi"},
  {961,  "rho"},
  {962,  "sigmaf"},
  {963,  "sigma"},
  {964,  "tau"},
  {965,  "upsilon"},
  {966,  "phi"},
  {967,  "chi"},
  {968,  "psi"},
  {969,  "omega"},
  {977,  "thetasym"},
  {978,  "upsih"},
  {982,  "piv"},
  {8194, "ensp"},
  {8195, "emsp"},
  {8201, "thinsp"},
  {8204, "zwnj"},
  {8205, "zwj"},
  {8206, "lrm"},
  {8207, "rlm"},
  {8211, "ndash"},
  {8212, "mdash"},
  {8216, "lsquo"},
  {8217, "rsquo"},
  {8218, "sbquo"},
  {8220, "ldquo"},
  {8221, "rdquo"},
  {8222, "bdquo"},
  {8224, "dagger"},
  {8225, "Dagger"},
  {8226, "bull"},
  {8230, "hellip"},
  {8240, "permil"},
  {8242, "prime"},
  {8243, "Prime"},
  {8249, "lsaquo"},
  {8250, "rsaquo"},
  {8254, "oline"},
  {8260, "frasl"},
  {8364, "euro"},
  {8465, "image"},
  {8472, "weierp"},
  {8476, "real"},
  {8482, "trade"},
  {8501, "alefsym"},
  {8592, "larr"},
  {8593, "uarr"},
  {8594, "rarr"},
  {8595, "darr"},
  {8596, "harr"},
  {8629, "crarr"},
  {8656, "lArr"},
  {8657, "uArr"},
  {8658, "rArr"},
  {8659, "dArr"},
  {8660, "hArr"},
  {8704, "forall"},
  {8706, "part"},
  {8707, "exist"},
  {8709, "empty"},
  {8711, "nabla"},
  {8712, "isin"},
  {8713, "notin"},
  {8715, "ni"},
  {8719, "prod"},
  {8721, "sum"},
  {8722, "minus"},
  {8727, "lowast"},
  {8730, "radic"},
  {8733, "prop"},
  {8734, "infin"},
  {8736, "ang"},
  {8743, "and"},
  {8744, "or"},
  {8745, "cap"},
  {8746, "cup"},
  {8747, "int"},
  {8756, "there4"},
  {8764, "sim"},
  {8773, "cong"},
  {8776, "asymp"},
  {8800, "ne"},
  {8801, "equiv"},
  {8804, "le"},
  {8805, "ge"},
  {8834, "sub"},
  {8835, "sup"},
  {8836, "nsub"},
  {8838, "sube"},
  {8839, "supe"},
  {8853, "oplus"},
  {8855, "otimes"},
  {8869, "perp"},
  {8901, "sdot"},
  {8942, "vellip"},
  {8968, "lceil"},
  {8969, "rceil"},
  {8970, "lfloor"},
  {8971, "rfloor"},
  {9001, "lang"},
  {9002, "rang"},
  {9674, "loz"},
  {9824, "spades"},
  {9827, "clubs"},
  {9829, "hearts"},
  {9830, "diams"},
  {0, ""}
};

const char *
Message::URIsafe = "-_.!~*'()/";  // see RFC 2396

ostream &
fl::operator << (ostream & stream, const Message & message)
{
  set<Header>::const_iterator i;
  for (i = message.headers.begin (); i != message.headers.end (); i++)
  {
	stream << *i << endl;
  }
  return stream;
}


// class Request --------------------------------------------------------------

void
Request::decodeURL (string & result)
{
  int position = result.find_first_of ('%');
  while (position != string::npos)
  {
	string charName = "0x";
	charName += result.substr (position + 1, 2);
	int c;
	sscanf (charName.c_str (), "%i", &c);
	result.replace (position, 3, 1, (char) c);
	position = result.find_first_of ('%', position + 1);
  }
}

void
Request::decodeCharacters (wstring & result)
{
  int position = result.find_first_of (L'&');
  while (position != wstring::npos)
  {
	wstring reference;
	wstring charName;
	int endPosition = result.find_first_of (L';', position);
	if (endPosition == wstring::npos)
	{
	  reference = result.substr (position);
	  charName = reference.substr (1);
	}
	else
	{
	  reference = result.substr (position, endPosition - position + 1);
	  charName = reference.substr (1, reference.length () - 2);
	}
	wstring Character;
	if (charName[0] == L'#')
	{
	  // Number of character
	  charName.erase (0, 1);
	  if (charName[0] == L'x')
	  {
		// Hex number
		charName.insert (0, L"0");
	  }
	  int c;
	  swscanf (charName.c_str (), L"%i", &c);
	  Character = (wchar_t) c;
	}
	else  // Named character
	{
	  string ncharName;
	  narrowCast (charName, ncharName);
	  characterEntity * e = characterEntities;
	  while (e->code)
	  {
		if (ncharName == e->name)
		{
		  Character = (wchar_t) e->code;
		  break;
		}
	  }
	}
	result.replace (position, reference.length (), Character);
	position = result.find_first_of (L'&', position + 1);
  }
}

void
Request::decodeBase64 (const string & in, string & result)
{
}


// class Response -------------------------------------------------------------

map<int, string> Response::reasons;

Response::Response (wstreambuf * buffer)
: wostream (buffer)
{
  statusCode = 200;
}

const char *
Response::reasonPhrase () const
{
  if (reasons.size () == 0) initReasons ();
  map<int, string>::const_iterator it = reasons.find (statusCode);
  if (it != reasons.end ()) return it->second.c_str ();

  switch (statusCode / 100)  // Remainder is thrown away by integer arithmetic.  Should give major status number.
  {
	case 1: return "Continue";
	case 2: return "OK";
	case 3: return "Multiple Choices";
	case 4: return "Bad Request";
	case 5:
	default:
	  return "Internal Server Error";
  }
}

void
Response::initReasons ()
{
  reasons.insert (make_pair (100, "Continue"));
  reasons.insert (make_pair (101, "Switching Protocols"));
  reasons.insert (make_pair (200, "OK"));
  reasons.insert (make_pair (201, "Created"));
  reasons.insert (make_pair (202, "Accepted"));
  reasons.insert (make_pair (203, "Non-Authoritative Information"));
  reasons.insert (make_pair (204, "No Content"));
  reasons.insert (make_pair (205, "Reset Content"));
  reasons.insert (make_pair (206, "Partial Content"));
  reasons.insert (make_pair (300, "Multiple Choices"));
  reasons.insert (make_pair (301, "Moved Permanently"));
  reasons.insert (make_pair (302, "Found"));
  reasons.insert (make_pair (303, "See Other"));
  reasons.insert (make_pair (304, "Not Modified"));
  reasons.insert (make_pair (305, "Use Proxy"));
  reasons.insert (make_pair (307, "Temporary Redirect"));
  reasons.insert (make_pair (400, "Bad Request"));
  reasons.insert (make_pair (401, "Unauthorized"));
  reasons.insert (make_pair (402, "Payment Required"));
  reasons.insert (make_pair (403, "Forbidden"));
  reasons.insert (make_pair (404, "Not Found"));
  reasons.insert (make_pair (405, "Method Not Allowed"));
  reasons.insert (make_pair (406, "Not Acceptable"));
  reasons.insert (make_pair (407, "Proxy Authentication Required"));
  reasons.insert (make_pair (408, "Request Timeout"));
  reasons.insert (make_pair (409, "Conflict"));
  reasons.insert (make_pair (410, "Gone"));
  reasons.insert (make_pair (411, "Length Required"));
  reasons.insert (make_pair (412, "Precondition Failed"));
  reasons.insert (make_pair (413, "Request Entity Too Large"));
  reasons.insert (make_pair (414, "Request-URI Too Long"));
  reasons.insert (make_pair (415, "Unsupported Media Type"));
  reasons.insert (make_pair (416, "Requested Range Not Satisfiable"));
  reasons.insert (make_pair (417, "Expectation Failed"));
  reasons.insert (make_pair (500, "Internal Server Error"));
  reasons.insert (make_pair (501, "Not Implemented"));
  reasons.insert (make_pair (502, "Bad Gateway"));
  reasons.insert (make_pair (503, "Service Unavailable"));
  reasons.insert (make_pair (504, "Gateway Timeout"));
  reasons.insert (make_pair (505, "HTTP Version Not Supported"));
}

void
Response::encodeURL (wstring & result)
{
  for (int i = 0; i < result.length (); i++)
  {
	int a = result[i];
	if (a > 255) throw "URIs should be in UTF8";

	int A = a & ~0x20;
	if ((A >= 'A' && A <= 'Z') || (a >= '0' && a <= '9')) continue;

	bool safe = false;
	const char * m = URIsafe;
	while (*m  &&  ! safe) safe = *m++ == a;
	if (safe) continue;

	wchar_t buffer[8];
	swprintf (buffer, sizeof (buffer), L"%%%02x", a);
	result.replace (i, 1, buffer);
	i += wcslen (buffer) - 1;
  }
}

void
Response::encodeCharacters (wstring & result)
{
  for (int i = 0; i < result.length (); i++)
  {
	int a = result[i];
	if (a > 255)
	{
	  wchar_t buffer[32];
	  swprintf (buffer, sizeof (buffer), L"&#%i;", a);
	  result.replace (i, 1, buffer);
	  i += wcslen (buffer) - 1;
	}
  }
}

void
Response::encodeBase64 (const string & in, string & result)
{
}


// Utility functions ----------------------------------------------------------

wostream &
fl::operator << (wostream & out, const char * data)
{
  const convertType & convert = use_facet<convertType> (out.getloc ());
  int length = strlen (data);
  wchar_t * buffer = new wchar_t[length + 1];
  mbstate_t state;
  memset (&state, 0, sizeof (state));
  const char * mid1;
  wchar_t * mid2;
  convert.in (state, data, data + length, mid1, buffer, buffer + length, mid2);
  *mid2 = L'\0';
  out << buffer;
  delete buffer;
  return out;
}
