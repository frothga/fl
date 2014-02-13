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


#define __STDC_FORMAT_MACROS

#include "fl/server.h"
#include "fl/string.h"
#include "fl/time.h"

#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <inttypes.h>

#ifndef WIN32
# include <dirent.h>
#endif


using namespace fl;
using namespace std;


// class ResponderTree -------------------------------------------------------

ResponderTree::ResponderTree (const wstring & name, bool caseSensitive)
: name          (name),
  caseSensitive (caseSensitive)
{
  if (! caseSensitive)
  {
	for (int i = 0; i < this->name.length (); i++)
	{
	  this->name[i] = towlower (this->name[i]);
	}
  }

  // All URLs evaluated by a Responder are absolute, so ensure that name starts with "/".
  if (this->name[0] != L'/') this->name.insert (0, L"/");
}

bool
ResponderTree::respond (Request & request, Response & response)
{
  wstring URL = L"";  // Won't match any ResponderName, because they always start with "/"
  request.getCGI (L"URL", URL);
  return respond (URL, request, response);
}

		
// class ResponderName -------------------------------------------------------

ResponderName::ResponderName (const wstring & name, bool caseSensitive)
: ResponderTree (name, caseSensitive)
{
}

bool
ResponderName::respond (const wstring & path, Request & request, Response & response)
{
  if (match (path))
  {
	generate (request, response);
	return true;
  }
  else
  {
	return false;
  }
}

bool
ResponderName::match (const wstring & path)
{
  if (caseSensitive) return regexpMatch (name, path);

  wstring buffer = path;
  for (int i = 0; i < buffer.length (); i++)
  {
	buffer[i] = towlower (buffer[i]);
  }
  return regexpMatch (name, buffer);
}

void
ResponderName::generate (Request & request, Response & response)
{
  response.error (501);
}


// class ResponderFile -------------------------------------------------------

ResponderFile::MIMEtype ResponderFile::MIMEtypes[] =
{
  {L".jpg",  "image/jpeg"},
  {L".htm",  "text/html"},
  {L".html", "text/html"},
  {L".css",  "text/css"},
  {L"*",     "text/*"},
  {L"",      ""}  // Indicates end of list
};

ResponderFile::ResponderFile (const wstring & name, const wstring & root, bool caseSensitive)
: ResponderTree (name, caseSensitive),
  root (root)
{
}

static inline FILE *
wfopen (const wstring & fileName)
{
  string nfn;
  narrowCast (fileName, nfn);
  return fopen (nfn.c_str (), "rb");
}

static inline int
wstat (const wstring & fileName, struct stat * buf)
{
  string nfn;
  narrowCast (fileName, nfn);
  return stat (nfn.c_str (), buf);
}

bool
ResponderFile::respond (const wstring & path, Request & request, Response & response)
{
  int length = name.length ();
  if (caseSensitive ? wcsncmp     (name.c_str (), path.c_str (), length) != 0
                    : wcsncasecmp (name.c_str (), path.c_str (), length) != 0)
  {
	return false;
  }

  wstring subdir = path.substr (length);
  wstring fileName = root + subdir;

  wstring name;
  wstring suffix;
  int position = fileName.find_last_of (L'/');
  if (position == wstring::npos)
  {
	name = fileName;
  }
  else
  {
	name = fileName.substr (position + 1);
  }
  position = name.find_last_of (L'.');
  if (position != wstring::npos)
  {
	suffix = name.substr (position);
  }

  struct stat filestats;
  wstat (fileName, &filestats);
  bool isDirectory = filestats.st_mode & S_IFDIR;

  wstring dirName;
  if (isDirectory)
  {
	// Attempt to open the "default" file for the directory.
	// If that fails, we will fall back to delivering a directory listing.
	dirName = fileName;
	if (fileName[fileName.size () - 1] != L'/') fileName += L'/';
	fileName += L"default";
	suffix.clear ();
  }

  FILE * file = wfopen (fileName);
  if (! file  &&  suffix.length () == 0)
  {
	fileName += L".htm";
	suffix = L".htm";
	file = wfopen (fileName);
  }
  if (! file  &&  suffix == L".htm")
  {
	fileName += L"l";
	suffix += L"l";
	file = wfopen (fileName);
  }

  if (file)
  {
	// Determine file size
	if (fstat (fileno (file), &filestats))
	{
	  uint64_t filelength = filestats.st_size;
	  char temp[20];
	  sprintf (temp, "%"PRIu64, filelength);
	  response.addHeader ("Content-Length", temp);
	}

	// Map a few basic file types to MIME types
	//   Prepare for case insensitive compare
	for (int i = 0; i < suffix.length (); i++)
	{
	  suffix[i] = towlower (suffix[i]);
	}
	//   Scan for MIME type
	MIMEtype * mt = MIMEtypes;
	while (mt->suffix[0])
	{
	  if (regexpMatch (mt->suffix, suffix))
	  {
		response.addHeader ("Content-Type", mt->type);
		break;
	  }
	  mt++;
	}

	// Pump out file
	do
	{
	  char buffer[4096];
	  int count = fread (buffer, sizeof (char), sizeof (buffer), file);
	  if (count > 0)
	  {
		response.raw (buffer, count);
	  }
	}
	while (! feof (file)  &&  ! ferror (file));
	if (ferror (file))
	{
	  request.disconnect ();
	}

	fclose (file);
  }
  else if (isDirectory)
  {
	generateDirectoryListing (path, dirName, request, response);
  }
  else
  {
	response.error (404, fileName);
  }

  return true;
}

