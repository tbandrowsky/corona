
/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This is the application root class.  It can be used for both 
console and desktop applications.  It manages a threadpool
that is used pervasively by Corona.  It also provides the entry
to open and work with files asynchronously.

This is a singleton.  

Notes

For Future Consideration
This is a singleton.  having a public constructor is wrong.
*/

#pragma once
#include <lmcons.h>
#include <secext.h>
#include <string.h>
#include <KnownFolders.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
#include <shtypes.h>
#include <Windows.h>
#include <ciso646>
#include <exception>
#include <memory>
#include <string>

#include "corona-file.hpp"
#include "corona-queue.hpp"
#include "corona-string_box.hpp"
#include "corona-system-monitor-bus.hpp"



namespace corona
{

	class application
	{
	public:

		std::string application_name;
		std::string application_folder_name;

		application()
		{
			if (!global_job_queue) {
				global_job_queue = std::make_unique<job_queue>();
				global_job_queue->start(0);
			}
		}

		application(int _max_threads)
		{
			if (!global_job_queue) {
				global_job_queue = std::make_unique<job_queue>();
				global_job_queue->start(_max_threads);
			}
		}

		~application()
		{
			global_job_queue->shutDown();
		}

		std::string get_config_filename(std::string _filename)
		{
			char buff[MAX_PATH * 2];
			memset(buff, 0, sizeof(buff) / sizeof(char));
			::GetModuleFileNameA(nullptr, buff, sizeof(buff) / sizeof(char));
			::PathRemoveFileSpecA(buff);
			std::string temp = buff;
			temp += "\\";
			temp += _filename;
			return temp;
		}

		std::string get_data_filename(std::string _filename) const
		{
			char path_name[MAX_PATH * 4] = {};
			wchar_t* wide_path = nullptr;
			std::string result;
			::SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &wide_path);
			if (wide_path)
			{
				istring<2048> temp = wide_path;
				PathAppendA(temp.c_str_w(), application_folder_name.c_str());
				if (CreateDirectoryA(temp.c_str(), NULL) or ERROR_ALREADY_EXISTS == GetLastError()) {
					PathAppendA(temp.c_str_w(), _filename.c_str());
					result = temp;
				}
				else {
					// Failed to create directory
				}
			}
			return result;
		}

		void wait()
		{
			global_job_queue->waitForEmptyQueue();
		}

		std::string get_user_display_name()
		{
			std::string result;
			char buffer[UNLEN + 1] = {};
			DWORD max_length = sizeof(buffer) / sizeof(char);
			if (GetUserNameExA(NameDisplay, buffer, &max_length)) {
				result = buffer;
			}
			return result;
		}

		std::string get_user_account_name()
		{
			std::string result;
			char buffer[UNLEN + 1] = {};
			DWORD max_length = sizeof(buffer) / sizeof(char);
			if (GetUserNameExA(NameUserPrincipal, buffer, &max_length)) {
				result = buffer;
			}
			return result;
		}

		file open_file(std::string filename, file_open_types _file_open_type)
		{
			file f(filename, _file_open_type);
			return f;
		}

		bool file_exists(std::string filename)
		{
			DWORD       fileAttr;

			fileAttr = GetFileAttributesA(filename.c_str());
			if (0xFFFFFFFF == fileAttr and GetLastError() == ERROR_FILE_NOT_FOUND)
				return false;
			return true;
		}

		file open_file(KNOWNFOLDERID folderId, std::string filename, file_open_types _file_open_type)
		{
			file f(folderId, filename, _file_open_type);
			return f;
		}

		file create_file(KNOWNFOLDERID folderId, std::string filename)
		{
			return file(folderId, filename, file_open_types::create_always);
		}

		file create_file(std::string filename)
		{
			return file(filename, file_open_types::create_always);
		}

		std::shared_ptr<file> open_file_ptr(KNOWNFOLDERID folderId, std::string filename, file_open_types _file_open_type)
		{
			std::shared_ptr<file> f = std::make_shared<file>(folderId, filename, _file_open_type);
			return f;
		}

		std::shared_ptr<file> open_file_ptr(std::string filename, file_open_types _file_open_type)
		{
			std::shared_ptr<file> f = std::make_shared<file>(filename, _file_open_type);
			return f;
		}

		std::shared_ptr<file> create_file_ptr(KNOWNFOLDERID folderId, std::string filename)
		{
			std::shared_ptr<file> f = std::make_shared<file>(folderId, filename, file_open_types::create_always);
			return f;
		}

		std::shared_ptr<file> create_file_ptr(std::string filename)
		{
			std::shared_ptr<file> f = std::make_shared<file>(filename, file_open_types::create_always);
			return f;
		}

		void submit_job(job* _job)
		{
			global_job_queue->submit_job(_job);
		}

		virtual void log(const char* _msg)
		{
			system_monitoring_interface::active_mon->log_information(_msg);
		}

		bool is_admin() {
			BOOL isAdmin = FALSE;
			HANDLE tokenHandle = nullptr;

			// Open the current process token
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
				TOKEN_ELEVATION elevation = {};
				DWORD size;

				// Query the token for elevation information
				if (GetTokenInformation(tokenHandle, TokenElevation, &elevation, sizeof(elevation), &size)) {
					isAdmin = elevation.TokenIsElevated;
				}
				CloseHandle(tokenHandle);
			}

			return isAdmin;
		}

	};

	bool application_tests()
	{
		try {
			application app;

			auto my_file = app.create_file("file.txt");

			my_file.add(32);

			char buffer[10] = { 0 };

			strcpy_s(buffer, "test1");
			my_file.write( 0, buffer, 5);
			strcpy_s(buffer, "test2");
			my_file.write( 5, buffer, 5);
			my_file.read( 0, buffer, 5);

			if (strcmp(buffer, "test1") != 0)
			{
				system_monitoring_interface::active_mon->log_warning("read / write test failed");
			}

			return true;
		}
		catch (std::exception exc)
		{
			system_monitoring_interface::active_mon->log_exception(exc, __FILE__, __LINE__);
			return false;
		}
	}
}
