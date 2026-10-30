#pragma once

namespace corona
{

	class from_join
	{
	public:
		std::string join_to;
		json		joined_by;

		json get_json()
		{
			json_parser jp;
			json result = jp.create_object();
			result.put_member("join_to", join_to);
			result.put_member("join_by", joined_by);
			return result;
		}

		void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "join_to", "join_by" })) {
				system_monitoring_interface::active_mon->log_warning("join missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
			}

			join_to = _src["join_to"].as_string();
			joined_by = _src["join_by"];
		}
	};

	class from_source
	{
	public:
		int													index;
		std::string											source_name;
		std::string											class_name;
		json												data;
		std::vector<std::shared_ptr<from_join>>				replacements;
		std::shared_ptr<from_join>							join_filter;
		json                                                filter;
		std::string											user_name;
		bool	                                            use_fetch;

		json replace_set(corona_database_interface* _db, json& _previous_item, int _filter_index)
		{
			json_parser jp;
			json		result = jp.create_array();

			if (_filter_index < replacements.size())
			{
				auto impl = _previous_item.array_impl();
				if (impl)
				{
					for (auto& item : *impl)
					{
						std::shared_ptr<json_object> jo;
						jo = std::dynamic_pointer_cast<json_object>(item);
						if (jo.get() == nullptr)
						{
							continue;
						}
						auto replacement = replacements[_filter_index];
						json filter_request = jp.create_object();
						json filter = jp.create_object();

						for (auto member : replacement->joined_by.get_members())
						{
							std::string src_field = member.first;
							std::string dst_field = member.second.as_string();
							json src = (*jo)[src_field];
							filter.put_member(dst_field, src);
						}
						filter_request.put_member("class_name", replacement->join_to);
						filter_request.put_member("filter", filter);
						json apply_result = _db->query_class(user_name, filter);
						if (apply_result[success_field].as_bool()) {
							json result_data = apply_result["data"];
							if (result_data.array())
							{
								for (auto r : result_data)
								{
									result.push_back(r);
								}
							}
						}
					}

					result = replace_set(_db, result, _filter_index + 1);
				}
			}
			else
			{
				result = _previous_item;
			}
			return result;
		}

		json fetch(corona_database_interface* _src, std::string _user_name, json _filter)
		{
			json result = _src->query_class(_user_name, _filter);

			data = result["data"];
			data = replace_set(_src, data, 0);

			return data;
		}
	};

	using from_sources = std::map<std::string, std::shared_ptr<from_source>>;

	class query_from
	{
		std::vector<std::shared_ptr<from_source>> ordered_sources;
		from_sources			sources;
		json					dest_array;
		int						join_index = 0;
		std::string             user_name;

	public:

		query_from(std::string _user_name, json from_classes)
		{
			json_parser jp;
			user_name = _user_name;

			// these run in order to allow for dependencies
			for (auto from_class : from_classes)
			{
				std::string from_class_name = from_class[class_name_field].as_string();
				std::string filter_source_name = from_class["name"].as_string();
				json data = from_class[data_field];

				std::shared_ptr<from_source> new_source = std::make_shared<from_source>();
				new_source->index = join_index++;
				new_source->source_name = filter_source_name;
				new_source->class_name = from_class_name;
				json jjoin = from_class["filter"];

				if (jjoin.object()) {
					if (jjoin.has_member("join_to")) {
						std::shared_ptr<from_join> new_join = std::make_shared<from_join>();
						new_join->put_json(jjoin);
						new_source->join_filter = new_join;
					}
					else {
						new_source->filter = jjoin;
					}
				}
				new_source->user_name = user_name;
				if (from_class.has_member("replace"))
				{
					json follows = from_class["replace"];
					for (auto replacements : follows)
					{
						std::shared_ptr<from_join> new_follow = std::make_shared<from_join>();
						new_follow->put_json(replacements);
						new_source->replacements.push_back(new_follow);
					}
				}

				// allow the query to have inline objects
				if (data.object())
				{
					new_source->use_fetch = false;
					new_source->data = jp.create_array();
					new_source->data.push_back(data);
				}
				else if (data.array())
				{
					new_source->use_fetch = false;
					new_source->data = data;
				}
				else if (data.empty())
				{
					new_source->use_fetch = true;
				}
				sources.insert_or_assign(filter_source_name, new_source);
				ordered_sources.push_back(new_source);
			}
		}

		json run_class(query_context* _dest,
			corona_database_interface* _db,
			std::shared_ptr<from_source>& _source)
		{
			json_parser jp;
			json result = jp.create_array();
			json join_filter = jp.create_object();

			json filter_request = jp.create_object();
			filter_request.put_member("class_name", _source->class_name);

			if (!_source->filter.empty()) {
				auto filter_impl = _source->filter.object_impl();
				auto& filter_members = *filter_impl;
				for (auto& member : filter_members)
				{
					std::string value = member.second->to_string();
					if (value.starts_with("$")) {

						std::string source_path = value.substr(1);

						json jxquery = jp.parse_query(source_path);

						std::string source_name = jxquery["source_name"].as_string();
						std::string query_path = jxquery["query_path"].as_string();
						auto source_it = sources.find(source_name);

						if (source_it != sources.end()) {
							json source_data = source_it->second->data;
							if (source_data.array() && source_data.size() > 0) {
								source_data = source_data.get_element(0);
								json result = source_data.query(query_path);
								member.second = result["value"].value();
							}
						}
						else
						{
							system_monitoring_interface::active_mon->log_warning(std::format("Source {} not found for filter replacement", source_name), __FILE__, __LINE__);
						}
					}
				}
				join_filter = _source->filter;
				filter_request.put_member("filter", _source->filter);
			}
			else if (_source->join_filter)
			{
				auto joined_by_members = _source->join_filter->joined_by.get_members();
				auto join_source = sources.find(_source->join_filter->join_to);
				if (join_source == sources.end())
				{
					system_monitoring_interface::active_mon->log_warning(std::format("Join source {} not found", _source->join_filter->join_to), __FILE__, __LINE__);
				}
				else
				{
					json _src = join_source->second->data;
					if (_src.empty())
					{
						system_monitoring_interface::active_mon->log_warning(std::format("Join source {} has no data", _source->join_filter->join_to), __FILE__, __LINE__);
					}
					if (_src.array() && _src.size() > 0)
					{
						_src = _src.get_element(0);
					}
					for (auto member : joined_by_members)
					{
						std::string src_field = member.first;
						std::string dst_field = member.second.as_string();
						json src = _src[src_field];
						join_filter.put_member(dst_field, src);
					}
				}
			}
			else
			{
				join_filter = jp.create_object();
			}

			if (_source->use_fetch)
			{
				if (not join_filter.empty())
				{
					filter_request.put_member("filter", join_filter);
				}
				result = _source->fetch(_db, user_name, filter_request);
			}
			else if (join_filter.empty())
			{
				result = _source->data;
			}
			else
			{
				result = _source->data.filter([&join_filter](json& _item)->bool {
					return join_filter.compare(_item) == 0;
					});
			}

			_dest->set_data_source(_source->source_name, result);

			return result;
		}

		void run(query_context* _dest, corona_database_interface* _db)
		{
			json_parser jp;

			for (auto& os : ordered_sources)
			{
				run_class(_dest, _db, os);
			}
		}

	};


	using class_method_key = std::tuple<std::string, std::string>;

	class field_options_base : public field_options_interface
	{
	public:


		field_options_base() = default;
		field_options_base(const field_options_base& _src) = default;
		field_options_base(field_options_base&& _src) = default;
		field_options_base& operator = (const field_options_base& _src) = default;
		field_options_base& operator = (field_options_base&& _src) = default;
		virtual ~field_options_base() = default;

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)  override
		{
			;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object)  override
		{
			json empty;
			return empty;
		}

		virtual json run_method(corona_database_interface* _db, std::string _method_name, std::string& _token, std::string& _class_name, json& _object)  override
		{
			json empty;
			return empty;
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)  override
		{
			bool is_empty = _object_to_test.empty();
			if (required and is_empty) {
				validation_error ve;
				ve.class_name = _class_name;
				ve.field_name = _field_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "required field";
				_validation_errors.push_back(ve);
				return false;
			};
			return true;

		}

		virtual std::shared_ptr<child_relations_interface> get_relations() override
		{
			return nullptr;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("string"));
			return schema;
		}

		virtual bool is_required() override
		{
			return required;
		}

		virtual bool is_server_only() override
		{
			return server_only;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual std::vector<std::string> get_allowed_classes() override
		{
			std::vector<std::string> results;
			auto relations = get_relations();
			if (relations) {
				for (auto item : relations->get_allowed_classes()) {
					item = unpad(item);
					results.push_back(item);
				}
			}
			return results;
		}


	};

	class child_relation_implementation : public child_relation_interface
	{
		std::string							child_class_name;
		json								copy_values;

	public:

		virtual void set_class_name(std::string _class_name) override
		{
			child_class_name = _class_name;
		}

		virtual std::string get_class_name() override
		{
			return child_class_name;
		}

		virtual void get_json(json& _dest)  override
		{
			_dest.put_member("child_class_name", child_class_name);
			_dest.put_member("copy_values", copy_values);
		}

		virtual void put_json(json& _src)  override
		{
			child_class_name = _src["child_class_name"].as_string();
			copy_values = _src["copy_values"];
		}

		virtual void put_child_object(std::string& _child_class_name, json& _copy_values)
		{
			child_class_name = _child_class_name;
			copy_values = _copy_values;
		}

		virtual void copy(json& _dest, json& _src)  override
		{
			if (copy_values.object()) {
				auto members = copy_values.get_members();

				for (auto member : members)
				{
					std::string _dest_key = member.first;
					std::string _src_key = member.second.as_string();
					if (_src_key == reference_field) {
						object_reference dest;
						dest.class_name = _src[class_name_field].as_string();
						dest.object_id = _src[object_id_field].as_int64_t();
						_dest.put_member(_dest_key, dest);
						continue;
					}
					json value = _src[_src_key];
					_dest.put_member(_dest_key, value);
				}
			}
		}

		virtual json get_key(json& _src) override
		{
			json_parser jp;
			json key;
			key = jp.create_object();
			auto members = copy_values.get_members();

			for (auto member : members)
			{
				std::string dest_key = member.first;
				std::string src_key = member.second.as_string();
				json v = _src[src_key];
				key.put_member(dest_key, v);
			}

			key.set_natural_order();

			return key;
		}

		virtual json get_copy_assignment() override
		{
			return copy_values;
		}
	};

	class child_relations : public child_relations_interface
	{
	public:
		std::map<std::string, std::shared_ptr<child_relation_interface>> base_constructors;
		std::map<std::string, std::shared_ptr<child_relation_interface>> all_constructors;

		virtual void get_json(json& _dest) override
		{
			json_parser jp;
			for (auto ctor : base_constructors) {
				json obj = jp.create_object();
				ctor.second->get_json(obj);
				_dest.share_member(ctor.first, obj);
			}
		}

		virtual void put_child_object(child_object_definition& _cod)
		{
			base_constructors.clear();
			all_constructors.clear();
			if (_cod.fundamental_type != field_types::ft_none)
				return;
			for (auto class_def : _cod.child_classes) {
				std::shared_ptr<child_relation_implementation> new_relation = std::make_shared<child_relation_implementation>();
				json_parser jp;
				json copy_values = jp.create_object();
				for (auto pair : class_def->copy_values)
				{
					copy_values.put_member(pair.first, pair.second);
				}
				new_relation->put_child_object(class_def->class_name, copy_values);
				new_relation->set_class_name(class_def->class_name);
				base_constructors.insert_or_assign(class_def->class_name, new_relation);
			}
		}

		virtual void put_json(json& _src) override
		{
			base_constructors.clear();
			all_constructors.clear();
			auto members = _src.get_members();
			for (auto member : members) {
				json obj = member.second;
				std::shared_ptr<child_relation_implementation> new_relation = std::make_shared<child_relation_implementation>();
				std::string class_name = member.first;
				new_relation->put_json(obj);
				new_relation->set_class_name(class_name);
				base_constructors.insert_or_assign(class_name, new_relation);
			}
		}

		virtual std::shared_ptr<child_relation_interface> get_relation(std::string _class_name) override
		{
			std::shared_ptr<child_relation_interface> result;
			auto iter = all_constructors.find(_class_name);
			if (iter != std::end(all_constructors))
				result = iter->second;
			return result;
		}

		virtual std::vector<std::string> get_allowed_classes() override
		{
			std::vector<std::string> results;
			for (auto item : all_constructors) {
				results.push_back(item.first);
			}
			return results;

		}


		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			all_constructors.clear();
			for (auto class_name_pair : base_constructors) {
				auto descendants = _db->get_class_descendants(class_name_pair.first);
				for (auto descendant : descendants) {
					all_constructors.insert_or_assign(descendant, class_name_pair.second);
				}
			}
		}

		virtual json get_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) override
		{
			json_parser jp;
			json result_array = jp.create_array();

			// This needs to get the children if they are of a derived class too.
			std::map<std::string, bool> read_classes;

			for (auto class_name_pair : all_constructors)
			{
				json key = class_name_pair.second->get_key(_parent_object);
				auto derived_classes = _db->get_class_descendants(class_name_pair.first);

				for (auto derived_class : derived_classes) {
					if (read_classes.find(derived_class) != read_classes.end()) {
						continue;
					}
					read_class_sp dclassy = _db->read_lock_class(derived_class);
					if (dclassy) {
						read_classes[derived_class] = true;
						json temp_array = dclassy->get_objects(_db, key, true, _permissions);
						if (temp_array.array()) {
							for (auto obj : temp_array) {
								result_array.push_back(obj);
							}
						}
					}
				}
			}

			return result_array;
		}

		virtual json delete_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) override
		{
			json_parser jp;
			json result_array = jp.create_array();

			// TODO.  This needs to delete the children if they are of a derived class too.

			for (auto class_name_pair : all_constructors)
			{
				json key = class_name_pair.second->get_key(_parent_object);
				read_class_sp classy = _db->read_lock_class(class_name_pair.first);
				if (classy) {
					classy->delete_objects(_db, key, true, _permissions);
				}
			}

			return result_array;
		}


		virtual bool empty()
		{
			return all_constructors.size() == 0;
		}

	};

	/// <summary>
	/// models the jsony idea that an array can be of a fundamental type or objects
	/// </summary>
	class array_field_options : public field_options_base
	{
	public:

		std::shared_ptr<child_relations> relations;
		field_types fundamental_type;

		array_field_options() = default;
		array_field_options(const array_field_options& _src) = default;
		array_field_options(array_field_options&& _src) = default;
		array_field_options& operator = (const array_field_options& _src) = default;
		array_field_options& operator = (array_field_options&& _src) = default;
		virtual ~array_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;

			if (relations) {
				json jctors = jp.create_object();
				relations->get_json(jctors);
				_dest.share_member("relations", jctors);
			}
			if (field_type_names.contains(fundamental_type)) {
				_dest.put_member("fundamental_type", field_type_names[fundamental_type]);
			}
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);

			json jctors = _src["relations"];
			std::string fte = _src["fundamental_type"].as_string();
			if (allowed_field_types.contains(fte)) {
				fundamental_type = allowed_field_types[fte];
			}

			relations = std::make_shared<child_relations>();
			if (jctors.object()) {
				relations->put_json(jctors);
			}
		}

		virtual void put_definition(child_object_definition& _cod)
		{
			fundamental_type = _cod.fundamental_type;
			relations = std::make_shared<child_relations>();
			relations->put_child_object(_cod);
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			if (relations)
				relations->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				if (_object_to_test.array())
				{
					for (auto obj : _object_to_test)
					{
						std::string object_class_name;

						if (fundamental_type == field_types::ft_object || fundamental_type == field_types::ft_array)
						{
							return true;
						}
						else if (fundamental_type == field_types::ft_string)
						{
							bool acceptable = obj.is_string();
							if (not acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Element must be a string.";
								_validation_errors.push_back(ve);
								return false;
							}
						}
						else if (fundamental_type == field_types::ft_int64)
						{
							bool acceptable = obj.is_int64();
							if (not acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Element must be a int64.";
								_validation_errors.push_back(ve);
								return false;
							}
						}
						else if (fundamental_type == field_types::ft_double)
						{
							bool acceptable = obj.is_double();
							if (not acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Element must be a double.";
								_validation_errors.push_back(ve);
								return false;
							}
						}
						else if (fundamental_type == field_types::ft_datetime)
						{
							bool acceptable = obj.is_int64();
							if (not acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Element must be a datetime.";
								_validation_errors.push_back(ve);
								return false;
							}
						}
						else if (fundamental_type == field_types::ft_vector)
						{
							bool acceptable = obj.is_vector();
							if (not acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Element must be a vector.";
								_validation_errors.push_back(ve);
								return false;
							}
						}
						else if (relations)
						{
							if (obj.object()) {
								object_class_name = obj[class_name_field].as_string();
								auto ctor = relations->get_relation(object_class_name);
								if (not ctor) {
									validation_error ve;
									ve.class_name = _class_name;
									ve.field_name = _field_name;
									ve.filename = get_file_name(__FILE__);
									ve.line_number = __LINE__;
									if (object_class_name.empty()) {
										ve.message = "child objects missing class_name.";
									}
									else {
										ve.message = "invalid child object class '" + object_class_name + "'";
									}
									_validation_errors.push_back(ve);
									return false;
								}
							}
							else {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "elements of this array must be objects.";
								_validation_errors.push_back(ve);
								return false;

							}
						}
					}
					return true;
				}
				else
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an array for an array field.";
					_validation_errors.push_back(ve);
					return false;
				}
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return fundamental_type == field_types::ft_none;
		}

		virtual std::shared_ptr<child_relations_interface> get_relations() override
		{
			return relations;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("array"));

			json jitems = jp.create_object();
			json joneof = jp.create_array();
			for (auto& bc : relations->base_constructors) {
				json joneofi = jp.create_object();
				joneofi.put_member("$ref", "#/components/schemas/" + bc.first);
				joneof.push_back(joneofi);
			}

			json joneofi = jp.create_object();
			switch (fundamental_type)
			{
			case field_types::ft_vector:
				joneofi.put_member_string("type", "array");
				break;
			case field_types::ft_datetime:
			case field_types::ft_string:
				joneofi.put_member_string("type", "string");
				break;
			case field_types::ft_double:
				joneofi.put_member_string("type", "number");
				break;
			case field_types::ft_int64:
				joneofi.put_member_string("type", "integer");
				break;
			}
			if (joneofi.has_member("type")) {
				joneof.push_back(joneofi);
			}


			jitems.share_member("oneOf", joneof);
			schema.share_member("items", jitems);

			return schema;
		}

	};


	/// <summary>
	/// models the jsony idea that an array can be of a fundamental type or objects
	/// </summary>
	class chest_field_options : public field_options_base, public chest_field_options_interface
	{
		std::map<std::string, bool> all_allowed_classes;

	public:

		chest_field_options() = default;
		chest_field_options(const chest_field_options& _src) = default;
		chest_field_options(chest_field_options&& _src) = default;
		chest_field_options& operator = (const chest_field_options& _src) = default;
		chest_field_options& operator = (chest_field_options&& _src) = default;
		virtual ~chest_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			_dest.put_member("allowed_class_name", allowed_class_name);
			_dest.put_member("allowed_item_type", allowed_item_type);
			_dest.put_member("unit_name", unit_name);
			_dest.put_member("max_items", max_items);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			allowed_class_name = _src["allowed_class_name"].as_string();
			allowed_item_type = _src["allowed_item_type"].as_string();
			unit_name = _src["unit_name"].as_string();
			max_items = _src["max_items"].as_int();
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			if (allowed_class_name.empty()) {
				return;
			}
			auto descendants = _db->get_class_descendants(allowed_class_name);
			all_allowed_classes.insert_or_assign(allowed_class_name, true);
			for (auto descendant : descendants) {
				all_allowed_classes.insert_or_assign(descendant, true);
			}
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			bool acceptable = false;

			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {

				if (_object_to_test.array())
				{
					for (auto obj : _object_to_test)
					{
						acceptable = obj.object();
						if (not acceptable) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = "Elements of a chest array must be objects.";
							_validation_errors.push_back(ve);
							return false;
						}

						if (allowed_class_name.empty() == false) {
							std::string object_class_name = obj[class_name_field].as_string();
							acceptable = all_allowed_classes.contains(object_class_name);
							if (!acceptable) {
								validation_error ve;
								ve.class_name = _class_name;
								ve.field_name = _field_name;
								ve.filename = get_file_name(__FILE__);
								ve.line_number = __LINE__;
								ve.message = "Invalid class name for chest item: " + object_class_name;
								_validation_errors.push_back(ve);
								return false;
							}
						}
					}
					return true;
				}
				else
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an array for an array field.";
					_validation_errors.push_back(ve);
					return false;
				}
			}
			return acceptable;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("chest"));
			return schema;
		}
	};

	/// <summary>
	/// keeps track of a selection state for a user
	/// </summary>
	class selection_field_options : public field_options_base, public selection_field_options_interface
	{
	public:

		selection_field_options() = default;
		selection_field_options(const selection_field_options& _src) = default;
		selection_field_options(selection_field_options&& _src) = default;
		selection_field_options& operator = (const selection_field_options& _src) = default;
		selection_field_options& operator = (selection_field_options&& _src) = default;
		virtual ~selection_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}


		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				if (_object_to_test.array())
				{
					for (auto obj : _object_to_test)
					{
						bool acceptable = obj.object();
						if (not acceptable) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = "Elements of a selection array must be objects.";
							_validation_errors.push_back(ve);
							return false;
						}

						std::string object_class_name = obj[class_name_field].as_string();
					}
					return true;
				}
				else
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an array for an array field.";
					_validation_errors.push_back(ve);
					return false;
				}
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("selection"));
			return schema;
		}
	};

	/// <summary>
	/// keeps track of a path field
	/// </summary>
	class path_field_options : public field_options_base, public path_field_options_interface
	{
	public:

		path_field_options() = default;
		path_field_options(const path_field_options& _src) = default;
		path_field_options(path_field_options&& _src) = default;
		path_field_options& operator = (const path_field_options& _src) = default;
		path_field_options& operator = (path_field_options&& _src) = default;
		virtual ~path_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {

				if (!_object_to_test.object())
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an object.";
					_validation_errors.push_back(ve);
					return false;
				}

				return true;
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("polygon"));
			return schema;
		}
	};

	/// <summary>
	/// keeps track of a brush field
	/// </summary>
	class brush_field_options : public field_options_base, public brush_field_options_interface
	{
	public:

		brush_field_options() = default;
		brush_field_options(const brush_field_options& _src) = default;
		brush_field_options(brush_field_options&& _src) = default;
		brush_field_options& operator = (const brush_field_options& _src) = default;
		brush_field_options& operator = (brush_field_options&& _src) = default;
		virtual ~brush_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {

				if (!_object_to_test.object())
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an object.";
					_validation_errors.push_back(ve);
					return false;
				}

				return true;
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("polygon"));
			return schema;
		}
	};

	/// <summary>
	/// keeps track of a rectangle field
	/// </summary>
	class rectangle_field_options : public field_options_base, public rectangle_field_options_interface
	{
	public:

		rectangle_field_options() = default;
		rectangle_field_options(const rectangle_field_options& _src) = default;
		rectangle_field_options(rectangle_field_options&& _src) = default;
		rectangle_field_options& operator = (const rectangle_field_options& _src) = default;
		rectangle_field_options& operator = (rectangle_field_options&& _src) = default;
		virtual ~rectangle_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}


		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				if (_object_to_test.object())
				{
					bool acceptable = _object_to_test.object();

					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a rectangle.";
						_validation_errors.push_back(ve);
						return false;
					}

					if (_object_to_test.has_members({ "x", "y", "width", "height" })) {
						if (not (_object_to_test["x"].is_double() && _object_to_test["y"].is_double() && _object_to_test["width"].is_double() && _object_to_test["height"].is_double())) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = "rectangle must have double x, y, width, and height.";
							_validation_errors.push_back(ve);
							return false;
						}
						return true;
					}
					else {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "rectangle must have x, y, width, and height.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("rectangle"));
			return schema;
		}
	};

	/// <summary>
