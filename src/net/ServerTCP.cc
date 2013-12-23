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
#include <stdlib.h>
#include <stdio.h>


using namespace fl;
using namespace std;


// ServerTCP -----------------------------------------------------------------

void
ServerTCP::processConnection (SocketStream & ss, struct sockaddr_in & clientAddress)
{
  char * peerAddr = inet_ntoa (clientAddress.sin_addr);
  wstring peerName;
  if (peerAddr)
  {
	widenCast (peerAddr, peerName);
  }

  bool persist = true;
  while (persist  &&  ! stop)
  {
	RequestTCP  request  (ss);
	ResponseTCP response (ss);

	if (request.parse (response))
	{
	  Header * host = request.getHeader ("Host");
	  if (! host)
	  {
		if (request.versionAtLeast (1, 1))
		{
		  // This error is mandatory by RFC2616
		  response.error (400);
		}
		else
		{
		  string hostName;
		  char buffer[256];  // A hostname longer than this is really in pain.
		  if (gethostname (buffer, sizeof (buffer)) == SOCKET_ERROR)
		  {
			// Default to localhost if we can't determine any other way.
			hostName = "127.0.0.1";
		  }
		  else
		  {
			hostName = buffer;
		  }
		  if (port != 80)
		  {
			sprintf (buffer, "%i", port);
			hostName += ":";
			hostName += buffer;
		  }
		  request.addHeader ("Host", hostName);
		}
	  }
	  request.peer = peerName;

	  if (response.statusCode == 200)
	  {
		string date;
		encodeTime (time (0), date);
		response.addHeader ("Date", date);

		if (request.method == L"HEAD") response.suppressBody = true;

		respond (request, response);
	  }
	}

	ss.flush ();
	if (request.connectionClose) persist = false;
  }
}


// class RequestTCP -----------------------------------------------------------

int RequestTCP::maxHeaderLines = 65536;
int RequestTCP::maxBodyLength  = 65536;
int RequestTCP::maxLineLength  = 65536;

RequestTCP::RequestTCP (SocketStream & ss)
: ss (ss)
{
  parsedQuery = false;
  connectionClose = false;
}

const wchar_t *
RequestTCP::getQuery (const wstring & name, wstring & value)
{
  if (! parsedQuery)
  {
	parseQuery (query);
	if (method == L"POST") parseQuery (body);
	parsedQuery = true;
  }

  map<wstring, wstring>::iterator i = queries.find (name);
  if (i != queries.end ()) value = i->second;
  return value.c_str ();
}

const wchar_t *
RequestTCP::getCGI (const wstring & name, wstring & value)
{
  if      (wcscasecmp (name.c_str (), L"URL"           ) == 0) value = URL;
  else if (wcscasecmp (name.c_str (), L"REQUEST_METHOD") == 0) value = method;
  else if (wcscasecmp (name.c_str (), L"REMOTE_ADDR"   ) == 0) value = peer;
  return value.c_str ();
}

void
RequestTCP::disconnect ()
{
  connectionClose = true;
}

void
RequestTCP::imbue (const locale & loc)
{
  this->loc = loc;
}

