#pragma once

namespace corona {

	class index_implementation : public index_interface
	{
		int64_t index_id;
		std::string index_class;
		std::string index_name;
		std::vector<std::string> index_keys;
		std::shared_ptr<xtable> table;

	public:

		index_implementation()
		{
			index_id = 0;
		}

		index_implementation(std::shared_ptr<index_interface> _ii_index, corona_database_interface* _db)
		{
			index_id = _ii_index->get_index_id();
			index_name = _ii_index->get_index_name();
			index_keys = _ii_index->get_index_keys();
			auto temp = _ii_index->get_xtable(_db);
		}

		index_implementation(std::string& _name, std::vector<std::string>& _keys, corona_database_interface* _db)
		{
			index_id = 0;
			index_name = _name;
			index_keys = _keys;

			if (_db) {

				auto temp = get_xtable(_db);

			}
		}

		index_implementation(const index_implementation& _src)
		{
			index_id = _src.index_id;
			index_name = _src.index_name;
			index_keys = _src.index_keys;
		}

		index_implementation(index_implementation&& _src)
		{
			index_id = _src.index_id;
			index_name = _src.index_name;
			index_keys = _src.index_keys;
		}

		index_implementation& operator = (const index_implementation& _src)
		{
			index_id = _src.index_id;
			index_name = _src.index_name;
			index_keys = _src.index_keys;
			return *this;
		}

		index_implementation& operator = (index_implementation&& _src)
		{
			index_id = _src.index_id;
			index_name = _src.index_name;
			index_keys = _src.index_keys;
			return *this;

		}

		int64_t get_count() override
		{
			if (table) {
				return table->get_count();
			}
			return 0;
		}

		virtual std::string get_index_key_string() override
		{
			return join(index_keys, ".");
		}

		virtual std::vector<std::string>& get_index_keys() override
		{
			return index_keys;
		}

		virtual void get_json(json& _dest) override
		{
			json_parser jp;
			_dest.put_member("index_name", index_name);
			_dest.put_member("index_class", index_class);

			json jindex_keys = jp.create_array();
			for (auto ikey : index_keys)
			{
				jindex_keys.push_back(ikey);
			}
			_dest.share_member("index_keys", jindex_keys);
		}

		virtual void put_json(validation_error_collection& _errors, json& _src) override
		{
			index_name = _src["index_name"].as_string();
			index_class = _src["index_class"].as_string();
			json jindex_keys = _src["index_keys"];
			if (jindex_keys.array())
			{
				index_keys.clear();
				bool add_object_id = true;
				for (auto key : jindex_keys) {
					std::string key_name = key.as_string();
					if (key_name == object_id_field)
						add_object_id = false;
					index_keys.push_back(key_name);
				}
				if (add_object_id) {
					index_keys.push_back(object_id_field);
				}
			}
		}

		int64_t get_index_id()  override
		{
			return index_id;
		}

		index_implementation& set_index_id(int64_t _index_id)
		{
			index_id = _index_id;
			return *this;
		}

		std::string get_index_name() override
		{
			return index_name;
		}

		virtual void set_index_name(const std::string& _name) override
		{
			index_name = _name;
		}

		virtual std::string	get_index_class() override
		{
			return index_class;
		}

		virtual void set_index_class(const std::string& _class) override
		{
			index_class = _class;
		}

		virtual std::string get_index_filename(corona_database_interface* _db) override {
			std::filesystem::path p = _db->get_directory();
			std::string bfn = std::format("idx_{0}_{1}.{2}", get_index_class(), get_index_name(), "coronaindex");
			p = p / bfn;
			return p.string();
		}

		virtual std::shared_ptr<xtable> create_xtable(corona_database_interface* _db, std::map<std::string, std::shared_ptr<field_interface>>& _fields) override
		{
			xtable_columns key_columns, object_columns;
			int column_id = 1;

			for (auto key : index_keys) {
				auto ifield = _fields.find(key);
				if (ifield != std::end(_fields)) {
					xcolumn cx;
					auto& ifs = ifield->second;
					cx.field_id = column_id;
					cx.field_name = ifs->get_field_name();
					cx.field_type = ifs->get_field_type();
					key_columns.columns[column_id] = cx;

					if ((ifs->get_field_name() == object_id_field || ifs->get_field_name() == class_id_field)) {
						object_columns.columns[column_id] = cx;
					}
				}
				column_id++;
			}

			auto table_header = std::make_shared<xtable_header>();
			table_header->key_members = key_columns;
			table_header->object_members = object_columns;
			table = std::make_shared<xtable>(get_index_filename(_db), table_header);
			return table;
		}

		virtual std::shared_ptr<xtable> get_xtable(corona_database_interface* _db) override
		{
			if (!table)
				table = std::make_shared<xtable>(get_index_filename(_db), corona_db_read_only);
			return table;
		}

		virtual std::shared_ptr<index_interface> clone() override
		{
			json_parser jp;
			json obj = jp.create_object();
			get_json(obj);
			auto new_index = std::make_shared<index_implementation>();
			validation_error_collection errors;
			new_index->put_json(errors, obj);
			if (errors.size() > 0) {
				log_warning(std::format("{0}.{1} index_implementation::clone()", index_class, index_name), __FILE__, __LINE__);
			}
			return new_index;
		}

	};

	class compiled_field_view {
	public:
		std::shared_ptr<field_interface> field_definition;
		std::shared_ptr<json_value>	field_value;
		bool index_required = false;
	};

	class compiled_object_view {
	public:
		std::vector<compiled_field_view> fields;
		class_interface* class_definition = nullptr;
		corona_database_interface* database = nullptr;
		json last_object;

		compiled_object_view() = default;
		compiled_object_view(corona_database_interface* _db, class_interface* _class_definition);

		bool check(json& _object, validation_error_collection& errors);
	};

	class class_implementation : public class_interface
	{

	protected:
		std::string class_name;
		std::string class_description;
		std::string base_class_name;
		std::string display;
		std::vector<std::string> parents;
		std::vector<std::string> full_text_fields;
		std::map<std::string, std::shared_ptr<field_interface>> fields;
		std::map<std::string, std::shared_ptr<index_interface>> indexes;
		std::map<std::string, bool> ancestors;
		std::map<std::string, bool> descendants;
		std::shared_ptr<sql_integration> sql;
		std::string class_author;
		std::string class_version;
		std::vector<std::string> field_order;

		std::string get_class_filename(corona_database_interface* _db) {
			std::filesystem::path p = _db->get_directory();
			std::string bfn = class_name + ".coronaclass";
			p = p / bfn;
			return p.string();
		}

		std::string get_text_index_filename(corona_database_interface* _db) {
			std::filesystem::path p = _db->get_directory();
			std::string bfn = class_name + ".coronatext";
			p = p / bfn;
			return p.string();
		}

		void copy_from(const class_interface* _src)
		{
			class_name = _src->get_class_name();
			class_description = _src->get_class_description();
			base_class_name = _src->get_base_class_name();
			parents = _src->get_parents();
			full_text_fields = _src->get_full_text_fields();

			auto new_fields = _src->get_fields();
			for (auto fld : new_fields) {
				fields.insert_or_assign(fld->get_field_name(), fld);
			}
			auto new_indexes = _src->get_indexes();
			for (auto idx : new_indexes) {
				indexes.insert_or_assign(idx->get_index_name(), idx);
			}
			ancestors = _src->get_ancestors();
			descendants = _src->get_descendants();
			display = _src->get_class_display();
			class_author = _src->get_class_author();
			class_version = _src->get_class_version();
			field_order = _src->get_field_display_order();
		}

		std::shared_ptr<xtable> table;
		std::shared_ptr<xtable> full_text_index;
		std::shared_ptr<sql_table> stable;

	public:

		class_implementation()
		{}

		class_implementation(const class_interface* _src)
		{
			copy_from(_src);
		}

		class_implementation(const class_implementation& _src)
		{
			copy_from(&_src);
		}

		class_implementation(class_implementation&& _src)
		{
			copy_from(&_src);
		}

		class_implementation& operator = (const class_implementation& _src)
		{
			copy_from(&_src);
			return *this;
		}

		class_implementation& operator = (class_implementation&& _src)
		{
			copy_from(&_src);
			return *this;
		}

		virtual int64_t get_next_object_id(corona_database_interface* _db)
		{
			auto tbl = get_table(_db);
			return tbl->get_next_object_id();
		}

		virtual int64_t get_object_count() override {
			if (table) {
				return table->get_count();
			}
			return 0;
		}

		virtual int64_t get_data_count() override {
			int64_t total_count = 0;
			if (table) {
				total_count += table->get_count();
			}
			if (full_text_index) {
				total_count += full_text_index->get_count();
			}
			for (auto idx : indexes) {
				total_count += idx.second->get_count();
			}
			return total_count;
		}

		virtual bool is_server_only(const std::string& _field_name) override
		{
			auto foundit = fields.find(_field_name);
			if (foundit != std::end(fields)) {
				auto field = foundit->second;
				if (field) {
					auto options = field->get_options();
					if (options) {
						return options->is_server_only();
					}
				}
			}
		}

		virtual json get_info(corona_database_interface* _db) override
		{
			json_parser jp;
			json all_info = jp.create_object();

			auto tbl = get_table(_db);
			json info = tbl->get_info();
			all_info.share_member(class_name, info);

			for (auto idx : indexes) {
				auto idx_name = idx.second->get_index_name();
				auto idx_table = idx.second->get_xtable(_db);
				info = idx_table->get_info();
				all_info.share_member(idx_name, info);
			}

			return all_info;
		}

		virtual std::string get_class_name() const override
		{
			return class_name;
		}

