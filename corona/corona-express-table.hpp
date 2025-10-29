
#pragma once

#ifndef CORONA_EXPRESS_TABLE_H
#define CORONA_EXPRESS_TABLE_H

const bool debug_xblock = false;
const bool debug_branch = false;
const bool debug_cache = true;

namespace corona
{
	class xblock_cache;

	const int block_age_seconds = 1;

	enum xblock_types
	{
		xb_none = 0,
		xb_branch = 1,
		xb_leaf = 2,
		xb_record = 3
	};

	template <typename storable_type>
	concept xblock_storable = requires(storable_type a, size_t b, char *buff, const char* src, int32_t *length)
	{
		{ a.before_read(b) == buff };
		{ a.after_read(buff, b) };
		{ a.before_write(length, length) == buff };
		{ a.after_write(buff) };
	};

	struct xblock_ref
	{
	public:

		xblock_types		block_type;
		relative_ptr_type	location;

		xblock_ref()
		{
		}

		xblock_ref(xblock_types _block_type, relative_ptr_type _location)
		{
			block_type = _block_type;
			location = _location;
		}

		xblock_ref(const char* _src, size_t _length)
		{
			xblock_ref* src = (xblock_ref*)_src;
			block_type = src->block_type;
			location = src->location;
		}

		xblock_ref(const xblock_ref& _src) = default;
		xblock_ref(xblock_ref&& _src) = default;
		xblock_ref& operator = (const xblock_ref& _src) = default;
		xblock_ref& operator = (xblock_ref&& _src) = default;

		virtual char* before_read(int32_t _size)
		{
			char* bytes = (char *)this;
			return bytes;
		}

		virtual void after_read(char* _bytes, int32_t _size)
		{
            *this = *(xblock_ref*)_bytes;
		}

		virtual void finished_read(char* _bytes)
		{
		}

		virtual char* before_write(int32_t* _size, int32_t *_capacity) const 
		{
			*_size = sizeof(xblock_ref);
            *_capacity = *_size;
			return (char *)this;
		}

		virtual void after_write(char* _t)
		{

		}

		virtual void finished_write(char* _bytes) const
		{
		}

	};

	class xfor_each_result
	{
	public:
		bool is_any;
		bool is_all;
		int64_t count;
	};


	class xtable_interface
	{
	public:

		virtual relative_ptr_type get_location() = 0;
		virtual json get(json _object) = 0;
		virtual bool put(json& _object) = 0;
		virtual void put_array(json& _object) = 0;
		virtual void erase(json _object)= 0;
		virtual void erase_array(json _object) = 0;
		virtual xfor_each_result for_each(json _object, std::function<relative_ptr_type(json& _item)> _process) = 0;
		virtual json select(json _object, std::function<json(json& _item)> _process) = 0;
		virtual void clear() = 0;
		virtual json get_info() = 0;
		virtual int64_t get_next_object_id() = 0;
		virtual int64_t get_count() = 0;
	};


	static const int xrecords_per_block = 200;

	struct xblock_location
	{
		int32_t key_offset;
		int32_t key_size;
		int32_t value_offset;
		int32_t value_size;
	};

	struct xrecord_block_header
	{
		xblock_types							type;
		xblock_types							content_type;
		int32_t									count;
		xblock_location							records[xrecords_per_block+1];

		xrecord_block_header()
		{
			;
		}

		xrecord_block_header(xrecord_block_header&& _src) = default;
		xrecord_block_header(const xrecord_block_header& _src) = default;
		xrecord_block_header& operator = (xrecord_block_header&& _src) = default;
		xrecord_block_header& operator = (const xrecord_block_header& _src) = default;

		size_t size() const
		{
			return sizeof(xrecord_block_header);
		}

		const char* data() const
		{
			return (const char*)this;
		}

	};

	template <typename data_type>
	requires xblock_storable<data_type>
	class xrecord_block : protected data_block
	{
		bool dirty;
		bool retain;

	protected:

        friend class xblock_cache;

		mutable xrecord_block_header						xheader;
		std::map<xrecord, data_type>						records;
		file_block_interface								*fb;

		friend class xbranch_block;
		friend class xleaf_block;
		
		virtual xrecord get_start_key_nl()
		{
			xrecord key;
			auto max_key = records.begin();
			if (max_key != records.end())
			{
				key = max_key->first;
			}
			return key;
		}

	
		void save_nl()
		{
			if (data_block::header.block_location >= 0) {
				write(fb);
			}
			else
			{
				append(fb);
                if (data_block::header.block_location < 0) {
                    throw std::runtime_error("Failed to append block");
                }
			}
			dirty = false;
			last_save = time(nullptr);
		}

		time_t last_access;
		time_t last_save;

	public:

		xrecord_block(file_block_interface *_fb, xrecord_block_header& _src, bool _retain)
		{
			fb = _fb;
			xheader = _src;
			xheader.count = 0;
			save_nl();
			dirty = true;
			retain = _retain;
            last_access = time(nullptr);
			last_save = last_access;
		}

		xrecord_block(file_block_interface* _fb, int64_t _location, bool _retain)
		{
			if (_location == null_row) {
				throw std::invalid_argument("Invalid location");
			}
			fb = _fb;
			retain = _retain;
			read(fb, _location);
			last_access = time(nullptr);
			last_save = last_access;
			dirty = false;
		}

		virtual ~xrecord_block()
		{
			if (dirty) {
                system_monitoring_interface::active_mon->log_warning("Destroying dirty block without saving", __FILE__, __LINE__);
				abort();
			}
		}

		void accessed()
		{
            last_access = time(nullptr);
		}

		bool is_dirty() const
		{
			return dirty;
		}

		bool get_retain() const
		{
			return retain;
		}

		time_t access_seconds() const
		{
            return time(nullptr) - last_access;
		}

		time_t saved_seconds() const
		{
			return time(nullptr) - last_save;
		}

		xblock_ref get_reference()
		{
			xblock_ref ref;
			ref.block_type = xheader.type;
			ref.location = data_block::header.block_location;
			return ref;
		}

		void dirtied()
		{
            last_access = time(nullptr);
			dirty = true;
		}

		xrecord get_start_key()
		{
			return get_start_key_nl();
		}

		virtual void release()
		{
			dirty = true;
			data_block::erase(fb);
		}


		bool is_full()
		{
			int s = records.size() + 1;
			return s >= xrecords_per_block;
		}

		bool is_empty()
		{
			return records.size() == 0;
		}

		virtual void clear()
		{
			dirty = true;
			records.clear();
		}

		int64_t save()
		{
			if (dirty) {
				last_save = time_t();
				save_nl();
			}
			return size();
		}

		HANDLE save_async(int64_t *_size)
		{
			if (_size) {
				InterlockedAdd64(_size, size());
			}

            HANDLE hwait = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            global_job_queue->submit_job([this]() {
                if (dirty) {
                    save_nl();
                }
                }, hwait);
			return hwait;
		}

	protected:

		virtual char* before_read(int32_t _size)  override
		{
			char *bytes = new char[_size + 10];
			return bytes;
		}

		virtual void after_read(char* _bytes, int32_t _size) override
		{
			records.clear();
			xheader = *((xrecord_block_header *)_bytes);
			for (int i = 0; i < xheader.count; i++)
			{
				auto& rl = xheader.records[i];
				xrecord k(_bytes + rl.key_offset, rl.key_size); // just deserializing the records.
				data_type v(_bytes + rl.value_offset, rl.value_size); // just deserializing the records.
				records.insert_or_assign(k, v);
			}
		}

        virtual void finished_read(char* _bytes) const override
        {
            if (_bytes)
                delete[] _bytes;
        }