bool
RequestTCP::parse (ResponseTCP & response)
{
  string line = "";

  // Parse request line
  int i = 0;
  do
  {
	if (i++ > maxHeaderLines  ||  ss.peek () == -1)
	{
	  connectionClose = true;
	  return false;
	}
	getline (line);
	trim (line);
  }
  while (line.length () < 1);
  parseRequestLine (line);
  response.versionMajor = versionMajor;
  response.versionMinor = versionMinor;

  // Parse headers
  string lastHeaderName = "";
  i = 0;
  while (true)
  {
	if (i++ > maxHeaderLines)
	{
	  connectionClose = true;
	  return false;
	}

	getline (line);
	trim (line);

	if (line.length () == 0) break;

	Header * header = parseHeader (line, lastHeaderName);

	if (   versionAtLeast (1, 1)
		&& strcasecmp (header->name.c_str (), "Expect") == 0
		&& header->hasValue ("100-continue"))
	{
	  response.sendStatusLine (100, "Continue");
	}
  }

  // Parse body
  //   Determine if a body is expected
  int bodyLength = 0;  // Content-Length of body.
  bool chunked = false;
  Header * header = getHeader ("Content-Length");
  if (header) bodyLength = atoi (header->values[0].c_str ());
  header = getHeader ("Transfer-Encoding");
  if (header) chunked = strcasecmp (header->values.back ().c_str (), "chunked") == 0;
  //   Receive the body by appropriate method
  if (chunked)
  {
	// TODO: This code has not been tested. Current (circa 2001) user agents don't send chunked bodies on POST, which is the only method of real interest.
	// Read chunks
	while (true)
	{
	  getline (line);
	  int size;
	  sscanf (line.c_str (), "%x", &size);
	  if (size == 0) break;
	  if (body.length () + size > maxBodyLength)
	  {
		connectionClose = true;
		return false;
	  }
	  char * temp = new char[size];
	  ss.read (temp, size);
	  body.append (temp, size);
	  delete temp;
	}
	// Parse headers at end of chunked message. (Does RFC2616 really call for this?)
	i = 0;
	while (true)
	{
	  if (i++ > maxHeaderLines)
	  {
		connectionClose = true;
		return false;
	  }
	  getline (line);
	  trim (line);
	  if (line.length () == 0) break;
	  parseHeader (line, lastHeaderName);
	}
  }
  else if (bodyLength > 0  &&  bodyLength < maxBodyLength)
  {
	char * temp = new char[bodyLength];
	ss.read (temp, bodyLength);
	body.append (temp, bodyLength);
	delete temp;
  }

  stripConnectionHeaders ();

  return true;
}

void
RequestTCP::getline (string & line)
{
  line = "";
  int next;
  while ((next = ss.get ()) != -1)
  {
	char c = next;
	if (c == '\r')
	{
	  next = ss.peek ();
	  if (next != -1  &&  (char) next == '\n')
	  {
		ss.get ();  // go ahead and consume the character
	  }
	  return;
	}
	else if (c == '\n')
	{
	  return;
	}
	else
	{
	  line += c;
	  if (line.length () > maxLineLength)
	  {
		line.erase ();
		break;
	  }
	}
  }
}

void
RequestTCP::parseRequestLine (const string & line)
{
  // Parse out request method
  string current = line;
  string next;
  split (current, " ", current, next);
  widenCast (current, method);

  // Parse out URL
  string URL;
  split (next, " ", current, next);
  split (current, "?", URL, query);

  // Parse HTTP version
  split (next, "/", current, next);
  split (next, ".", current, next);
  versionMajor = atoi (current.c_str ());
  versionMinor = atoi (next.c_str ());
  if (versionMajor == 0  &&  versionMinor == 0)
  {
	versionMajor = 0;
	versionMinor = 9;
  }

  // Remove host from absolute URL if present.
  if (URL.find ("http://") == 0)
  {
	URL = URL.substr (7);
	string host;
	split (URL, "/", host, URL);
	URL.insert (0, "/");
	// Don't create Host header unless we are less than HTTP/1.1, because we are
	// required to return an error if a HTTP/1.1 client fails to send Host itself.
	if (! versionAtLeast (1, 1))
	{
	  addHeader ("Host", host);
	}
  }
  decodeURL (URL);
  widenCast (URL, this->URL);
}

void
RequestTCP::parseQuery (string & query)
{
  while (query.length () > 0)
  {
	string item;
	int index = query.find_first_of ("&;");  // HTML4 standard, appendix B.2.2 suggests using ";" as an additional delimter beside "&"
	if (index == std::string::npos)
	{
	  item = query;
	  query.erase ();
	}
	else
	{
	  item  = query.substr (0, index);
	  query = query.substr (index + 1);
	}

	string name;
	string value;
	split (item, "=", name, value);

	decodeURL (name);
	decodeURL (value);
	wstring wname;
	wstring wvalue;
	const convertType & convert = use_facet<convertType> (loc);
	widen (name,  wname,  convert);
	widen (value, wvalue, convert);
	setQuery (wname, wvalue);
  }
}

