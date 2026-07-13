#ifndef N2T_WIDGET_LOG_HPP
#define N2T_WIDGET_LOG_HPP

#include "imgui.h"
#include <mutex>
#include <string>
#include <vector>

enum class LogType {
   Error,
   Success,
};

struct LogMessage {
   LogType type;
   std::string msg;
};

namespace gui::widget {
class Log {
   std::vector<LogMessage> _logs;
   std::mutex _logs_mutex;
   ImFont *_monofont;

   public:
   Log(ImFont *monofont);
   void show(int default_height = 0);
   void push(LogType type, const char *msg);
};
}

#endif
