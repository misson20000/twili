#pragma once

#include<memory>

namespace twili {
namespace log {

enum class Level {
	DEBUG,
	INFO,
	MSG,
	WARN,
	ERROR,
	FATAL,
	MAX
};

#define log(lvl, format, ...) \
	_log(::twili::log::Level::lvl, __FILE_SHORT__, __LINE__,	\
			 format, ##__VA_ARGS__);

class Logger {
 public:
	virtual ~Logger();
	virtual void do_log(Level lvl, const char *fname, int line, const char *msg) = 0;
 protected:
	char *format(char *buf, int size, bool use_color, Level lvl, const char *fname, int line, const char *msg);
};

class FileLogger : public Logger {
 public:
	FileLogger(FILE *f, Level minlvl, Level maxlvl = Level::MAX);
	virtual ~FileLogger();

	virtual void do_log(Level lvl, const char *fname, int line, const char *msg);
 protected:
	FILE *file;
	Level minlevel;
	Level maxlevel;
};

class PrettyFileLogger : public FileLogger {
 public:
	PrettyFileLogger(FILE *f, Level minlvl, Level maxlvl = Level::MAX) : FileLogger(f, minlvl, maxlvl) {}

	virtual void do_log(Level lvl, const char *fname, int line, const char *msg);
};

void _log(Level lvl, const char *fname, int line, const char *format, ...);
void add_log(std::shared_ptr<Logger> l);

} // namespace log
} // namespace twili
