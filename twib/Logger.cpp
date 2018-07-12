#include<stdio.h>
#include<time.h>
#include<forward_list>
#include<stdarg.h>
#include<unistd.h>

#include "Logger.hpp"
#include "ansi-colors.h"

namespace twili {
namespace log {

const size_t BUFFER_SIZE = 2048;

std::forward_list<std::shared_ptr<Logger>> logs;

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

// utility function
char *Logger::format(char *buf, int size, bool color, Level lvl, const char *fname, int line, const char *msg) {
  time_t rawtime;
  struct tm *timeinfo;
  
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  
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
             lvl, timebuf, color, fname, line, msg);
  } else {
    snprintf(buf, size, "[%d][%s] %s:%d: %s",
             lvl, timebuf, fname, line, msg);
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


void add_log(std::shared_ptr<Logger> l) {
  logs.push_front(l);
}

} // namespace log
} // namespace twili