void
ResponderFile::generateDirectoryListing (const wstring & path, const wstring & dirName, Request & request, Response & response)
{
  response << "<html>";
  response << "<head>";
  response << "<style>";
  response << "table {border-collapse: collapse;}";
  response << "table, th, td {border: 1px solid black;}";
  response << "td {border-style: none solid;}";
  response << "</style>";
  response << "<title>" << path << "</title>";
  response << "</head>";
  response << "<body>";

  // Clickable bread-crumb trail back to root
  response << "<h1>";
  response << "<a href=\"/\">/</a>"; // click to get root directory
  wstring tempPath = path.substr (1);  // get rid of leading /
  int last = tempPath.size () - 1;
  if (tempPath[last] == L'/') tempPath.erase (last, 1);  // get rid of trailing /
  wstring trail;
  wstring first;
  split (tempPath, L"/", first, tempPath);
  while (first.size ())
  {
	trail = trail + L"/" + first;
	response << " <a href=\"" << trail << "/\">" << first << "</a> /";
	split (tempPath, L"/", first, tempPath);
  }
  response << "</h1>";

  // Clickable listing of files

  //   Table header
  wstring sort = L"name";
  request.getQuery (L"sort", sort);
  int sortBy = 0;
  if      (sort == L"name") sortBy = 0;
  else if (sort == L"size") sortBy = 1;
  else if (sort == L"time") sortBy = 2;

  wstring order; // default is blank
  request.getQuery (L"order", order);
  wstring nextOrder = (order == L"up") ? L"down" : L"up";
  wstring nameOrder = (sortBy == 0) ? nextOrder : L"up";
  wstring sizeOrder = (sortBy == 1) ? nextOrder : L"up";
  wstring timeOrder = (sortBy == 2) ? nextOrder : L"down";

  response << "<table>";
  response << "<tr>";
  response << "<th><a href=\"" << path << "?sort=name&order=" << nameOrder << "\">Name</a></th>";
  response << "<th><a href=\"" << path << "?sort=size&order=" << sizeOrder << "\">Size</a></th>";
  response << "<th><a href=\"" << path << "?sort=time&order=" << timeOrder << "\">Time</a></th>";
  response << "</tr>";

  //   Enumerate files in directory, sorted by current column
  multimap<string, DirEntry> sorted;
  scan (dirName, sortBy, sorted);
  wstring pathWithSlash = path;
  last = pathWithSlash.size () - 1;
  if (pathWithSlash[last] != L'/') pathWithSlash += L'/';
  if (order == L"down")
  {
	multimap<string, DirEntry>::reverse_iterator it;
	for (it = sorted.rbegin (); it != sorted.rend (); it++) write (it->second, pathWithSlash, response);
  }
  else
  {
	multimap<string, DirEntry>::iterator it;
	for (it = sorted.begin (); it != sorted.end (); it++) write (it->second, pathWithSlash, response);
  }

  response << "</table>";
  response << "</body>";
  response << "</html>";
}

