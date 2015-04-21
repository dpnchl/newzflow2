The goal is to develop a compact low-resource-consuming usenet downloader with support for:
  * NZB-Download
  * RSS-Feeds
  * NZB-Searchengine support (nzbmatrix.com, etc.)
  * TV-Show management

In a nutshell, [SABnzbd](http://www.sabnzbd.org) mixed with [Sickbeard](http://code.google.com/p/sickbeard/), but with [µTorrent](http://www.utorrent.com)'s GUI.

Unfortunately [Unzbin](http://www.unzbin.com/) didn't quite live up to my standards. It's coded in .NET and the GUI feels kind of sluggish.

External Programs/Libraries used (included):
  * [par2cmdline-tbb 0.4-20100203](http://chuchusoft.com/par2_tbb/index.html)
  * [unrar.dll 3.93](http://www.rarlabs.com/rar_add.htm)
  * [zlib 1.2.5](http://www.zlib.net/)
  * [SQLite 3.7.4](http://www.sqlite.org/)

Used for building:
  * Microsoft Visual Studio 2008 Standard
  * [Windows 7 SDK](http://www.microsoft.com/downloads/details.aspx?FamilyID=c17ba869-9671-4330-a63e-1fd44e0e2505&displaylang=en)
  * [WTL 8.1.9127](http://sourceforge.net/projects/wtl/)
  * [NSIS 2.46-Unicode](http://www.scratchpaper.com/)

Newzflow runs on Windows XP SP2+, Vista and Windows 7 and doesn't require any additional dependencies or run-time libraries.

Currently, newzflow is in active development and no releases have been made so far.