		class_implementation& set_class_name(const std::string& _class_name)
		{
			class_name = _class_name;
			return *this;
		}

		virtual std::string get_class_description() const override
		{
			return class_description;
		}

		class_implementation& set_class_description(const std::string& _class_description)
		{
			class_description = class_description;
			return *this;
		}

		virtual std::string get_base_class_name() const override
		{
			return base_class_name;
		}

		class_implementation& set_base_class_name(const std::string& _base_class_name)
		{
			base_class_name = _base_class_name;
			return *this;
		}


		virtual std::string get_class_display() const override
		{
			return display;
		}

		virtual std::vector<std::string> get_parents() const override
		{
			return parents;
		}

		virtual std::vector<std::string> get_full_text_fields() const override
		{
			return full_text_fields;
		}

		virtual std::vector<std::string> get_field_display_order() const override
		{
			return field_order;
		}

		virtual std::string get_class_author() const override
		{
			return class_author;
		}

		virtual std::string get_class_version() const override
		{
			return class_version;
		}


		virtual bool ready() override
		{
			return table.get() != nullptr;
		}

		virtual json get_parent_objects(std::string _user_name, json& _reference_object, corona_database_interface* _db) const override
		{
			json_parser jp;
			json parents_object = jp.create_object();
			json parents_all_object = jp.create_object();
			for (auto parent_class : parents) {
				if (fields.find(parent_class) != std::end(fields)) {
					auto parent_field = fields.at(parent_class);
					if (parent_field) {
						auto options = parent_field->get_options();
						if (options) {
							auto obj_options = std::dynamic_pointer_cast<object_field_options>(options);
							if (obj_options) {
								json parent_filter = jp.create_object();
								parent_filter.put_member(class_name_field, parent_class);
								int64_t parent_object_id = _reference_object[parent_class].as_int64_t();
								parent_filter.put_member_i64(object_id_field, parent_object_id);
								auto permissions = _db->get_class_permission(_user_name, parent_class);
								auto parent_object = _db->select_single_object(parent_filter, false, permissions);
								parents_object.share_member(parent_class, parent_object);
								for (auto m : parent_object.get_members())
								{
									parents_all_object.copy_member(m.first, parent_object);
								}
							}
						}
					}
				}
			}
			parents_object.put_member("all", parents_all_object);
			return parents_object;
		}

		virtual std::shared_ptr<xtable> create_xtable(corona_database_interface* _db) override
		{
			auto table_header = std::make_shared<xtable_header>();
			int column_id = 1;
			for (auto& f : fields) {

				xcolumn col;
				col.field_type = f.second->get_field_type();
				col.field_name = f.first;
				col.field_id = column_id;
				if (f.second->is_relational_children()) {
					continue;
				}
				if (f.second->get_field_name() == object_id_field ||
					f.second->get_field_name() == class_id_field) {
					table_header->key_members.columns[column_id] = col;
				}
				table_header->object_members.columns[column_id] = col;
				column_id++;
			}
			table = std::make_shared<xtable>(get_class_filename(_db), table_header);
			if (full_text_fields.size() > 0) {
				full_text_index = create_full_text_index(_db);
			}
			else
			{
				full_text_index = nullptr;
			}
			return table;
		}

		virtual std::shared_ptr<xtable> get_xtable(corona_database_interface* _db) override
		{
			if (!table) {
				table = std::make_shared<xtable>(get_class_filename(_db), corona_db_read_only);
			}

			if (!full_text_index && full_text_fields.size() > 0) {
				full_text_index = std::make_shared<xtable>(get_text_index_filename(_db), corona_db_read_only);
			}

			return table;
		}

		virtual std::shared_ptr<xtable> create_full_text_index(corona_database_interface* _db) override
		{
			auto table_header = std::make_shared<xtable_header>();
			int column_id = 1;

			// here, fill out the full text index definition
			xtable_columns index_definition;

			xcolumn col;
			col.field_type = field_types::ft_string;
			col.field_name = "text";
			col.field_id = 1;
			table_header->key_members.columns[1] = col;
			col.field_type = field_types::ft_int64;
			col.field_name = "object_id";
			col.field_id = 2;
			table_header->key_members.columns[2] = col;

			full_text_index = std::make_shared<xtable>(get_text_index_filename(_db), table_header);
			return full_text_index;
		}

		virtual std::shared_ptr<xtable> get_full_text_index(corona_database_interface* _db) override
		{
			if (full_text_fields.size() > 0) {
				if (!full_text_index) {
					full_text_index = std::make_shared<xtable>(get_text_index_filename(_db), corona_db_read_only);
				}
			}
			return full_text_index;
		}

		/// <summary>
		/// Alters an xtable in the database, updating its structure and optionally migrating data.
		/// </summary>
		/// <param name="_db">Pointer to the corona_database_interface representing the database connection.</param>
		/// <returns>A shared pointer to the current xtable after alteration.</returns>
		virtual std::shared_ptr<xtable> alter_xtable(corona_database_interface* _db) override
		{
			std::shared_ptr<xtable> current_table, new_table;

			if (std::filesystem::exists(get_class_filename(_db)))
			{
				auto table_header = std::make_shared<xtable_header>();
				int column_id = 1;

				for (auto& f : fields) {

					xcolumn col;
					col.field_type = f.second->get_field_type();
					col.field_name = f.first;
					col.field_id = column_id;
					if (f.second->get_field_name() == object_id_field ||
						f.second->get_field_name() == class_id_field) {
						table_header->key_members.columns[column_id] = col;
					}
					table_header->object_members.columns[column_id] = col;
					column_id++;
				}

				std::string field_check_current;
				for (auto& existing_col : table->get_table_header()->object_members.columns) {
					field_check_current += std::string(existing_col.second.field_name.c_str()) + "." + field_type_names[existing_col.second.field_type] + ";";
				}
				std::string field_check_new;
				for (auto& new_col : table->get_table_header()->object_members.columns) {
					field_check_new += std::string(new_col.second.field_name.c_str()) + "." + field_type_names[new_col.second.field_type] + ";";
				}

				if (field_check_current == field_check_new)
					return table;

				std::string file_name_old = get_class_filename(_db);
				std::string file_name_new = "new_";
				file_name_new += get_class_filename(_db);

				new_table = std::make_shared<xtable>(file_name_new, table_header);
				json_parser jp;
				json empty = jp.create_object();
				current_table->for_each(nullptr, empty, [new_table](json& _src)->relative_ptr_type {
					new_table->put(_src);
					return 1;
					});

				std::filesystem::remove(file_name_old);
				std::filesystem::rename(file_name_new, file_name_old);

				table = new_table;
			}
			else
			{
				auto table_header = std::make_shared<xtable_header>();
				int column_id = 1;
				for (auto& f : fields) {

					xcolumn col;
					col.field_type = f.second->get_field_type();
					col.field_name = f.first;
					col.field_id = column_id;
					if (f.second->get_field_name() == object_id_field ||
						f.second->get_field_name() == class_id_field) {
						table_header->key_members.columns[column_id] = col;
					}
					table_header->object_members.columns[column_id] = col;
					column_id++;
				}
				current_table = std::make_shared<xtable>(get_class_filename(_db), table_header);
				table = current_table;
			}
			return current_table;
		}

		virtual std::shared_ptr<sql_table> create_stable(corona_database_interface* _db) override
		{
			if (not sql)
				return nullptr;

			std::string connection = _db->connections.get_connection(sql->connection_name);

			auto stable = std::make_shared<sql_table>(sql, connection);

			create_xtable(_db);

			return stable;
		}

		virtual std::shared_ptr<sql_table> get_stable(corona_database_interface* _db) override
		{
			if (not sql)
				return nullptr;

			std::string connection = _db->connections.get_connection(sql->connection_name);

			stable = std::make_shared<sql_table>(sql, connection);
			table = create_xtable(_db);
			table->commit();
			table = nullptr;
			table = std::make_shared<xtable>(get_class_filename(_db), corona_db_read_only);

			// we're going to make our xtable anyway so we can slap our object id 
			// on top of a sql server primary key
			// but we don't do this all the time, because we'd like to keep the data around.

			table = get_xtable(_db);
			return stable;
		}

		virtual std::shared_ptr<xtable_interface> create_table(corona_database_interface* _db) override
		{
			if (corona_db_read_only) {
				throw std::logic_error("Can't create tables on a read only database");
			}

			if (sql)
			{
				stable = create_stable(_db);
			}
			else {
				table = create_xtable(_db);
				table->commit();
				table = nullptr;
				table = std::make_shared<xtable>(get_class_filename(_db), false);
			}
			return table;
		}

		virtual std::shared_ptr<xtable_interface> get_table(corona_database_interface* _db) override
		{
			if (sql)
			{
				return get_stable(_db);
			}
			else
				return get_xtable(_db);
		}

		virtual bool any(corona_database_interface* _db) override
		{
			auto tbl = get_table(_db);
			return  tbl->get_count() > 0;
		}

		virtual bool any_descendants(corona_database_interface* _db) override
		{
			for (auto desc : descendants) {
				auto dclass = _db->read_lock_class(desc.first);
				if (dclass) {
					if (dclass->any(_db))
						return true;
				}
			}
			return false;
		}

		virtual std::map<std::string, bool>  const& get_descendants() const override
		{
			return descendants;
		}

		virtual std::map<std::string, bool>  const& get_ancestors() const override
		{
			return ancestors;
		}

		virtual std::map<std::string, bool>& update_descendants()  override
		{
			return descendants;
		}

		virtual std::map<std::string, bool>& update_ancestors()  override
		{
			return ancestors;
		}