void
RequestTCP::setQuery (const wstring & name, const wstring & value)
{
  pair<map<wstring, wstring>::iterator, bool> result;
  result = queries.insert (make_pair (name, value));
  if (! result.second)
  {
	wstring & current = result.first->second;
	if (current.size ())
	{
	  if (value.size ()) current = current + L"," + value;
	}
	else                 current = value;
  }
}

Header *
RequestTCP::parseHeader (const string & line, string & lastHeaderName)
{
  string name;
  string values;
  split (line, ":", name, values);

  if (values.length () == 0  &&  (name[0] == ' '  ||  name[0] == '\t'))
  {
	values = name;
	name = lastHeaderName;
  }

  trim (name);
  lastHeaderName = name;

  return addHeader (name, values);
}

void
RequestTCP::stripConnectionHeaders ()
{
  Header * header = getHeader ("Connection");
  if (header)
  {
	vector<string> values = header->values;
	vector<string>::iterator i;
	for (i = values.begin (); i < values.end (); i++)
	{
	  removeHeader (*i);
	  if (strcasecmp (i->c_str (), "close") == 0) connectionClose = true;
	}
  }
}


// class ResponseTCP::Streambuf -----------------------------------------------

ResponseTCP::Streambuf::Streambuf (ResponseTCP & container, SocketStream & ss)
: container (container),
  ss (ss)
{
  clear ();
  memset (&state, 0, sizeof (state));
}

inline void
ResponseTCP::Streambuf::clear ()
{
  next = buffer;
}

inline int
ResponseTCP::Streambuf::size ()
{
  return next - buffer;
}

inline void
ResponseTCP::Streambuf::flush ()
{
  int length = size ();
  if (length > 0)
  {
	ss.write (buffer, length);
	clear ();
  }
}

inline void
ResponseTCP::Streambuf::raw (const char * data, int length)
{
  // Writing raw data implies that any MBCS conversion state is blown away.
  // Perhaps a better move here would be to send whatever characters necessary
  // to move back to state 0 gracefully.
  memset (&state, 0, sizeof (state));

  const char * i = data;
  const char * endData = data + length;
  char * endBuffer = buffer + sizeof (buffer);
  while (i < endData)
  {
	if (next >= endBuffer) container.chunk ();
	*next++ = *i++;
  }
}

inline ResponseTCP::Streambuf::int_type
ResponseTCP::Streambuf::overflow (ResponseTCP::Streambuf::int_type character)
{
  if (character == WEOF)
  {
	container.done ();
  }
  else
  {
	// Narrow to sequence of bytes
	wchar_t c = (wchar_t) character;
	const wchar_t * mid1;
	char temp[32];
	char * mid2;
	convert->out (state, &c, (&c) + 1, mid1, temp, temp + sizeof (temp), mid2);
	int length = mid2 - temp;

	// If no room for character in buffer, transmit & clear buffer
	if (size () + length > sizeof (buffer))
	{
	  container.chunk ();  // Results in call to this->flush ()
	}

	// Insert sequence of bytes in buffer
	char * t = temp;
	while (t < mid2)
	{
	  *next++ = *t++;
	}
  }
  return 0;
}

void
ResponseTCP::Streambuf::imbue (const locale & loc)
{
  convert = & use_facet<convertType> (loc);
}


// class ResponseTCP ---------------------------------------------------------

ResponseTCP::ResponseTCP (SocketStream & ss)
: streamBuffer (*this, ss),
  Response (&streamBuffer)
{
  streamBuffer.imbue (getloc ());
  started = false;
  chunked = false;
  suppressBody = false;
}

