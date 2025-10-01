#ifndef LOGGER_H
#define LOGGER_H

#include<string>

namespace logger {
	void Logs(const std::string& message);

	void Cleanup(); 
}


#endif // !LOGGER_H