		virtual char* before_write(int32_t* _size, int32_t *_capacity) const override
		{
			int32_t offset = xheader.size();

			int i = 0;

			std::vector<std::pair<char*, char*>> temp_records;

			for (auto& r : records)
			{
				xblock_location rl = {};
                rl.key_offset = offset;
				int32_t capacity;
                char *write_key = r.first.before_write(&rl.key_size, &capacity);
				offset += rl.key_size;
				rl.value_offset = offset;
				char* write_value = r.second.before_write(&rl.value_size, &capacity);
                offset += rl.value_size;
				xheader.records[i] = rl;
                temp_records.push_back(std::make_pair(write_key, write_value));	
				i++;
                if (i >= xrecords_per_block) {
                    throw std::runtime_error("Too many records in block");
                }
			}

			*_size = offset;
			*_capacity = 1;
            while (*_capacity < *_size)
            {
                *_capacity *= 2;
            }

			char *bytes = new char[offset + 10];

			i = 0;
			for (auto& r : records)
			{
				auto& rl = xheader.records[i];
				auto& pair = temp_records[i];

				const char *ksrc = pair.first;
                char* kdest = bytes + rl.key_offset;
				std::copy(ksrc, ksrc + rl.key_size, kdest);

				const char *vsrc = pair.second;
				char* vdest = bytes + rl.value_offset;
				std::copy(vsrc, vsrc + rl.value_size, vdest);
				i++;

				r.first.finished_write(pair.first);
				r.second.finished_write(pair.second);
			}

			xrecord_block_header* check_it = (xrecord_block_header*)bytes;
			*check_it = xheader;
			check_it->count = records.size();

			return bytes;
		}

		virtual void after_write(char* _t) override
		{

		}

		virtual void finished_write(char* _bytes)  const override
		{
			if (_bytes)
				delete[] _bytes;
		}

	};


	class xleaf_block : public xrecord_block<xrecord>
	{
	public:

		xleaf_block(file_block_interface *_file, xrecord_block_header& _header, bool _retain) : 
			xrecord_block(_file, _header, _retain)
		{
			
		}

		xleaf_block(file_block_interface* _file, xblock_ref _ref, bool _retain) :
			xrecord_block(_file, _ref.location, _retain)
		{
			;
		}

		bool put(int _indent, const xrecord& key, const xrecord& value)
		{
			dirtied();

			if (records.size() >= xrecords_per_block)
				return false;

			if constexpr (debug_xblock) {
				std::string indent(_indent * 4, ' ');
				std::string message = std::format("{2} leaf:{0}, key:{1}", get_reference().location, key.to_string(), indent);
				system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
			}

			bool added = !records.contains(key);
			records[key] = value;
			xheader.count = records.size();
			return added;
		}

		xrecord get(const xrecord& key)
		{
			xrecord temp;
			auto ri = records.find(key);
			if (ri != records.end()) {
				temp = ri->second;
			}
			return temp;
		}

		void erase(const xrecord& key)
		{
			dirtied();
			records.erase(key);
			xheader.count = records.size();
		}
		
		virtual xfor_each_result for_each(xrecord _key, std::function<relative_ptr_type(const xrecord& _key, const xrecord& _value)> _process)
		{
			xfor_each_result result;
			result.is_all = true;
			result.is_any = false;
			result.count = 0;

			for (auto& item : records) 
			{
				if (item.first.matches(_key) or _key.empty()) {
					if (_process(item.first, item.second) != null_row)
					{
						result.count++;
						result.is_any = true;
					}
				} 
				else 
				{
					result.is_all = false;
				}
			}
			return result;
		}

		virtual std::vector<xrecord> select(xrecord& _key, std::function<xrecord(const xrecord& _key, const xrecord& _value)> _process)
		{
			std::vector<xrecord> result = {};
			for (auto& item : records)
			{
				if (item.first.matches(_key) or _key.empty()) {
					xrecord temp = _process(item.first, item.second);
					if (not temp.empty())
					{
						result.push_back(temp);
					}
				}
			}
			return result;
		}

		json get_info()
		{
			json_parser jp;
			using namespace std::literals;

			json result = jp.create_object();
			auto start_key = get_start_key();
			result.put_member("block", "leaf"sv);
			result.put_member_i64("block_count", records.size());
			std::string starts = start_key.to_string();
			result.put_member("block_key_start", starts);
			result.put_member_i64("block_location", get_reference().location);
			return result;
		}

	};

	class xbranch_block : public xrecord_block<xblock_ref>
	{
	private:
		xblock_ref find_block(const xrecord& key);
		std::map<xrecord, xblock_ref>::iterator find_xrecord(const xrecord& key);

	public:

		xbranch_block(file_block_interface* _file, xrecord_block_header& _header, bool _retain) :
			xrecord_block(_file, _header, _retain)
		{
			;
		}

		xbranch_block(file_block_interface* _file, xblock_ref _ref, bool _retain) :
			xrecord_block(_file, _ref.location, _retain)
		{
			;
		}

		virtual bool put(xblock_cache* cache, int _indent, const xrecord& _key, const xrecord& _value);

		virtual xrecord get(xblock_cache* cache, const xrecord& _key);

		virtual void erase(xblock_cache* cache, const xrecord& _key);

		virtual void clear(xblock_cache* cache);
		
		virtual xfor_each_result for_each(xblock_cache* cache, xrecord _key, std::function<relative_ptr_type(const xrecord& _key, const xrecord& _value)> _process);

		virtual std::vector<xrecord> select(xblock_cache* cache, xrecord& _key, std::function<xrecord(const xrecord& _key, const xrecord& _value)> _process);

		json get_info(xblock_cache* cache);

	};


	template <typename block_type> class xblock_lease
	{
	public:

		block_type* block;

		xblock_lease()
		{
			block = nullptr;
		}

		xblock_lease(const xblock_lease& _src) = default;
		xblock_lease(xblock_lease&& _src) = default;
		xblock_lease& operator =(const xblock_lease& _src) = default;
		xblock_lease& operator = (xblock_lease&& _src) = default;

		void check()
		{
			if (block) {
				block->save();
			}
		}

        xblock_lease(block_type* _block)
        {
            block = _block;
		}

		~xblock_lease()
        {
        }

        block_type* operator->()
        {
            return block;
        }
	};

	template <typename block_type> class xblock_leaseable
	{

		block_type* block;
		std::mutex lease_lock;
		int use_count;
		shared_lockable lockme;

	public:

		xblock_leaseable()
		{
			use_count = 0;
			block = nullptr;
		}

        xblock_leaseable(const xblock_leaseable& _src) = delete;
		xblock_leaseable(xblock_leaseable&& _src) = delete;
		xblock_leaseable& operator = (xblock_leaseable&& _src) = delete;
        xblock_leaseable& operator = (const xblock_leaseable& _src) = delete;

		bool empty()
		{
			return block == nullptr;
		}

		void open(block_type *_block)
		{
			block = _block;
		}

		bool check()
		{
			bool keep = true;
			lease_lock.lock();
			if (block and 
				!block->is_dirty() and 				
				use_count == 0 and 
				block->access_seconds() > block_age_seconds and 
				block->get_retain() == false) 
			{
                delete block;
                block = nullptr;
				keep = false;
			}
			lease_lock.unlock();
			return keep;
		}

		int get_use_count() const { return use_count; }

        xblock_lease<block_type> lease()
        {
			lease_lock.lock();
			use_count++;
            return xblock_lease<block_type>(block);
        }

		void lease_end(xblock_lease<block_type>& _src)
		{
			use_count--;
            xblock_lease<block_type> dummy = std::move(_src);
			lease_lock.unlock();
		}

		block_type* operator->()
		{
			return block;
		}

		virtual ~xblock_leaseable()
		{
			if (block)
				delete block;
			block = nullptr;
		}

	};