		bool empty()
		{
			return class_name.empty();
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			for (auto& fld : fields) {
				fld.second->init_validation(_db, _permissions);
			}
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;

			_dest.put_member(class_name_field, class_name);
			_dest.put_member("class_description", class_description);
			_dest.put_member("base_class_name", base_class_name);
			_dest.put_member("class_version", class_version);
			_dest.put_member("class_author", class_author);

			if (table) {
				_dest.put_member("object_count", table->get_count());
			}
			if (full_text_index) {
				_dest.put_member("word_count", full_text_index->get_count());
			}

			json jfield_order = jp.create_array();
			for (auto& field_name : field_order) {
				jfield_order.push_back(field_name);
			}
			_dest.put_member("field_order", jfield_order);

			json ja = jp.create_array();
			for (auto p : parents)
			{
				ja.push_back(p);
			}
			_dest.put_member("parents", ja);

			ja = jp.create_array();
			for (auto p : full_text_fields)
			{
				ja.push_back(p);
			}
			_dest.put_member("full_text", ja);

			if (fields.size() > 0) {
				json jfield_object = jp.create_object();
				for (auto field : fields) {
					json jfield_definition = jp.create_object();
					field.second->get_json(jfield_definition);
					jfield_object.put_member(field.first, jfield_definition);

				}
				_dest.put_member("fields", jfield_object);
			}

			if (indexes.size() > 0) {
				json jindex_object = jp.create_object();
				for (auto index : indexes) {
					json jindex_definition = jp.create_object();
					index.second->get_json(jindex_definition);
					jindex_object.put_member(index.first, jindex_definition);
				}
				_dest.put_member("indexes", jindex_object);
			}

			if (ancestors.size() > 0) {
				json jancestor_array = jp.create_array();
				for (auto& class_ancestor : ancestors) {
					jancestor_array.push_back(class_ancestor.first);
				}
				_dest.put_member("ancestors", jancestor_array);
			}

			if (descendants.size() > 0) {
				json jdescendants_array = jp.create_array();
				for (auto& class_descendant : descendants) {
					jdescendants_array.push_back(class_descendant.first);
				}
				_dest.put_member("descendants", jdescendants_array);
			}

			if (sql) {
				json jsql = jp.create_object();
				sql->get_json(jsql);
				_dest.put_member("sql", jsql);
			}
		}

