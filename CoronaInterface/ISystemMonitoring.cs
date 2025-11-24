using System;

namespace CoronaInterface
{
    public interface ISystemMonitoring
    {
        void LogUserCommandStart(string commandName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogUserCommandStop(string commandName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogCommandStart(string commandName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogCommandStop(string commandName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogJobStart(string apiName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogJobStop(string apiName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogJobSectionStart(string apiName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogJobSectionStop(string apiName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogFunctionStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogFunctionStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogBaseBlockStart(int indent, string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogBaseBlockStop(int indent, string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogTableStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogTableStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogJsonStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogJsonStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogPocoStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogPocoStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogBlockStart(string functionName, string message, DateTime requestTime, string file = null, int line = 0);
        void LogBlockStop(string functionName, string message, double elapsedSeconds, string file = null, int line = 0);

        void LogInformation(string message, string file = null, int line = 0);

        void LogActivity(string message, DateTime time, string file = null, int line = 0);
        void LogActivity(string message, double elapsedSeconds, string file = null, int line = 0);

        void LogPut(string message, double elapsedSeconds, string file = null, int line = 0);
        void LogAdapter(string message);

        void LogWarning(string message, string file = null, int line = 0);
        void LogException(string message, string file = null, int line = 0);

        void LogJson<T>(T src, int indent = 2);
    }
}