	class xblock_cache
	{
		std::map<int64_t, std::shared_ptr<xblock_leaseable<xleaf_block>>> leaf_blocks;
		std::map<int64_t, std::shared_ptr<xblock_leaseable<xbranch_block>>> branch_blocks;
		file_block_interface* fb;
		int64_t maximum_memory_bytes;
		time_t block_lifetime;
        std::mutex cache_lock;

	public:

		xblock_cache(file_block_interface* _fb, int64_t _maximum_memory_bytes = 0)
		{
			fb = _fb;
			maximum_memory_bytes = _maximum_memory_bytes;
			block_lifetime = 0;
		}

		virtual ~xblock_cache()
		{
			clear();
		}

		file_block_interface* get_fb() const
		{
			return fb;
		}

		xblock_lease<xleaf_block> create_leaf_block();
		xblock_lease<xbranch_block> create_branch_block(xblock_types _content_type, bool _retain);
		xblock_lease<xleaf_block> open_leaf_block(xblock_ref& _header);
		xblock_lease<xbranch_block> open_branch_block(xblock_ref& _header, bool _retain);

		void close_block_nl(xblock_lease<xbranch_block>& _block);
		void close_block_nl(xblock_lease<xleaf_block>& _block);
		void close_block(xblock_lease<xbranch_block>& _block);
		void close_block(xblock_lease<xleaf_block>& _block);

		time_t get_block_lifetime()
		{
			return block_lifetime;
		}

		int64_t save();
		void clear();

		xblock_lease<xbranch_block> split_branch(xblock_lease<xbranch_block>& _block, int _indent);
		xblock_lease<xleaf_block> split_leaf(xblock_lease<xleaf_block>& _block, int _indent);
		void split_root(xblock_lease<xbranch_block>& _block, int _indent);
	};

	class xtable_header : public data_block
	{
	public:

		xblock_ref		  root_block;
		xtable_columns key_members, object_members;
		int64_t count = 0;
		int64_t next_id = 1;
		bool    use_object_id = false;

		relative_ptr_type get_location()
		{
			return header.block_location;
		}

		virtual void get_json(json& _dest) const
		{
			json_parser jp;
			_dest.put_member_i64("root_type", root_block.block_type);
			_dest.put_member_i64("root_location", root_block.location);
			json kms = jp.create_object();
			key_members.get_json(kms);
			_dest.share_member("key_members", kms);
            json oms = jp.create_object();
			object_members.get_json(oms);
			_dest.share_member("object_members", oms); 
			_dest.put_member_i64("count", count);
			_dest.put_member_i64("next_id", next_id);
		}

		virtual void put_json(json _src)
		{
			json_parser jp;
			root_block.location = _src["root_location"];
			root_block.block_type = (xblock_types)((int64_t)_src["root_type"]);
			json kms = _src["key_members"];
			key_members.put_json(kms);
			json oms = _src["object_members"];
			object_members.put_json(oms);
			count = (int64_t)_src["count"];
            next_id = (int64_t)_src["next_id"];
            for (auto m = key_members.columns.begin(); m != key_members.columns.end(); m++) {
				if (object_id_field == (std::string)m->second.field_name) {
                    use_object_id = true;
                }
            }
		}

		virtual char* before_read(int32_t _size)  override
		{
			char *t = new char[_size + 1];
			return t;
		}

		virtual void after_read(char* _bytes, int32_t _size) override
		{
			json_parser parser;
			json temp;
			bool safe = std::any_of(_bytes, _bytes + _size, [](char c) { return c == 0; });

			if (safe) {
				temp = parser.parse_object(_bytes);
				put_json(temp);
			}
		}

		virtual void finished_read(char* _bytes) const override
		{
			if (_bytes)
				delete [] _bytes;
		}

		virtual char* before_write(int32_t* _size, int32_t *_capacity) const override
		{
			json_parser jp;
			json temp = jp.create_object();
			get_json(temp);
			std::string data = temp.to_json_typed();
			if (data.size() > giga_to_bytes(1))
				throw std::logic_error("Block too big");
			*_size = (int32_t)data.size() + 1;
			*_capacity = 1;
            while (*_capacity < *_size)
                *_capacity *= 2;
			char* r = new char[data.size() + 10];
			std::copy(data.c_str(), data.c_str() + data.size() + 1, r);
			return r;
		}

		virtual void after_write(char* _t) override
		{

		}


		virtual void finished_write(char* _bytes) const override
		{
			if (_bytes)
				delete[] _bytes;
		}

	};

	class xtable : public xtable_interface, public file_block_interface
	{

		std::string								file_name;
		std::string								data;
		std::shared_ptr<xtable_header>			table_header;
		shared_lockable							locker, file_locker;
		std::shared_ptr<xblock_cache>			cache;
		std::shared_ptr<file>					fp;

		int64_t commit_nl() 
		{
			table_header->write(this);
			auto root = cache->open_branch_block(table_header->root_block, true);
			root->save();
			cache->close_block(root);
			int64_t bytes_written = cache->save();
			return bytes_written;
		}

	public:

		xtable(std::string _file_name, std::shared_ptr<xtable_header> _header) :
			table_header(_header)
		{
			file_name = _file_name;
			fp = std::make_shared<file>(_file_name, file_open_types::create_always);
			cache = std::make_shared<xblock_cache>(this, giga_to_bytes(1));

			table_header->append(this);
			auto root = cache->create_branch_block(xblock_types::xb_leaf, true);
			table_header->root_block = root->get_reference();
			table_header->write(this);
			root->save();
			cache->close_block(root);
			int64_t bytes_written = cache->save();
			system_monitoring_interface::global_mon->log_information(std::format("create table {0}", file_name), __FILE__, __LINE__);
		}

		xtable(std::string _file_name)
		{			
			file_name = _file_name;
			fp = std::make_shared<file>(_file_name, file_open_types::open_existing);
			cache = std::make_shared<xblock_cache>(this, giga_to_bytes(1));

			table_header = std::make_shared<xtable_header>();
			table_header->read(this, 0);
			system_monitoring_interface::global_mon->log_information(std::format("open table {0}", file_name), __FILE__, __LINE__);
		}

		virtual ~xtable()
		{
            commit();            
			system_monitoring_interface::global_mon->log_information(std::format("Closing table {0}", file_name), __FILE__, __LINE__);
		}

		bool put_direct(xrecord& key, xrecord &data)
		{
			auto root = cache->open_branch_block(table_header->root_block, true);

			bool new_record = root->put(cache.get(), 0, key, data);

			if (new_record) {
				table_header->count++;
			}

			if (root->is_full()) {
				cache->split_root(root, 0);
				cache->close_block(root);
				cache->save();
			}
			else 			
			{
				cache->close_block(root);
			}

			return new_record;
		}

		bool put_impl(json& _object)
		{
			if (table_header->use_object_id) {
				if (not _object.has_member(object_id_field) or (int64_t)_object[object_id_field] == 0) {
					int64_t new_id = get_next_object_id();
					_object.put_member_i64(object_id_field, new_id);
				}
			}

			xrecord key;
			key.put_json(&table_header->key_members, _object);
			xrecord data;
			data.put_json(&table_header->object_members, _object);

			auto new_record = put_direct(key, data);
			return new_record;
		}

		std::shared_ptr<xtable_header>			get_table_header()
        {
            return table_header;
        }

