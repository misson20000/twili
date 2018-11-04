//
// Twili - Homebrew debug monitor for the Nintendo Switch
// Copyright (C) 2018 misson20000 <xenotoad@xenotoad.net>
//
// This file is part of Twili.
//
// Twili is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Twili is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Twili.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include<memory>
#include<ostream>

#include "config.hpp"

namespace neolib
{
	template<class Elem, class Traits>
	inline void hex_dump(const void* aData, std::size_t aLength, std::basic_ostream<Elem, Traits>& aStream, std::size_t aWidth = 16)
	{
		const char* const start = static_cast<const char*>(aData);
		const char* const end = start + aLength;
		const char* line = start;
		while (line != end)
		{
			aStream.width(4);
			aStream.fill('0');
			aStream << std::hex << line - start << " : ";
			std::size_t lineLength = std::min(aWidth, static_cast<std::size_t>(end - line));
			for (std::size_t pass = 1; pass <= 2; ++pass)
			{	
				for (const char* next = line; next != end && next != line + aWidth; ++next)
				{
					char ch = *next;
					switch(pass)
					{
					case 1:
						aStream << (ch < 32 ? '.' : ch);
						break;
					case 2:
						if (next != line)
							aStream << " ";
						aStream.width(2);
						aStream.fill('0');
						aStream << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
						break;
					}
				}
				if (pass == 1 && lineLength != aWidth)
					aStream << std::string(aWidth - lineLength, ' ');
				aStream << " ";
			}
			aStream << std::endl;
			line = line + lineLength;
		}
	}
}


namespace twili {
namespace log {

enum class Level {
	Debug,
	Info,
	Message,
	Warning,
	Error,
	Fatal,
	Max
};

#define LogMessage(lvl, format, ...) \
	_log(::twili::log::Level::lvl, __FILE__, __LINE__,	\
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
	FileLogger(FILE *f, Level minlvl, Level maxlvl = Level::Max);
	virtual ~FileLogger();

	virtual void do_log(Level lvl, const char *fname, int line, const char *msg);
 protected:
	FILE *file;
	Level minlevel;
	Level maxlevel;
};

class PrettyFileLogger : public FileLogger {
 public:
	PrettyFileLogger(FILE *f, Level minlvl, Level maxlvl = Level::Max) : FileLogger(f, minlvl, maxlvl) {}

	virtual void do_log(Level lvl, const char *fname, int line, const char *msg);
};

#if WITH_SYSTEMD == 1
// formats log messages syslog-style
class SystemdLogger : public FileLogger {
 public:
	SystemdLogger(FILE *f, Level minlvl, Level maxlvl = Level::Max) : FileLogger(f, minlvl, maxlvl) {}

	virtual void do_log(Level lvl, const char *fname, int line, const char *msg);
};
#endif // WITH_SYSTEMD == 1

void _log(Level lvl, const char *fname, int line, const char *format, ...);
void add_log(std::shared_ptr<Logger> l);
void init_color();

} // namespace log
} // namespace twili
