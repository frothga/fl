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


#ifndef fl_server_h
#define fl_server_h


#include "fl/socket.h"
#include "fl/string.h"

#include <vector>
#include <set>
#include <map>
#include <string>
#include <ostream>
#include <streambuf>
#include <locale>
#include <time.h>
#include <stdint.h>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNet_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  // Generic web server -------------------------------------------------------

  class SHARED Server;
  class SHARED Responder;
  class SHARED Header;
  class SHARED Message;
  class SHARED Request;
  class SHARED Response;

  /// Adapts the external web service technology to the internal "Responder" style.
  class SHARED Server
  {
  public:
	~Server (); ///< Destructs all responders.

	void respond (Request & request, Response & response);

	std::vector<Responder *> responders;  ///< Collection of services this server provides. We take ownership of these objects.

	// Global interface
	static time_t parseTime  (const std::string & time);
	static void   encodeTime (time_t time, std::string & result);

	static const char * wkday[];
	static const char * month[];
  };

  /// Returns a web page when presented with an appropriate URL.
  class SHARED Responder
  {
  public:
	virtual ~Responder ();

	virtual bool respond (Request & request, Response & response) = 0;  ///< Returns true if this object "claims" the URL. If so, this object is responsible to write something appropriate to the Response object.
  };

  class SHARED Header
  {
  public:
	Header (const std::string & name);

	void addValue    (const std::string & value,  bool caseSensitive = true);
	void addValues   (const std::string & values, bool caseSensitive = true);
	void removeValue (const std::string & value,  bool caseSensitive = true);
	bool hasValue    (const std::string & value,  bool caseSensitive = true);

	bool sent () const;  ///< Indicates that this header has been completely sent.
	void unsentValues (std::string & result);  ///< Concatenates to result a comma separated list of values. Assumes these will in fact be sent.

	bool operator < (const Header & that) const
	{
	  // Header names are by definition case insensitive.
	  return strcasecmp (name.c_str (), that.name.c_str ()) < 0;
	}

	std::string name;
	std::vector<std::string> values;
	bool headerSent;  ///< Indicates that at least part of this Header has been sent already.
	int valuesSent;   ///< Count of how many entries in the values vector have already been sent.
  };
  SHARED std::ostream & operator << (std::ostream & stream, const Header & header);

  /// Common part of HTTP messages.
  class SHARED Message
  {
  public:
	Message (int versionMajor = 1, int versionMinor = 1);

	virtual Header *     addHeader    (const std::string & name, const std::string & value, bool caseSensitive = true);  ///< Appends value to end of list of values associated with named Header. Creates Header if it doesn't already exist.
	virtual Header *     getHeader    (const std::string & name);  ///< Returns a pointer to the named Header. Returns 0 if header hasn't been created yet.
	virtual const char * getHeader    (const std::string & name, std::string & values);  ///< Puts comma-separated list of header values in "values", or leaves it unchanged if header is not found. @return values.c_str ()
	virtual void         removeHeader (const std::string & name);  ///< Removes the named Header.
	virtual void         removeHeader (const std::string & name, const std::string & value, bool caseSensitive = true);  ///< Removes value from the list associated with named Header. Convenience method for getHeader()->removeValue().

	bool versionAtLeast (int major, int minor)  ///< Indicates that the HTTP version of this message is greater than or equal to major.minor
	{
	  return versionMajor > major  ||  (versionMajor == major  &&  versionMinor >= minor);
	}

	std::set<Header> headers;
	std::string protocol;
	int versionMajor;
	int versionMinor;

	static const char * nonDelimitedHeaders[];  ///< Array of header names which don't use comma as a multi-item delimiter (e.g.: dates). In alphabetical order, but not canonized w.r.t. capitalization.

	struct characterEntity
	{
	  int          code;
	  const char * name;
	};
	static characterEntity characterEntities[];

	static const char * URIsafe;  ///< Characters other than alphanumeric which may be safely used in a URI without escaping.
  };
  SHARED std::ostream & operator << (std::ostream & stream, const Message & message);

  /// Encapsulates the request from the client.
  class SHARED Request : public Message
  {
  public:
	/**
	   Fetches query or form value with given name.
	   @param value Contains the default value. Will be replaced by the
	   actual value if it exists, otherwise unchanged.
	   @returns value.c_str()
	**/
	virtual const wchar_t * getQuery (const std::wstring & name,       std::wstring & value) = 0;
	virtual const wchar_t * getCGI   (const std::wstring & name,       std::wstring & value) = 0;  ///< Fetches CGI variable with given name. @see getQuery() for interface.
	virtual void            setQuery (const std::wstring & name, const std::wstring & value = L"") = 0;
	virtual void disconnect () {}  ///< Indicates that persistent connection should be closed after this request is processed.
	virtual void imbue (const std::locale & loc) {}  ///< What locale to use when widening query and form data.

	static void decodeURL        (std::string &  result);  ///< Convert %HexHex values into octets in place
	static void decodeCharacters (std::wstring & result);  ///< Convert &{name}; character values into wchars in place
	static void decodeBase64     (const std::string & in, std::string & result);
  };

  /// Encapsulates the message to be returned to the client.
  class SHARED Response : public Message, public std::wostream
  {
  public:
	Response (std::wstreambuf * buffer);
	Response (Response &&) = delete;  ///< Having a move ctor conflicts with MSVC version of basic_ios. Move construction isn't used in normal operation of Server.

	virtual void raw (const char * data, int length) = 0;  ///< Writes raw bytes to stream, ie: without any code conversion.
	virtual void done () = 0;  ///< Finalizes message. No more data should be inserted after this method is called. Flushes self and all underlying streams.
	virtual void error (int statusCode, const std::wstring & explanation = L"") = 0;  ///< Finalizes message, throws away any unsent data, and transmits an error message.

	virtual const char * reasonPhrase () const;  ///< Maps current statusCode to a reason phrase stored in codeNames.
	static void initReasons ();  ///< Build reasons table. Called by reasonPhrase() if the table is empty. Use this to preemptively initialize and then modify the table.

	static void encodeURL        (std::wstring & result);  ///< Convert appropriate characters to %HexHex and append to result.
	static void encodeCharacters (std::wstring & result);  ///< Convert characters higher than 0xFF to &{name}; and append to result.
	static void encodeBase64     (const std::string &  in,     std::string &  result);

	int statusCode;
	static std::map<int, std::string> reasons; ///< Standard map between status codes and their reason phrases for HTTP.
  };


  // TCP implementation -------------------------------------------------------

  typedef std::codecvt<wchar_t, char, mbstate_t> convertType;

  class SHARED ServerTCP;
  class SHARED MessageTCP;
  class SHARED RequestTCP;
  class SHARED ResponseTCP;

  class SHARED ServerTCP : public Server, public Listener
  {
  public:
	virtual void processConnection (SocketStream & ss, struct sockaddr_in & clientAddress);
  };

  class SHARED RequestTCP : public Request
  {
  public:
	RequestTCP (SocketStream & ss);

	virtual const wchar_t * getQuery (const std::wstring & name,       std::wstring & value);
	virtual const wchar_t * getCGI   (const std::wstring & name,       std::wstring & value);
	virtual void            setQuery (const std::wstring & name, const std::wstring & value = L"");
	virtual void disconnect ();
	virtual void imbue (const std::locale & loc);

	bool parse (ResponseTCP & response);  ///< Parse the entire request message. Response object is needed for any immediate replies to client (such as "100 Continue"). Returns true if parse was completed successfully, or false if connection should close.
	void getline (std::string & line);  ///< Read the next CRLF delimited piece of input (but throw away the CRLF). Tolerates naked CR or LF.
	void parseRequestLine (const std::string & line);
	void parseQuery (std::string & query);
	Header * parseHeader (const std::string & line, std::string & lastHeaderName);
	void stripConnectionHeaders ();

	SocketStream & ss;
	std::locale loc;
	std::wstring peer;  // IP address of requester.
	std::wstring method;
	std::wstring URL;
	std::string query;
	std::string body;
	std::map<std::wstring, std::wstring> queries;
	bool parsedQuery;  // Also includes body
	bool connectionClose;  // Indicates that header "Connection: close" was seen.

	static int maxHeaderLines;
	static int maxBodyLength;
	static int maxLineLength;
  };
  SHARED std::ostream & operator << (std::ostream & stream, const RequestTCP & request);

  class SHARED ResponseTCP : public Response
  {
  public:
	ResponseTCP (SocketStream & ss);

	virtual void raw (const char * data, int length);
	virtual void done ();  ///< Only executes once. After that, this object will not touch ss again.
	virtual void error (int statusCode, const std::wstring & explanation = L"");

	virtual const char * reasonPhrase () const;  ///< Adds RTSP specific codes, so this class can double as HTTP and RTSP.
	static void initReasonsRTSP ();

	void start ();  // Sends response line and initial headers. Called automatically. Only executes once.
	void sendStatusLine (int statusCode, const std::string & reasonPhrase);  // Send the status line.  Called automatically.  More than one status line may be sent per response (such as if "100 Continue" is indicated).
	void sendHeaders ();  // Sends any headers that haven't yet been sent.  Called automatically.
	void chunk ();  // Sends blocks of data to client.  Handles both non-chunked and chunked transfer modes.  Called automatically.

	/**
	   Provides the underlying wstreambuf to make the Response object into
	   a wostream.
	**/
	class SHARED Streambuf : public std::wstreambuf
	{
	public:
	  Streambuf (ResponseTCP & container, SocketStream & ss);

	  void clear (); ///< Remove any characters in buffer without sending them.
	  int  size  (); ///< Number of characters currently in buffer.
	  void flush (); ///< Same job as sync(). However, sync() gets called by stream classes in a way that makes it unuseable for chunking.
	  void raw (const char * data, int length); ///< Immediately insert byes in buffer, bypassing character conversion.

	  virtual int_type overflow (int_type character = WEOF);
	  virtual void imbue (const std::locale & loc);

	  SocketStream & ss;
	  ResponseTCP & container;
	  char buffer[65536];  ///< 64K buffer. Any larger, then we chunk. (Or, if HTTP/1.0, we simply write to connection and continue.)
	  char * next;
	  const convertType * convert;
	  mbstate_t state;
	};

	Streambuf streamBuffer;
	bool started;   ///< start() has already executed
	bool finished;  ///< done() has already executed
	bool chunked;
	bool suppressBody;

	static std::map<int, std::string> reasonsRTSP; ///< RTSP specific map between status codes and their reason phrases.
  };
  SHARED std::ostream & operator << (std::ostream & stream, const ResponseTCP & response);


  // Standard Responders ------------------------------------------------------

  /**
	 Base of utility classes for parsing URLs as paths to resources.
	 Interpretation of given name is different in different subclasses.
  **/
  class SHARED ResponderTree : public Responder
  {
  public:
	ResponderTree (const std::wstring & name, bool caseSensitive = false);

	virtual bool respond (Request & request, Response & response);
	virtual bool respond (Request & request, Response & response, const std::wstring & path) = 0;

	std::wstring name;
	bool caseSensitive;
  };

  /**
	 Creates a hierarchical composition of responders. This responder acts
	 as the root for all responders embedded in it. The relationship is
	 much like a directory to its files. This allows you to name each object
	 more concisely, without repeating all the names of its parents.
  **/
  class SHARED ResponderDirectory : public ResponderTree
  {
  public:
	ResponderDirectory (const std::wstring & name, bool caseSensitive = false);

	virtual bool respond (Request & request, Response & response, const std::wstring & path);

	std::vector<ResponderTree *> responders;
  };

  /**
	 Serves a named document that is generated upon every request.
	 The given name can be a regular expression.
  **/
  class SHARED ResponderName : public ResponderTree
  {
  public:
	ResponderName (const std::wstring & name, bool caseSensitive = false);

	virtual bool respond (Request & request, Response & response, const std::wstring & path);
	bool match (const std::wstring & path);

	virtual void generate (Request & request, Response & response);  ///< Override this function to output documents. Called by respond() if URL matches name and if security checks out.
  };

  /**
	 Maps URLs to some portion of the filesystem. Any suffix in the URL past
	 the given name is appended to the given root. Careful use of these strings
	 allows acces to both file and directories.
  **/
  class SHARED ResponderFile : public ResponderTree
  {
  public:
	ResponderFile (const std::wstring & name, const std::wstring & root, bool caseSensitive = false);

	virtual bool respond (Request & request, Response & response, const std::wstring & path);

	virtual void generateFile     (Request & request, Response & response, const std::wstring & fileName, std::wstring & suffix, FILE * file);  ///< Output contents of file.
	virtual void setContentLength (Request & request, Response & response, const std::wstring & fileName,                        FILE * file);  ///< subroutine of generateFile(). Sets the Content-Length header. Override to (for example) allow unbounded length.
	virtual void setContentType   (Request & request, Response & response, const std::wstring & fileName, std::wstring & suffix);               ///< subroutine of generateFile(). Sets the Content-Type header. Ovrride to (for example) use a more intelligent method than suffix matching.
	virtual bool hasMore          (Request & request, Response & response, const std::wstring & fileName,                        FILE * file);  ///< subroutine of generateFile(). If the file is one that is still growing, then this function blocks until enough data arrives and returns true. Otherwise return false.

	virtual void generateDirectoryListing (Request & request, Response & response, const std::wstring & path, const std::wstring & dirName);  ///< Output contents of directory as a web-page, with clickable links to files. Override this if you want to provide a different format, or to forbid directory browsing.

	struct DirEntry
	{
	  std::string name;  ///< @todo: Use system locale to widen filenames and store as wstring
	  uint64_t    size;
	  time_t      time;
	};
	void scan (const std::wstring & dirName, int sortBy, std::multimap<std::string, DirEntry> & result);
	virtual void write (Response & response, const DirEntry & entry, const std::wstring & pathWithSlash); ///< Output one row in the table. Override this to change output format or to filter which files to list.

	std::wstring root;

	struct MIMEtype
	{
	  std::wstring suffix;
	  std::string  type;
	  MIMEtype *   next;
	};
	static MIMEtype * MIMEtypes;  ///< Mapping form file suffix to returned MIME type. No deeper analysis is done on file.
	static void addMIMEtype    (const std::wstring & suffix, const std::string & MIME);
	static void removeMIMEtype (const std::wstring & suffix);
  };


  // Utility functions --------------------------------------------------------

  inline bool
  regexpMatch (const std::wstring & regexp, const std::wstring & target)
  {
	// This function is an FSA that handles "?" and "*"
	const wchar_t * state      = regexp.c_str ();
	const wchar_t * startState = state;
	const wchar_t * t          = target.c_str ();
	while (*t)
	{
	  if (*t == *state  ||  *state == L'?')
	  {
		state++;
	  }
	  else if (*state == L'*')
	  {
		if (*t == *(state + 1)  ||  *(state + 1) == L'?')
		{
		  startState = state;
		  state += 2;
		}
	  }
	  else
	  {
		if (*startState == L'*')
		{
		  state = startState;
		}
		else
		{
		  return false;
		}
	  }
	  t++;
	}
	if (*state == L'*')
	{
	  state++;
	}
	return *state == L'\0';
  }

  std::wostream & operator << (std::wostream & out, const char * data);

  inline std::wostream &
  operator << (std::wostream & out, const std::string & data)
  {
	return out << data.c_str ();
  }

  inline void
  widen (const std::string & source, std::wstring & result, const convertType & convert = std::use_facet<convertType> (std::locale ()))
  {
	int length = source.length ();
	const char * data = source.c_str ();
	wchar_t * buffer = new wchar_t[length + 1];
	mbstate_t state;
	memset (&state, 0, sizeof (state));
	const char * mid1;
	wchar_t * mid2;
	convert.in (state, data, data + length, mid1, buffer, buffer + length, mid2);
	*mid2 = L'\0';
	result = buffer;
	delete buffer;
  }

  inline void
  narrow (const std::wstring & source, std::string & result, const convertType & convert = std::use_facet<convertType> (std::locale ()))
  {
	int length1 = source.length ();
	int length2 = length1 * sizeof (wchar_t);  // Is this enough capacity?
	const wchar_t * data = source.c_str ();
	char * buffer = new char[length2 + 1];
	mbstate_t state;
	memset (&state, 0, sizeof (state));
	const wchar_t * mid1;
	char * mid2;
	convert.out (state, data, data + length1, mid1, buffer, buffer + length2, mid2);
	*mid2 = L'\0';
	result = buffer;
	delete buffer;
  }

  inline void
  widenCast (const std::string & in, std::wstring & result)
  {
	const char * i = in.c_str ();
	while (*i)
	{
	  result += (wchar_t) *i++;
	}
  }

  inline void
  narrowCast (const std::wstring & in, std::string & result)
  {
	const wchar_t * i = in.c_str ();
	while (*i)
	{
	  result += (char) *i++;
	}
  }
}

#endif