		virtual file_result write(int64_t _location, void* _buffer, int _buffer_length) override
		{
            return fp->write(_location, _buffer, _buffer_length);
		}

		virtual file_result read(int64_t _location, void* _buffer, int _buffer_length) override
		{
			return fp->read(_location, _buffer, _buffer_length);
		}

		virtual file_result append(void* _buffer, int _buffer_length) override
		{
			return fp->append(_buffer, _buffer_length);
		}

		virtual relative_ptr_type allocate_space(int64_t _size, int64_t* _actual_size)  override
		{
            *_actual_size = _size;
			relative_ptr_type location = fp->add(_size);
			return location;
		}

		virtual void free_space(int64_t _location) override {

		}

		virtual int64_t add(int _bytes_to_add) override 
		{
			return fp->add(_bytes_to_add);
		}

		virtual file* get_fp() override
		{
			return fp.get();
		}

		virtual int64_t commit() override
		{
			int64_t bytes_written = commit_nl();
			return bytes_written;
		}

		virtual int64_t get_next_object_id() override
		{
            InterlockedIncrement64(&table_header->next_id);
            return table_header->next_id;
		}

		virtual int buffer_count() override
		{
			return 0;
		}

		virtual void clear() override
		{
			auto root = cache->open_branch_block(table_header->root_block, true);
			root->clear(cache.get());
            cache->close_block(root);
		}

		virtual int64_t size() override
		{
			return fp->size();
		}

		relative_ptr_type get_location() override
		{
			read_scope_lock lockme(locker);
			return table_header->get_location();
		}

		template <typename key_type> xrecord create_key(key_type& _key)
		{
			xrecord temp;
			if (table_header->key_members.columns.size() != 1)
				throw std::logic_error("Invalid key type for table");

			auto km = table_header->key_members.columns.begin();
			temp.add(km->second.field_id, _key);
			return temp;
		}

		xrecord create_key_i64(int64_t _key)
		{
			xrecord temp;
			if (table_header->key_members.columns.size() != 1)
                throw std::logic_error("Invalid key type for table");

			auto km = table_header->key_members.columns.begin();
			temp.add_int64(km->second.field_id, _key);
			return temp;
		}

		virtual json get(std::string _key)
		{
			read_scope_lock lockme(locker);
			json_parser jp;
			json jresult;
			xrecord key = create_key<std::string>(_key);
			auto root = cache->open_branch_block(table_header->root_block, true);
			xrecord result = root->get(cache.get(), key);
			if (not result.empty()) {
				jresult = jp.create_object();
				key.get_json(&table_header->key_members, jresult);
				result.get_json(&table_header->object_members, jresult);
				cache->close_block(root);
				return jresult;
			}
			cache->close_block(root);
			return jresult;
		}

		virtual json get(int64_t _key)
		{
			read_scope_lock lockme(locker);
			json_parser jp;
			json jresult;
			xrecord key = create_key_i64(_key);
			auto root = cache->open_branch_block(table_header->root_block, true);
			xrecord result = root->get(cache.get(), key);
			if (not result.empty()) {
				jresult = jp.create_object();
				key.get_json(&table_header->key_members, jresult );
				result.get_json(&table_header->object_members, jresult );
				cache->close_block(root);
				return jresult;
			}
			cache->close_block(root);
			return jresult;
		}
		
		virtual json get(json _object) override
		{
			read_scope_lock lockme(locker);
			json_parser jp;
			json jresult;
			xrecord key;
            key.put_json(&table_header->key_members, _object);
			auto root = cache->open_branch_block(table_header->root_block, true);
			xrecord result = root->get(cache.get(), key);
			if (not result.empty()) {
				jresult = jp.create_object();
				key.get_json(&table_header->key_members, jresult);
				result.get_json(&table_header->object_members, jresult);
				cache->close_block(root);
				return jresult;
			}
			cache->close_block(root);
			return jresult;
		}

		virtual bool put(json& _object) override
		{
			write_scope_lock lockme(locker);

			bool new_record = put_impl(_object);

			commit_nl();

			return new_record;
		}

		virtual void put_array(json& _array) override
		{
			write_scope_lock lockme(locker);

			if (_array.array()) {
				for (auto item : _array) {
					put_impl(item);
				}
				commit_nl();
				cache->clear();
			}
		}

		virtual void erase(int64_t _id)
		{
			write_scope_lock lockme(locker);
			xrecord key;
			key = create_key_i64(_id);
			auto root = cache->open_branch_block(table_header->root_block, true);
			root->erase(cache.get(), key);
			cache->close_block(root);
			commit_nl();
		}

		virtual void erase(json _object) override
		{
			write_scope_lock lockme(locker);
			xrecord key;
			key.put_json(&table_header->key_members, _object);
			::InterlockedDecrement64(&table_header->count);
			auto root = cache->open_branch_block(table_header->root_block, true);
			root->erase(cache.get(), key);
			cache->close_block(root);
			commit_nl();
		}

		virtual void erase_array(json _array) override
		{
			if (_array.array()) {
				auto root = cache->open_branch_block(table_header->root_block, true);
				for (auto item : _array) {
					write_scope_lock lockme(locker);
					xrecord key;
					key.put_json(&table_header->key_members, item);
					::InterlockedDecrement64(&table_header->count);
					root->erase(cache.get(), key);
				}
				cache->close_block(root);
			}
			commit_nl();
		}

		virtual int64_t get_count() override
		{
			return table_header->count;
		}
		
		json get_info()
		{
			auto root = cache->open_branch_block(table_header->root_block, true);
			json jinfo = root->get_info(cache.get());
			cache->close_block(root);
			return jinfo;
		}

		virtual xfor_each_result for_each(json _object, std::function<relative_ptr_type(json& _item)> _process) override
		{
			
			xrecord key;
			key.put_json(&table_header->key_members, _object);
			xfor_each_result result;
			auto root = cache->open_branch_block(table_header->root_block, true);
			result = root->for_each(cache.get(), key, [_process, this](const xrecord& _key, const xrecord& _data)->relative_ptr_type {
				json_parser jp;
				json obj = jp.create_object();
				_key.get_json(&table_header->key_members, obj);
				_data.get_json(&table_header->object_members, obj);
				return _process(obj);
				});
			cache->close_block(root);
			commit_nl();
			return result;
		}

		virtual json select(json _object, std::function<json(json& _item)> _process) override
		{
			xrecord key;
			key.put_json(&table_header->key_members, _object);
			json_parser jp;
			json target = jp.create_array();
			auto root = cache->open_branch_block(table_header->root_block, true);
			root->select(cache.get(), key, [_process, this, &target](const xrecord& _key, const xrecord& _data)->xrecord {
				json_parser jp;
				json obj = jp.create_object();
				xrecord empty;
				_key.get_json(&table_header->key_members, obj);
				_data.get_json(&table_header->object_members, obj);
				json jresult = _process(obj);
				if (jresult.object()) {
					target.push_back(jresult);
					return _key;
				}
				return empty;
				});
			cache->close_block(root);
			commit_nl();
			return target;
		}

	};

	xblock_lease<xleaf_block> xblock_cache::open_leaf_block(xblock_ref& _ref)
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		auto foundit = leaf_blocks.find(_ref.location);
		if (foundit != std::end(leaf_blocks)) {
			if (not foundit->second->empty()) {
				return foundit->second->lease();
			}
			else {
				auto new_block = new xleaf_block(fb, _ref, false);
                foundit->second->open(new_block);
                return foundit->second->lease();
			}
		}

		auto new_block = new xleaf_block(fb, _ref, false);
		auto loc = new_block->get_reference().location;

