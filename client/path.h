/*
   Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/
extern "C"
{
#include <dirent.h>
#include <pwd.h>
#include "my_dir.h"
}
#include <string>

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_C '/'
#define MAX_PATH_LENGTH 512

/**
  A helper class for handling file paths. The class can handle the memory
  on its own or it can wrap an external string.
  @note This is a rather trivial wrapper which doesn't handle malformed paths
  or filenames very well.
  @see unittest/gunit/path-t.cc

*/
class Path
{
public:
  Path() : m_path(""), m_filename("") {}

  Path(const std::string &s) : m_filename("") { path(s); }

  Path(const Path &p) { m_filename= p.m_filename; m_path= p.m_path; }

  bool getcwd(void)
  {
    char path[MAX_PATH_LENGTH];
    if (::getcwd(path, MAX_PATH_LENGTH) == 0)
       return false;
    m_path.clear();
    m_path.append(path);
    trim();
    return true;
  }

  bool validate_filename()
  {
    size_t idx= m_filename.find(PATH_SEPARATOR);
    if (idx != std::string::npos)
      return false;
    return true;
  }

  void trim()
  {
    if (m_path.length() <= 1)
      return;
    std::string::iterator it= m_path.end();
    --it;

    while((*it) == PATH_SEPARATOR_C)
    {
      m_path.erase(it--);
    }
  }

  void parent_directory(Path *out)
  {
    size_t idx= m_path.rfind(PATH_SEPARATOR);
    if (idx == std::string::npos)
    {
      out->path("");
    }
    out->path(m_path.substr(0, idx));
  }

  Path &up()
  {
    size_t idx= m_path.rfind(PATH_SEPARATOR);
    if (idx == std::string::npos)
    {
      m_path.clear();
    }
    m_path.assign(m_path.substr(0, idx));
    return *this;
  }

  Path &append(const std::string &path)
  {
    if (m_path.length() > 1 && path[0] != PATH_SEPARATOR_C)
      m_path.append(PATH_SEPARATOR);
    m_path.append(path);
    trim();
    return *this;
  }

  Path &filename_append(const std::string &ext)
  {
    m_filename.append(ext);
    trim();
    return *this;
  }

  void path(const std::string &p)
  {
    m_path.clear();
    m_path.append(p);
    trim();
  }

  void filename(const std::string &f)
  {
    m_filename= f;
  }

  void path(const Path &p) { path(p.m_path); }

  void filename(const Path &p) { path(p.m_filename); }

  bool qpath(const std::string &qp)
  {
    MY_STAT s;
    if (my_stat(qp.c_str(), &s, MYF(0)) == NULL)
      return false;
    if (!S_ISREG(s.st_mode))
      return false;
    size_t idx= qp.rfind(PATH_SEPARATOR);
    if (idx == std::string::npos)
    {
      m_filename= qp;
      m_path.clear();
    }
    else
    {
      filename(qp.substr(idx + 1, qp.size() - idx));
      path(qp.substr(0, idx));
    }
    return true;
  }

  bool is_qualified_path()
  {
    return m_filename.length() > 0;
  }

  bool exists()
  {
    if (!is_qualified_path())
    {
      DIR *dir= opendir(m_path.c_str());
      if (dir == 0)
        return false;
      return true;
    }
    else
    {
      MY_STAT s;
      std::string qpath(m_path);
      qpath.append(PATH_SEPARATOR).append(m_filename);
      if (my_stat(qpath.c_str(), &s, MYF(0)) == NULL)
        return false;
      return true;
    }
  }

  const std::string to_str()
  {
    std::string qpath(m_path);
    if (m_filename.length() != 0)
    {
      qpath.append(PATH_SEPARATOR);
      qpath.append(m_filename);
    }
    return qpath;
  }

  bool empty()
  {
    if (!exists())
      return true;
    DIR *dir;
    struct dirent *ent;
    bool ret= false;
    if ((dir= opendir(m_path.c_str())) == NULL)
      ret= false;
    else
    {
      int c= 0;
      ent= readdir(dir);
      while (ent != NULL && c < 4)
      {
        //std::cout << "[DEBUG] c= " << c << " "
        //     << ent->d_name << std::endl;
        ent= readdir(dir);
        ++c;
      }
      if (c == 2) // Don't count . and ..
       ret= true;
    }
    closedir(dir);
    return ret;
  }
  
  void get_homedir()
  {
    struct passwd *pwd;
    pwd= getpwuid(geteuid());
    if (pwd == NULL)
      return;
    if (pwd->pw_dir != 0)
      path(pwd->pw_dir);
    else
      path("");
  }

  friend std::ostream &operator<<(std::ostream &op, const Path &p);
private:
  std::string m_path;
  std::string m_filename;
};

std::ostream &operator<<(std::ostream &op, const Path &p)
{
  std::string qpath(p.m_path);
  if (p.m_filename.length() != 0)
  {
    qpath.append(PATH_SEPARATOR);
    qpath.append(p.m_filename);
  }
  return op << qpath;
}
