#ifndef INCLUDE_DIRUTIL_H_
#define INCLUDE_DIRUTIL_H_

// dirutil.h
// Revision 3-oct-2004

#include <string>
#include <vector>

namespace dirutil {

bool is_directory (const std::string & str);

std::string base_name (const std::string & str);
std::string dir_name (const std::string & str);
std::string extension (const std::string & str);

std::string enter_dir (const std::string & old, const std::string & enter);

void directory (const std::string & dirname, std::vector <std::string> & v);

void delete_file (const std::string & filename);

std::string create_temp_file ();

void copy_remote_file (const std::string & source, const std::string & dest);

} // namespace dirutil

#endif

// End of dirutil.h
