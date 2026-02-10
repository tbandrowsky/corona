
#ifndef APPLICATION_ENTREPRENEUR_HPP
#define APPLICATION_ENTREPRENEUR_HPP

#include "resource.h"
#include "corona.hpp"

namespace corona
{
	void run_entrepreneur_application(HINSTANCE hInstance, LPSTR  lpszCmdParam);
	
	std::string config_filename = "config.json";

	/*

	This is a boilerplate entry point for a corona based Windows client application.
	It creates an application object, and illustrates how all the pre-requisites are tied together.

	*/

	/*

	These are general constants, and they are used to identify specific controls.
	The presentation system lets you use win32 controls and your own dx based controls interchangably.

	*/

	// important general
	const int IDC_COMPANY_NAME = 1001;
	const int IDC_COMPANY_LOGO = 1002;
	const int IDC_TITLE_BAR = 1003;
	const int IDC_SYSTEM_MENU = 1004;

	// menu options
	const int IDM_VIEW_MENU = 3001;
	const int IDM_HOME = 3002;
	const int IDM_LOGIN = 3003;

	// forms
	const int IDC_FORM_VIEW = 4001;

	// bits of status
	const int IDC_STATUS_MESSAGE = 5001;
	const int IDC_STATUS_DETAIL = 5002;

	/* And now, this is the application */

	void run_entrepreneur_application(HINSTANCE hInstance, LPSTR lpszCmdParam)
	{

		std::shared_ptr<corona::comm_app_bus> service;

		char dir[500];
		GetModuleFileNameA(NULL, dir, 500);

        std::string config_path = std::filesystem::path(dir).parent_path().string();

		config_path += "\\configuration\\";

		std::string database_path = config_path + "database";

        std::cout << "Config path: " << config_path << std::endl;

			// database path
		PWSTR userFolderPath = nullptr;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &userFolderPath);

		if (result == S_OK) {
			istring<4096> dataPath(userFolderPath);

			std::filesystem::path application_path = "entrepreneur";
			std::filesystem::path database_path_path = dataPath.c_str();
			database_path_path /= application_path;
			database_path = database_path_path.string();
			CoTaskMemFree(userFolderPath); // Free memory allocated by SHGetKnownFolderPath
		}
		else 
		{
			log_warning("Could not get app_data folder path");
			return;
		}

		std::cout << "Database path: " << database_path << std::endl;

		CreateDirectoryA(database_path.c_str(), NULL);

		std::string config_full_file = config_path + config_filename;

		std::string config_contents = corona::read_all_string(config_full_file);

		corona::json_parser jp;
		corona::json config = jp.parse_object(config_contents);

		bool show_console = config["Settings"]["show_console"].as_bool();

		if (show_console) {
			EnableGuiStdOuts();
		}

		corona::json servers = config["Servers"];
	
		if (servers.array()) {
			if (servers.size() > 0) {
				auto server = servers.get_element(0);

				service = std::make_shared<corona::comm_app_bus>(
					config_path,
					database_path,
					config,
					server,
					true);

				service->run_app_ui(hInstance, lpszCmdParam, false);
			}
		}

		if (show_console) {
			DisableGuiStdOuts();
		}
	}

}

#endif
