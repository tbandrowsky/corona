/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This contains a file access object along with its associated awaiters.
file i/o in corona is based on C++20  with io completion ports.

Notes

For Future Consideration
*/


#ifndef CORONA_FILE_H
#define CORONA_FILE_H

namespace corona
{
	class file;
}

template <typename class_type>
void* operator new(size_t size, class_type& _src, int64_t, void*, int) {
	// Allocate memory and initialize with arguments
	return ::operator new(size);
}

namespace corona
{
	const int debug_file = 0;

	enum class file_open_types
	{
		create_new,
		create_always,
		open_existing,
		open_always
	};

	static const int JsonTableMaxNumberOfLevels = 32;
	static const int JsonTableMaxLevel = JsonTableMaxNumberOfLevels - 1;

	class file_batch;
	class json_node;

	struct list_block_header
	{
	public:
		int64_t		first_block;
		int64_t		last_block;
	};

	struct tree_block_header
	{
	public:
		list_block_header 	 index_list;
		int64_t				 children[256];
	};

	struct table_header_struct
	{
	public:
		int64_t			count;
		int64_t			data_root_location;
		int32_t			level;
	};

	template <typename data> class poco_node;

	class file_result
	{
	public:
		bool success;
		int64_t location;
		DWORD bytes_transferred;
		DWORD bytes_requested;
		os_result last_error;
		char* buff;
	};

	class file
	{
		std::string		filename;
		HANDLE			hfile;
		HANDLE          hport;
		lockable		file_locker;

		// you actually can't put stuff like this here, 
		// because you will have multiple file users.  
		// so tempting.
		os_result		last_result;

		void open(const std::string& _filename, file_open_types _file_open_type)
		{
			filename = _filename;

			DWORD disposition;

			switch (_file_open_type)
			{
			case file_open_types::create_new:
				disposition = CREATE_NEW;
				break;
			case file_open_types::create_always:
				disposition = CREATE_ALWAYS;
				break;
			case file_open_types::open_existing:
				disposition = OPEN_EXISTING;
				break;
			case file_open_types::open_always:
				disposition = OPEN_ALWAYS;
				break;
			default:
				throw std::runtime_error("Invalid enum to open file ");
			}

			CREATEFILE2_EXTENDED_PARAMETERS params = { 0 };

			params.dwSize = sizeof(params);
			params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			params.dwSecurityQosFlags = 0;
			params.hTemplateFile = NULL;
			params.lpSecurityAttributes = NULL;
			int retry_count = 10;

			do
			{
				hfile = ::CreateFileA(filename.c_str(), (GENERIC_READ | GENERIC_WRITE), 0, nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr );
				if (hfile != INVALID_HANDLE_VALUE) {
					break;
				}
				os_result what_wrong;
				std::cout << what_wrong.message << std::endl;
				if (what_wrong.error_code < 90) {
					Sleep(200);
					retry_count--;
				}
				else {
					break;
				}
			} while (retry_count >= 5);
			if (hfile == INVALID_HANDLE_VALUE) {
				os_result osr;
				{
					last_result = osr;
					std::string temp = filename + ":" + osr.message;
					throw std::logic_error(temp.c_str());
				}
			}
			last_result = os_result(0);
		}


	public:

		friend class buffered_file_block;
		friend class file_buffer;

		file() = default;
		file(const file& _src) = delete;
		file& operator=(const file& _src) = delete;
		file(file&& _src)
		{
			filename = _src.filename;
			last_result = _src.last_result;
			hfile = _src.hfile;
			_src.hfile = nullptr;
		}
		file& operator=(file&& _src)
		{
			filename = _src.filename;
			last_result = _src.last_result;
			hfile = _src.hfile;
			_src.hfile = nullptr;
			return *this;
		}

		file(KNOWNFOLDERID _folder_id, const file_path& _filename, file_open_types _file_open_type)
			: filename(_filename), hfile(INVALID_HANDLE_VALUE)
		{
			wchar_t* wide_path = nullptr;
			::SHGetKnownFolderPath(_folder_id, KF_FLAG_DEFAULT, NULL, &wide_path );
			if (wide_path) 
			{
				istring<2048> temp = wide_path;
				temp += "\\";
				temp += _filename;
				open(temp.c_str(), _file_open_type);
			}
		}