		virtual void put_json(validation_error_collection& _errors, json& _src)
		{

			json jfields, jindexes, jancestors, jdescendants, jtable_fields;

			std::string temp_class_name = _src[class_name_field].as_string();
			std::string temp_base_class_name = _src["base_class_name"].as_string();

			json jdisplay_order = _src["field_order"];
			field_order = jdisplay_order.to_string_array();

			if (temp_class_name.empty()) {
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "Missing class definition";
				_errors.push_back(ve);

				system_monitoring_interface::active_mon->log_warning("Malformed class", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(_src);
				return;
			}

			if (temp_base_class_name == temp_class_name) {
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "class can't be derived from itself";
				_errors.push_back(ve);

				system_monitoring_interface::active_mon->log_warning("Malformed class", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(_src);
				return;
			}

			class_name = _src[class_name_field].as_string();
			base_class_name = _src["base_class_name"].as_string();
			class_description = _src["class_description"].as_string();
			class_author = _src["class_author"].as_string();
			class_version = _src["class_version"].as_string();

			parents.clear();
			json jparents = _src["parents"];

			if (jparents.is_string())
			{
				std::string jparento = jparents.as_string();
				parents.push_back(jparento);
			}
			else if (jparents.array())
			{
				for (auto jparent : jparents) {
					parents.push_back(jparent.as_string());
				}
			}

			full_text_fields.clear();
			json jfulltext = _src["full_text"];

			if (jfulltext.is_string())
			{
				std::string jparento = jfulltext.as_string();
				full_text_fields.push_back(jparento);
			}
			else if (jfulltext.array())
			{
				for (auto jparent : jfulltext) {
					full_text_fields.push_back(jparent.as_string());
				}
			}

			class_author = _src["class_author"].as_string();
			class_version = _src["class_version"].as_string();

			jtable_fields = _src["table_fields"];

			ancestors.clear();
			jancestors = _src["ancestors"];
			if (jancestors.array())
			{
				for (auto jancestor : jancestors)
				{
					std::string ancestor = jancestor.as_string();
					ancestors.insert_or_assign(ancestor, true);
				}
			}
			else if (not jancestors.empty()) {
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "ancestors must be an array of strings";
				_errors.push_back(ve);
			}

			descendants.clear();
			jdescendants = _src["descendants"];
			if (jdescendants.array())
			{
				for (auto jdescendant : jdescendants)
				{
					std::string descendant = jdescendant.as_string();
					descendants.insert_or_assign(descendant, true);
				}
			}
			else if (not jdescendants.empty())
			{
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "descendants must be an array of strings";
				_errors.push_back(ve);
			}

			fields.clear();

			jfields = _src["fields"];

			if (jfields.empty())
			{
				json_parser jp;
				jfields = jp.create_object();
			}

			for (auto parent : parents)
			{
				if (!jfields.has_member(parent)) {
					jfields.put_member(parent, std::format("->{}", parent));
				}
			}

			if (jfields.object()) {
				auto jfield_members = jfields.get_members();
				for (auto jfield : jfield_members) {
					std::shared_ptr<field_implementation> field = std::make_shared<field_implementation>();
					field->set_field_type(field_types::ft_none);

					child_object_definition cod;
					reference_definition  rd;

					if (jfield.second.object())
					{
						field->put_json(_errors, jfield.second, this);
					}
					else if (jfield.second.is_string())
					{
						auto fi = allowed_field_types.find(jfield.second.as_string());
						if (fi != std::end(allowed_field_types)) {

							// if this is an array or an object whose type is one of our primitive types
							field->set_field_type(fi->second);
							if (field->get_field_type() == field_types::ft_array)
							{
								auto options = std::make_shared<array_field_options>();
								options->fundamental_type = field_types::ft_array;
								field->set_options(options);
							}
							else if (field->get_field_type() == field_types::ft_object)
							{
								auto options = std::make_shared<object_field_options>();
								options->fundamental_type = field_types::ft_object;
								field->set_options(options);
							}
						}
						else {
							auto parse_temp = jfield.second.as_string();
							cod = child_object_definition::parse_definition(parse_temp.c_str(), get_class_name());
							if (cod.is_undefined)
							{
								rd = reference_definition::parse_definition(parse_temp.c_str());
								if (not rd.is_undefined) {
									field->set_field_type(field_types::ft_reference);
									auto options = std::make_shared<reference_field_options>();
									options->put_definition(rd);
									field->set_options(options);
								}
							}
							else if (cod.is_reference)
							{
								field->set_field_type(field_types::ft_reference);
								auto options = std::make_shared<reference_field_options>();
								options->put_definition(cod);
								field->set_options(options);
							}
							else if (cod.is_array)
							{
								field->set_field_type(field_types::ft_array);
								auto options = std::make_shared<array_field_options>();
								options->put_definition(cod);
								field->set_options(options);
							}
							else
							{
								field->set_field_type(field_types::ft_object);
								auto options = std::make_shared<object_field_options>();
								options->put_definition(cod);
								field->set_options(options);
							}
						}
					}
					else if (jfield.second.array())
					{
						field->set_field_type(field_types::ft_array);
						cod.is_array = true;
						std::shared_ptr<child_object_class> coc;

						for (auto jelement_class_name : jfield.second) {
							if (jelement_class_name.is_string())
							{
								auto ecn = jelement_class_name.as_string();
								auto found_type = allowed_field_types.find(ecn);
								if (found_type != std::end(allowed_field_types)) {
									auto options = std::make_shared<array_field_options>();
									options->fundamental_type = found_type->second;
									field->set_options(options);
									break;
								}
								else
								{
									coc = std::make_shared<child_object_class>();
									coc->class_name = ecn;
									coc->copy_values.insert_or_assign(class_name, reference_field);
									cod.child_classes.push_back(coc);
								}
							}
						}

						if (coc) {
							auto afo = std::make_shared<array_field_options>();
							afo->put_definition(cod);
							field->set_options(afo);
						}
					}
					else
					{
						validation_error ve;
						ve.class_name = class_name;
						ve.field_name = field->get_field_name();
						ve.message = "Is not a valid field specification. Can either be a string or an object.";
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						_errors.push_back(ve);
					}

					if (field->get_field_name().empty())
					{
						field->set_field_name(jfield.first);
					}

					if (field->get_field_type() != field_types::ft_none)
					{
						if (field->get_field_class().empty()) {
							field->set_field_class(class_name);
						}
						fields.insert_or_assign(field->get_field_name(), field);
					}
					else {
						validation_error ve;
						ve.class_name = class_name;
						ve.field_name = field->get_field_name();
						ve.message = "Invalid field type '" + jfield.second.as_string() + "'";
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						_errors.push_back(ve);
					}
				}
			}
			else if (not jfields.empty())
			{
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "fields must be an object.  each field is a member, with a field as the '\"field_name\" : \"field_type\"' or '\"field_name\" : { options }'";
				_errors.push_back(ve);
			}

			jindexes = _src["indexes"];

			if (jindexes.empty())
			{
				json_parser jp;
				jindexes = jp.create_object();
			}

			for (auto parent : parents)
			{
				std::string index_name = "idx_" + class_name + "_" + parent;
				if (jindexes.has_member(index_name)) {
					continue;
				}
				else
				{
					json_parser jp;
					json new_index = jp.create_object();
					new_index.put_member("index_class", class_name);
					new_index.put_member("index_name", index_name);
					json new_index_keys = jp.create_array();
					new_index_keys.push_back(parent);
					new_index.put_member("index_keys", new_index_keys);
					jindexes.put_member(index_name, new_index);
				}
			}

			if (jindexes.object()) {

				indexes.clear();

				auto jindex_members = jindexes.get_members();
				for (auto jindex : jindex_members) {
					std::shared_ptr<index_implementation> index = std::make_shared<index_implementation>();
					index->put_json(_errors, jindex.second);
					index->set_index_name(jindex.first);
					index->set_index_class(class_name);

					if (index->get_index_name().empty())
					{
						validation_error ve;
						ve.class_name = class_name;
						ve.field_name = index->get_index_key_string();
						ve.message = "Missing index name.";
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						_errors.push_back(ve);
					}

					indexes.insert_or_assign(jindex.first, index);
				}
			}
			else if (not jindexes.empty())
			{
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "indexes must be an object.";
				_errors.push_back(ve);
			}

			json jsql = _src["sql"];
			if (jsql.object()) {
				sql = std::make_shared<sql_integration>();
				sql->put_json(_errors, jsql);
				int column_id = 0;

				std::vector<std::string> keys;

				for (auto& mp : sql->mappings) {
					auto fi = fields.find(mp.corona_field_name);

					if (fi != fields.end()) {
						xcolumn col;
						col.field_id = column_id;
						col.field_name = mp.corona_field_name;
						col.field_type = fi->second->get_field_type();
						if (mp.string_size <= 0) {
							mp.string_size = 100;
						}
						if (mp.primary_key) {
							sql->primary_key.columns[column_id] = col;
							keys.push_back(mp.corona_field_name);
						}
						sql->all_fields.columns[column_id] = col;
					}
				}

				std::string backing_index_name = sql->sql_table_name + "_idx";
				std::shared_ptr<index_implementation> idx = std::make_shared<index_implementation>(backing_index_name, keys, nullptr);
				idx->set_index_class(class_name);
				indexes.insert_or_assign(backing_index_name, idx);
			}

		}

		virtual json run_method(corona_database_interface* _db, std::string& _method_name, std::string& _token, std::string& _classname, json& _target) override
		{
			json_parser jp;
			json results = jp.create_array();
			auto fldpair = fields.find(_method_name);
			if (fldpair != fields.end()) {
				auto query_field = fldpair->second;
				if (query_field->get_field_type() == field_types::ft_query) {
					results = query_field->run_method(_db, _method_name, _token, _classname, _target);
				}
			}
			return results;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _target) override
		{
			json_parser jp;
			json classes = jp.create_object();
			for (auto fldpair : fields) {
				auto query_field = fldpair.second;
				if (query_field->get_field_type() == field_types::ft_query) {
					json objects = query_field->run_queries(_db, _token, _classname, _target);
					std::string target_field_name = query_field->get_field_name();
					_target.put_member(target_field_name, objects);
					for (auto obj : objects)
					{
						std::string class_name = obj[class_name_field].as_string();
						if (not class_name.empty()) {
							if (!classes.has_member(class_name)) {
								auto classd = _db->read_lock_class(class_name);
								json classdef = jp.create_object();
								if (classd) {
									classd->get_json(classdef);
								}
								classes.put_member(class_name, classdef);
							}
						}
					}
				}
				else if (query_field->get_field_type() == field_types::ft_array || query_field->get_field_type() == field_types::ft_object) {
					std::shared_ptr<child_relations_interface> brs = query_field->get_relations();
					if (brs) {
						auto class_list = brs->get_allowed_classes();

						for (auto& class_name : class_list)
						{
							if (not class_name.empty()) {
								if (!classes.has_member(class_name)) {
									auto classd = _db->read_lock_class(class_name);
									json classdef = jp.create_object();
									if (classd) {
										classd->get_json(classdef);
									}
									classes.put_member(class_name, classdef);
								}
							}
						}
					}
				}
			}
			return classes;
		}

		virtual std::shared_ptr<xtable> find_index(corona_database_interface* _db, json& _object)  const
		{
			std::shared_ptr<xtable> index_table;
			std::shared_ptr<index_interface> matched_index;
			int max_matched_key_count = 0;

			for (auto idx : indexes)
			{
				auto& keys = idx.second->get_index_keys();

				int matched_key_count = 0;

				for (auto ikey : keys)
				{
					if (not _object.has_member(ikey))
					{
						break;
					}
					else
					{
						matched_key_count++;
					}
				}

				if (matched_key_count > max_matched_key_count)
				{
					matched_index = idx.second;
					max_matched_key_count = matched_key_count;
				}
			}

			if (matched_index) {
				index_table = matched_index->get_xtable(_db);
			}

			return index_table;
		}

		virtual	bool open(activity* _context, json definition, int64_t _location) override
		{
			json_parser jp;

			put_json(_context->errors, definition);

			std::string class_file_name = get_class_filename(_context->db);
			if (not std::filesystem::exists(class_file_name)) {
				std::string stuff = std::format("Attempt to open {0} at {1} but not created", definition[class_name_field].as_string(), class_file_name);
				throw std::logic_error(stuff);
			}

			if (_context->errors.size())
			{
				system_monitoring_interface::active_mon->log_warning(std::format("Errors on updating class {0}", class_name), __FILE__, __LINE__);
				for (auto error : _context->errors) {
					system_monitoring_interface::active_mon->log_information(std::format("{0} {1} {2}  @{3} {4}: {5} times", error.class_name, error.field_name, error.message, error.filename, error.line_number, error.count), __FILE__, __LINE__);
				}

				return false;
			}

			get_table(_context->db);
			get_full_text_index(_context->db);

			return true;
		}

		virtual bool update(activity* _context, json _changed_class) override
		{
			class_implementation changed_class;
			json_parser jp;

			changed_class.put_json(_context->errors, _changed_class);

			if (_context->errors.size())
			{
				system_monitoring_interface::active_mon->log_warning(std::format("Errors on updating class {0}", changed_class.class_name), __FILE__, __LINE__);
				for (auto error : _context->errors) {
					system_monitoring_interface::active_mon->log_information(std::format("{0} {1} {2}  @{3} {4}", error.class_name, error.field_name, error.message, error.filename, error.line_number), __FILE__, __LINE__);
				}

				return false;
			}

			if (changed_class.class_description.empty()) {
				validation_error ve;
				ve.class_name = changed_class.class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "class description not found";
				_context->errors.push_back(ve);
				return false;
			}

			if (not base_class_name.empty() and changed_class.base_class_name != base_class_name)
			{
				validation_error ve;
				ve.class_name = changed_class.class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "cannot change base class of a class.";
				_context->errors.push_back(ve);
				return false;
			}

			class_name = changed_class.class_name;
			base_class_name = changed_class.base_class_name;
			class_description = changed_class.class_description;
			field_order = changed_class.field_order;
			sql = changed_class.sql;

			ancestors.clear();

			if (not base_class_name.empty()) {

				auto base_class = _context->db->read_get_class(base_class_name);
				if (base_class and base_class->ready()) {

					ancestors = base_class->get_ancestors();
					ancestors.insert_or_assign(base_class_name, true);
					base_class->update_descendants().insert_or_assign(class_name, true);
					auto base_indeces = base_class->get_indexes();
					for (auto base_index : base_indeces) {
						auto idx_name = base_index->get_index_name();
						if (indexes.find(idx_name) == std::end(indexes)) {
							base_index = base_index->clone();
							base_index->set_index_name(idx_name);
							base_index->set_index_class(class_name);
							indexes[idx_name] = base_index;
						}
					}
					descendants.insert_or_assign(class_name, true);
					std::vector<std::string> base_fields = base_class->get_full_text_fields();
					full_text_fields.insert(full_text_fields.end(), base_fields.begin(), base_fields.end());

					// the little change I made here allows a class to inherit fields from its base class, 
					// but also to override them by redefining them in the child class.  
					// if a field is defined in the base class but not in the child class, it will be inherited.  
					// if it is defined in both, the child class's definition will be used.
					for (auto temp_field : base_class->get_fields())
					{
						if (changed_class.fields.find(temp_field->get_field_name()) == changed_class.fields.end()) {
							changed_class.fields.insert_or_assign(temp_field->get_field_name(), temp_field);
						}
					}
					_context->db->save_class(base_class.get());
				}
				else {
					validation_error ve;
					ve.class_name = changed_class.class_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = std::format("base class {0} not found", base_class_name);
					_context->errors.push_back(ve);
					return false;
				}
			}

			std::map<std::string, std::shared_ptr<index_interface>> combined_indexes;
			std::map<std::string, std::shared_ptr<field_interface>> combined_fields;
			std::map<std::string, bool> existing_table_fields;
			std::vector<std::shared_ptr<field_interface>> new_fields;

			for (auto f : changed_class.fields)
			{
				combined_fields[f.first] = f.second;
			}

			for (auto f : fields)
			{
				combined_fields[f.first] = f.second;
			}

			for (auto idx : changed_class.indexes)
			{
				combined_indexes[idx.first] = idx.second;
			}

			for (auto idx : indexes)
			{
				combined_indexes[idx.first] = idx.second;
			}

			for (auto idx : combined_indexes)
			{
				for (auto f : idx.second->get_index_keys()) {
					if (not fields.contains(f)) {
						validation_error ve;
						ve.class_name = class_name;
						ve.field_name = f;
						ve.message = "Invalid field for index";
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						_context->errors.push_back(ve);
					}
				}
			}

			indexes = combined_indexes;

			if (_context->errors.size() > 0)
			{
				return false;
			}

			auto view_descendants = descendants | std::views::filter([this](auto& pair) {
				return pair.first != class_name;
				});

			// and once again through the indexes
			// we make a copy of the existing index, so as to keep its table,
			// while at the same time not trusting this index, which was passed in.
			for (auto& new_index : indexes)
			{
				std::shared_ptr<index_interface> index_to_create;

				if (std::filesystem::exists(new_index.second->get_index_filename(_context->db)))
				{
					continue;
				}
				else
				{
					auto class_data = get_table(_context->db);
					auto idx_table = new_index.second->create_xtable(_context->db, fields);

					auto& keys = new_index.second->get_index_keys();
					date_time dt = date_time::now();
					timer tx;
					system_monitoring_interface::active_mon->log_job_section_start("index", new_index.first, dt, __FILE__, __LINE__);
					json empty_key = jp.create_object();
					class_data->for_each(nullptr, empty_key, [idx_table](json& _item) -> relative_ptr_type {
						idx_table->put(_item);
						return 1;
						});
					system_monitoring_interface::active_mon->log_job_section_stop("index:", new_index.first, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				}
			}

			_context->db->save_class(this);

			for (auto descendant : view_descendants)
			{
				auto desc_class = _context->get_class(descendant.first);
				if (desc_class) {

					json descendant_json = jp.create_object();

					desc_class->update_ancestors().insert_or_assign(class_name, true);

					if (desc_class->get_base_class_name() == class_name) {

						desc_class->get_json(descendant_json);

						for (auto nf : new_fields) {
							json jfld = jp.create_object();
							nf->get_json(jfld);
							descendant_json["fields"].share_member(nf->get_field_name(), jfld);
						}

						desc_class->update(_context, descendant_json);
					}

					_context->db->save_class(desc_class);
				}
				else
				{
					validation_error ve;
					ve.class_name = descendant.first;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "descendant class not found";
					_context->errors.push_back(ve);
					return false;
				}
			}

			alter_xtable(_context->db);

			return true;
		}

		virtual bool create(activity* _context, json _changed_class) override
		{
			put_json(_context->errors, _changed_class);

			if (_context->errors.size())
			{
				system_monitoring_interface::active_mon->log_warning(std::format("Errors on updating class {0}", class_name), __FILE__, __LINE__);
				for (auto error : _context->errors) {
					system_monitoring_interface::active_mon->log_information(std::format("{0} {1} {2}  @{3} {4}", error.class_name, error.field_name, error.message, error.filename, error.line_number), __FILE__, __LINE__);
				}

				return false;
			}

			if (class_description.empty()) {
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "class description not found";
				_context->errors.push_back(ve);
				return false;
			}

			ancestors.clear();

			if (not base_class_name.empty()) {

				auto base_class = _context->db->read_get_class(base_class_name);
				if (base_class and base_class->ready()) {

					ancestors = base_class->get_ancestors();
					ancestors.insert_or_assign(base_class_name, true);
					base_class->update_descendants().insert_or_assign(class_name, true);
					descendants.insert_or_assign(class_name, true);
					std::vector<std::string> base_fields = base_class->get_full_text_fields();
					full_text_fields.insert(full_text_fields.end(), base_fields.begin(), base_fields.end());
					for (auto temp_field : base_class->get_fields())
					{
						fields.insert_or_assign(temp_field->get_field_name(), temp_field);
					}
					for (auto temp_index : base_class->get_indexes())
					{
						std::string idx_name = temp_index->get_index_name();
						if (indexes.find(idx_name) == std::end(indexes)) {
							temp_index = temp_index->clone();
							temp_index->set_index_name(idx_name);
							temp_index->set_index_class(class_name);
							indexes[idx_name] = temp_index;
						}
					}
					_context->db->save_class(base_class.get());
				}
				else {
					validation_error ve;
					ve.class_name = class_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "base class not found or not created";
					_context->errors.push_back(ve);
					return false;
				}
			}

			auto view_descendants = descendants | std::views::filter([this](auto& pair) {
				return pair.first != class_name;
				});

			// check the indexes here, because here we have all of our fields from class ancestors.

			for (auto idx : indexes)
			{
				for (auto f : idx.second->get_index_keys()) {
					if (not fields.contains(f)) {
						validation_error ve;
						ve.class_name = class_name;
						ve.field_name = f;
						ve.message = "Invalid field for index";
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						_context->errors.push_back(ve);
					}
				}
			}

			if (_context->errors.size() > 0)
			{
				return false;
			}

			create_table(_context->db);
			for (auto idx : indexes)
			{
				idx.second->create_xtable(_context->db, fields);
			}

			return true;
		}

		virtual void put_field(std::shared_ptr<field_interface>& _new_field) override
		{
			fields.insert_or_assign(_new_field->get_field_name(), _new_field);
		}

		virtual std::shared_ptr<field_interface>		get_field(const std::string& _name)  const override
		{
			auto found = fields.find(_name);
			if (found != std::end(fields)) {
				return found->second;
			}
			return nullptr;
		}

		virtual std::shared_ptr<field_interface>		get_field_for_reference(class_interface* _class)  const override
		{
			for (auto& f : fields) {
				if (f.second->get_field_type() == field_types::ft_reference) {
					auto options = f.second->get_options();
					if (options) {
						auto ref_options = std::dynamic_pointer_cast<reference_field_options>(options);
						if (ref_options) {
							auto& desc = _class->get_ancestors();
							if (_class->get_class_name() == ref_options->reference_class || desc.contains(ref_options->reference_class)) {
								return f.second;
							}
						}
					}
				}
			}
			return nullptr;
		}

		virtual std::map<std::string, std::shared_ptr<field_interface>>& use_fields() override
		{
			return fields;
		}

		virtual std::vector<std::shared_ptr<field_interface>> get_fields()  const override
		{
			std::vector<std::shared_ptr<field_interface>> fields_list;
			for (auto fld : fields) {
				fields_list.push_back(fld.second);
			}
			return fields_list;
		}

		/// <summary>
		/// This method returns a list of field sections that are used for the user interface.  
		/// Each section is in order from the current class up to its base, and is named
		/// for its class and description.
		/// </summary>
		/// <param name="_db"></param>
		/// <returns></returns>
		virtual std::vector<std::shared_ptr<field_section>> get_fields_for_ux(corona_database_interface* _db)  const override
		{
			// go through the ancestors, and for each ancestor, get its fields, and put them in a section.  Then add the current class's fields in a section.  Return the list of sections.
			std::vector<std::shared_ptr<field_section>> sections;
			auto current_class_name = class_name;
			auto current_class = _db->read_lock_class(current_class_name);

			auto flds = get_fields();
			std::map<std::string, std::shared_ptr<field_interface>> field_map;
			for (auto f : flds) {
				field_map[f->get_field_name()] = f;
			}

			std::vector<std::shared_ptr<field_interface>> ordered_fields;

			for (auto fs : field_order) {
				if (field_map.contains(fs)) {
					ordered_fields.push_back(field_map[fs]);
					field_map.erase(fs);
				}
			}

			for (auto fs : flds) {
				if (field_map.contains(fs->get_field_name())) {
					ordered_fields.push_back(field_map[fs->get_field_name()]);
				}
			}

			while (current_class) {
				auto section = std::make_shared<field_section>();
				section->section_name = current_class->get_class_name();
				section->section_description = current_class->get_class_description();
				for (auto& fld : ordered_fields) {
					if (fld->get_field_class() == current_class->get_class_name()) {
						section->section_fields.push_back(fld);
					}
				}
				sections.push_back(section);
				auto base_class_name = current_class->get_base_class_name();
				if (base_class_name.empty()) {
					break;
				}
				current_class = _db->read_lock_class(base_class_name);
			}
			return sections;
		}

		virtual std::shared_ptr<index_interface> get_index(const std::string& _name)  const override
		{
			auto found = indexes.find(_name);
			if (found != std::end(indexes)) {
				return found->second;
			}
			return nullptr;
		}

		virtual std::vector<std::shared_ptr<index_interface>> get_indexes()  const override
		{
			std::vector<std::shared_ptr<index_interface>> indexes_list;
			for (auto fld : indexes) {
				indexes_list.push_back(fld.second);
			}
			return indexes_list;
		}

		virtual json get_object(corona_database_interface* _db, int64_t _object_id, class_permissions _grant, bool& _exists)
		{
			json_parser jp;
			json result;

			if (not std::filesystem::exists(get_class_filename(_db)))
			{
				throw std::logic_error(get_class_filename(_db) + " not found");
			}

			if (sql) {
				auto tb = get_xtable(_db);
				result = tb->get(_object_id);
				if (result.object()) {
					auto stb = get_stable(_db);
					json sqlobja = stb->get(result);
					json sqlobj = sqlobja.get_first_element();
					result.merge(sqlobj);
				}
			}
			else {
				auto tb = get_table(_db);
				json key = jp.create_object();
				key.put_member_i64(object_id_field, _object_id);
				result = tb->get(key);
			}

			if (result.object())
			{
				_exists = true;

				if (_grant.get_grant & class_grants::grant_any)
				{
					// all good
				}
				else if (_grant.get_grant == class_grants::grant_own)
				{
					if (result["created_by"].as_string() != _grant.user_name) {
						result.clear();
					}
				}
				else if (_grant.get_grant == class_grants::grant_team) {
					if (result["team"].as_string() != _grant.team_name) {
						result.clear();
					}
				}
				else if (_grant.get_grant == class_grants::grant_teamorown) {
					if (result["team"].as_string() != _grant.team_name && result["created_by"].as_string() != _grant.user_name) {
						result.clear();
					}
				}
				if (result.empty())
				{
					std::string msg = std::format("{0} does not have permissions for {1}", _grant.user_name, class_name);
					system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
				}
				else {
					auto parent_data = get_parent_objects(_grant.user_name, result, _db);
					result.put_member("parents", parent_data);
				}
			}
			else
			{
				_exists = false;
			}

			return result;
		}

		json check_single_object(compiled_object_view& _view, date_time& current_date, json& _object_definition, validation_error_collection& _validation_errors)
		{
			json_parser jp;
			using namespace std::literals;

			bool validation_ok = false;

			json result = jp.create_object();

			validation_error_collection local_errors;

			validation_ok = _view.check(_object_definition, local_errors);

			for (auto err : local_errors) {
				_validation_errors.push_back(err);
			}

			// check the object against the class definition for correctness
			// first we see which fields are in the class not in the object

			// check to make sure that we have all the fields 
			// for the index

			// then we see which fields are in the object that are not 
			// in the class definition.

			if (!validation_ok) {
				json warnings = jp.create_array();
				for (auto& ve : local_errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					warnings.push_back(jve);
				}
				result.put_member(message_field, "Failed"sv);
				result.put_member(success_field, 0);
				result.put_member("errors", warnings);
				result.share_member(data_field, _object_definition);
			}
			else {
				result.put_member(message_field, "Ok"sv);
				result.put_member(success_field, 1);
				result.share_member(data_field, _object_definition);
			}

			return result;
		}

		struct index_object_pair {
			std::shared_ptr<index_interface> index;
			json objects_to_add;
			json objects_to_delete;
		};


		class put_objects_job : public job
		{
			corona_database_interface* db;
			class_implementation* class_def;
			HANDLE job_complete_signal;
			json_parser jp;
			date_time current_date;


		public:

			json source_list;
			json child_objects, results, put_list;
			class_permissions grant;
			validation_error_collection validation_errors;
			int64_t success_object_count;
			int64_t fail_object_count;
			std::vector<index_object_pair> index_updates;

			put_objects_job(
				corona_database_interface* _db,
				class_implementation* _class_def,
				json& _source_list,
				int start_index,
				int length,
				class_permissions& _grant,
				date_time& _current_date
			)
			{
				db = _db;
				class_def = _class_def;
				grant = _grant;
				put_list = jp.create_array();
				results = jp.create_array();
				child_objects = jp.create_array();
				source_list = jp.create_array();
				current_date = date_time::now();

				for (auto& idx : class_def->get_indexes()) {
					index_object_pair iop;
					iop.index = idx;
					iop.objects_to_add = jp.create_array();
					iop.objects_to_delete = jp.create_array();
					index_updates.push_back(iop);
				}

				int e = start_index + length;
				if (e >= _source_list.size()) {
					e = _source_list.size();
				}

				for (int i = start_index; i < e; i++) {
					json el = _source_list.get_element(i);
					source_list.push_back(el);
				}

				success_object_count = 0;
				fail_object_count = 0;
				job_complete_signal = CreateEventA(NULL, FALSE, FALSE, NULL);
			}

			put_objects_job(const put_objects_job&) = delete;
			put_objects_job& operator=(const put_objects_job&) = delete;
			put_objects_job(put_objects_job&&) = delete;
			put_objects_job& operator=(put_objects_job&&) = delete;

			virtual ~put_objects_job()
			{
				CloseHandle(job_complete_signal);
			}

			void run_validation()
			{
				global_job_queue->submit_job(this);
			}

			void run_all_save()
			{
				std::vector<runnable> jobs;
				jobs.push_back([this]() { run_save(); });
				jobs.push_back([this]() { run_full_text(); });
				jobs.push_back([this]() { run_stable(); });
				jobs.push_back([this]() { run_indexes(); });
				global_job_queue->run_jobs(jobs);
			}

			virtual bool queued(job_queue* _callingQueue) override
			{
				return true;
			}

			virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success) override
			{
				job_notify jn;

				try {
					validate();
				}
				catch (const std::exception& ex)
				{
					system_monitoring_interface::active_mon->log_warning(std::format("Exception in put_objects_job: {0}", ex.what()), __FILE__, __LINE__);
				}

				jn.setSignal(job_complete_signal);
				jn.shouldDelete = false;

				return jn;
			}

			void wait_for_completion()
			{
				WaitForSingleObject(job_complete_signal, INFINITE);
			}


		private:

			void validate()
			{
				try {
					compiled_object_view compiled_object(db, class_def);

					int i = 0;
					int s = source_list.size();

					for (int i = 0; i < s; i++)
					{
						auto _src_obj = source_list.get_element(i);

						db_object_id_type object_id = -1;
						json write_object;
						json old_object;

						if (_src_obj.has_member(object_id_field))
						{

							int64_t parent_object_id = _src_obj[object_id_field].as_int64_t();

							bool exists = false;
							old_object = class_def->get_object(db, parent_object_id, grant, exists);

							if (old_object.object()) {
								write_object = old_object.clone();
								write_object.merge(_src_obj);
							}
							else if (not exists)
							{
								write_object = _src_obj;
								write_object.put_member("created", current_date);
								write_object.put_member("created_by", grant.user_name);
							}
							else
							{
								// in this case, exists == true, but old_object is empty. This means that the object
								// was there, but the user did not have permissions to change it.
								continue;
							}

							write_object.put_member("updated", current_date);
							write_object.put_member("updated_by", grant.user_name);

						}
						else
						{
							int64_t new_object_id = class_def->get_next_object_id(db);
							write_object = _src_obj;
							write_object.put_member_i64(object_id_field, new_object_id);
							write_object.put_member("created", current_date);
							write_object.put_member("created_by", grant.user_name);
							write_object.put_member("team", grant.team_name);
						}

						// if the user was a stooser and saved the query results with the object,
						// blank that out here because we will run that on load

						bool use_write_object = false;

						if (grant.put_grant == class_grants::grant_any || grant.put_grant == class_grants::grant_anyorown)
						{
							use_write_object = true;
						}
						else if (grant.put_grant == class_grants::grant_own)
						{
							std::string owner = write_object["created_by"].as_string();
							if (grant.user_name == owner) {
								use_write_object = true;
							}
						}
						else if (grant.put_grant == class_grants::grant_team)
						{
							std::string team = write_object["team"].as_string();
							if (grant.team_name == team) {
								use_write_object = true;
							}
						}
						else if (grant.put_grant == class_grants::grant_teamorown)
						{
							std::string owner = write_object["created_by"].as_string();
							std::string team = write_object["team"].as_string();
							if (grant.user_name == owner or grant.team_name == team) {
								use_write_object = true;
							}
						}

						if (use_write_object) {

							auto these_fields = class_def->get_fields();

							for (auto& fld : these_fields) {

								if (fld->get_field_type() == field_types::ft_array)
								{
									json array_field = write_object[fld->get_field_name()];
									if (array_field.array() and fld->is_relational_children()) {
										auto relations = fld->get_relations();
										if (relations) {
											for (auto obj : array_field) {
												std::string obj_class_name = obj[class_name_field].as_string();
												auto relation = relations->get_relation(obj_class_name);
												if (relation) {
													relation->copy(obj, write_object);
												}
												else {
													log_warning(std::format("No relation found for {0} in {1}", obj_class_name, fld->get_field_name()));
												}
												child_objects.push_back(obj);
											}
											write_object.erase_member(fld->get_field_name());
										}
									}
								}
								else if (fld->get_field_type() == field_types::ft_object)
								{
									json obj = write_object[fld->get_field_name()];
									if (obj.object() and fld->is_relational_children()) {
										std::string obj_class_name = obj[class_name_field].as_string();
										auto relations = fld->get_relations();
										if (relations) {
											auto relation = relations->get_relation(obj_class_name);
											if (relation) {
												relation->copy(obj, write_object);
											}
											else {
												log_warning(std::format("No relation found for {0} in {1}", obj_class_name, fld->get_field_name()));
											}
											json empty;
											write_object.erase_member(fld->get_field_name());
											child_objects.push_back(obj);
										}
									}
								}
							}

							json check_result = class_def->check_single_object(compiled_object, current_date, write_object, validation_errors);
							if (check_result[success_field].as_bool()) {
								success_object_count++;
								put_list.push_back(write_object);
								if (index_updates.size() > 0)
								{
									int64_t object_id = write_object[object_id_field].as_int64_t();
									if (old_object.object())
									{
										for (auto& iop : index_updates)
										{
											auto& idx_keys = iop.index->get_index_keys();

											json obj_to_delete = old_object.extract(idx_keys);
											json obj_to_add = write_object.extract(idx_keys);
											if (obj_to_delete.compare(obj_to_add) != 0) {
												iop.objects_to_delete.push_back(obj_to_delete);
											}
											if (obj_to_add.size() > 0) {
												iop.objects_to_add.push_back(obj_to_add);
											}
										}
									}
									else
									{
										for (auto& iop : index_updates)
										{
											auto& idx_keys = iop.index->get_index_keys();
											// check to make sure that we have all the fields 
											// for the index
											json obj_to_add = write_object.extract(idx_keys);
											iop.objects_to_add.push_back(obj_to_add);
										}
									}
								}
							}
							else {
								fail_object_count++;
							}

							results.push_back(check_result);

						}
					}
				}
				catch (std::exception exc)
				{
					system_monitoring_interface::global_mon->active_mon->log_exception(exc, __FILE__, __LINE__);
				}
			}

			void run_save()
			{
				auto tb = class_def->get_xtable(db);
				tb->put_array(put_list);
			}

			void run_full_text()
			{
				auto ftb = class_def->get_full_text_index(db);
				if (ftb) {
					json_parser jp;
					json full_text_array = jp.create_array();
					text_tokenizer ctt;

					for (auto item : put_list) {

						int64_t object_id = item[object_id_field].as_int64_t();

						std::map<std::string, int64_t> record_words;

						for (auto& item_field : class_def->full_text_fields)
						{
							auto fld = item.get_member(item_field);

							if (fld.get_field_type() == field_types::ft_string) {

								std::string temp = fld.as_string();
								auto terms = ctt.tokenize(temp);

								std::string word;
								for (auto& word : terms) {
									if (word.empty() or record_words.contains(word))
										continue;
									record_words[word] = object_id;
								}
							}
						}

						for (auto& rw : record_words) {
							xrecord key, value;
							key.add(1, rw.first);
							key.add_int64(2, rw.second);
							ftb->put_direct(key, value);
						}
					}
					ftb->commit();
				}
			}

			void run_stable()
			{
				auto stb = class_def->get_stable(db);
				if (stb) {
					stb->put_array(put_list);
				}
			}

			void run_indexes()
			{
				for (auto& iop : index_updates)
				{
					auto idx_table = iop.index->get_xtable(db);
					idx_table->erase_array(iop.objects_to_delete);
					idx_table->put_array(iop.objects_to_add);
				}
			}


		};

		const int objects_per_thread_size = 500;

		virtual json put_objects(corona_database_interface* _db, json& _child_objects, json& _src_list, class_permissions _grant, validation_error_collection& validation_errors, int64_t& success_object_count, int64_t& fail_object_count) override
		{
			bool index_ready = true;

			json_parser jp;
			json results = jp.create_array();

			date_time current_date = date_time::now();

			int total_objects = (int)_src_list.size();

			std::vector<std::shared_ptr<put_objects_job>> jobs;

			int i = 0;

			while (i < total_objects) {
				std::shared_ptr<put_objects_job> current_job = std::make_shared<put_objects_job>(_db,
					this,
					_src_list,
					i,
					objects_per_thread_size,
					_grant,
					current_date
				);
				jobs.push_back(current_job);
				current_job->run_validation();
				i += objects_per_thread_size;
			}

			for (auto& ptr : jobs)
			{
				ptr->wait_for_completion();
			}

			for (auto& ptr : jobs)
			{
				ptr->run_all_save();
				for (auto& ve : ptr->validation_errors) {
					validation_errors.push_back(ve);
				}
				success_object_count += ptr->success_object_count;
				fail_object_count += ptr->fail_object_count;
			}

			for (auto& ptr : jobs)
			{
				for (int i = 0; i < ptr->child_objects.size(); i++) {
					json obj_to_add = ptr->child_objects.get_element(i);
					_child_objects.push_back(obj_to_add);
				}

				for (int i = 0; i < ptr->results.size(); i++) {
					json obj_to_add = ptr->results.get_element(i);
					results.push_back(obj_to_add);
				}
			}

			return results;
		}

		template <typename object_type, field_types ft, typename options_interface_type>
		std::shared_ptr<object_type> get_field_object(json _src, const std::string& _name)
		{
			std::shared_ptr<object_type> cf;

			auto cfi = fields.find(_name);

			if (cfi != std::end(fields)) {
				if (cfi->second->get_field_type() == ft) {
					auto existing_field_options = cfi->second->get_options();
					auto field_options = std::dynamic_pointer_cast<options_interface_type>(existing_field_options);
					if (field_options) {
						json field_data = _src[_name];
						if (field_data.object()) {
							if constexpr (std::is_base_of_v<rectangle, object_type>) {
								cf = std::make_shared<rectangle>();
							}
							else
							{
								cf = std::make_shared<object_type>(field_options, field_data);
							}
						}
						else {
							throw std::logic_error(std::format("field {0} data is not an array", _name));
						}
					}
					else {
						throw std::logic_error(std::format("field {0} does not have {1} field options", _name, field_type_names[ft]));
					}
				}
				else {
					throw std::logic_error(std::format("field {0} is not a {1} field", _name, field_type_names[ft]));
				}
			}
			else {
				throw std::logic_error(std::format("field {0} not found in class {1}", _name, class_name));
			}

			return cf;
		}

		virtual std::shared_ptr<chest_field> get_chest(json _src, const std::string& _name) override
		{
			std::shared_ptr<chest_field> cf;

			cf = get_field_object<chest_field, field_types::ft_chest, chest_field_options_interface>(_src, _name);

			return cf;
		}

		virtual std::shared_ptr<selection_field> get_selection(json _src, const std::string& _name) override
		{
			std::shared_ptr<selection_field> cf;

			cf = get_field_object<selection_field, field_types::ft_selection, selection_field_options_interface>(_src, _name);

			return cf;
		}

		virtual std::shared_ptr<pathDto> get_path(json _src, const std::string& _name) override
		{
			std::shared_ptr<pathDto> cf;

			cf = get_field_object<pathDto, field_types::ft_path, path_field_options_interface>(_src, _name);

			return cf;
		}

		virtual std::shared_ptr<generalBrushRequest> get_brush(json _src, const std::string& _name) override
		{
			std::shared_ptr<generalBrushRequest> cf;

			cf = get_field_object<generalBrushRequest, field_types::ft_brush, brush_field_options_interface>(_src, _name);

			return cf;
		}

		virtual std::shared_ptr<bitmapInstanceDto> get_bitmap(json _src, const std::string& _name) override
		{
			std::shared_ptr<bitmapInstanceDto> cf;

			cf = get_field_object<bitmapInstanceDto, field_types::ft_bitmap, bitmap_field_options_interface>(_src, _name);

			return cf;
		}

		template <typename factory_type, typename object_type, field_types ft, typename options_interface_type>
		object_type get_field_object_factory(json _src, const std::string& _name)
		{
			object_type cf;

			auto cfi = fields.find(_name);

			if (cfi != std::end(fields)) {
				if (cfi->second->get_field_type() == ft) {
					auto field_options = cfi->second->get_options();
					auto chest_field_options = std::dynamic_pointer_cast<options_interface_type>(field_options);
					if (chest_field_options) {
						json chest_data = _src[_name];
						if (chest_data.object()) {
							cf = factory_type::from_json(chest_data);
						}
						else
						{
							throw std::logic_error(std::format("field {0} data is not an array", _name));
						}
					}
					else
					{
						throw std::logic_error(std::format("field {0} does not have {1} field options", _name, field_type_names[ft]));
					}
				}
				else
				{
					throw std::logic_error(std::format("field {0} is not a {1} field", _name, field_type_names[ft]));
				}
			}
			else
			{
				throw std::logic_error(std::format("field {0} not found in class {1}", _name, class_name));
			}

			return cf;
		}

		virtual audio_function get_audio(json _src, const std::string& _name) override
		{
			audio_function cf;

			cf = get_field_object_factory<audio_graph, audio_function, field_types::ft_audio, audio_field_options_interface>(_src, _name);

			return cf;
		}

		template <typename object_type, field_types ft, typename field_options_interface> void put_field_object(json& _dest, const std::string& _name, std::shared_ptr<object_type>& _src)
		{
			auto cfi = fields.find(_name);

			if (cfi != std::end(fields)) {
				if (cfi->second->get_field_type() == ft) {
					auto field_options = cfi->second->get_options();
					auto specific_field_options = std::dynamic_pointer_cast<field_options_interface>(field_options);
					if (specific_field_options) {
						json_parser jp;
						json field_data = jp.create_object();
						_src->get_json(field_data);
						_dest.put_member(_name, field_data);
					}
					else {
						throw std::logic_error(std::format("field {0} does not have {1} field options", _name, field_type_names[ft]));
					}
				}
				else {
					throw std::logic_error(std::format("field {0} is not a {1} field", _name, field_type_names[ft]));
				}
			}
			else {
				throw std::logic_error(std::format("field {0} not found in class {1}", _name, class_name));
			}
		}

		template <typename object_type, field_types ft, typename field_options_interface> void put_field_object(json& _dest, const std::string& _name, object_type& _src)
		{
			auto cfi = fields.find(_name);

			if (cfi != std::end(fields)) {
				if (cfi->second->get_field_type() == ft) {
					auto field_options = cfi->second->get_options();
					auto specific_field_options = std::dynamic_pointer_cast<field_options_interface>(field_options);
					if (specific_field_options) {
						json_parser jp;
						json field_data = jp.create_object();
						_src.get_json(field_data);
						_dest.put_member(_name, field_data);
					}
					else {
						throw std::logic_error(std::format("field {0} does not have {1} field options", _name, field_type_names[ft]));
					}
				}
				else {
					throw std::logic_error(std::format("field {0} is not a {1} field", _name, field_type_names[ft]));
				}
			}
			else {
				throw std::logic_error(std::format("field {0} not found in class {1}", _name, class_name));
			}
		}

		virtual void put_chest(json& _dest, const std::string& _name, std::shared_ptr<chest_field>& _src) override
		{
			put_field_object<chest_field, field_types::ft_chest, chest_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_selection(json& _dest, const std::string& _name, std::shared_ptr<selection_field>& _src) override
		{
			put_field_object<selection_field, field_types::ft_selection, selection_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_path(json& _dest, const std::string& _name, std::shared_ptr<pathDto>& _src) override
		{
			put_field_object<pathDto, field_types::ft_path, path_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_brush(json& _dest, const std::string& _name, std::shared_ptr<generalBrushRequest>& _src) override
		{
			put_field_object<generalBrushRequest, field_types::ft_brush, brush_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_bitmap(json& _dest, const std::string& _name, std::shared_ptr<bitmapInstanceDto>& _src) override
		{
			put_field_object<bitmapInstanceDto, field_types::ft_bitmap, bitmap_field_options_interface>(_dest, _name, _src);
		}


		virtual void put_chest(json& _dest, const std::string& _name, chest_field& _src) override
		{
			put_field_object<chest_field, field_types::ft_chest, chest_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_selection(json& _dest, const std::string& _name, selection_field& _src) override
		{
			put_field_object<selection_field, field_types::ft_selection, selection_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_path(json& _dest, const std::string& _name, pathDto& _src) override
		{
			put_field_object<pathDto, field_types::ft_path, path_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_brush(json& _dest, const std::string& _name, generalBrushRequest& _src) override
		{
			put_field_object<generalBrushRequest, field_types::ft_brush, brush_field_options_interface>(_dest, _name, _src);
		}

		virtual void put_bitmap(json& _dest, const std::string& _name, bitmapInstanceDto& _src) override
		{
			put_field_object<bitmapInstanceDto, field_types::ft_bitmap, bitmap_field_options_interface>(_dest, _name, _src);
		}


		virtual json get_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant)
		{
			// Now, if there is an index set specified, let's go see if we can find one and use it 
			// rather than scanning the table

			json_parser jp;
			json obj;
			obj = jp.create_array();

			if (!table)
			{
				return obj;
			}

			if (_key.has_member("full_text"))
			{

				std::string full_text = _key["full_text"].as_string();

				if (full_text.size() > 0)
				{

					auto ftb = get_full_text_index(_db);
					if (ftb) {
						text_tokenizer ctt;
						auto terms = ctt.tokenize(full_text);
						std::set<int64_t> found_object_ids, join_result;

						bool first_word = true;

						json ft_key = jp.create_object();
						if (terms.size() > 0) {

							auto word = terms[0];
							ft_key.put_member("text", word);

							json* pobj = &obj;

							json ft_results = ftb->select(nullptr, ft_key, [&terms, &_grant, &_db, pobj, this](json& _item) -> json {
								int64_t object_id = _item["object_id"].as_int64_t();
								bool exists;
								json temp = get_object(_db, object_id, _grant, exists);
								if (temp.object()) {
									bool found = true;
									for (auto& tword : terms) {
										bool found_word = false;
										for (auto& ftx : full_text_fields) {
											std::string str = temp[ftx].as_string();
											std::transform(str.begin(), str.end(), str.begin(), ::tolower);
											bool result = str.find(tword) != std::string::npos;
											if (result) {
												found_word = true;
												break;
											}
										}
										if (found_word == false) {
											found = false;
											break;
										}
									}
									if (found && pobj->size() < max_query_result_rows) {
										pobj->push_back(temp);
									}
								}
								json emptio;
								return emptio;
								});

						}

					}
				}
				else {
					auto class_data = get_table(_db);
					int max_rows = max_query_result_rows;
					obj = class_data->select(&max_rows, _key, [&full_text, &_key, &_grant](json& _j)-> json
						{
							json result;
							auto members = _j.get_members();
							for (auto& mem : members) {
								std::string str = mem.second.as_string();
								if (str.find(full_text) != std::string::npos) {
									if (_grant.get_grant & class_grants::grant_any) {
										result = _j;
										break;
									}
									else if ((_grant.get_grant & class_grants::grant_own) && _j["created_by"].as_string() == _grant.user_name) {
										result = _j;
										break;
									}
									else if ((_grant.get_grant & class_grants::grant_team) && _j["team"].as_string() == _grant.team_name) {
										result = _j;
										break;
									}
								}
							}
							return result;
						});
				}
			}
			else if (_key.has_member(object_id_field)) {
				int64_t object_id = _key[object_id_field].as_int64_t();
				bool exists;
				json temp = get_object(_db, object_id, _grant, exists);
				obj.push_back(temp);
			}
			else
			{
				auto index_table = find_index(_db, _key);
				if (index_table)
				{
					json temp;
					int max_rows = max_query_result_rows;

					temp = index_table->select(&max_rows, _key, [](json& _item) -> json {
						return _item;
						});

					int i = 0;
					int s = temp.size();
					for (i = 0; i < s; i++) {
						if (obj.size() >= max_query_result_rows) {
							break;
						}
						json obi = temp.get_element(i);
						if (_key.compare(obi) != 0) {
							continue;
						}
						int64_t object_id = obi[object_id_field].as_int64_t();
						json bojdetail = _db->select_object(class_name, object_id, _grant);
						json detail = bojdetail.get_first_element();
						obj.push_back(detail);
					}

				}
				else
				{
					auto class_data = get_table(_db);
					int max_rows = max_query_result_rows;
					obj = class_data->select(&max_rows, _key, [&_key, &_grant](json& _j)-> json
						{
							json select_result;
							if (_key.compare(_j) == 0) {
								if (_grant.get_grant & class_grants::grant_any) {
									select_result = _j;
								}
								else if ((_grant.get_grant & class_grants::grant_own) && _j["created_by"].as_string() == _grant.user_name) {

									select_result = _j;
								}
								else if ((_grant.get_grant & class_grants::grant_team) && _j["team"].as_string() == _grant.team_name) {

									select_result = _j;
								}
							}
							return select_result;
						});
				}
			}

			if (_include_children) {
				auto obj_items = obj.array_impl();
				for (auto item : *obj_items)
				{
					json _src_obj(item);
					for (auto& fpair : fields) {
						auto& fld = fpair.second;

						if (fld->is_relational_children()) {
							if (fld->get_field_type() == field_types::ft_array)
							{
								auto relations = fld->get_relations();
								if (relations) {
									json results = relations->get_children(_db, _src_obj, _grant);
									_src_obj.put_member(fld->get_field_name(), results);
								}
							}
							else if (fld->get_field_type() == field_types::ft_object)
							{
								auto relations = fld->get_relations();
								if (relations) {
									json results = relations->get_children(_db, _src_obj, _grant);
									json first = results.get_first_element();
									_src_obj.put_member(fld->get_field_name(), first);
								}
							}
						}

						// we don't run the query at this level because we do not have the token here to do so.  happens up after this call completes
					}
				}
			}

			// since we got the objects from sql, we want to now get them from
			if (sql)
			{
				auto backing_table = get_xtable(_db);
				json idx_search = jp.create_object();
				std::vector<std::string> key_field_names;
				for (auto& s : sql->primary_key.columns) {
					std::string name = std::string(s.second.field_name);
					idx_search.put_member(name, name);
					key_field_names.push_back(name);
				}

				auto index_table = find_index(_db, idx_search);

				for (auto ob : obj)
				{
					json key = ob.extract(key_field_names);
					json ob_found;

					int max_rows = max_query_result_rows;

					if (index_table)
					{
						ob_found = index_table->select(&max_rows, key, [this, &backing_table](json& _item) -> json {
							int64_t object_id = _item[object_id_field].as_int64_t();
							auto objfound = backing_table->get(object_id);
							return objfound;
							});
					}
					else
					{
						ob_found = backing_table->select(&max_rows, key, [&key](json& _j)
							{
								json result;
								if (key.compare(_j) == 0)
									result = _j;
								return result;

							});
					}

					int64_t object_id = -1;
					json old_value;

					if (ob_found.array())
					{
						old_value = ob_found.get_first_element();
						if (old_value.object()) {
							object_id = old_value[object_id_field].as_int64_t();
						}
						else
						{
							object_id = table->get_next_object_id();
						}
					}
					else
					{
						object_id = table->get_next_object_id();
					}

					ob.put_member_i64(object_id_field, object_id);
					backing_table->put(ob);
					if (old_value.compare(ob) != 0)
					{
						for (auto idx : indexes)
						{
							auto idx_table = idx.second->get_xtable(_db);
							if (not old_value.empty()) {
								idx_table->erase(old_value);
							}
							idx_table->put(ob);
						}
					}
				}
			}

			return obj;
		}

		virtual json get_single_object(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) override
		{
			json temp = get_objects(_db, _key, _include_children, _grant);
			return temp.get_first_element();
		}

		virtual json delete_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _permission)
		{
			json_parser jp;
			json child_objects = jp.create_array();
			json matching_objects = get_objects(_db, _key, _include_children, _permission);
			auto tb = get_table(_db);

			for (auto _src_obj : matching_objects)
			{
				if ((_permission.delete_grant == class_grants::grant_any)
					or ((_permission.delete_grant == class_grants::grant_own || _permission.delete_grant == class_grants::grant_teamorown) and _src_obj["created_by"].as_string() == _permission.user_name)
					or ((_permission.delete_grant == class_grants::grant_team || _permission.delete_grant == class_grants::grant_teamorown) and _src_obj["team"].as_string() == _permission.team_name))
				{
					tb->erase(_src_obj);

					for (auto& fpair : fields) {
						auto& fld = fpair.second;
						if (not fld->is_relational_children()) {
							continue;
						}
						auto relations = fld->get_relations();
						relations->delete_children(_db, _src_obj, _permission);
					}
				}
			}
			return matching_objects;
		}

		virtual json	get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			json definition = jp.create_object();
			json properties = jp.create_object();
			json required = jp.create_array();

			definition.put_member("description", get_class_description());
			definition.put_member("type", std::string("object"));

			for (auto fld : fields) {
				auto field = fld.second;
				json field_definition = field->get_openapi_schema(_db);
				properties.share_member(field->get_field_name(), field_definition);

				auto options = field->get_options();
				if (options and options->is_required()) {
					required.push_back(field->get_field_name());
				}
			}

			definition.share_member("properties", properties);

			return definition;
		}
	};

}
