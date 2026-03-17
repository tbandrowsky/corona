
#ifndef APPLICATION_MY_APP_HPP
#define APPLICATION_MY_APP_HPP

#include "resource.h"
#include "corona.hpp"

std::string my_application_name = "MyApp";

namespace corona
{


	void my_app(HINSTANCE hInstance, LPSTR  lpszCmdParam);

	/****************************
	KEY BIT TO FLIP, RIGHT HERE

	if use_project_for_config is true, then, this C++ project will use the configuration folder
	in the project directory to get things like the pages and the schema.
	This means you can use Visual Studio or any other favorite editor - Code is good too,
	to edit both the database schema and the pages for it. 

	if use_project_for_config is false, then, it will use the configuration folder underneath where the EXE
	lives, which is good, for if it is really deployed.

	don't forget to set the default directory
	*****************************/

    const bool use_project_for_config = true;

	std::string config_filename = "app_config.json";

	/*

	This is a boilerplate entry point for a corona based Windows client application.
	It creates an application object, and illustrates how all the pre-requisites are tied together.

	*/

	/*

	These are general constants, and they are used to identify specific controls.
	The presentation system lets you use win32 controls and your own dx based controls interchangably.

	this is old.
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

	void my_app(HINSTANCE hInstance, LPSTR lpszCmdParam)
	{

		corona::corona_db_read_only = false;

		std::shared_ptr<corona::desktop_app_bus> service;

		char dir[500];
		GetModuleFileNameA(NULL, dir, 500);

        std::string config_path = std::filesystem::path(dir).parent_path().string();

		config_path += "\\configuration\\";

		if constexpr (use_project_for_config) {
			config_path = "D:\\countrybit\\coronaproject\\configuration\\";
		}

		std::string database_path;

        std::cout << "Config path: " << config_path << std::endl;

		std::string config_full_file = config_path + config_filename;
		std::string config_contents = corona::read_all_string(config_full_file);

        json config_json = corona::json_parser().parse_object(config_contents);
		if (config_json.has_member("Servers")) {
			json servers = config_json["Servers"];
			if (servers.array() && servers.size() > 0) {
				json jserver = servers.get_element(0);
                std::string application_name = jserver["application_name"].as_string();
				if (application_name.size() > 0) {
					my_application_name = application_name;
				}
            }
		}

		// database path
		PWSTR userFolderPath = nullptr;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &userFolderPath);

		if (result == S_OK) {
			istring<4096> dataPath(userFolderPath);

			std::filesystem::path application_path = my_application_name;
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

				try {
					service = std::make_shared<corona::desktop_app_bus>(
						config_path,
						database_path,
						config,
						server,
						true);

					service->application_icon_id = IDI_CORONAPROJECT;

					service->run_app_ui(hInstance, lpszCmdParam, false);
					log_warning("Shutting "+ my_application_name+" down");
				}
				catch (std::exception exc)
				{
					std::cerr << "Error:" << __FILE__ << " " << __LINE__ << " starting application failed:" << exc.what() << std::endl;
                    log_warning(std::format("starting application failed: {0}", exc.what()));
				}
			}
		}
	}

}

#endif