/// keeps track of a bitmap field
/// </summary>
	class bitmap_field_options : public field_options_base, public bitmap_field_options_interface
	{
	public:

		bitmap_field_options() = default;
		bitmap_field_options(const bitmap_field_options& _src) = default;
		bitmap_field_options(bitmap_field_options&& _src) = default;
		bitmap_field_options& operator = (const bitmap_field_options& _src) = default;
		bitmap_field_options& operator = (bitmap_field_options&& _src) = default;
		virtual ~bitmap_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}


		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				if (_object_to_test.object())
				{
					bool acceptable = _object_to_test.object();

					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a bitmap reference.";
						_validation_errors.push_back(ve);
						return false;
					}

					if (_object_to_test.has_members({ "x", "y", "width", "height", "bitmapName" })) {
						if (not (_object_to_test["x"].is_double() && _object_to_test["y"].is_double() && _object_to_test["width"].is_double() && _object_to_test["height"].is_double())) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = "bitmap must have double x, y, width, and height.";
							_validation_errors.push_back(ve);
							return false;
						}
					}
					else {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "bitmap must have x, y, width, and height and a bitmapName, and a bitmapPath optional.";
						_validation_errors.push_back(ve);
						return false;
					}

					return true;
				}
				else
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an bitmapPath for a bitmapPath field.";
					_validation_errors.push_back(ve);
					return false;
				}
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("bitmap"));
			return schema;
		}
	};


	/// <summary>
	/// keeps track of a audio field
	/// </summary>
	class audio_field_options : public field_options_base, public audio_field_options_interface
	{
	public:

		audio_field_options() = default;
		audio_field_options(const audio_field_options& _src) = default;
		audio_field_options(audio_field_options&& _src) = default;
		audio_field_options& operator = (const audio_field_options& _src) = default;
		audio_field_options& operator = (audio_field_options&& _src) = default;
		virtual ~audio_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{

		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				if (!_object_to_test.object())
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Value must be an object.";
					_validation_errors.push_back(ve);
					return false;
				}

				return true;
			}
			return false;
		}

		virtual bool is_relational_children() override
		{
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("audio"));
			return schema;
		}
	};


	class object_field_options : public field_options_base
	{
	public:
		std::shared_ptr<child_relations> relations;
		field_types fundamental_type;

		object_field_options() = default;
		object_field_options(const object_field_options& _src) = default;
		object_field_options(object_field_options&& _src) = default;
		object_field_options& operator = (const object_field_options& _src) = default;
		object_field_options& operator = (object_field_options&& _src) = default;
		virtual ~object_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);

			json_parser jp;

			if (relations) {
				json jctors = jp.create_object();
				relations->get_json(jctors);
				_dest.share_member("child_objects", jctors);
			}
			_dest.put_member_string("fundamental_type", field_type_names[fundamental_type]);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);

			json jctors = _src["child_objects"];
			relations = std::make_shared<child_relations>();
			if (jctors.object()) {
				relations->put_json(jctors);
			}
			std::string ft = _src["fundamental_type"].as_string();
			fundamental_type = allowed_field_types[ft];
		}

		virtual void put_definition(child_object_definition& _cod)
		{
			fundamental_type = _cod.fundamental_type;
			relations = std::make_shared<child_relations>();
			relations->put_child_object(_cod);
		}

		virtual bool is_relational_children() override
		{
			return fundamental_type == field_types::ft_none;
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			if (relations)
				relations->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;

				json& obj = _object_to_test;

				std::string object_class_name;

				if (fundamental_type == field_types::ft_object || fundamental_type == field_types::ft_array)
				{
					return true;
				}
				else if (fundamental_type == field_types::ft_string)
				{
					bool acceptable = obj.is_string();
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a string.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_int64)
				{
					bool acceptable = obj.is_int64();
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a int64.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_double)
				{
					bool acceptable = obj.is_double();
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a double.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_datetime)
				{
					bool acceptable = obj.is_int64();
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a datetime.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_vector)
				{
					bool acceptable = obj.is_vector();
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a vector.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_path)
				{
					bool acceptable = obj.has_members({ "name", "points" });
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a path.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_brush)
				{
					bool acceptable = obj.has_members({ "name" });
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a brush.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_rectangle)
				{
					bool acceptable = obj.has_members({ "x", "y", "width", "height" });
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a rectangle.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (fundamental_type == field_types::ft_bitmap)
				{
					bool acceptable = obj.has_members({ "bitmapName", "x", "y", "width", "height" });
					if (not acceptable) {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Element must be a rectangle.";
						_validation_errors.push_back(ve);
						return false;
					}
				}
				else if (relations)
				{
					if (obj.object()) {
						object_class_name = obj[class_name_field].as_string();
						auto ctor = relations->get_relation(object_class_name);
						if (not ctor) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							if (object_class_name.empty()) {
								ve.message = "Array has child objects without a class_name.";
							}
							else {
								ve.message = "Array does not accept child objects of '" + object_class_name + "'";
							}
							_validation_errors.push_back(ve);
							return false;
						}
					}
					else {
						validation_error ve;
						ve.class_name = _class_name;
						ve.field_name = _field_name;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "elements of this array must be objects.";
						_validation_errors.push_back(ve);
						return false;

					}
				}
			}
			return false;
		}

		virtual std::shared_ptr<child_relations_interface> get_relations() override
		{
			return relations;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("array"));

			json jitems = jp.create_object();
			json joneof = jp.create_array();
			for (auto& bc : relations->base_constructors) {
				json joneofi = jp.create_object();
				joneofi.put_member("$ref", "#/components/schemas/" + bc.first);
				joneof.push_back(joneofi);
			}

			jitems.share_member("oneOf", joneof);
			schema.share_member("items", jitems);

			return schema;
		}
	};

	const int max_query_result_rows = 10000;

	class string_field_options : public field_options_base
	{
	public:
		int maximum_length;
		int minimum_length;
		std::string match_pattern;
		std::vector<std::string> allowed_values;
		std::string match_name;

		string_field_options() = default;
		string_field_options(const string_field_options& _src) = default;
		string_field_options(string_field_options&& _src) = default;
		string_field_options& operator = (const string_field_options& _src) = default;
		string_field_options& operator = (string_field_options&& _src) = default;
		virtual ~string_field_options() = default;

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			field_options_base::get_json(_dest);
			_dest.put_member_i64("max_length", maximum_length);
			_dest.put_member_i64("min_length", minimum_length);
			_dest.put_member("match_pattern", match_pattern);
			if (allowed_values.size()) {
				json jallowed_values = jp.create_array();
				for (auto s : allowed_values) {
					jallowed_values.push_back(s);
				}
				_dest.share_member("enum", jallowed_values);
			}
			_dest.put_member("match_name", match_name);
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			field_options_base::put_json(_src);
			minimum_length = _src["min_length"].as_int();
			maximum_length = _src["max_length"].as_int();
			match_pattern = _src["match_pattern"].as_string();
            match_name = _src["match_name"].as_string();
			for (auto& c : match_pattern) {
				if (c == '/') c = '\\';
			}

			allowed_values.clear();
			json jallowed_values = _src["enum"];
			if (jallowed_values.array()) {
				for (auto s : jallowed_values) {
					allowed_values.push_back(s.as_string());
				}
			}
		}

		bool is_allowed_value(const std::string& _src)
		{
			bool is_legit = true;
			if (allowed_values.size() > 0) {
				auto iter = std::find(allowed_values.begin(), allowed_values.end(), _src);
				if (iter == std::end(allowed_values)) {
					is_legit = false;
				}
			}
			return is_legit;
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				std::string chumpy = _object_to_test.as_string();

				if (chumpy.size() >= minimum_length and (maximum_length == 0 or chumpy.size() <= maximum_length))
				{
					if (not match_pattern.empty()) {
						try {
							std::regex rgx(match_pattern);
							if (std::regex_match(chumpy, rgx)) {
								is_legit = is_allowed_value(chumpy);
							}
							else {
								is_legit = false;
							}
						}
						catch (std::regex_error& e) {
							validation_error ve;
							ve.class_name = _class_name;
							ve.field_name = _field_name;
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = std::format("Invalid regex pattern '{0}'", match_pattern);
							_validation_errors.push_back(ve);
							return false;
						}
					}
					else {
						is_legit = is_allowed_value(chumpy);
					}
				}
				else
				{
					is_legit = false;
				}
				if (not is_legit) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
                    if (maximum_length &&chumpy.size() > maximum_length) {
                        ve.message = std::format("{0} Characters too long.  ", chumpy.size() - maximum_length);
                    }
                    if (minimum_length && chumpy.size() < minimum_length) {
                        ve.message = std::format("{0} Characters short.  ", minimum_length - chumpy.size());
                    }
					if (match_pattern.size() > 0) {
						ve.message = std::format("Match '{0}'.  ", match_pattern);
					}
					if (allowed_values.size())
					{
						ve.message = "Select from list:(";
						std::string setx = join(allowed_values, ", ");
						setx = trim(setx, 25);
						ve.message += std::format(" '{0}').  ", setx);
					}
					_validation_errors.push_back(ve);
					return false;
				};
				return true;
			}
			return false;

		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("string"));

			if (minimum_length > 0) {
				schema.put_member("minLength", minimum_length);
			}
			if (maximum_length > 0) {
				schema.put_member("maxLength", maximum_length);
			}
			if (not match_pattern.empty()) {
				schema.put_member("pattern", match_pattern);
			}
			return schema;
		}

	};

	class int64_field_options : public field_options_base
	{
	public:
		int64_t min_value;
		int64_t max_value;

		int64_field_options() = default;
		int64_field_options(const int64_field_options& _src) = default;
		int64_field_options(int64_field_options&& _src) = default;
		int64_field_options& operator = (const int64_field_options& _src) = default;
		int64_field_options& operator = (int64_field_options&& _src) = default;
		virtual ~int64_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member_i64("min_value", min_value);
			_dest.put_member_i64("max_value", max_value);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			min_value = _src["min_value"].as_int64_t();
			max_value = _src["max_value"].as_int64_t();
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				int64_t chumpy = _object_to_test.as_int64_t();

				if (min_value == 0 and max_value == 0)
				{
					is_legit = true;
				}
				else if (chumpy >= min_value and chumpy <= max_value)
				{
					is_legit = true;
				}
				else
				{
					is_legit = false;
				}
				if (not is_legit) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = std::format("Value '{0}' must be between {1} and {2}", chumpy, min_value, max_value);
					_validation_errors.push_back(ve);
					return false;
				};
				return true;
			}
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("integer"));

			if (min_value > 0) {
				schema.put_member("minimum", min_value);
			}
			if (max_value > 0) {
				schema.put_member("maximum", max_value);
			}
			return schema;
		}

	};

	class reference_field_options : public field_options_base
	{
	public:
		std::string reference_class;
		std::map<std::string, bool> reference_class_descendants;

		reference_field_options() = default;
		reference_field_options(const reference_field_options& _src) = default;
		reference_field_options(reference_field_options&& _src) = default;
		reference_field_options& operator = (const reference_field_options& _src) = default;
		reference_field_options& operator = (reference_field_options&& _src) = default;
		virtual ~reference_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("ref_class", reference_class);
		}

		virtual void put_definition(child_object_definition& _rd)
		{
			if (_rd.child_classes.size() > 0) {
				reference_class = _rd.child_classes[0]->class_name;
			}
			reference_class_descendants.clear();
		}

		virtual void put_definition(reference_definition& _rd)
		{
			reference_class = _rd.base_class;
			reference_class_descendants.clear();
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			reference_class = _src["ref_class"].as_string();
			reference_class_descendants.clear();
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			reference_class_descendants.clear();
			auto descendants = _db->get_class_descendants(reference_class);
			for (auto descendant : descendants) {
				reference_class_descendants.insert_or_assign(descendant, true);
			}
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				object_reference chumpy = _object_to_test.as_object_reference();

				if (chumpy and reference_class_descendants.contains(chumpy.class_name))
				{
					is_legit = true;
				}
				else
				{
					is_legit = false;
				}
				if (not is_legit) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = std::format("Value '{0}' must be a {1}", (std::string)chumpy, reference_class);
					_validation_errors.push_back(ve);
					return false;
				};
				return true;
			}
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("string"));

			return schema;
		}

	};

	class datetime_field_options : public field_options_base
	{
	public:
		date_time min_value;
		date_time max_value;

		datetime_field_options() = default;
		datetime_field_options(const datetime_field_options& _src) = default;
		datetime_field_options(datetime_field_options&& _src) = default;
		datetime_field_options& operator = (const datetime_field_options& _src) = default;
		datetime_field_options& operator = (datetime_field_options&& _src) = default;
		virtual ~datetime_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("min_value", min_value);
			_dest.put_member("max_value", max_value);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			min_value = _src["min_value"].as_date_time();
			max_value = _src["max_value"].as_date_time();
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				date_time chumpy = _object_to_test.as_date_time();
				date_time zero(0);

				if (min_value == zero and max_value == zero)
				{
					is_legit = true;
				}
				else if (chumpy >= min_value and chumpy <= max_value)
				{
					is_legit = true;
				}
				else
				{
					is_legit = false;
				}
				if (not is_legit) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "datetime out of range";
					_validation_errors.push_back(ve);
					return false;
				};
				return true;
			}
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("datetime"));

			schema.put_member("minimum", min_value);
			schema.put_member("maximum", max_value);

			return schema;
		}


	};

	class vector_field_options : public field_options_base
	{
	public:

		vector_field_options() = default;
		vector_field_options(const vector_field_options& _src) = default;
		vector_field_options(vector_field_options&& _src) = default;
		vector_field_options& operator = (const vector_field_options& _src) = default;
		vector_field_options& operator = (vector_field_options&& _src) = default;
		virtual ~vector_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			return field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test);
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("vector"));
			return schema;
		}

	};

	class double_field_options : public field_options_base
	{
	public:
		double min_value;
		double max_value;

		double_field_options() = default;
		double_field_options(const double_field_options& _src) = default;
		double_field_options(double_field_options&& _src) = default;
		double_field_options& operator = (const double_field_options& _src) = default;
		double_field_options& operator = (double_field_options&& _src) = default;
		virtual ~double_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("min_value", min_value);
			_dest.put_member("max_value", max_value);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			min_value = _src["min_value"].as_double();
			max_value = _src["max_value"].as_double();
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				double chumpy = _object_to_test.as_double();
				double zero = 0.0;

				if (min_value == zero and max_value == zero)
				{
					is_legit = true;
				}
				else if (chumpy >= min_value and chumpy <= max_value)
				{
					is_legit = true;
				}
				else
				{
					is_legit = false;
				}
				if (not is_legit) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "value out of range";
					_validation_errors.push_back(ve);
					return false;
				};
				return true;
			}
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("number"));

			double imin = min_value;
			double imax = max_value;
			if (imin) {
				schema.put_member("minimum", min_value);
			}
			if (imax) {
				schema.put_member("maximum", max_value);
			}

			return schema;
		}


	};

	class choice_field_options : public field_options_base
	{
	public:
		json		filter;
		json		options;

		std::string	id_field_name;
		std::string	description_field_name;

		choice_field_options() = default;
		choice_field_options(const choice_field_options& _src) = default;
		choice_field_options(choice_field_options&& _src) = default;
		choice_field_options& operator = (const choice_field_options& _src) = default;
		choice_field_options& operator = (choice_field_options&& _src) = default;
		virtual ~choice_field_options() = default;

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			json_parser jp;
			json key = jp.create_object();
			options = _db->select_object(filter, false, _permissions);
		}

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("filter", filter);
			_dest.put_member("id_field_name", id_field_name);
			_dest.put_member("description_field_name", description_field_name);
			_dest.put_member("options", options);
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			field_options_base::put_json(_src);
			filter = _src["filter"];
			id_field_name = _src["id_field_name"].as_string();
			description_field_name = _src["description_field_name"].as_string();
			options = jp.create_array();
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_empty = _object_to_test.empty();
				if (is_empty) {
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "value is empty";
					_validation_errors.push_back(ve);
					return false;
				}
				else if (options.empty())
				{
					validation_error ve;
					ve.class_name = _class_name;
					ve.field_name = _field_name;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "validation list not loaded";
					_validation_errors.push_back(ve);
					return false;
				}
				else
				{
					std::string object_test_id;
					if (_object_to_test.object())
					{
						object_test_id = _object_to_test[id_field_name].as_string();
					}
					else if (_object_to_test.is_string())
					{
						object_test_id = _object_to_test.as_string();
					}
					else
						return false;

					for (auto option : options)
					{
						std::string id_field = option[id_field_name].as_string();
						if (id_field == object_test_id)
							return true;
					}
				}
				return true;
			}
			return false;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("string"));
			return schema;
		}

	};

	class query_field_options : public field_options_base
	{
	public:
		json		query_body;
		bool        run_on_demand = false;

		query_field_options() = default;
		query_field_options(const query_field_options& _src) = default;
		query_field_options(query_field_options&& _src) = default;
		query_field_options& operator = (const query_field_options& _src) = default;
		query_field_options& operator = (query_field_options&& _src) = default;
		virtual ~query_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("query", query_body);
			_dest.put_member("run_on_demand", run_on_demand);
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			field_options_base::put_json(_src);
			query_body = _src["query"];
			run_on_demand = _src["run_on_demand"].as_bool();
		}

		virtual json run_method(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object)
		{
			using namespace std::literals;
			json_parser jp;
			if (query_body.empty()) {
				return jp.create_array();
			}
			json this_query_body = query_body.clone();
			this_query_body.put_member("include_children", false);

			json froms = this_query_body["from"];
			if (froms.array()) {
				// this adds us as an input source
				json new_from = jp.create_object();
				json thisobj = _object.clone();
				new_from.put_member(data_field, thisobj);
				new_from.put_member(class_name_field, _classname);
				new_from.put_member("name", "this"sv);
				auto arr = froms.array_impl();
				arr->push_front(new_from.value());
			}
			this_query_body.put_member(token_field, _token);
			json query_results, query_data_results;
			query_results = _db->query(this_query_body);
			if (query_results[success_field].as_bool()) {
				query_data_results = query_results[data_field];
			}
			else
			{
				validation_error_collection ves;
				validation_error ve;
				json errors = query_results["errors"];
				for (auto err : errors) {
					ve.put_json(err);
					ves.push_back(ve);
				}
				_db->log_errors(ves);
				query_data_results = jp.create_array();
			}
			return query_data_results;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object)
		{
			json_parser jp;
			json results = jp.create_array();

			if (run_on_demand) {
				return results;
			}

			results = run_method(_db, _token, _classname, _object);
			return results;
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			return false;
		}


		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("object"));

			return schema;
		}

	};

	class field_implementation : public field_interface {

	public:

		field_implementation() = default;
		field_implementation(const field_implementation& _src) = default;
		field_implementation(field_implementation&& _src) = default;
		field_implementation& operator = (const field_implementation& _src) = default;
		field_implementation& operator = (field_implementation&& _src) = default;
		virtual ~field_implementation() = default;

		virtual field_types get_field_type()
		{
			return field_type;
		}
		virtual field_implementation& set_field_type(field_types _field_type)
		{
			field_type = _field_type;
			return *this;
		}

		virtual const std::string& get_field_name() const
		{
			return field_name;
		}
		field_implementation& set_field_name(const std::string& _field_name)
		{
			field_name = _field_name;
			return *this;
		}

		virtual std::shared_ptr<field_options_interface> get_options() {
			return options;
		}

		virtual std::shared_ptr<field_options_interface> set_options(std::shared_ptr<field_options_interface> _src) {
			options = _src;
			return options;
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			if (options) options->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) override
		{
			if (options) {
				return options->accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test);
			}
			return true;
		}

		virtual json get_openapi_schema(corona_database_interface* _db)
		{
			json result;

			if (options) {
				result = options->get_openapi_schema(_db);
			}
			else {
				json_parser jp;

				result = jp.create_object();
				result.put_member("type", field_type_names_openapi[field_type]);
			}

			return result;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;

			json joptions = jp.create_object();

			_dest.put_member("field_name", field_name);
			_dest.put_member("field_type", field_type_names[field_type]);
			_dest.put_member("field_class", get_field_class());

			if (options) {
				options->get_json(_dest);
			}
		}

		virtual void put_json(validation_error_collection& _errors, json& _src, class_interface_base* _classdb)
		{

			class_interface* _classd = dynamic_cast<class_interface*>(_classdb);

			std::string s = _src["field_type"].as_string();
			auto aft = allowed_field_types.find(s);
			if (aft != std::end(allowed_field_types)) {
				field_type = aft->second;
			}
			field_name = _src["field_name"].as_string();
			field_class = _src["field_class"].as_string();

			if (field_type == field_types::ft_object)
			{
				options = std::make_shared<object_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_array)
			{
				options = std::make_shared<array_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_double)
			{
				options = std::make_shared<double_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_int64)
			{
				options = std::make_shared<int64_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_string)
			{
				options = std::make_shared<string_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_chest)
			{
				options = std::make_shared<chest_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_bool)
			{
				options = std::make_shared<field_options_base>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_datetime)
			{
				options = std::make_shared<datetime_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_vector)
			{
				options = std::make_shared<vector_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_selection)
			{
				options = std::make_shared<selection_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_path)
			{
				options = std::make_shared<path_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_brush)
			{
				options = std::make_shared<brush_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_rectangle)
			{
				options = std::make_shared<rectangle_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_reference)
			{
				options = std::make_shared<reference_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_bitmap)
			{
				options = std::make_shared<bitmap_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_query)
			{
				options = std::make_shared<query_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_audio)
			{
				options = std::make_shared<audio_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_function)
			{
				;
			}
		}

		virtual json run_method(corona_database_interface* _db, std::string _method_name, std::string& _token, std::string& _class_name, json& _object)
		{
			json results;
			if (options) {
				results = options->run_method(_db, _method_name, _token, _class_name, _object);
			}
			return results;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object) override
		{
			json results;
			if (options) {
				results = options->run_queries(_db, _token, _classname, _object);
			}
			return results;
		}

		virtual std::shared_ptr<child_relations_interface> get_relations() override
		{
			if (options) {
				return options->get_relations();
			}
			return nullptr;
		}

	};



}
