
/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT LICENSE


About this File

Notes

For Future Consideration
*/
#pragma once


namespace corona {

	std::string read_all_string(std::string& _filename)
	{
		std::string results;

		std::filesystem::path p = _filename;

		try {

			int64_t length = std::filesystem::file_size(p);
			if (length > 0)
			{
				FILE* fp = nullptr;
				fopen_s(&fp, _filename.c_str(), "rt");
				if (fp != nullptr) {
					char* buffer = new char[length + 1];
					bool skip_bom = true;
					while (fgets(buffer, (int)length, fp) != nullptr) {
						if (skip_bom) {
							skip_bom = false;
							// check for BOM
							if (length >= 3) {
								if ((unsigned char)buffer[0] == 0xEF &&
									(unsigned char)buffer[1] == 0xBB &&
									(unsigned char)buffer[2] == 0xBF) {
									// skip BOM
									results += (buffer + 3);
									continue;
								}
							}
						}
						results += buffer;
					}
					fclose(fp);
					delete[] buffer;
				}
			}
		}
        catch (std::exception& e) {
			std::string err = e.what();
            std::string message = std::format("{0}: {1}", _filename, err);
            std::cout << std::endl << std::endl << message << std::endl << std::endl;
		}
		return results;
	}

}

