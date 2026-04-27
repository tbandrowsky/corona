/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This file is for monitoring for directory changes.

Notes
Needs unit test script

For Future Consideration
*/

#pragma once

namespace corona
{

	class directory_checker
	{

	public:

		struct check_options
		{
			std::map<std::string, bool> files_to_ignore;
		};

	private:

		bool check_changes(std::string path, const check_options& _options)
		{
			bool result = false;
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				auto& entry_path = entry.path();
				auto temp = entry_path.string();

				// by default, ignore corona data files
				if (entry_path.extension().string().find("corona") != std::string::npos)
					continue;

				// in case we are too lazy to do this
				if (_options.files_to_ignore.contains(temp))
					continue;

                if (entry.is_directory())
				{
					if (check_changes(temp, _options))
					{
						result = true;
					}
					continue;
				}

				auto foundi = entries.find(temp);
				if (foundi != std::end(entries))
				{
					bool matches = foundi->second != entry.last_write_time();
					if (matches)
					{
						system_monitoring_interface::active_mon->log_information(std::format("file changed {0}", temp));
						result = true;
					}
				}
				else
				{
					result = true;
				}
				entries.insert_or_assign(temp, entry.last_write_time());
			}
			return result;
		}

	public:

		std::map<std::string, std::filesystem::file_time_type> entries;
		std::filesystem::path path;

		directory_checker() = default;
		~directory_checker() = default;

		bool check_changes(const check_options& _options)
		{
			return check_changes(path, _options);
		}
	};
}