		if (leaf_blocks.contains(loc)) {
			leaf_blocks[loc]->open(new_block);
		}
		else {
			auto leaseable = std::make_shared<xblock_leaseable<xleaf_block>>();
			leaseable->open(new_block);
			leaf_blocks[loc] = leaseable;
		}

		auto temp = leaf_blocks[loc]->lease();
		return temp;
	}

	xblock_lease<xbranch_block> xblock_cache::open_branch_block(xblock_ref& _ref, bool _retain)
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		auto foundit = branch_blocks.find(_ref.location);
		if (foundit != std::end(branch_blocks)) {
			if (not foundit->second->empty()) {
				return foundit->second->lease();
			}
			else {
				auto new_block = new xbranch_block(fb, _ref, _retain);
				foundit->second->open(new_block);
				return foundit->second->lease();
			}
		}

		auto new_block = new xbranch_block(fb, _ref, _retain);
		auto loc = new_block->get_reference().location;

		if (branch_blocks.contains(loc)) {
			branch_blocks[loc]->open(new_block);
		}
		else 
		{
			auto leaseable = std::make_shared<xblock_leaseable<xbranch_block>>();
			leaseable->open(new_block);
			branch_blocks[loc] = leaseable;
		}

		auto temp = branch_blocks[loc]->lease();
		return temp;
	}

	void xblock_cache::close_block_nl(xblock_lease<xleaf_block>& _block)
	{
		if (_block.block) {
			auto loc = _block.block->get_reference().location;
			auto foundit = leaf_blocks.find(loc);
			if (foundit != std::end(leaf_blocks)) {
				foundit->second->lease_end(_block);
			}
			else
			{
				system_monitoring_interface::active_mon->log_warning("Closing branch block not found", __FILE__, __LINE__);
				abort();
			}
		}
	}

	void xblock_cache::close_block_nl(xblock_lease<xbranch_block>& _block)
	{
		if (_block.block) {
			auto loc = _block.block->get_reference().location;
			auto foundit = branch_blocks.find(loc);
			if (foundit != std::end(branch_blocks)) {
				foundit->second->lease_end(_block);
			}
			else
			{
				system_monitoring_interface::active_mon->log_warning("Closing leaf block not found", __FILE__, __LINE__);
				abort();
			}
		}
	}

	void xblock_cache::close_block(xblock_lease<xbranch_block>& _block)
	{
		std::lock_guard<std::mutex> lockme(cache_lock);
		close_block_nl(_block);
	}

	void xblock_cache::close_block(xblock_lease<xleaf_block>& _block)
	{
		std::lock_guard<std::mutex> lockme(cache_lock);
		close_block_nl(_block);
	}

	xblock_lease<xleaf_block> xblock_cache::create_leaf_block()
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		xrecord_block_header header;

		header.type = xblock_types::xb_leaf;
		header.content_type = xblock_types::xb_record;

		auto new_block = new xleaf_block(fb, header, false);
		auto loc = new_block->get_reference().location;

		if (leaf_blocks.contains(loc)) {
			leaf_blocks[loc]->open(new_block);
			auto temp = leaf_blocks[loc]->lease();
			return temp;
		}
		else {
			auto leaseable = std::make_shared<xblock_leaseable<xleaf_block>>();
            leaseable->open(new_block);
            leaf_blocks[loc] = leaseable;
            auto temp = leaf_blocks[loc]->lease();
            return temp;
		}

	}

	xblock_lease<xbranch_block> xblock_cache::create_branch_block(xblock_types _content_type, bool _retain)
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		xrecord_block_header header;

		header.type = xblock_types::xb_branch;
		header.content_type = _content_type;

		auto new_block = new xbranch_block(fb, header, false);
		auto loc = new_block->get_reference().location;

		if (branch_blocks.contains(loc)) {
			branch_blocks[loc]->open(new_block);
			auto temp = branch_blocks[loc]->lease();
			return temp;
		}
		else {
			auto leaseable = std::make_shared<xblock_leaseable<xbranch_block>>();
			leaseable->open(new_block);
			branch_blocks[loc] = leaseable;
			auto temp = branch_blocks[loc]->lease();
			return temp;
		}
	}

	template <typename data_type> class xblock_save_job : public job
	{
	public:
		xblock_cache* cache;
		HANDLE notification_handle;
		double execution_time_seconds;
		std::shared_ptr<xblock_leaseable<data_type>> leasable;

		xblock_save_job(xblock_cache* _cache, HANDLE _notification_handle, std::shared_ptr<xblock_leaseable<data_type>>& _src)
		{
			cache = _cache;
            notification_handle = _notification_handle;
			leasable = _src;
		}

		virtual bool queued(job_queue* _callingQueue) override {
			return true;
		}

		virtual ~xblock_save_job()
		{
		}

		virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
		{
			job_notify jn;

			xblock_lease<data_type> lease;

			lease = leasable->lease();
            lease.check();
            cache->close_block_nl(lease);

			jn.setSignal(notification_handle);
			jn.shouldDelete = false;

			return jn;
		}

		friend class job_queue;
	};

	int64_t xblock_cache::save()
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		date_time current = date_time::now();
		int64_t total_memory;

		total_memory = 0;

		std::vector<HANDLE> tasks;

		int branch_count = 0;
		int leaf_count = 0;

		for (auto& sv : branch_blocks)
		{
			if (not sv.second->empty()) {
				if (sv.second->get_use_count() == 0) {
					auto leased_block = sv.second->lease();
                    leased_block.check();
                    sv.second->lease_end(leased_block);
					branch_count++;
				}
				else {
					system_monitoring_interface::active_mon->log_warning("Branch handle in use, skipped, indicates code problem.");
				}
			}
		}

		for (auto& sv : leaf_blocks)
		{
			if (not sv.second->empty()) {
				if (sv.second->get_use_count() == 0) {
					auto leased_block = sv.second->lease();
					leased_block.check();
					sv.second->lease_end(leased_block);
					leaf_count++;
				}
				else {
					system_monitoring_interface::active_mon->log_warning("Branch handle in use, skipped, indicates code problem.");
				}
			}
		}

        for (auto bi = branch_blocks.begin(); bi != branch_blocks.end(); )
        {
            if (bi->second->check() == false) {
                bi = branch_blocks.erase(bi);
            }
            else {
                bi++;
            }
        }

		for (auto bi = leaf_blocks.begin(); bi != leaf_blocks.end(); )
		{
			if (bi->second->check() == false) {
				bi = leaf_blocks.erase(bi);
			}
			else {
				bi++;
			}
		}

		return total_memory;
	}

	void xblock_cache::clear()
	{
		std::lock_guard<std::mutex> lockme(cache_lock);

		leaf_blocks.clear();
		branch_blocks.clear();
	}

	std::map<xrecord, xblock_ref>::iterator xbranch_block::find_xrecord(const xrecord& key)
	{
		if constexpr (debug_branch) {
			std::string key_render = key.to_string();
		}
		if (records.size() == 0)
		{
			return records.end();
		}
		else 
		{
			auto range_hit = records.lower_bound(key);
			while (range_hit != records.end())
			{
				if constexpr (debug_branch) {
					std::string key_found_render = range_hit->first.to_string();
				}
				if (range_hit->first <= key or range_hit->first.matches(key)) {
					return range_hit;
				}
				range_hit--;
			}
			auto backhit = records.rbegin();
			if (backhit != records.rend())
			{
				range_hit = records.find(backhit->first);
				return range_hit;
			}
			return records.end();
		}
	}

	xblock_ref xbranch_block::find_block(const xrecord& key)
	{
		xblock_ref found_block = {};
		found_block.block_type = xblock_types::xb_none;

		if constexpr (debug_branch) {
			std::string key_render = key.to_string();
		}

		auto find_iter = find_xrecord(key);

		if (find_iter != records.end())
			found_block = find_iter->second;

		return found_block;
	}

	bool xbranch_block::put(xblock_cache* cache, int _indent, const xrecord& _key, const xrecord& _value)
	{
		bool added = false;

		if constexpr (debug_xblock) {
			std::string indent(_indent * 4, ' ');
			std::string message = std::format("{2} branch:{0}, key:{1}", get_reference().location, _key.to_string(), indent);
			system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
		}

		xblock_ref found_block = find_block(_key);

		if (found_block.block_type == xblock_types::xb_branch)
		{
			auto branch_block = cache->open_branch_block(found_block, false);
			auto old_key = branch_block->get_start_key();
			added = branch_block->put(cache, _indent, _key, _value);
			if (branch_block->is_full()) {
				auto new_branch = cache->split_branch(branch_block, _indent);
				auto new_branch_key = new_branch->get_start_key();
				auto new_branch_ref = new_branch->get_reference();
				records.insert_or_assign(new_branch_key, new_branch_ref);
				cache->close_block(new_branch);
				dirtied();
			}
			auto new_key = branch_block->get_start_key();
			if ((not old_key.empty()) and (not old_key.exact_equal(new_key))) {
				records.erase(old_key);
				records.insert_or_assign(new_key, found_block);
				dirtied();
			}
			cache->close_block(branch_block);
		}
		else if (found_block.block_type == xblock_types::xb_leaf)
		{
			auto leaf_block = cache->open_leaf_block(found_block);
			auto old_key = leaf_block->get_start_key();
			added = leaf_block->put(_indent, _key, _value);
			if (leaf_block->is_full()) {
				auto new_leaf = cache->split_leaf(leaf_block, _indent);
				auto new_leaf_key = new_leaf->get_start_key();
				auto new_leaf_ref = new_leaf->get_reference();
				if constexpr (debug_branch) {
					std::string debug_render = new_leaf_key.to_string();
				}
				records.insert_or_assign(new_leaf_key, new_leaf_ref);
				cache->close_block(new_leaf);
				dirtied();
			}
			auto new_key = leaf_block->get_start_key();
			if constexpr (debug_branch) {
				std::string debug_render = new_key.to_string();
			}
			if ((not old_key.empty()) and (not old_key.exact_equal(new_key))) {
				records.erase(old_key);
				records.insert_or_assign(new_key, found_block);
				dirtied();
			}
			cache->close_block(leaf_block);
		}
		else if (found_block.block_type == xblock_types::xb_none)
		{
			xblock_ref new_ref;
			if (xheader.content_type == xblock_types::xb_leaf)
			{
				auto new_leaf = cache->create_leaf_block();
				added = new_leaf->put(_indent + 1, _key, _value);
				new_ref = new_leaf->get_reference();
				cache->close_block(new_leaf);
			}
			else if (xheader.content_type == xblock_types::xb_branch)
			{
				auto new_branch = cache->create_branch_block(xblock_types::xb_branch, false);
				added = new_branch->put(cache, _indent + 1, _key, _value);
				new_ref = new_branch->get_reference();
				cache->close_block(new_branch);
			}
			records.insert_or_assign(_key, new_ref);
			dirtied();
		}
		xheader.count = records.size();
		return added;
	}

	xrecord xbranch_block::get(xblock_cache* cache, const xrecord& _key)
	{
		xrecord result;
		xblock_ref found_block = find_block(_key);

		if (found_block.block_type == xblock_types::xb_branch)
		{
			auto branch_block = cache->open_branch_block(found_block, false);
			result = branch_block->get(cache, _key);
			cache->close_block(branch_block);
		}
		else if (found_block.block_type == xblock_types::xb_leaf)
		{
			auto leaf_block = cache->open_leaf_block(found_block);
			result = leaf_block->get(_key);
			cache->close_block(leaf_block);
		}
		return result;
	}

	void xbranch_block::erase(xblock_cache* cache, const xrecord& _key)
	{
		dirtied();

		xblock_ref found_block = find_block(_key);

		if (found_block.block_type == xblock_types::xb_branch)
		{
			auto branch_block = cache->open_branch_block(found_block, false);
			branch_block->erase(cache, _key);
			cache->close_block(branch_block);
		}
		else if (found_block.block_type == xblock_types::xb_leaf)
		{
			auto leaf_block = cache->open_leaf_block(found_block);
			leaf_block->erase(_key);
			cache->close_block(leaf_block);
		}
		xheader.count = records.size();
	}

	void xbranch_block::clear(xblock_cache* cache)
	{
		dirtied();

		for (auto item : records) {
			auto found_block = item.second;
			if (found_block.block_type == xblock_types::xb_branch)
			{
				auto branch_block = cache->open_branch_block(found_block, false);
				branch_block->clear(cache);
				branch_block->release();
				cache->close_block(branch_block);
			}
			else if (found_block.block_type == xblock_types::xb_leaf)
			{
				auto leaf_block = cache->open_leaf_block(found_block);
				leaf_block->clear();
				leaf_block->release();
				cache->close_block(leaf_block);
			}
		}
		records.clear();
		xheader.count = records.size();
	}

	xfor_each_result xbranch_block::for_each(xblock_cache* cache, xrecord _key, std::function<relative_ptr_type(const xrecord& _key, const xrecord& _value)> _process)
	{
		xfor_each_result result;
		result.is_all = false;
		result.is_any = false;
		result.count = 0;

		auto iter = find_xrecord(_key);
		while (iter != records.end() and iter->first <= _key)
		{
			xfor_each_result temp;
			auto& found_block = iter->second;
			if (found_block.block_type == xblock_types::xb_branch)
			{
				auto branch_block = cache->open_branch_block(found_block, false);
				temp = branch_block->for_each(cache, _key, _process);
				cache->close_block(branch_block);
			}
			else if (found_block.block_type == xblock_types::xb_leaf)
			{
				auto leaf_block = cache->open_leaf_block(found_block);
				temp = leaf_block->for_each(_key, _process);
				cache->close_block(leaf_block);
			}
			else
				temp = {};

			result.count += temp.count;
			if (temp.is_any)
				result.is_any = true;
			if (not temp.is_all)
				result.is_all = false;
		}
		return result;
	}

	std::vector<xrecord> xbranch_block::select(xblock_cache* cache, xrecord& _key, std::function<xrecord(const xrecord& _key, const xrecord& _value)> _process)
	{
		std::vector<xrecord> result = {};

		if (_key.empty())
		{
			for (auto& item : records)
			{
				std::vector<xrecord> temp;
				auto& found_block = item.second;

				if (found_block.block_type == xblock_types::xb_branch)
				{
					auto branch_block = cache->open_branch_block(found_block, false);
					temp = branch_block->select(cache, _key, _process);
					cache->close_block(branch_block);
				}
				else if (found_block.block_type == xblock_types::xb_leaf)
				{
					auto leaf_block = cache->open_leaf_block(found_block);
					temp = leaf_block->select(_key, _process);
					cache->close_block(leaf_block);
				}
				else
					temp = {};

				result.insert(result.end(), temp.begin(), temp.end());
			}
		}
		else {
			auto iter = find_xrecord(_key);
			while (iter != records.end() and (iter->first <= _key or iter->first.matches(_key)))
			{
				std::vector<xrecord> temp;
				auto& found_block = iter->second;
				if (found_block.block_type == xblock_types::xb_branch)
				{
					auto branch_block = cache->open_branch_block(found_block, false);
					temp = branch_block->select(cache, _key, _process);
					cache->close_block(branch_block);
				}
				else if (found_block.block_type == xblock_types::xb_leaf)
				{
					auto leaf_block = cache->open_leaf_block(found_block);
					temp = leaf_block->select(_key, _process);
					cache->close_block(leaf_block);
				}
				else
					temp = {};

				result.insert(result.end(), temp.begin(), temp.end());
				iter++;
			}
		}
		return result;
	}

	json xbranch_block::get_info(xblock_cache* cache)
	{
		using namespace std::literals;

		json_parser jp;
		json result = jp.create_object();
		auto start_key = get_start_key();
		result.put_member("block", "branch"sv);
		std::string starts = start_key.to_string();
		result.put_member("block_key_start", starts);
		result.put_member_i64("block_count", records.size());
		switch (xheader.content_type) {
		case xblock_types::xb_branch:
			result.put_member("block_content", "branch"sv);
			break;
		case xblock_types::xb_leaf:
			result.put_member("block_content", "leaf"sv);
			break;
		}
		result.put_member_i64("block_location", get_reference().location);

		json children = jp.create_array();
		for (auto r : records)
		{
			if (r.second.block_type == xblock_types::xb_leaf)
			{
				auto block = cache->open_leaf_block(r.second);
				json info = block->get_info();
				children.push_back(info);
				cache->close_block(block);
			}
			else if (r.second.block_type == xblock_types::xb_branch)
			{
				auto block = cache->open_branch_block(r.second, false);
				json info = block->get_info(cache);
				children.push_back(info);
				cache->close_block(block);
			}
		}

		result.share_member("children", children);
		return result;
	}

	void xblock_cache::split_root(xblock_lease<xbranch_block>& _root, int _indent)
	{
		/************************************************

		split into children from this:

		Block A - Full [ 0........n ]

		to this

			  Block A - [ Block B, Block C ]
					//                  \\
				   //                    \\
		Block B - [ 0...3/4n ]        Block C - [ 3/4n...n ]

		***********************************************/

		// newly created blocks are always created dirty, as like
		// think about it, why would you create a block and not use it...
		auto new_child1 = create_branch_block(_root->xheader.content_type, false);
		auto new_child2 = create_branch_block(_root->xheader.content_type, false);

		_root->xheader.content_type = xblock_types::xb_branch;

		int64_t rsz = _root->records.size() / 2i64;
		int64_t count = 0;

		for (auto r : _root->records)
		{
			if (count < rsz)
			{
				new_child1->records.insert_or_assign(r.first, r.second);
			}
			else
			{
				new_child2->records.insert_or_assign(r.first, r.second);
			}
			count++;
		}

		new_child1->xheader.count = new_child1->records.size();
		new_child2->xheader.count = new_child2->records.size();
		new_child1->dirtied();
		new_child2->dirtied();

		xblock_ref child1_ref = new_child1->get_reference();
		xblock_ref child2_ref = new_child2->get_reference();

		// we find the keys for the new children so that we can insert
		// then right things into our map
		xrecord child1_key = new_child1->get_start_key_nl();
		xrecord child2_key = new_child2->get_start_key_nl();

		_root->records.clear();
		_root->records.insert_or_assign(child1_key, child1_ref);
		_root->records.insert_or_assign(child2_key, child2_ref);
		_root->xheader.count = _root->records.size();
		_root->dirtied();
		
        close_block(new_child1);
		close_block(new_child2);
	}

	xblock_lease<xbranch_block> xblock_cache::split_branch(xblock_lease<xbranch_block>& _block, int _indent)
	{

		/************************************************

		split into children from this:

		Block A - Full [ 0........n ]

		to this

		Block A - [ 0...1/2n ]        Block B - [ 1/2n...n ]

		***********************************************/


		auto new_xb = create_branch_block(_block->xheader.content_type, false);

		int64_t rsz = _block->records.size() / 2;

		// time to split the block
		std::vector<xrecord> keys_to_delete;

		int64_t count = 0;

		for (auto& kv : _block->records)
		{
			if (count > rsz) {
				keys_to_delete.push_back(kv.first);
				new_xb->records.insert_or_assign(kv.first, kv.second);
			}
			count++;
		}
		new_xb->xheader.count = new_xb->records.size();

		for (auto& kv : keys_to_delete)
		{
			_block->records.erase(kv);
		}
		_block->xheader.count = _block->records.size();
		_block->save();
		new_xb->save();

		return new_xb;
	}


	xblock_lease<xleaf_block> xblock_cache::split_leaf(xblock_lease<xleaf_block>& _block, int _indent)
	{

		/************************************************

		split into children from this:

		Block A - Full [ 0........n ]

		to this

		Block A - [ 0...1/2n ]        Block B - [ 1/2n...n ]

		***********************************************/

		auto new_xb = create_leaf_block();

		int64_t rsz = _block->records.size() / 2;

		// time to split the block
		std::vector<xrecord> keys_to_delete;

		int64_t count = 0;

		for (auto& kv : _block->records)
		{
			if (count > rsz) {
				keys_to_delete.push_back(kv.first);
				new_xb->put(_indent, kv.first, kv.second);
			}
			count++;
		}

		new_xb->xheader.count = new_xb->records.size();

		for (auto& kv : keys_to_delete)
		{
			_block->records.erase(kv);
		}

		_block->xheader.count = _block->records.size();
		_block->dirtied();
		_block->save();

		new_xb->dirtied();
		new_xb->save();
		return new_xb;
	}

	void test_xleaf(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xleaf", "start", start, __FILE__, __LINE__);

		std::shared_ptr<file> fp = _app->open_file_ptr("test_leaf.corona", file_open_types::create_always);
		buffered_file_block fb(fp);

		xblock_cache cache(&fb, giga_to_bytes(1));

		auto pleaf = cache.create_leaf_block();
        int64_t location = pleaf->get_reference().location;

		json_parser jp;

		int64_t id = 1;
		while (not pleaf->is_full())
		{
			xrecord key, value;
			key.add(1, id);
			value.add(2,10 + id % 50);
			value.add(3,100 + (id % 4) * 50);
			pleaf->put(0, key, value);
			id++;
		}

		json saved_info = pleaf->get_info();
		auto leaf_ref = pleaf->get_reference();
		cache.close_block(pleaf);

		cache.save();
		_tests->test({ "put_survived", true, __FILE__, __LINE__ });

		pleaf = cache.open_leaf_block(leaf_ref);

		json loaded_info = pleaf->get_info();

		_tests->test({ "read_survived", true, __FILE__, __LINE__ });

		std::string s_saved, s_loaded;
		s_saved = saved_info.to_json_typed();
		s_loaded = loaded_info.to_json_typed();

		bool data_correct = s_saved == s_loaded;

		_tests->test({ "summary_correct", data_correct, __FILE__, __LINE__ });

		bool round_trip_success = true;
		for (int64_t i = 1; i < id; i++)
		{
			xrecord key, value, valueread;
			key.add(1, i);
			value.add(2, 10 + i % 50);
			value.add(3, 100 + (i % 4) * 50);
			valueread = pleaf->get(key);
			
			if constexpr (debug_xblock) {
				std::string message;
				message = key.to_string();
				message += " correct:";
				message += value.to_string();
				message += " read:";
				message += valueread.to_string();
				system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
			}

			if (not valueread.exact_equal(value)) {
				round_trip_success = false;
				break;
			}
		}
		_tests->test({ "round_trip", round_trip_success, __FILE__, __LINE__ });

		system_monitoring_interface::active_mon->log_function_stop("xleaf", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

	void test_xbranch(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xbranch", "start", start, __FILE__, __LINE__);

		std::shared_ptr<file> fp = _app->open_file_ptr("test_branch.corona", file_open_types::create_always);
		buffered_file_block fb(fp);

		xblock_cache cache(&fb, giga_to_bytes(1));

		auto pbranch = cache.create_branch_block(xblock_types::xb_leaf, false);

		json_parser jp;

		int test_record_count = 1000;
		bool simple_put = true;

		for (int64_t i = 1; i < test_record_count; i++)
		{
			xrecord key, value;
			key.add(1, i);
			value.add(2, 10 + i % 50);
			value.add(3, 100 + (i % 4) * 50);
			pbranch->put(&cache, 0, key, value);
			xrecord valueread = pbranch->get(&cache, key);
			if (not valueread.exact_equal(value)) {
				simple_put = false;
				break;
			}
			if (pbranch->is_full()) {
				cache.split_root(pbranch, 0);
			}
		}


		auto ref = pbranch->get_reference();
		cache.close_block(pbranch);

		cache.save();

		pbranch = cache.open_branch_block(ref, false);

		json saved_info = pbranch->get_info(&cache);
		_tests->test({ "put_survived", simple_put, __FILE__, __LINE__ });

		json loaded_info = pbranch->get_info(&cache);

		_tests->test({ "read_survived", true, __FILE__, __LINE__ });

		std::string s_saved, s_loaded;
		s_saved = saved_info.to_json_typed();
		s_loaded = loaded_info.to_json_typed();

		bool data_correct = s_saved == s_loaded;

		_tests->test({ "summary_correct", data_correct, __FILE__, __LINE__ });

		bool round_trip_success = true;
		for (int64_t i = 1; i < test_record_count; i++)
		{
			xrecord key, value, valueread;
			key.add(1, i);
			value.add(2, 10 + i % 50);
			value.add(3, 100 + (i % 4) * 50);
			valueread = pbranch->get(&cache, key);

			if constexpr (debug_xblock) {
				std::string message;
				message = key.to_string();
				message += " correct:";
				message += value.to_string();
				message += " read:";
				message += valueread.to_string();
				system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
			}

			if (not valueread.exact_equal(value)) {
				round_trip_success = false;
				break;
			}
		}
		cache.close_block(pbranch);
		_tests->test({ "round_trip", round_trip_success, __FILE__, __LINE__ });


		system_monitoring_interface::active_mon->log_function_stop("xbranch", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

	const int max_fill_records = 5000;

	void test_xtable_write(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xtable write", "start", start, __FILE__, __LINE__);

		std::shared_ptr<xtable_header> header = std::make_shared<xtable_header>();
		header->key_members.columns[1] = { field_types::ft_int64, 1, object_id_field };
		header->object_members.columns[2] = { field_types::ft_string, 2, "name" };
		header->object_members.columns[3] = { field_types::ft_double, 3, "age" };
		header->object_members.columns[4] = { field_types::ft_double, 4, "weight" };

		std::shared_ptr<xtable> ptable;
		ptable = std::make_shared<xtable>("test.corona", header);

		json_parser jp;

		for (int i = 1; i < max_fill_records; i++)
		{
			json obj = jp.create_object();
			obj.put_member_i64(object_id_field, i);
			obj.put_member("age", 10 + i % 50);
			obj.put_member("weight", 100 + (i % 4) * 50);
			ptable->put(obj);
			json check_obj = ptable->get(i);
            if (check_obj.empty()) {
                _tests->test({ "put_failed", false, __FILE__, __LINE__ });
                return;
            }
		}

		_tests->test({ "put_survived", true, __FILE__, __LINE__ });
		ptable->commit();
		_tests->test({ "commit_survived", true, __FILE__, __LINE__ });

		system_monitoring_interface::active_mon->log_function_stop("xtable read", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

	void test_xtable_read(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xtable read", "start", start, __FILE__, __LINE__);

		std::shared_ptr<xtable> ptable;
		ptable = std::make_shared<xtable>("test.corona");

		int count52 = 0;
		std::vector<std::string> keys = { object_id_field, "age", "weight" };
		bool round_trip_success = true;
		json_parser jp;

		for (int i = 1; i < max_fill_records; i++)
		{
			// create a key
			json key = jp.create_object();
			key.put_member_i64(object_id_field, i);

			// create an object with the same algorithm
			json obj = jp.create_object();
			obj.put_member_i64(object_id_field, i);
			int age = 10 + i % 50;
			if (age == 52)
				count52++;
			obj.put_member("age", age);
			obj.put_member("weight", 100 + (i % 4) * 50);
			obj.set_compare_order(keys);

			// fetch our object and check it.
			json objd;
			objd = ptable->get(key);
			if (objd.empty())
			{
				round_trip_success = false;
				break;
			}
			objd.set_compare_order(keys);
			if (objd.compare(obj) != 0) {
				round_trip_success = false;
				break;
			}
		}
		_tests->test({ "round_trip", round_trip_success, __FILE__, __LINE__ });

		bool simple_select = false;
		json object_key = jp.create_object();
		object_key.put_member_i64(object_id_field, 42);
		json select_key_results = ptable->select(object_key, [&object_key](json& _target)-> json {
			if (object_key.compare(_target) == 0) {
				return _target;
			}
			json_parser jp;
			json jx;
			return jx;
			});
		simple_select = select_key_results.array() and select_key_results.size() == 1;
		_tests->test({ "simple_select", simple_select, __FILE__, __LINE__ });

		object_key = jp.create_object();
		object_key.put_member("age", 52);
		json select_match_results = ptable->select(object_key, [&object_key](json& _target)-> json {
			if (object_key.compare(_target) == 0) {
				return _target;
			}
			json_parser jp;
			json jx;
			return jx;
			});

		bool range_key_success = select_match_results.size() == count52;
		_tests->test({ "range_key_success", range_key_success, __FILE__, __LINE__ });

		std::map<int, bool> erased_keys;
		bool erase_success = true;
		for (int i = 1; i < max_fill_records; i += 10)
		{
			erased_keys.insert_or_assign(i, true);
			ptable->erase(i);
		}

		for (int i = 1; i < max_fill_records; i++)
		{
			json result = ptable->get(i);
			if (result.object() and erased_keys.contains(i)) {
				erase_success = false;
				break;
			}
			if (result.empty() and not erased_keys.contains(i)) {
				erase_success = false;
				break;
			}
		}
		_tests->test({ "erase", erase_success, __FILE__, __LINE__ });

		bool clear_success = true;

		// now cleared, make sure the table is empty.
		ptable->clear();

		for (int i = 1; i < max_fill_records; i++)
		{
			json key = jp.create_object();
			key.put_member_i64(object_id_field, i);
			json obj = jp.create_object();
			obj.put_member_i64(object_id_field, i);
			obj.put_member("age", 10 + i % 50);
			obj.put_member("weight", 100 + (i % 4) * 50);
			obj.set_compare_order(keys);
			json objd;
			objd = ptable->get(key);
			if (not objd.empty())
			{
				clear_success = false;
				break;
			}
		}

		_tests->test({ "clear", clear_success, __FILE__, __LINE__ });

		json info = ptable->get_info();
		system_monitoring_interface::active_mon->log_json(info);

		system_monitoring_interface::active_mon->log_function_stop("xtable read", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}


	void test_xtable(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xtable", "start", start, __FILE__, __LINE__);
		test_xtable_write(_tests, _app);
		test_xtable_read(_tests, _app);

		system_monitoring_interface::active_mon->log_function_stop("xtable", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

}

#endif