void
ResponderFile::scan (const wstring & dirName, int sortBy, multimap<string, DirEntry> & result)
{
  string ndirName;
  narrowCast (dirName, ndirName);
  int last = ndirName.size () - 1;
  if (ndirName[last] == '/') ndirName.erase (last, 1);

# ifdef WIN32

  WIN32_FIND_DATA fd;
  HANDLE hFind = ::FindFirstFile ((ndirName + "/*").c_str (), &fd);
  if (hFind == INVALID_HANDLE_VALUE) return;

  do
  {
	DirEntry entry;
	entry.name = fd.cFileName;
	if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	  entry.name += "/";
	  entry.size = 0;
	}
	else
	{
	  entry.size = (uint64_t) fd.nFileSizeHigh * (MAXDWORD+1) + fd.nFileSizeLow;
	}
	uint64_t time = (uint64_t) fd.ftCreationTime.dwHighDateTime << 32 + fd.ftCreationTime.dwLowDateTime;  // a FILETIME structure should not be cast directly to a 64-bit integer
	entry.time = (int64_t) (time / 10000000) - 116444736000000000;

	string key;
	char buffer[30];
	switch (sortBy)
	{
	  case 1:
		sprintf (buffer, "%020"PRIu64, entry.size);
		key = buffer;
		break;
	  case 2:
		sprintf (buffer, "%i", entry.time);
		key = buffer;
		break;
	  default:
		key = entry.name;
	}
	result.insert (make_pair (key, entry));
  }
  while (::FindNextFile (hFind, &fd));

  ::FindClose (hFind);

# else

  DIR * dirh = opendir (ndirName.c_str ());
  if (! dirh) return;

  while (struct dirent * dirp = readdir (dirh))
  {
	if (strcmp (dirp->d_name, ".") == 0  ||  strcmp (dirp->d_name, "..") == 0) continue;

	struct stat filestats;
	stat ((ndirName + "/" + dirp->d_name).c_str (), &filestats);

	DirEntry entry;
	entry.name = dirp->d_name;
	entry.size = filestats.st_size;
	entry.time = filestats.st_ctime;
	if (filestats.st_mode & S_IFDIR) entry.name += "/";

	string key;
	char buffer[30];
	switch (sortBy)
	{
	  case 1:
		sprintf (buffer, "%020"PRIu64, entry.size);
		key = buffer;
		break;
	  case 2:
		sprintf (buffer, "%i", entry.time);
		key = buffer;
		break;
	  default:
		key = entry.name;
	}
	result.insert (make_pair (key, entry));
  }

  closedir (dirh);

# endif
}

void
ResponderFile::write (const DirEntry & entry, const wstring & pathWithSlash, Response & response)
{
  response << "<tr>";
  wstring encodedPath = pathWithSlash;
  widenCast (entry.name, encodedPath);
  response.encodeURL (encodedPath);
  response << "<td style=\"text-align:left\"><a href=\"" << encodedPath << "\">" << entry.name << "</a></td>";
  response << "<td style=\"text-align:right\">" << entry.size << "</td>";
  struct tm t;
  localtime_r (&entry.time, &t);
  char buffer[32];
  strftime (buffer, sizeof (buffer), "%x %X %Z", &t);
  response << "<td style=\"text-align:left\">" << buffer << "</td>";
  response << "</tr>";
}


// class ResponderDirectory --------------------------------------------------

ResponderDirectory::ResponderDirectory (const wstring & name, bool caseSensitive)
: ResponderTree (name, caseSensitive)
{
}

bool
ResponderDirectory::respond (const wstring & path, Request & request, Response & response)
{
  int length = name.length ();
  if (caseSensitive ? wcsncmp     (name.c_str (), path.c_str (), length) != 0
                    : wcsncasecmp (name.c_str (), path.c_str (), length) != 0)
  {
	return false;
  }

  wstring subdir = path.substr (length);

  if (subdir.length () == 0)
  {
	// This directory is being requested as if it were a regular object, so redirect
	// client to the correct form of the path.  This action is necessary for the client
	// (ie: Netscape or IE) to understand that it should change its base URL.

	Header * host = request.getHeader ("Host");  // Guaranteed to exist
	string location = "http://";
	location += host->values.front ();
	narrowCast (path, location);
	location += "/";
	response.addHeader ("Location", location);

	wstring explanation = L"The object you requested is actually a directory. Please use the following URL instead: ";
	explanation += L"<A HREF=\"";
	widenCast (location, explanation);
	explanation += L"\">";
	widenCast (location, explanation);
	explanation += L"</A>";
	response.error (302, explanation);

	return true;
  }

  if (subdir[0] == L'/')
  {
	// This directory has been correctly requested.

	bool found = false;
	vector<ResponderTree *>::iterator i;
	for (i = responders.begin (); i < responders.end ()  &&  ! found; i++)
	{
	  found = (*i)->respond (subdir, request, response);
	}

	if (! found)
	{
	  response.error (404);
	}

	return true;
  }

  // The name of the object contained this directory's name, but it was a false lead.
  return false;
}