void
ResponseTCP::raw (const char * data, int length)
{
  streamBuffer.raw (data, length);
}

void
ResponseTCP::done ()
{
  if (! started)
  {
	Header * header = getHeader ("Transfer-Encoding");
	if (header != 0)
	{
	  if (   versionAtLeast (1, 1)
		  && ! (header->values.size () == 1  &&  strcasecmp (header->values[0].c_str (), "identity") != 0))
	  {
		chunked = true;
		// Ensure that "chunked" occurs at the end of the list of encodings.
		addHeader ("Transfer-Encoding", "chunked", false);
	  }
	  else
	  {
		chunked = false;
	  }
	}

	if (! chunked)
	{
	  // Add Content-Length header, because we can.
	  char line[256];
	  sprintf (line, "%i", streamBuffer.size ());
	  addHeader ("Content-Length", line);
	}

	start ();
  }

  if (! suppressBody)
  {
	chunk ();

	if (chunked)
	{
	  streamBuffer.ss.write ("0\r\n", 3);
	  sendHeaders ();
	  streamBuffer.ss.write ("\r\n", 2);
	}
  }
}

void
ResponseTCP::error (int statusCode, const wstring & explanation)
{
  this->statusCode = statusCode;

  streamBuffer.clear ();

  if (! started)
  {
	*this << L"<HTML><HEAD><TITLE>";
	*this << reasonPhrase ();
	*this << L"</TITLE></HEAD><BODY>";
  }
  else
  {
	*this << L"<HR>";
  }
  *this << L"<H1>Error " << statusCode << L" " << reasonPhrase () << L"</H1>";
  *this << explanation;
  if (! started)  // Assumes buffer is big enough that inserting explanation will not trigger start.
  {
	*this << L"</BODY></HTML>";
  }

  done ();
}

void
ResponseTCP::start ()
{
  sendStatusLine (statusCode, reasonPhrase ());
  sendHeaders ();
  streamBuffer.ss.write ("\r\n", 2);

  started = true;

  if (statusCode == 204  ||  statusCode == 205  ||  statusCode == 304)
  {
	suppressBody = true;
  }
}

void
ResponseTCP::sendStatusLine (int statusCode, const string & reasonPhrase)
{
  char buffer[40];
  sprintf (buffer, "HTTP/1.1 %i ", statusCode);  // We always respond HTTP/1.1, regardless of the version of the client.
  string statusLine = buffer;
  statusLine += reasonPhrase;
  statusLine += "\r\n";
  streamBuffer.ss.write (statusLine.c_str (), statusLine.length ());
}

void
ResponseTCP::sendHeaders ()
{
  set<Header>::iterator i;
  for (i = headers.begin (); i != headers.end (); i++)
  {
	Header & h = const_cast<Header &> (*i);
	if (! h.sent ())
	{
	  string line = h.name;
	  line += ": ";
	  h.unsentValues (line);
	  line += "\r\n";
	  streamBuffer.ss.write (line.c_str (), line.length ());
	}
  }
}

void
ResponseTCP::chunk ()
{
  if (! started)
  {
	Header * header = getHeader ("Content-Length");
	if (! header  &&  versionAtLeast (1, 1))
	{
	  chunked = true;
	  // Ensure Transfer-Encoding exists and that "chunked" occurs at the end of the list of encodings.
	  addHeader ("Transfer-Encoding", "chunked", false);
	}
	else
	{
	  chunked = false;
	}

	start ();
  }

  if (suppressBody)
  {
	streamBuffer.clear ();
  }
  else
  {
	if (streamBuffer.size () > 0)
	{
	  if (chunked)
	  {
		char line[20];
		sprintf (line, "%X\r\n", streamBuffer.size ());
		streamBuffer.ss.write (line, strlen (line));
	  }
	  streamBuffer.flush ();
	  if (chunked)
	  {
		streamBuffer.ss.write ("\r\n", 2);
	  }
	}
  }
}
