#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include <filesystem>
#include <iostream>
#include <strstream>
#include <cctype> // 确保包含此头文件以使用 std::isdigit


#include "s57_utils.h"

using namespace std;

const char *INDENT = "    ";
const int PATH_MAX = 255;

static std::string STR_DATA_DIR = "../../Data/";

string strToPrintable(string s)
{
    string res;

    for (int i = 0; i < (int)s.size(); ++i)
		if (!isprint(s[i])) {
			res.push_back('%');
			int n = (s[i] >> 4) & 0x0f;
			if (n < 10)
				res.push_back('0' + n);
			else
				res.push_back('A' + n - 10);
			n = s[i] & 0x0f;
			if (n < 10)
				res.push_back('0' + n);
			else
				res.push_back('A' + n - 10);

		}
        else 
            res.push_back(s[i]);

    return res;
}

string intToStr(long i)
{
    ostrstream os;
    os << i << ends;
    char *ss = os.str();
    string s(ss);
    delete[] ss;
    return s;
}

long strToInt(string s)
{
    long val = 0L;

    if (s.empty())
        return 0L;

    const char *p = s.c_str();
    char *endp;
    errno = 0;
    val = strtol(p, &endp, 10);
    if (errno != 0) {
        perror("strtol");
        val = 0L;
    }
    if (endp == p) {
        fprintf(stderr, "No digits found\n");
        val = 0L;
    }

    return val;
}

double strToFloat(string s)
{
    double val = 0.0;

    if (s.empty())
        return 0.0;

    const char *p = s.c_str();
    char *endp;
    errno = 0;
    val = strtod(p, &endp);
    if (errno != 0) {
        perror("strtol");
        val = 0.0;
    }
    if (endp == p) {
        fprintf(stderr, "No digits found\n");
        val = 0.0;
    }

    return val;
}


void setDataDir(const std::string& dir)
{
    STR_DATA_DIR = dir;
}

/*
StringList splitString(char sep, string str, bool allowEmptyEntry)
{
    StringList sl;
    string::size_type pos1, pos2 = 0;

    while ((pos1 = str.find(sep, pos2)) != string::npos) {
        if (pos1 > pos2 || allowEmptyEntry)
            sl.push_back(str.substr(pos2, pos1 - pos2));
        pos2 = pos1 + 1;
    }

    if (pos2 < str.length() || allowEmptyEntry)
        sl.push_back(str.substr(pos2));

    return sl;
}

StringList splitString(string sep, string str, bool allowEmptyEntry)
{
    StringList sl;
    string::size_type pos1, pos2 = 0;

    while ((pos1 = str.find(sep, pos2)) != string::npos) {
        if (pos1 > pos2 || allowEmptyEntry)
            sl.push_back(str.substr(pos2, pos1 - pos2));
        pos2 = pos1 + sep.length();
    }

    if (pos2 < str.length() || allowEmptyEntry)
        sl.push_back(str.substr(pos2));

    return sl;
}

void scanS57DataSetFiles(StringList &fl, const char *prefix, const char *dirName, bool recursive)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    assert(dirName != NULL);

    string cpath;
    if (prefix != NULL)
        cpath = prefix;
    cpath.append(dirName);
    if (cpath[cpath.length() - 1] != '/')
        cpath.push_back('/');

    if ((dp = opendir(dirName)) == NULL) {
        fprintf(stderr, "%s: %s\n", dirName, strerror(errno));
        return;
    }

    if (dirName != NULL)
        chdir(dirName);

    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(entry->d_name, ".") != 0
                    && strcmp(entry->d_name, "..") != 0
                    && recursive)
                scanS57DataSetFiles(fl, cpath.c_str(), entry->d_name, recursive);
        }
        else {
            string fn(entry->d_name);
            if (fn.length() == 12 
                    && fn[8] == '.' 
                    && isdigit(fn[9]) && isdigit(fn[10]) && isdigit(fn[11]))
                fl.push_back(cpath + fn);
        }
    }

    chdir("..");
    closedir(dp);
}
*/

std::string getAbsolutePath(const std::string& path)
{
    std::string res;

    if (!path.empty() && path[0] == '/') {
        res = path;
        if (res.back() != '/')
            res.push_back('/');
        return res;
    }

//    std::filesystem::path cwd = std::filesystem::current_path();
 //   std::filesystem::path abs = cwd / path;

    std::filesystem::path abs = std::filesystem::absolute(STR_DATA_DIR + path);

    if (std::filesystem::exists(abs) && std::filesystem::is_directory(abs)) {
        res = abs.string();
        res.push_back('/');
    }

    return res;
}

// S57DatasetScanner members

void S57DatasetScanner::doScan(const std::string& prefix, const std::string& dirName)
{
    std::string cpath = prefix + dirName;
    if (cpath.back() != '/')
        cpath.push_back('/');
    std::cout << "scanning " << cpath << std::endl;

    std::filesystem::path dirPath(dirName);
    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::cerr << dirName << ": " << strerror(errno) << std::endl;
        return;
    }

    std::filesystem::current_path(dirPath);

    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        const std::filesystem::path& filePath = entry.path();
        std::error_code ec;

        if (std::filesystem::is_directory(filePath, ec)) {
            if (filePath.filename() != "." && filePath.filename() != ".." && _recursive)
                doScan(cpath, filePath.filename().string());
        }
        else if (std::filesystem::is_regular_file(filePath, ec)) {
            std::string fileName = filePath.filename().string();
            if (fileName.length() == 12 && fileName[8] == '.' && std::isdigit(fileName[9]) && std::isdigit(fileName[10]) && std::isdigit(fileName[11])) {
                std::string absolutePath = std::filesystem::current_path().string() + "/" + fileName;
                onDataset(absolutePath);
            }
        }
        else {
            std::cerr << filePath << ": " << strerror(ec.value()) << std::endl;
        }
    }

    std::filesystem::current_path("..");
}

void S57DatasetScanner::scan(string path)
{
//    char cwd[256];
//
//    if (getcwd(cwd, 255) == NULL) {
//        fprintf(stderr, "%s: %s\n", cwd, strerror(errno));
//        exit(1);
//    }

    doScan("", path);

//    if (chdir(cwd) == -1) {
//        fprintf(stderr, "%s: %s\n", cwd, strerror(errno));
//        exit(1);
//    }
}
