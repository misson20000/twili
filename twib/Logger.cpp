#include<stdio.h>
#include<time.h>
#include<forward_list>
#include<stdarg.h>
#include<string.h>

#ifdef _WIN32
#include<Windows.h>
#endif

#ifdef _MSC_VER
#include<io.h>
#else
#include<unistd.h>
#endif

#include "config.hpp"
#if WITH_SYSTEMD == 1
#include<systemd/sd-daemon.h>
#endif

#include "Logger.hpp"
#include "ansi-colors.h"

namespace twili {
namespace log {

const size_t BUFFER_SIZE = 2048;

std::forward_list<std::shared_ptr<Logger>> logs;

void init_color() {
#ifdef _WIN32
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hOut == INVALID_HANDLE_VALUE) {
		exit(GetLastError());
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) {
		exit(GetLastError());
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode)) {
		exit(GetLastError());
	}
#endif
}

void _log(Level lvl,
          const char *fname,
          int line,
          const char *format, ...) {
          
  va_list ar;
  
  va_start(ar, format);
  char buf[BUFFER_SIZE];
  vsnprintf(buf, BUFFER_SIZE, format, ar);

  for(auto i = logs.begin(); i != logs.end(); i++) {
    (*i)->do_log(lvl, fname, line, buf);
  }
  
  va_end(ar);
}

static const char *trim_filename(const char *fname) {
	if(strncmp(fname, PROJECT_ROOT, sizeof(PROJECT_ROOT)-1) == 0) {
		fname+= sizeof(PROJECT_ROOT);
	}
	return fname;
}

// utility function
char *Logger::format(char *buf, int size, bool color, Level lvl, const char *fname, int line, const char *msg) {
  time_t rawtime;
  struct tm timeinfo_storage;
  struct tm *timeinfo;
  
  time(&rawtime);
#ifdef _WIN32
  localtime_s(&timeinfo_storage, &rawtime);
  timeinfo = &timeinfo_storage;
#else
  timeinfo = localtime_r(&rawtime, &timeinfo_storage);
#endif
  
  char timebuf[64];
  strftime(timebuf, 64,
           "%Y-%m-%d %R",
           timeinfo);

  if(color) {
    const char *color = "";
    
    switch(lvl) {
    case Level::Debug: color = ANSI_BOLD ANSI_COLOR_BLACK; break;  
    case Level::Info:  color = ANSI_COLOR_WHITE; break;
    case Level::Message:   color = ANSI_BOLD ANSI_COLOR_WHITE; break;
    case Level::Warning:  color = ANSI_COLOR_BLUE; break;
    case Level::Error: color = ANSI_BOLD ANSI_COLOR_YELLOW; break;
    case Level::Max:
    case Level::Fatal: color = ANSI_BOLD ANSI_COLOR_RED; break;
    }
    
    snprintf(buf, size, ANSI_RESET "[%d][%s]%s %s:%d: %s" ANSI_RESET,
             lvl, timebuf, color, trim_filename(fname), line, msg);
  } else {
    snprintf(buf, size, "[%d][%s] %s:%d: %s",
             lvl, timebuf, trim_filename(fname), line, msg);
  }
  return buf;
}

Logger::~Logger() {
}

FileLogger::FileLogger(FILE *fp, Level minlvl, Level maxlvl) {
  this->file = fp;
  this->minlevel = minlvl;
  this->maxlevel = maxlvl;
}

FileLogger::~FileLogger() {
  fclose(this->file);
}

void FileLogger::do_log(Level lvl, const char *fname, int line, const char *msg) {
  if(lvl >= this->minlevel && lvl < this->maxlevel) {
    char buf[BUFFER_SIZE + 256];
    fputs(format(buf, sizeof(buf), false, lvl, fname, line, msg), this->file);
    fputc('\n', this->file);
  }
}

void PrettyFileLogger::do_log(Level lvl, const char *fname, int line, const char *msg) {
  if(lvl >= this->minlevel && lvl < this->maxlevel) {
    char buf[BUFFER_SIZE + 256];
    fputs(format(buf, sizeof(buf), isatty(fileno(this->file)), lvl, fname, line, msg), this->file);
    fputc('\n', this->file);
  }
}

#if WITH_SYSTEMD == 1
void SystemdLogger::do_log(Level lvl, const char *fname, int line, const char *msg) {
	const char *lvl_str;
	switch(lvl) {
	case Level::Debug:   lvl_str = SD_DEBUG; break;
	case Level::Info:    lvl_str = SD_INFO; break;
	case Level::Message: lvl_str = SD_NOTICE; break;
	case Level::Warning: lvl_str = SD_WARNING; break;
	case Level::Error:   lvl_str = SD_ERR; break;
	case Level::Fatal:   lvl_str = SD_CRIT; break;
	default:
		lvl_str = SD_ERR;
	}
	fprintf(file, "%s%s:%d: %s\n", lvl_str, trim_filename(fname), line, msg);
}
#endif // WITH_SYSTEMD == 1

void add_log(std::shared_ptr<Logger> l) {
  logs.push_front(l);
}

} // namespace log
} // namespace twili
