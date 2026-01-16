
#ifndef APPLICATION_POLITICSDX_HPP
#define APPLICATION_POLITICSDX_HPP

#include "resource.h"
#include "corona.hpp"

namespace corona
{
	void run_politicsdx_application(HINSTANCE hInstance, LPSTR  lpszCmdParam);
	
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

	void run_politicsdx_application(HINSTANCE hInstance, LPSTR lpszCmdParam)
	{

		std::shared_ptr<corona::comm_app_bus> service;

		// config path
		std::filesystem::path config_path;
		std::filesystem::current_path(config_path);

		// database path
		PWSTR userFolderPath = nullptr;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &userFolderPath);
		std::string database_path;

		if (result == S_OK) {
			istring<4096> dataPath(userFolderPath);

			std::filesystem::path application_path = "politicsdx";
			std::filesystem::path database_path = dataPath.c_str();
			database_path /= application_path;
			CoTaskMemFree(userFolderPath); // Free memory allocated by SHGetKnownFolderPath
		}
		else {
			log_warning("Could not get app_data folder path", __FILE__, __LINE__);
			return;
		}

		EnableGuiStdOuts();

		std::string config_contents = corona::read_all_string(config_filename);

		corona::json_parser jp;
		corona::json config = jp.parse_object(config_contents);

		corona::json servers = config["Servers"];

		if (servers.array()) {
			if (servers.size() > 0) {
				auto server = servers.get_element(0);
				std::string listen_point = server["listen_point"].as_string();
				std::string server_name = server["application_name"].as_string();
				std::cout << "launching " << server_name << " on " << listen_point << std::endl;

				service = std::make_shared<corona::comm_app_bus>(
					config_path,
					database_path,
					config,
					server,
					true);

				service->run_app_ui(hInstance, lpszCmdParam, false);
			}
		}

		DisableGuiStdOuts();
	}

}

#endif
