#pragma once

#ifndef CORONA_DATABLOCK_HPP
#define CORONA_DATABLOCK_HPP

namespace corona
{

	const int ENABLE_JSON_LOGGING = 0;

	// data blocks
	class data_block;

	// poco nodes
	template <typename T> class poco_node;

	struct allocation_index
	{
		int		index;
		int64_t size;
	};

	// root data block.  Everything is a block.  It's almost like minecraft, except, it makes money
	// and thus far, I do not.
	class data_block
	{
	public:

		data_block_struct				header;

		data_block()
		{
			header = {};
			header.block_location = null_row;
			header.data_location = null_row;
		}

		data_block(const data_block& _src) = default;
		data_block& operator = (const data_block& _src) = default;

		virtual char* before_write(int32_t* _size, int32_t* _capacity)  const = 0;

		virtual void after_write(char* _buff)
		{
			;
		}

		virtual char* before_read(int32_t _size) = 0;
		virtual void after_read(char* _bytes, int32_t _size) = 0;

		virtual void finished_read(char* _bytes)  const  = 0;
		virtual void finished_write(char* _bytes) const = 0;

		relative_ptr_type read(file_block_interface* _file, relative_ptr_type location)
		{
			date_time start_time = date_time::now();
			timer tx;

			if (ENABLE_JSON_LOGGING) {
				system_monitoring_interface::active_mon->log_block_start("block", "read block", start_time, __FILE__, __LINE__);
			}

			file_result header_result = _file->read(location, &header, sizeof(header));

			if (header_result.success)
			{
				char* bytes = before_read(header.data_size);
				file_result data_result = _file->read(header.data_location, bytes, header.data_size);

				if (data_result.success)
				{
					after_read(bytes, header.data_size);
					finished_read(bytes);
					if (ENABLE_JSON_LOGGING) {
						system_monitoring_interface::active_mon->log_block_stop("block", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					}
					return header_result.location; // want to make this 0 or -1 if error
				}
				else
				{
					finished_read(bytes);
					if (ENABLE_JSON_LOGGING) {
						system_monitoring_interface::active_mon->log_function_stop("block", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					}
				}
			}

			if (ENABLE_JSON_LOGGING) {
				system_monitoring_interface::active_mon->log_block_stop("block", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			return -1i64;
		}

		relative_ptr_type write(file_block_interface* _file)
		{

			if (header.block_location < 0)
			{
				throw std::invalid_argument("use append for new blocks");
			}

			date_time start_time = date_time::now();
			timer tx;

			if (ENABLE_JSON_LOGGING) {
				system_monitoring_interface::active_mon->log_block_start("block", "write block", start_time, __FILE__, __LINE__);
			}

			int32_t size;
			int32_t capacity;
			char* bytes = before_write(&size, &capacity);

			if (capacity > header.data_capacity)
			{
				// the header.data_length is the max size of the block.  Since there's stuff past that in the file, then,
				// there's not going to be a way we can write this, so we must have another block.
				_file->free_space(header.data_location);
				header.data_size = size;
				header.data_capacity = capacity;
				int64_t actual_size;
				header.data_location = _file->allocate_space(capacity, &actual_size);
				header.data_capacity = actual_size;
			}
			else
			{
				header.data_size = size;
			}

			file_result data_result = _file->write(header.data_location, bytes, size);

			if (data_result.success)
			{
				file_result header_result = _file->write(header.block_location, &header, sizeof(header));
				after_write(bytes);
				finished_write(bytes);
				if (ENABLE_JSON_LOGGING) {
					system_monitoring_interface::active_mon->log_block_stop("block", "write complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
				return header_result.location;
			}
			else
			{
				finished_write(bytes);
				if (ENABLE_JSON_LOGGING) {
					system_monitoring_interface::active_mon->log_block_stop("block", "write failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
			}
			return -1i64;
		}

		relative_ptr_type append(file_block_interface* _file)
		{

			int32_t size, capacity;
			char* bytes;
			bytes = before_write(&size, &capacity);

			date_time start_time = date_time::now();
			timer tx;

			if (ENABLE_JSON_LOGGING) {
				system_monitoring_interface::active_mon->log_block_start("block", "append", start_time, __FILE__, __LINE__);
			}

			int64_t actual_size = 0;
			header.block_location = _file->allocate_space(sizeof(header), &actual_size);
			header.data_size = size;
			header.data_location = _file->allocate_space(capacity, &actual_size);
			header.data_capacity = actual_size;

			if (header.block_location < 0 or header.data_location < 0)
				return -1;

			auto hdr_status = _file->write(header.block_location, &header, sizeof(header));
			auto data_status = _file->write(header.data_location, bytes, size);

			if (ENABLE_JSON_LOGGING) {
				system_monitoring_interface::active_mon->log_block_stop("block", "append", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}

			after_write(bytes);
			finished_write(bytes);

			if (not (hdr_status.success and data_status.success)) {
				return -1;
			}

			return header.block_location;
		}

		void erase(file_block_interface* _fb)
		{
			_fb->free_space(header.data_location);
			_fb->free_space(header.block_location);
		}

		int64_t size()
		{
			return sizeof(header) + header.data_size;
		}
	};

	class string_block : public data_block
	{
	public:

		std::string data;

		string_block()
		{
		}

		bool is_empty()
		{
			return data.empty();
		}

		void clear()
		{
			data.clear();
		}

		virtual char* before_read(int32_t size) override
		{
			data.resize(size, 0);
			return (char*)data.c_str();
		}

		virtual void after_read(char* _bytes, int32_t _size) override
		{
			;
		}
		virtual void finished_read(char* _bytes) const override
		{
			;
		}

		virtual char* before_write(int32_t* _size, int32_t* _capacity) const override
		{
			*_size = strlen(data.c_str());
			for (int k = 1; k < *_size; k *= 2) {
                *_capacity = k * 2;
			}
			return (char*)data.c_str();
		}

		virtual void after_write(char* _bytes) override
		{
			;
		}

		virtual void finished_write(char* _bytes) const override
		{
			;
		}

	};

	class json_data_block : public data_block
	{
	public:

		json							data;
		std::string						bytes;

		json_data_block()
		{
		}

		bool is_empty()
		{
			return data.empty();
		}

		void clear()
		{
			json_parser jp;
			data = jp.create_object();
		}

		virtual char* before_read(int32_t _size)  override
		{
			bytes.resize(_size);
			return (char*)bytes.c_str();
		}

		virtual void after_read(char* _bytes, int32_t _size) override
		{
			const char* contents = _bytes;
			if (contents) {
				json_parser jp;
				if (*contents == '[') {
					data = jp.parse_array(contents);
				}
				else {
					data = jp.parse_object(contents);
				}
			}
		}

		virtual void finished_read(char* _bytes) const override
		{
		}

		virtual char* before_write(int32_t* _size, int32_t * _capacity) const override
		{
			std::stringstream buff;

			data.serialize(buff);
			std::string temp = buff.str();
			char* bytes = new char[temp.size() + 10];
			std::copy(temp.c_str(), temp.c_str() + temp.size() + 1, bytes);

			for (int k = 1; k < *_size; k *= 2) {
				*_capacity = k * 2;
			}

            *_size = (int32_t)temp.size() + 1;
			return bytes;
		}

		virtual void after_write(char* _t) override
		{

		}

		virtual void finished_write(char* _bytes) const override
		{
			if (_bytes) delete [] _bytes;
		}

	};


	template <typename poco_type> class poco_node : public data_block
	{
	public:

		poco_node()
		{
			clear();
		}

		void clear()
		{
			data = {};
		}

		poco_type				data;

		virtual char* before_read(int32_t size) override
		{
			char* io_bytes = (char*)&data;
			return io_bytes;
		}

		virtual void after_read(char* _bytes, int32_t _size) override
		{
		}


		virtual void finished_read(char* _bytes)  const override
		{

		}

		virtual char* before_write(int32_t* _size, int32_t *_capacity) const override
		{
			*_size = sizeof(data);
			*_capacity = *_size;
			char* io_bytes = (char*)&data;
			return io_bytes;
		}

		virtual void after_write(char* _bytes) override
		{
			;
		}

		virtual void finished_write(char* _bytes)  const override
		{

		}

		poco_node& operator = (const poco_type& _src)
		{
			data = _src;
			return *this;
		}

	};
}

#endif