		file(const std::string& _filename, file_open_types _file_open_type)
			: filename(_filename), hfile(INVALID_HANDLE_VALUE)
		{
			open(_filename, _file_open_type);
		}

		~file()
		{
			if (hfile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hfile);
				hfile = INVALID_HANDLE_VALUE;
			}

			// https://devblogs.microsoft.com/oldnewthing/20130823-00/?p=3423
			// see why this is commented out, so we know, to never do this.
//			if (hport != INVALID_HANDLE_VALUE)
//			{
//				CloseHandle(hport);
//				hport = INVALID_HANDLE_VALUE;
//			}
		}

		std::string read()
		{
			auto fsize = size();
			buffer b(fsize + 1);
			char* s = b.get_ptr();
			std::fill(s, s + fsize, 0);
			read(0, s, fsize);
            return b.get_ptr();
		}

		void write(std::string _src)
		{
			if (not _src.empty()) {
				int length = _src.size();
				int64_t location = add(length);
				write(location, (void*)_src.c_str(), length);
			}
		}

		file_result write(int64_t location, void* _buffer, int32_t _buffer_length)
		{
			file_result fcr;

			OVERLAPPED o = {};
			o.Offset = static_cast<DWORD>(location);
			o.OffsetHigh = static_cast<DWORD>(location >> 32);
            fcr.success = ::WriteFile(hfile, _buffer, _buffer_length, &fcr.bytes_transferred, &o);
            fcr.last_error = os_result(::GetLastError());
            fcr.buff = (char*)_buffer;
			fcr.bytes_requested = _buffer_length;
			fcr.location = location;
			return fcr;
		}

		file_result read(int64_t location, void* _buffer, int32_t _buffer_length)
		{
			int32_t bytes_read = 0;
			file_result fcr;

			OVERLAPPED o = {};
			o.Offset = static_cast<DWORD>(location);
			o.OffsetHigh = static_cast<DWORD>(location >> 32);
			
			fcr.success = ::ReadFile(hfile, _buffer, _buffer_length, &fcr.bytes_transferred, &o);
			fcr.last_error = os_result(::GetLastError());
			fcr.buff = (char*)_buffer;
			fcr.bytes_requested = _buffer_length;
			fcr.location = location;
			return fcr;
		}

		int64_t add(int64_t _bytes_to_add) // adds size_bytes to file and returns the position of the start
		{
			if (hfile == INVALID_HANDLE_VALUE)
				return -1;

			scope_lock add_lock(file_locker);

			LARGE_INTEGER position, new_position;
			BOOL success = ::GetFileSizeEx(hfile, &position);
			if (not success) {
				os_result err;
				system_monitoring_interface::active_mon->log_warning(std::format("GetFileSizeEx failed on {0} with error #{1} - {2}", filename, err.message, err.success), __FILE__, __LINE__);
			}
			new_position = position;
			new_position.QuadPart += _bytes_to_add;
			::SetFilePointerEx(hfile, new_position, nullptr, FILE_BEGIN);
			::SetEndOfFile(hfile);

			return position.QuadPart;
		}

		int64_t trim(int64_t _location) // trims the file size to _location
		{

			if (hfile == INVALID_HANDLE_VALUE)
				return -1;

			scope_lock add_lock(file_locker);

			LARGE_INTEGER new_position;
			new_position.QuadPart = _location;
			::SetFilePointerEx(hfile, new_position, nullptr, FILE_BEGIN);
			::SetEndOfFile(hfile);

			return _location;
		}

		file_result append(void* _buffer, int _buffer_length)
		{
			int64_t file_position = add(_buffer_length);

            return write(file_position, _buffer, _buffer_length);
		}

		int64_t size()
		{
			if (hfile == INVALID_HANDLE_VALUE)
				return 0;
			LARGE_INTEGER file_size;
			file_size.QuadPart = 0;
			::GetFileSizeEx(hfile, &file_size);
			return file_size.QuadPart;
		}
	};
}


#endif
