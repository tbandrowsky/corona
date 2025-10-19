/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This is the core database engine for the corona database server.  

*/


#ifndef CORONA_DATABASE_ENGINE_HPP
#define CORONA_DATABASE_ENGINE_HPP

const bool debug_teams = false;

/*********************************************** 

Journal

3/14/2024 - Objectives
Create and list teams and users, 
Team permission checks for all objects,
Create 

3/15/2024 - log
Create and edit 

3/28/2024 - 
API close to complete, rounding out for testing.

4/6/2024 -
A good chunk of this is coded, but I am dealing 
with getting the headers straightened out from 
the great module disaster.

9/18/2024 - rewrote most of the above because the design was stupid.
9/25/2024 - factored classes and indexes out to help with concurrency.
10/28/2024 - now with xtable implementation for faster and more efficient storage,
			and querying that works increasingly well.
8/24/2025 - now with xtables, sign on ceremony, 20k puts / sec.  

***********************************************/

template <typename T>
constexpr bool is_convertible_to_int = std::is_convertible_v<T, int>;

namespace corona
{

	const std::string class_definition_string = R"(
{	
	"class_name" : "sys_class",
	"class_description" : "Class definitions",
	"fields" : {			
			"class_name":"string",
			"class_description":"string",
			"class_color":"string",
			"grid_template_rows":"string",
			"grid_template_columns":"string",
			"base_class_name":"string",	
			"ancestors":"[ string ]",
			"descendants":"[ string ]",
			"fields" : "object",
			"indexes" : "object",
			"sql" : "object"	
	}
}
)";


	/// <summary>
    /// Class permissions	
	/// </summary>
	const std::string class_permission_put = "put";
	const std::string class_permission_get = "get";
	const std::string class_permission_delete = "delete";
	const std::string class_permission_alter = "alter";

	/// <summary>
	/// Represents a string constant with the value "any".
	/// </summary>
	const std::string class_grant_any = "any"; // any object can be modified by someone on the team
	const std::string class_grant_own = "own"; // only objects you own can be modified if you are on the team
	const std::string class_grant_team = "team"; // only objects owned by your team
	const std::string class_grant_teamorown = "teamorown"; // only objects owned by your team

	enum class_grants {
		grant_none = 0,
		grant_any = 1,
		grant_own = 2,
		grant_team = 4,
		grant_teamorown = 6
	};

	class child_object_class
	{
	public:
		std::string class_name;
		std::map<std::string, std::string> copy_values;
	};

	/// <summary>
	/// Parses a child object definition from a string and returns a child_object_definition instance representing the parsed structure.
	/// </summary>
	class child_object_definition 
	{
	public:
		bool					 is_undefined;
		bool					 is_array;
		field_types				 fundamental_type;
		std::vector<std::shared_ptr<child_object_class>> child_classes;

		child_object_definition()
		{
			is_undefined = true;
			is_array = false;
			fundamental_type = field_types::ft_none;
		}

		child_object_definition(const child_object_definition& _src) = default;
		child_object_definition(child_object_definition&& _src) = default;
		child_object_definition& operator =(const child_object_definition& _src) = default;
		child_object_definition& operator =(child_object_definition&& _src) = default;

		/// <summary>
		/// Parses a child object definition from a source string, extracting class names and field mappings according to a custom syntax.
		/// </summary>
		/// <param name="_src">A pointer to a null-terminated C string containing the definition to parse.</param>
		/// <returns>A child_object_definition structure populated with parsed class names, field mappings, and array status. If parsing fails or the input is empty, returns a default-initialized child_object_definition.</returns>
		static child_object_definition parse_definition(const char* _src, std::string _default_target_field)
		{
			parser_base pb;
			child_object_definition cod;

			// parse symbol until end or [
			// if there is a [
			// if end, end 
			// 
			// parse classname until end or ] or ,
			// if comma, add cn, reset classname
			// if ] end declaration
			// if end, end 

			if ((not _src) or (*_src == 0))
			{
				return cod;
			}

			_src = pb.eat_white(_src);
			if (*_src == '[') 
			{
				_src++;
				cod.is_array = true;
			}

			enum parse_states {
				parsing_class_name,
				parsing_dst_field,
				parsing_src_field,
				parsing_complete,
				parsing_error
			} status;

			status = parsing_class_name;

			std::string class_name;
			std::string dest_field;
			std::string src_field;
			std::shared_ptr<child_object_class> new_class = std::make_shared<child_object_class>();

			while (status != parsing_complete and status != parsing_error)
			{
				_src = pb.eat_white(_src);

				if (*_src == 0)
				{
					status = parsing_complete;
				}
				else if (status == parsing_class_name)
				{
					if (pb.parse_symbol(class_name, _src, &_src)) {
						_src = pb.eat_white(_src);
						new_class->class_name = class_name;

						if (cod.child_classes.size() == 0) {
							auto ft = allowed_field_types.find(class_name);
							if (ft != std::end(allowed_field_types)) {
								cod.fundamental_type = ft->second;
							}
							else
							{
								cod.fundamental_type = field_types::ft_none;
							}
						}
						else {
							cod.fundamental_type = field_types::ft_none;
						}

						if (*_src == ':')
						{
							_src++;
							status = parsing_dst_field;
						}
						else if (*_src == ';')
						{
							_src++;
							cod.child_classes.push_back(new_class);
							new_class = std::make_shared<child_object_class>();
							status = parsing_class_name;
						}
						else if (*_src == 0 || *_src == ']')
						{
                            if (cod.fundamental_type == field_types::ft_none) {
								new_class->copy_values.insert_or_assign(_default_target_field, "object_id");
							}
							cod.child_classes.push_back(new_class);
							status = parsing_complete;
						}
						else
						{
							pb.error("declaration", "syntax error after class name");
							status = parsing_error;
						}
					}
					else
					{
						pb.error("declaration", "invalid class name");
						status = parsing_error;

					}
				}
				else if (status == parsing_dst_field) 
				{
					if (pb.parse_symbol(dest_field, _src, &_src)) {
						_src = pb.eat_white(_src);
						if (*_src == '=')
						{
							_src++;
							status = parsing_src_field;
						}
						else if (*_src == ',')
						{
							_src++;
							status = parsing_dst_field;
							new_class->copy_values.insert_or_assign(dest_field, "object_id");
						}
						else if (*_src == ';')
						{
							_src++;
							status = parsing_class_name;
							new_class->copy_values.insert_or_assign(dest_field, "object_id");
							cod.child_classes.push_back(new_class);
							new_class = std::make_shared<child_object_class>();
						}
						else if (*_src == 0 || *_src == ']')
						{
							new_class->copy_values.insert_or_assign(dest_field, "object_id");
							cod.child_classes.push_back(new_class);
							status = parsing_complete;
						}
						else 
						{
							pb.error("declaration", "syntax error after dst field");
							status = parsing_error;
						}
					}
					else
					{
						pb.error("declaration", "invalid dest symbol");
						status = parsing_error;
					}
				}
				else if (status == parsing_src_field)
				{
					if (pb.parse_symbol(src_field, _src, &_src)) {
						new_class->copy_values.insert_or_assign(dest_field, src_field);
						_src = pb.eat_white(_src);
						if (*_src == ',')
						{
							_src++;
							status = parsing_dst_field;
						}
						else if (*_src == ';')
						{
							_src++;
							status = parsing_class_name;
							new_class->copy_values.insert_or_assign(dest_field, src_field);
							cod.child_classes.push_back(new_class);
						}
						else if (*_src == 0 || *_src == ']')
						{
							new_class->copy_values.insert_or_assign(dest_field, src_field);
							cod.child_classes.push_back(new_class);
							new_class = std::make_shared<child_object_class>();
							status = parsing_complete;
						}
						else
						{
							pb.error("declaration", "syntax error after src field");
							status = parsing_error;
						}
					}
					else 
					{
						pb.error("declaration", "invalid src symbol");
						status = parsing_error;
					}
				}
			}

			if (not cod.is_array and *_src == ']') 
			{
				pb.error("declaration", "invalid array terminator");
				status = parsing_error;
			}

			if (not pb.has_errors())
			{
				cod.is_undefined = false;
			}

			return cod;
		}
	};

	class reference_definition
	{
	public:
		bool is_undefined;
		std::string base_class;

		reference_definition()
		{
			is_undefined = true;
		}

		reference_definition(const reference_definition& _src) = default;
		reference_definition(reference_definition&& _src) = default;
		reference_definition& operator =(const reference_definition& _src) = default;
		reference_definition& operator =(reference_definition&& _src) = default;

		static reference_definition parse_definition(const char* _src)
		{
			parser_base pb;
			reference_definition cod;

			_src = pb.eat_white(_src);

			if (_src[0] == '-' and _src[1] == '>') {
				_src += 2;
				_src = pb.eat_white(_src);
				cod.base_class = _src;
				cod.is_undefined = not cod.base_class.empty();
			}
			else 
			{
				pb.error("reference", "reference field definitions must have form ->class_name");
			}

			if (not pb.has_errors())
			{
				cod.is_undefined = false;
			}

			return cod;
		}
	};

	void test_parse_child_field(std::shared_ptr<test_set> _tests)
	{
		date_time st = date_time::now();
		timer tx;
		system_monitoring_interface::active_mon->log_function_start("parse_child_field", "start", st, __FILE__, __LINE__);

		child_object_definition cd;
		bool result;

		const char *case1 = "class1";
		cd = child_object_definition::parse_definition(case1, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1 and cd.child_classes[0]->class_name == case1 and cd.child_classes[0]->copy_values.size()==1 and cd.child_classes[0]->copy_values.contains("XOXO");
		_tests->test({ std::format("child object {0}", case1), result , __FILE__, __LINE__ });

		const char* case2 = "class1:target";
		cd = child_object_definition::parse_definition(case2, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1 and cd.child_classes[0]->class_name == "class1" and cd.child_classes[0]->copy_values.contains("target");
		_tests->test({ std::format("child object {0}", case1), result , __FILE__, __LINE__ });

		const char* case3 = "class1:target = src";
		cd = child_object_definition::parse_definition(case3, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1 
			and cd.child_classes[0]->class_name == "class1"
			and cd.child_classes[0]->copy_values.contains("target")
			and cd.child_classes[0]->copy_values["target"] == "src";
		_tests->test({ std::format("child object {0}", case3), result , __FILE__, __LINE__ });

		const char *case4 = "class1:target1;class2:target2 = src2;";
		cd = child_object_definition::parse_definition(case4, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 2
			and cd.child_classes[0]->class_name == "class1"
			and cd.child_classes[1]->class_name == "class2"
			and cd.child_classes[0]->copy_values.contains("target1")
			and cd.child_classes[1]->copy_values.contains("target2")
			and cd.child_classes[0]->copy_values["target1"] == "object_id"
			and cd.child_classes[1]->copy_values["target2"] == "src2";
		_tests->test({ std::format("child object {0}", case4), result , __FILE__, __LINE__ });

		const char* acase1 = "[ class1 ]";
		cd = child_object_definition::parse_definition(case1, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1 and cd.child_classes[0]->class_name == "class1" and cd.child_classes[0]->copy_values.size() == 1 and cd.child_classes[0]->copy_values.contains("XOXO");
		_tests->test({ std::format("child object {0}", case1), result , __FILE__, __LINE__ });

		const char* acase2 = "[ class2:target ]";
		cd = child_object_definition::parse_definition(acase2, "XOXO");
		result = not cd.is_undefined 
			and cd.child_classes.size() == 1 
			and cd.child_classes[0]->class_name == "class2" 
			and cd.child_classes[0]->copy_values.contains("target");
		_tests->test({ std::format("child object {0}", acase2), result , __FILE__, __LINE__ });

		const char* acase3 = "[ class2:target = src ]";
		cd = child_object_definition::parse_definition(acase3, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1
			and cd.child_classes[0]->class_name == "class2"
			and cd.child_classes[0]->copy_values.contains("target")
			and cd.child_classes[0]->copy_values["target"] == "src";
		_tests->test({ std::format("child object {0}", acase3), result , __FILE__, __LINE__ });

		const char* acase4 = "[ class1:target1;class2:target2 = src2 ]";
		cd = child_object_definition::parse_definition(acase4, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 2
			and cd.child_classes[0]->class_name == "class1"
			and cd.child_classes[1]->class_name == "class2"
			and cd.child_classes[0]->copy_values.contains("target1")
			and cd.child_classes[1]->copy_values.contains("target2")
			and cd.child_classes[0]->copy_values["target1"] == "object_id"
			and cd.child_classes[1]->copy_values["target2"] == "src2";
		_tests->test({ std::format("child object {0}", acase4), result , __FILE__, __LINE__ });

		const char* acase5 = "[ class2:target ]";
		cd = child_object_definition::parse_definition(acase5, "XOXO");
		result = not cd.is_undefined
			and cd.child_classes.size() == 1
			and cd.child_classes[0]->class_name == "class2"
			and cd.child_classes[0]->copy_values.contains("target");
		_tests->test({ std::format("child object {0}", acase5), result , __FILE__, __LINE__ });

		const char* acase6 = "[ class2:target = src ]";
		cd = child_object_definition::parse_definition(acase6, "XOXO");
		result = not cd.is_undefined 
			and cd.child_classes.size() == 1 
			and cd.child_classes[0]->class_name == "class2" 
			and cd.child_classes[0]->copy_values.contains("target") 
			and cd.child_classes[0]->copy_values["target"] == "src";
		_tests->test({ std::format("child object {0}", acase6), result , __FILE__, __LINE__ });

		system_monitoring_interface::active_mon->log_function_stop("parse_child_field", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

	class class_permissions {
	public:
		std::string		user_name;
        std::string     team_name;
		class_grants	put_grant;
		class_grants	get_grant;
		class_grants	delete_grant;
		class_grants	alter_grant;
		class_permissions operator|(const class_permissions& _other) const
		{
			class_permissions cp;
			cp.put_grant = (class_grants)((int)put_grant | (int)_other.put_grant);
			cp.get_grant = (class_grants)((int)get_grant | (int)_other.get_grant);
			cp.delete_grant = (class_grants)((int)delete_grant | (int)_other.delete_grant);
			cp.alter_grant = (class_grants)((int)alter_grant | (int)_other.alter_grant);
			return cp;
		}
	};


	class corona_database_interface;
	
	class to_field
	{
	public:
		std::string			  source_name;
		std::string			  field_name;
	};

	class from_field
	{
	public:
		std::string			  field_name;
		std::vector<std::shared_ptr<to_field>> targets;
	};

	class from_source
	{
	public:
		int													index;
		std::string											source_name;
		std::string											parent_source_name;
		std::map<std::string, std::shared_ptr<from_field>>  fields;
		std::shared_ptr<json_object>						context;
		std::vector<std::shared_ptr<to_field>>				origins;
		std::map<int64_t, std::shared_ptr<json_object>>		data;
	};

	using from_sources = std::map<std::string, std::shared_ptr<from_source>>;

	class from_join
	{
		from_sources			sources;
		json					class_filter;
		json					dest_array;
		int						join_index;

		void get_filters_impl(json& dest_array, json& class_filter, from_sources::iterator _it)
		{
			json_parser jp;

			// this basically permutes the data we have in our join tree
			// at the end, we then take the chain of contexts, and from 
			// that result, we load up our new filter objects

			if (_it != std::end(sources)) {
				auto next_it = _it;
				next_it++;
				if (_it->second->data.size()) {
					for (auto item : _it->second->data)
					{
						_it->second->context = item.second;
						get_filters_impl(dest_array, class_filter, next_it);
					}
				}
				else {
					_it->second->context = std::make_shared<json_object>();
					get_filters_impl(dest_array, class_filter, next_it);
				}
			}
			else 
			{
				json_parser jp;
				json new_filter_object = class_filter.clone();
				std::string source_name = new_filter_object["name"];

				auto jobj = new_filter_object["filter"];
				if (jobj.empty()) {
					jobj = jp.create_object();
					new_filter_object.share_member("filter", jobj);
				}
				auto jmembers = jobj.get_members();

				from_source* parent = nullptr;

				for (auto member : jmembers)
				{
					if (member.second.is_string())
					{
						std::string value = (std::string)member.second;
						if (value.starts_with("$"))
						{
							std::string path(value.substr(1));
							std::vector<std::string> split_path = split(path, '.');
							if (split_path.size() == 2) {
								std::string& source_from_name = split_path[0];
								std::string& source_from_member = split_path[1];
								auto fsi = sources.find(source_from_name);
								if (fsi != sources.end())
								{
									jobj.object_impl()->members.insert_or_assign(source_from_member, fsi->second->context->members[source_from_member]);
									if (not parent or fsi->second->index > parent->index) {
										parent = fsi->second.get();
									}
								}
							}
						}
					}
				}

				if (parent) {
					new_filter_object.put_member("parent_source", parent->source_name);
					int64_t object_id = parent->context->members[object_id_field]->to_int64();
					new_filter_object.put_member_i64("parent_id", object_id);
				}

				dest_array.push_back(new_filter_object);
			}
		}

	public:

		from_join()
		{
			json_parser jp;
			join_index = 0;
		}
		
		void add_join(std::string _from_source, std::string _from_field, std::string _to_source, std::string _to_field)
		{
			std::shared_ptr<to_field> dest = std::make_shared<to_field>();
			dest->source_name = _to_source;
			dest->field_name = _to_field;

			std::shared_ptr<to_field> src = std::make_shared<to_field>();
			src->source_name = _from_source;
			src->field_name = _from_field;

			// from->to
			auto fsi = sources.find(_from_source);
			if (fsi != sources.end()) {
				auto& fs = fsi->second;
				auto ffi = fs->fields.find(_from_field);
				if (ffi != fs->fields.end()) 
				{
					ffi->second->targets.push_back(dest);
				}
				else 
				{
					std::shared_ptr<from_field> new_field = std::make_shared<from_field>();
					new_field->field_name = _from_field;
					new_field->targets.push_back(dest);
					fs->fields.insert_or_assign(_from_field, new_field);
				}
			}
			else 
			{
				std::shared_ptr<from_source> new_source = std::make_shared<from_source>();
				std::shared_ptr<from_field> new_field = std::make_shared<from_field>();
				new_field->field_name = _from_field;
				new_field->targets.push_back(dest);
				new_source->index = join_index++;
				new_source->source_name = _from_source;
				new_source->fields.insert_or_assign(_from_field, new_field);
				sources.insert_or_assign(_from_source, new_source);
			}

			// now update for origins
			auto fsio = sources.find(dest->source_name);
			if (fsio != sources.end()) {
				fsio->second->origins.push_back(src);
			}
			else 
			{
				std::shared_ptr<from_source> new_source = std::make_shared<from_source>();
				std::shared_ptr<from_field> new_field = std::make_shared<from_field>();
				new_field->field_name = _from_field;
				new_field->targets.push_back(dest);
				new_source->index = join_index++;
				new_source->source_name = dest->source_name;
				new_source->origins.push_back(src);
				new_source->fields.insert_or_assign(_from_field, new_field);
				sources.insert_or_assign(dest->source_name, new_source);
			}

		}

		json get_filters(json _class_filter)
		{
			json_parser jp;

			json class_filter = _class_filter;
			json dest_array = jp.create_array();

			from_sources::iterator _it = sources.begin();
			get_filters_impl(dest_array, class_filter, _it);
			return dest_array;
		}


		json get_data(std::string _source_name)
		{
			std::shared_ptr<json_array> jsarray = std::make_shared<json_array>();
			auto sit = sources.find(_source_name);
			if (sit != sources.end()) {
				for (auto dit : sit->second->data) {
					jsarray->elements.push_back(dit.second);
				}
			}
			return json(jsarray);
		}

		void add_data(json _class_filter, json& _src_array)
		{
			json_parser jp;
			
			std::shared_ptr<from_source> csp, cs;

			std::string source_name = _class_filter["name"];
			json parent_object;

			// first, own data gets added;

			auto csi = sources.find(source_name);
			if (csi == sources.end())
			{
				std::shared_ptr<from_source> new_source = std::make_shared<from_source>();
				std::shared_ptr<from_field> new_field = std::make_shared<from_field>();
				new_source->index = join_index++;
				new_source->source_name = source_name;
				sources.insert_or_assign(source_name, new_source);
			}
			csi = sources.find(source_name);
			if (csi != sources.end())
			{
				for (auto item : _src_array) {
					int64_t object_id = item.get_member(object_id_field);
					csi->second->data.insert_or_assign(object_id, item.object_impl());
				}

				std::string parent_source = _class_filter["parent_source"];
				int64_t parent_id = (int64_t)_class_filter["parent_id"];
				if (not parent_source.empty()) {
					auto cspi = sources.find(parent_source);
					if (cspi != sources.end())
					{
						auto fit = cspi->second->data.find(parent_id);
						if (fit != cspi->second->data.end())
						{
							std::shared_ptr<json_array> children;
							auto ci = fit->second->members.find(source_name);
							if (ci == fit->second->members.end()) {
								children = std::make_shared<json_array>();
								fit->second->members.insert_or_assign(source_name, children);
							}
							else 
							{
								children = std::dynamic_pointer_cast<json_array>(ci->second);
								if (not children) {
									children = std::make_shared<json_array>();
									fit->second->members.insert_or_assign(source_name, children);
								}
							}
							children->elements.insert(children->elements.end(), _src_array.array_impl()->elements.begin(), _src_array.array_impl()->elements.end());
						}
					}
				}

			}

		}
	};

	class child_bridge_interface
	{
	public:
		virtual std::string get_class_name() = 0;
		virtual void set_class_name(std::string _class_name) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(json& _src) = 0;
		virtual void copy(json& _dest, json& _src) = 0;
		virtual json get_key(json& src) = 0;
	};

	class child_bridges_interface
	{
	public:
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(json& _src) = 0;

		virtual std::shared_ptr<child_bridge_interface> get_bridge(std::string _class_name) = 0;
		virtual std::vector<std::string> get_bridge_list() = 0;

		virtual void init_validation() = 0;
		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual json get_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) = 0;
		virtual json delete_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) = 0;
	};

	class field_options_interface
	{
	public:

		virtual void get_json(json& _dest) = 0;
		virtual void put_json(json& _src) = 0;
		virtual void init_validation() = 0;
		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json & _object) = 0;
		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) = 0;
		virtual std::shared_ptr<child_bridges_interface> get_bridges() = 0;
		virtual bool is_relational_children() = 0;
		virtual json get_openapi_schema(corona_database_interface* _db) = 0;
        virtual bool is_required() = 0;
		virtual bool is_server_only() = 0;
	};

	class class_interface_base
	{
	public:
		virtual ~class_interface_base() {
			;
		}
	};

	class field_interface {
	protected:
		std::string field_name;
		field_types field_type;
		std::string field_class;
		std::shared_ptr<field_options_interface> options;

	public:

		virtual void init_validation() = 0;
		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(std::vector<validation_error>& _errors, json& _src, class_interface_base*_owner) = 0;

		virtual std::string get_field_name()
		{
			return field_name;
		}
		virtual std::string get_field_class()
		{
			return field_class;
		}
		virtual std::string set_field_class(const std::string& _field_class)
		{
			field_class = _field_class;
			return field_class;
		}
		virtual field_types get_field_type()
		{
			return field_type;
		}

		virtual std::shared_ptr<field_options_interface> get_options() {
			return options;
		}
		
		virtual std::shared_ptr<field_options_interface> set_options(std::shared_ptr<field_options_interface> _src) {
			options = _src;
			return options;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json& _object) = 0;
		virtual std::shared_ptr<child_bridges_interface> get_bridges() = 0;

        virtual json get_openapi_schema(corona_database_interface* _db) = 0;
		virtual bool is_relational_children() {
            if (options) {
                return options->is_relational_children();
            }
			return false;
		}

	};

	class index_interface
	{
	protected:
	public:
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(std::vector<validation_error>& _errors, json& _src) = 0;

		virtual int64_t									get_index_id() = 0;
		virtual std::string								get_index_name() = 0;
		virtual std::vector<std::string>				&get_index_keys() = 0;
		virtual std::shared_ptr<xtable>					get_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					create_xtable(corona_database_interface *_db, std::map<std::string, std::shared_ptr<field_interface>>& _fields) = 0;
		virtual std::string								get_index_key_string() = 0;
		virtual std::string								get_index_filename() = 0;
	};

	class activity;

	class class_interface : public shared_lockable, public class_interface_base
	{
	protected:
	public:

		virtual bool is_server_only(const std::string& _field_name) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(std::vector<validation_error>& _errors, json& _src) = 0;

		virtual bool ready() {
			return false;
		}

		virtual std::string								get_class_name()  const = 0;
		virtual std::string								get_class_description()  const = 0;
		virtual std::string								get_base_class_name()  const = 0;
        virtual std::string                             get_grid_template_rows() const = 0;
		virtual std::string                             get_grid_template_columns() const = 0;
		virtual std::string                             get_class_color() const = 0;
		virtual std::string                             get_class_display () const = 0;
		virtual std::map<std::string, bool>  const&		get_descendants()  const = 0;
		virtual std::map<std::string, bool>  const&		get_ancestors()  const = 0;
		virtual std::map<std::string, bool>  &			update_descendants() = 0;
		virtual std::map<std::string, bool>  &			update_ancestors() = 0;
		virtual std::vector<std::string>				get_parents() const = 0;

		// creates whatever table representation
		virtual std::shared_ptr<xtable_interface>		create_table(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable_interface>		get_table(corona_database_interface* _db) = 0;

		// creates a sql server table with an index table backing it.
		virtual std::shared_ptr<sql_table>				create_stable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<sql_table>				get_stable(corona_database_interface* _db) = 0;

        // creates a no-sql table with multiple optional indexes
		virtual std::shared_ptr<xtable>					create_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					get_xtable(corona_database_interface* _db) = 0;

		virtual std::shared_ptr<xtable>					alter_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					find_index(corona_database_interface* _db, json& _keys) const = 0;
		virtual	bool									open(activity* _context, json _existing_definition, int64_t _location) = 0;
		virtual	bool									update(activity* _context, json _changed_class) = 0;
		virtual	bool									create(activity* _context, json _changed_class) = 0;

		virtual	void									put_field(std::shared_ptr<field_interface>& _name) = 0;
		virtual std::shared_ptr<field_interface>		get_field(const std::string& _name)  const = 0;
		virtual std::vector<std::shared_ptr<field_interface>> get_fields()  const = 0;

		virtual std::shared_ptr<index_interface>		get_index(const std::string& _name)  const = 0;
		virtual std::vector<std::shared_ptr<index_interface>> get_indexes()  const = 0;

		virtual void	init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;

		virtual json	get_object(corona_database_interface* _db, int64_t _object_id, class_permissions _grant, bool& _exists) = 0;
		virtual void	put_objects(corona_database_interface* _db, json& _children, json& _src_objects, class_permissions _grant) = 0;
		virtual json	get_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;
		virtual json	delete_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;
		virtual json    get_single_object(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;

		virtual void	run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json& _target) = 0;
		virtual void	clear_queries(json& _target) = 0;

		virtual json	get_info(corona_database_interface* _db) = 0;
        virtual json	get_openapi_schema(corona_database_interface* _db) = 0;

		virtual bool	any(corona_database_interface* _db) = 0;
		virtual bool	any_descendants(corona_database_interface* _db) = 0;

		virtual int64_t get_next_object_id() = 0;
	};

	using read_class_sp = read_locked_sp<class_interface>;
	using write_class_sp = write_locked_sp<class_interface>;

	class activity
	{
	public:
		corona_database_interface* db;

		std::map<std::string, write_class_sp> classes;
		std::vector<validation_error> errors;

		class_interface* get_class(std::string _class_name);
		class_interface* create_class(std::string _class_name);
	};

	class corona_simulation_interface
	{
	public:

		virtual void on_frame(json& _commands) = 0;
	};

	class corona_database_interface 
	{
	protected:

	public:
		corona_connections connections;
		std::string onboarding_email;
		std::string recovery_email;

		corona_database_interface()
		{
			
		}

		virtual json create_database() = 0;
		virtual relative_ptr_type open_database() = 0;

		virtual void apply_config(json _config) = 0;
		virtual json apply_schema(json _schema) = 0;

		virtual std::string get_random_code() = 0;

		virtual class_permissions get_class_permission(
			std::string _user_name,
			std::string _class_name) = 0;

		virtual class_permissions get_class_permission(
			const class_permissions& _src,
			std::string _class_name) = 0;

		virtual std::shared_ptr<class_interface> read_get_class(const std::string& _class_name) = 0;
		virtual std::shared_ptr<class_interface> put_class_impl(activity* _activity, json& _class_definition) = 0;
		virtual std::shared_ptr<class_interface> get_class_impl(activity* _activity, std::string _class_name) = 0;

		virtual void scrub_object(json& object_to_scrub) = 0;
		virtual json create_user(json create_user_request, bool _trusted_user, bool _system_user) = 0;
		virtual json login_user(json _login_request) = 0;
		virtual json login_user_sso(json _create_user_request) = 0;
		virtual json user_confirm_code(json _login_request) = 0;
		virtual json user_send_code(json _send_user_request) = 0;
		virtual json set_user_password(json _set_password_request) = 0;
		virtual json get_classes(json get_classes_request) = 0;
		virtual json get_class(json get_class_request) = 0;
		virtual json put_class(json put_class_request) = 0;
		virtual json user_home(json user_home_request) = 0;
		virtual json user_set_team(json user_home_request) = 0;

		virtual json edit_object(json _edit_object_request) = 0;
		virtual json run_object(json _edit_object_request) = 0;
		virtual json create_object(json create_object_request) = 0;
		virtual json put_object(json put_object_request) = 0;
		virtual json get_object(json get_object_request) = 0;
		virtual json delete_object(json delete_object_request) = 0;

		virtual json copy_object(json copy_request) = 0;
		virtual json query(json query_request) = 0;

		virtual json select_object(std::string _class_name, int64_t _object_id, class_permissions _permissions) = 0;
		virtual json select_object(json _key, bool _include_children, class_permissions _permissions) = 0;
		virtual json select_single_object(json _key, bool _include_children, class_permissions _permissions) = 0;
		virtual read_class_sp read_lock_class(const std::string& _class_name) = 0;
		virtual write_class_sp write_lock_class(const std::string& _class_name) = 0;
		virtual json save_class(class_interface* _class_to_save) = 0;
		virtual bool check_message(json& _message, std::vector<std::string> _authorizations, std::string& _user_name, std::string& _token_authority) = 0;
		virtual json get_openapi_schema(std::string _user_name) = 0;
	};

	using class_method_key = std::tuple<std::string, std::string>;

	class field_options_base : public field_options_interface
	{
	public:
		bool		required;
		std::string format;
		std::string input_mask;
		std::string label;
		std::string description;
		std::string grid_row;
		std::string grid_column;
		std::string display;
		bool		read_only;
		bool		server_only;

		field_options_base() = default;
		field_options_base(const field_options_base& _src) = default;
		field_options_base(field_options_base&& _src) = default;
		field_options_base& operator = (const field_options_base& _src) = default;
		field_options_base& operator = (field_options_base&& _src) = default;
		virtual ~field_options_base() = default;

		virtual void get_json(json& _dest)
		{
			_dest.put_member("required", required);
			_dest.put_member("format", format);
			_dest.put_member("input_mask", input_mask);
			_dest.put_member("server_only", server_only);
			_dest.put_member("label", label);
			_dest.put_member("description", description);
			_dest.put_member("grid_row", grid_row);
			_dest.put_member("grid_column", grid_column);
			_dest.put_member("display", display);
			_dest.put_member("read_only", read_only);
		}

		virtual void put_json(json& _src)
		{
			required = (bool)_src["required"];
            server_only = (bool)_src["server_only"];
			read_only = (bool)_src["read_only"];
			format = (std::string)_src["format"];
            input_mask = (std::string)_src["input_mask"];	
            label = (std::string)_src["label"]; 
			description = (std::string)_src["description"];
			grid_row = (std::string)_src["grid_row"];
			grid_column = (std::string)_src["grid_column"];
			display = (std::string)_src["display"];
		}

		virtual void init_validation() override
		{

		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)  override
		{
			;
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json & _object)  override
		{
			json empty;
			return empty;
		}

		virtual bool accepts(corona_database_interface *_db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)  override
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

		virtual std::shared_ptr<child_bridges_interface> get_bridges() override
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

	};

	class child_bridge_implementation : public child_bridge_interface
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
			child_class_name = _src["child_class_name"];
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
					std::string _src_key = member.second;
					if (not (_src_key.empty() or _dest_key.empty()))
					{
						json value = _src[_src_key];
						_dest.share_member(_dest_key, value);
					}
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
				std::string _dest_key = member.first;
				std::string _src_key = member.second;
				if (not (_src_key.empty() or _dest_key.empty()))
				{
					json value = _src[_src_key];
					key.share_member(_dest_key, value);
				}
			}

			return key;
		}

	};

	class child_bridges: public child_bridges_interface
	{
	public:
		std::map<std::string, std::shared_ptr<child_bridge_interface>> base_constructors;
		std::map<std::string, std::shared_ptr<child_bridge_interface>> all_constructors;

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
				std::shared_ptr<child_bridge_implementation> new_bridge = std::make_shared<child_bridge_implementation>();
				json_parser jp;
				json copy_values = jp.create_object();
				for (auto pair : class_def->copy_values)
				{
					copy_values.put_member(pair.first, pair.second);
				}				
				new_bridge->put_child_object(class_def->class_name, copy_values);
				new_bridge->set_class_name(class_def->class_name);
				base_constructors.insert_or_assign(class_def->class_name, new_bridge);
			}
		}

		virtual void put_json(json& _src) override
		{
			base_constructors.clear();
			all_constructors.clear();
			auto members = _src.get_members();
			for (auto member : members) {
				json obj = member.second;
				std::shared_ptr<child_bridge_implementation> new_bridge = std::make_shared<child_bridge_implementation>();
				std::string class_name = member.first;
				new_bridge->put_json(obj);
				new_bridge->set_class_name(class_name);
				base_constructors.insert_or_assign(class_name, new_bridge);
			}
		}

		virtual std::shared_ptr<child_bridge_interface> get_bridge(std::string _class_name) override
		{
			std::shared_ptr<child_bridge_interface> result;
			auto iter = all_constructors.find(_class_name);
			if (iter != std::end(all_constructors))
				result = iter->second;
			return result;
		}

  		virtual std::vector<std::string> get_bridge_list() override
		{
			std::vector<std::string> results;
			for (auto item : all_constructors) {
				results.push_back(item.first);
			}
			return results;
		}

		virtual void init_validation() override
		{

		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)
		{
			all_constructors.clear();
			for (auto class_name_pair : base_constructors) {
				auto ci = _db->read_lock_class(class_name_pair.first);
				if (ci) {
					auto descendants = ci->get_descendants();
					for (auto descendant : descendants) {
						all_constructors.insert_or_assign(descendant.first, class_name_pair.second);
					}
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
				read_class_sp classy = _db->read_lock_class(class_name_pair.first);
                auto& derived_classes = classy->get_descendants();

                for (auto derived_class : derived_classes) {
                    if (read_classes.find(derived_class.first) != read_classes.end()) {
                        continue;
                    }
                    read_class_sp dclassy = _db->read_lock_class(derived_class.first);
                    if (dclassy) {
                        read_classes[derived_class.first] = true;
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

		std::shared_ptr<child_bridges> bridges;
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

			if (bridges) {
				json jctors = jp.create_object();
				bridges->get_json(jctors);
				_dest.share_member("child_objects", jctors);
			}
			if (field_type_names.contains(fundamental_type)) {
				_dest.put_member("fundamental_type", field_type_names[fundamental_type]);
			}
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);

			json jctors = _src["child_objects"];
			std::string fte = _src["fundamental_type"];
			if (allowed_field_types.contains(fte)) {
				fundamental_type = allowed_field_types[fte];
			}

			bridges = std::make_shared<child_bridges>();
			if (jctors.object()) {
				bridges->put_json(jctors);
			}
		}

		virtual void put_definition(child_object_definition& _cod)
		{
			fundamental_type = _cod.fundamental_type;
			bridges = std::make_shared<child_bridges>();
			bridges->put_child_object(_cod);
		}

		virtual void init_validation() override
		{

		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)
		{
			if (bridges)
				bridges->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
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
						else if (bridges)
						{
							if (obj.object()) {
								object_class_name = obj[class_name_field];
								auto ctor = bridges->get_bridge(object_class_name);
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

		virtual std::shared_ptr<child_bridges_interface> get_bridges() override
		{
			return bridges;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("array"));

			json jitems = jp.create_object();
			json joneof = jp.create_array();
			for (auto& bc : bridges->base_constructors) {
				json joneofi = jp.create_object();
				joneofi.put_member("$ref", "#/components/schemas/" + bc.first);
				joneof.push_back(joneofi);
			}

			json joneofi = jp.create_object();
			switch (fundamental_type)
			{
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

	class object_field_options : public field_options_base
	{
	public:
		std::shared_ptr<child_bridges> bridges;
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

			if (bridges) {
				json jctors = jp.create_object();
				bridges->get_json(jctors);
				_dest.share_member("child_objects", jctors);
			}
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);

			json jctors = _src["child_objects"];
			bridges = std::make_shared<child_bridges>();
			if (jctors.object()) {
				bridges->put_json(jctors);
			}
		}

		virtual void put_definition(child_object_definition& _cod)
		{
			fundamental_type = _cod.fundamental_type;
			bridges = std::make_shared<child_bridges>();
			bridges->put_child_object(_cod);
		}

		virtual bool is_relational_children() override
		{
			return fundamental_type == field_types::ft_none;
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)
		{
			if (bridges)
				bridges->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
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
				else if (bridges)
				{
					if (obj.object()) {
						object_class_name = obj[class_name_field];
						auto ctor = bridges->get_bridge(object_class_name);
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

		virtual std::shared_ptr<child_bridges_interface> get_bridges() override
		{
			return bridges;
		}

		virtual json get_openapi_schema(corona_database_interface* _db) override
		{
			json_parser jp;
			json schema = jp.create_object();
			schema.put_member("type", std::string("array"));

			json jitems = jp.create_object();
			json joneof = jp.create_array();
			for (auto& bc : bridges->base_constructors) {
				json joneofi = jp.create_object();
				joneofi.put_member("$ref", "#/components/schemas/" + bc.first);
				joneof.push_back(joneofi);
			}

			jitems.share_member("oneOf", joneof);
			schema.share_member("items", jitems);

			return schema;
		}

	};

	class string_field_options : public field_options_base
	{
	public:
		int maximum_length;
		int minimum_length;
		std::string match_pattern;
		std::vector<std::string> allowed_values;

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
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			field_options_base::put_json(_src);
			minimum_length = _src["min_length"];
			maximum_length = _src["max_length"];
			match_pattern = _src["match_pattern"];
			for (auto& c : match_pattern) {
                if (c == '/') c = '\\';
			}

			allowed_values.clear();
			json jallowed_values = _src["enum"];
			if (jallowed_values.array()) {
				for (auto s : jallowed_values) {
					allowed_values.push_back(s);
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

		virtual bool accepts(corona_database_interface *_db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				std::string chumpy = (std::string)_object_to_test;

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
					ve.message = std::format( "Value '{0}' must be between {1} and {2} characters long", chumpy, minimum_length, maximum_length);
					if (match_pattern.size() > 0) {
						ve.message += std::format(" and must match '{0}'", match_pattern);
					}
					if (allowed_values.size())
					{
						std::string setx = join(allowed_values, ", ");
						ve.message += std::format(" and must be within '{0}'", setx);
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
			min_value = (int64_t)_src["min_value"];
			max_value = (int64_t)_src["max_value"];
		}

		virtual bool accepts(corona_database_interface *_db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				int64_t chumpy = (int64_t)_object_to_test;

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

		virtual void put_definition(reference_definition& _rd)
		{
			reference_class = _rd.base_class;
			reference_class_descendants.clear();
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			reference_class = _src["ref_class"];
			reference_class_descendants.clear();
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)
		{
			reference_class_descendants.clear();
			auto ci = _db->read_lock_class(reference_class);
			if (ci) {
				auto descendants = ci->get_descendants();
				for (auto descendant : descendants) {
					reference_class_descendants.insert_or_assign(descendant.first, true);
				}
			}
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				object_reference_type chumpy = (object_reference_type)_object_to_test;

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
					ve.message = std::format("Value '{0}' must be derived from {1}", (std::string)chumpy, reference_class);
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
			min_value = (date_time)_src["min_value"];
			max_value = (date_time)_src["max_value"];
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				date_time chumpy = (date_time)_object_to_test;
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

	template <typename scalar_type> 
	class general_field_options : public field_options_base
	{
	public:
		scalar_type min_value;
		scalar_type max_value;

		general_field_options() = default;
		general_field_options(const general_field_options& _src) = default;
		general_field_options(general_field_options&& _src) = default;
		general_field_options& operator = (const general_field_options& _src) = default;
		general_field_options& operator = (general_field_options&& _src) = default;
		virtual ~general_field_options() = default;

		virtual void get_json(json& _dest)
		{
			field_options_base::get_json(_dest);
			_dest.put_member("min_value", min_value);
			_dest.put_member("max_value", max_value);
		}

		virtual void put_json(json& _src)
		{
			field_options_base::put_json(_src);
			min_value = (scalar_type)_src["min_value"];
			max_value = (scalar_type)_src["max_value"];
		}

		virtual bool accepts(corona_database_interface *_db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
		{
			if (field_options_base::accepts(_db, _validation_errors, _class_name, _field_name, _object_to_test)) {
				bool is_legit = true;
				scalar_type chumpy = (scalar_type)_object_to_test;
				scalar_type zero = {};

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

			if constexpr (is_convertible_to_int<scalar_type>)
			{ 	
				int imin = min_value;
				int imax = max_value;
				if (imin) {
					schema.put_member("minimum", min_value);
				}
				if (imax) {
					schema.put_member("maximum", max_value);
				}
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

		virtual void init_validation() override
		{

		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions)
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
			id_field_name = _src["id_field_name"];
			description_field_name = _src["description_field_name"];
			options = jp.create_array();
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
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
						object_test_id = _object_to_test[id_field_name];
					}
					else if (_object_to_test.is_string())
					{
						object_test_id = _object_to_test;
					}
					else
						return false;

					for (auto option : options)
					{
						std::string id_field = option[id_field_name];
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
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			field_options_base::put_json(_src);
			query_body = _src["query"];
		}



		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object)
		{
			using namespace std::literals;
			json_parser jp;
			json this_query_body = query_body.clone();
			this_query_body.put_member("include_children", false);

			json froms = this_query_body["from"];
			if (froms.array()) {
				json new_from = jp.create_object();
				new_from.put_member(data_field, _object);
				new_from.put_member(class_name_field, _classname);
				new_from.put_member("name", "this"sv);
				auto arr = froms.array_impl();
				arr->elements.insert(arr->elements.begin(), new_from.object_impl());
			}
			this_query_body.put_member(token_field, _token);
			json query_results, query_data_results;
			query_results = _db->query(this_query_body);
			if (query_results[success_field]) {
				query_data_results = query_results[data_field];
			}
			else 
			{
				query_data_results = jp.create_array();
			}
			return query_data_results;
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test)
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

		virtual std::string get_field_name()
		{
			return field_name;
		}
		field_implementation& set_field_name(const std::string &_field_name)
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

		virtual void init_validation() override
		{
			class_permissions default_perms;
			if (options) options->init_validation();
		}

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) override
		{
			if (options) options->init_validation(_db, _permissions);
		}

		virtual bool accepts(corona_database_interface* _db, std::vector<validation_error>& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) override
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

		virtual void put_json(std::vector<validation_error>& _errors, json& _src, class_interface_base* _classdb)
		{

            class_interface* _classd = dynamic_cast<class_interface*>(_classdb);

			std::string s = _src["field_type"];
			auto aft = allowed_field_types.find(s);
			if (aft != std::end(allowed_field_types)) {
				field_type = aft->second;
			}
			field_name = _src["field_name"];
			field_class = _src["field_class"];

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
				options = std::make_shared<general_field_options<double>>();
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
			else if (field_type == field_types::ft_query)
			{
				options = std::make_shared<query_field_options>();
				options->put_json(_src);
			}
			else if (field_type == field_types::ft_function)
			{
				;
			}
			else
			{
				reference_definition rd = reference_definition::parse_definition(s.c_str());

				if (not rd.is_undefined)
				{
					auto obj_options = std::make_shared<reference_field_options>();
					obj_options->put_definition(rd);
					options = obj_options;
				}
				else {

					child_object_definition cod = child_object_definition::parse_definition(s.c_str(), _classd->get_class_name());

					if (not cod.is_undefined)
					{
						if (not cod.is_array)
						{
							auto obj_options = std::make_shared<object_field_options>();
							obj_options->put_definition(cod);
							options = obj_options;
						}
						else
						{
							auto arr_options = std::make_shared<array_field_options>();
							arr_options->put_definition(cod);
							options = arr_options;
						}
					}
				}
			}
		}

		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _object) override
		{
			json results;
			if (options) {
				results = options->run_queries(_db, _token, _classname, _object);
			}
			return results;
		}

		virtual std::shared_ptr<child_bridges_interface> get_bridges() override
		{
			if (options) {
				return options->get_bridges();
			}
			return nullptr;
		}
	};

	class index_implementation : public index_interface
	{
		int64_t index_id;
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

		index_implementation(std::string &_name, std::vector<std::string> &_keys, corona_database_interface* _db)
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

		virtual std::string get_index_key_string() override
		{
			return join(index_keys, ".");
		}
		
		virtual std::vector<std::string> &get_index_keys() override
		{
			return index_keys;
		}

		virtual void get_json(json& _dest) override
		{
			json_parser jp;
			_dest.put_member("index_name", index_name);

			json jindex_keys = jp.create_array();
			for (auto ikey : index_keys) 
			{
				jindex_keys.push_back(ikey);
			}
			_dest.share_member("index_keys", jindex_keys);
		}

		virtual void put_json(std::vector<validation_error>& _errors, json& _src) override
		{			
			index_name = _src["index_name"];

			json jindex_keys = _src["index_keys"];
			if (jindex_keys.array())
			{
				index_keys.clear();
				bool add_object_id = true;
				for (auto key : jindex_keys) {
					std::string key_name = key;
					if (key_name == object_id_field)
						add_object_id = false;
					index_keys.push_back(key);
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

		index_implementation& set_index_name(const std::string& _name)
		{
			index_name = _name;
			return *this;
		}

        virtual std::string get_index_filename() override { return index_implementation::get_index_name() + ".coronaidx"; }

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

                    if ((ifs->get_field_name() == object_id_field || ifs->get_field_name()==class_id_field)) {
						object_columns.columns[column_id] = cx;
                    }
				}
				column_id++;
            }

			auto table_header = std::make_shared<xtable_header>();
			table_header->key_members = key_columns;
			table_header->object_members = object_columns;
			table = std::make_shared<xtable>(get_index_filename(), table_header);
			return table;
		}

		virtual std::shared_ptr<xtable> get_xtable(corona_database_interface* _db) override
		{
			if (!table)
				table = std::make_shared<xtable>(get_index_filename());
			return table;
		}

	};


	class class_implementation : public class_interface
	{

	protected:
		std::string class_name;
		std::string class_description;
		std::string base_class_name;
		std::string grid_template_rows;
		std::string grid_template_columns;
		std::string display;
		std::string class_color;
		std::vector<std::string> parents;
		std::map<std::string, std::shared_ptr<field_interface>> fields;
		std::map<std::string, std::shared_ptr<index_interface>> indexes;
		std::map<std::string, bool> ancestors;
		std::map<std::string, bool> descendants;
		std::shared_ptr<sql_integration> sql;

		std::string get_class_filename() { return class_name + ".coronaclass"; }

		void copy_from(const class_interface* _src)
		{
			class_name = _src->get_class_name();
			class_description = _src->get_class_description();
			base_class_name = _src->get_base_class_name();
			parents = _src->get_parents();
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
            grid_template_rows = _src->get_grid_template_rows();
            grid_template_columns = _src->get_grid_template_columns();
			class_color = _src->get_class_color();
			display = _src->get_class_display();
		}

		std::shared_ptr<xtable> table;
		std::shared_ptr<sql_table> stable;

	public:

		class_implementation()
		{
		} 

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

		virtual int64_t get_next_object_id()
		{
            return table->get_next_object_id();
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

		virtual std::string get_grid_template_rows() const override
		{
			return grid_template_rows;
		}

		virtual std::string get_grid_template_columns() const override
		{
			return grid_template_columns;
		}

        virtual std::string get_class_color() const override
        {
            return class_color;
        }

		virtual std::string get_class_display() const override
		{
			return display;
		}

		virtual std::vector<std::string> get_parents() const override
		{
			return parents;
		}

		virtual bool ready() override 
		{
			return table.get() != nullptr;
		}

		virtual std::shared_ptr<xtable> create_xtable(corona_database_interface* _db) override
		{		
			auto table_header = std::make_shared<xtable_header>();
			int column_id = 1;
			for (auto &f : fields) {
			
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
			table = std::make_shared<xtable>(get_class_filename(), table_header);
			return table;
		}

		virtual std::shared_ptr<xtable> get_xtable(corona_database_interface* _db) override
		{
			if (!table) {
				table = std::make_shared<xtable>(get_class_filename());
			}
			return table;
		}

		/// <summary>
		/// Alters an xtable in the database, updating its structure and optionally migrating data.
		/// </summary>
		/// <param name="_db">Pointer to the corona_database_interface representing the database connection.</param>
		/// <returns>A shared pointer to the current xtable after alteration.</returns>
		virtual std::shared_ptr<xtable> alter_xtable(corona_database_interface* _db) override
		{
			std::shared_ptr<xtable> current_table, new_table;

			if (std::filesystem::exists(get_class_filename()))
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

				std::string file_name_old = get_class_filename();
				std::string file_name_new = "new_";
				file_name_new += get_class_filename();

				new_table = std::make_shared<xtable>(file_name_new, table_header);
				json_parser jp;
				json empty = jp.create_object();
				current_table->for_each(empty, [new_table](json& _src)->relative_ptr_type {
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
				current_table = std::make_shared<xtable>(get_class_filename(), table_header);
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
			table = std::make_shared<xtable>(get_class_filename());

			// we're going to make our xtable anyway so we can slap our object id 
			// on top of a sql server primary key
			// but we don't do this all the time, because we'd like to keep the data around.

			table = get_xtable(_db);
			return stable;
		}

		virtual std::shared_ptr<xtable_interface> create_table(corona_database_interface* _db) override
		{
			if (sql)
			{
				stable = create_stable(_db);
			}
			else {
				table = create_xtable(_db);
				table->commit();
				table = nullptr;
				table = std::make_shared<xtable>(get_class_filename());
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

		virtual std::map<std::string, bool>  & update_descendants()  override
		{
			return descendants;
		}

		virtual std::map<std::string, bool>  & update_ancestors()  override
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
			_dest.put_member("grid_template_rows", grid_template_rows);
			_dest.put_member("grid_template_columns", grid_template_columns);
			_dest.put_member("class_color", class_color);
			_dest.put_member("display", display);

			json ja = jp.create_array();
			for (auto p : parents)
			{
				ja.push_back(p);
			}
			_dest.share_member("parents", ja);

			if (fields.size() > 0) {
				json jfield_object = jp.create_object();
				for (auto field : fields) {
					json jfield_definition = jp.create_object();
					field.second->get_json(jfield_definition);
					jfield_object.share_member(field.first, jfield_definition);
				
				}
				_dest.share_member("fields", jfield_object);
			}

			if (indexes.size() > 0) {
				json jindex_object = jp.create_object();
				for (auto index : indexes) {
					json jindex_definition = jp.create_object();
					index.second->get_json(jindex_definition);
					jindex_object.share_member(index.first, jindex_definition);
				}
				_dest.share_member("indexes", jindex_object);
			}

			if (ancestors.size() > 0) {
				json jancestor_array = jp.create_array();
				for (auto class_ancestor : ancestors) {
					jancestor_array.push_back(class_ancestor.first);
				}
				_dest.share_member("ancestors", jancestor_array);
			}

			if (descendants.size() > 0) {
				json jdescendants_array = jp.create_array();
				for (auto class_descendant : descendants) {
					jdescendants_array.push_back(class_descendant.first);
				}
				_dest.share_member("descendants", jdescendants_array);
			}

			if (sql) {
				json jsql = jp.create_object();
				sql->get_json(jsql);
				_dest.share_member("sql", jsql);
			}
		}

		virtual void put_json(std::vector<validation_error>& _errors, json& _src)
		{

			json jfields, jindexes, jancestors, jdescendants, jtable_fields;

			class_name = _src[class_name_field];
			class_description = _src["class_description"];
			base_class_name = _src["base_class_name"];
			grid_template_rows = _src["grid_template_rows"];
			grid_template_columns = _src["grid_template_columns"];
			class_color = _src["class_color"];
			display = _src["display"];

			if (base_class_name == class_name) {
				validation_error ve;
				ve.class_name = class_name;
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "class can't be derived from itself";
				_errors.push_back(ve);
			}

			parents.clear();
			json jparents = _src["parents"];

			if (jparents.is_string())
			{
				std::string jparento = (std::string)jparents;
				parents.push_back(jparento);
			}
			else if (jparents.array())
			{
				for (auto jparent : jparents) {
					parents.push_back((std::string)(jparent));
				}
			}

			jtable_fields = _src["table_fields"];

			ancestors.clear();
			jancestors = _src["ancestors"];
			if (jancestors.array())
			{
				for (auto jancestor : jancestors)
				{
					std::string ancestor = jancestor;
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
					std::string descendant = jdescendant;
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
				if (jfields.has_member(parent)) {
					continue;
				}
				else {
					std::string parent_name = (std::string)parent;
					jfields.put_member(parent_name, std::string("int64"));
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
						auto fi = allowed_field_types.find(jfield.second);
						if (fi != std::end(allowed_field_types)) {
							field->set_field_type( fi->second);
						}
						else {
							auto parse_temp = (std::string)jfield.second;
							cod = child_object_definition::parse_definition(parse_temp.c_str(), get_class_name());
							if (cod.is_undefined)
							{
								rd = reference_definition::parse_definition(parse_temp.c_str());
								if (not rd.is_undefined) {
									field->set_field_type(field_types::ft_reference);
								}
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
						for (auto jfield_grant : jfield.second) {
							if (jfield_grant.is_string())
							{
								std::shared_ptr<child_object_class> coc = std::make_shared<child_object_class>();
                                coc->class_name = jfield_grant;
								coc->copy_values.insert_or_assign(class_name, object_id_field);
								cod.child_classes.push_back( coc );
							}
                        }
						auto afo = std::make_shared<array_field_options>();
						afo->put_definition(cod);
						field->set_options(afo);
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
						ve.message = "Invalid field type '" + (std::string)jfield.second;
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

			indexes.clear();
			jindexes = _src["indexes"];

			if (jindexes.empty())
			{
				json_parser jp;
				jindexes = jp.create_object();
			}

			for (auto parent : parents)
			{
				std::string index_name = std::format("idx_{0}_{0}", class_name, parent);
				if (jindexes.has_member(index_name)) {
					continue;
				}
				else 
				{
					json_parser jp;
					json new_index = jp.create_object();
					new_index.put_member("index_name", index_name);
					json new_index_keys = jp.create_array();
					new_index_keys.push_back(parent);
					new_index.share_member("index_keys", new_index_keys);
					jindexes.share_member(index_name, new_index);
				}
			}

			if (jindexes.object()) {
				auto jindex_members = jindexes.get_members();
				for (auto jindex : jindex_members) {
					std::shared_ptr<index_implementation> index = std::make_shared<index_implementation>();
					index->put_json(_errors, jindex.second);
					index->set_index_name(jindex.first);

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
				indexes.insert_or_assign(backing_index_name, idx);
			}

		}

		virtual void clear_queries(json& _target) override
		{

			json_parser jp;
			for (auto fldpair : fields) {
				auto query_field = fldpair.second;
				if (query_field->get_field_type() == field_types::ft_query) {
					json empty_array = jp.create_array();
					_target.share_member(query_field->get_field_name(), empty_array);
				}
			}
		}

		virtual void run_queries(corona_database_interface* _db, std::string& _token, std::string& _classname, json& _target) override
		{
			for (auto fldpair : fields) {
				auto query_field = fldpair.second;
				if (query_field->get_field_type() == field_types::ft_query) {
					json objects = query_field->run_queries(_db, _token, _classname, _target);
					_target.share_member(query_field->get_field_name(), objects);
				}
			}
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
			
			if (not std::filesystem::exists(get_class_filename())) {
				std::string stuff = std::format("Attempt to open {0} but not created", (std::string)definition[class_name_field]);
				throw std::logic_error(stuff);
			}

			if (_context->errors.size())
			{
				system_monitoring_interface::active_mon->log_warning(std::format("Errors on updating class {0}", class_name), __FILE__, __LINE__);
				for (auto error : _context->errors) {
					system_monitoring_interface::active_mon->log_information(std::format("{0} {1} {2}  @{3} {4}", error.class_name, error.field_name, error.message, error.filename, error.line_number), __FILE__, __LINE__);
				}

				return false;
			}

            get_table(_context->db);
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
			sql = changed_class.sql;

			ancestors.clear();

			if (not base_class_name.empty()) {

				auto base_class = _context->db->read_get_class(base_class_name);
				if (base_class and base_class->ready()) {

					if (class_color.empty()) {
                        class_color = base_class->get_class_color();
					}

					ancestors = base_class->get_ancestors();
					ancestors.insert_or_assign(base_class_name, true);
					base_class->update_descendants().insert_or_assign(class_name, true);
					descendants.insert_or_assign(class_name, true);

					for (auto temp_field : base_class->get_fields())
					{
						changed_class.fields.insert_or_assign(temp_field->get_field_name(), temp_field);
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

				if (std::filesystem::exists(new_index.second->get_index_filename()))
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
					class_data->for_each(empty_key, [idx_table](json& _item) -> relative_ptr_type {
						idx_table->put(_item);
						return 1;
						});
					system_monitoring_interface::active_mon->log_job_section_stop("index:", new_index.first, tx.get_elapsed_seconds(), __FILE__, __LINE__);

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
				else {
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

					for (auto temp_field : base_class->get_fields())
					{
						fields.insert_or_assign(temp_field->get_field_name(), temp_field);
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

		virtual std::vector<std::shared_ptr<field_interface>> get_fields()  const override
		{
			std::vector<std::shared_ptr<field_interface>> fields_list;
			for (auto fld : fields) {
				fields_list.push_back(fld.second);
			}
			return fields_list;
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

			if (not std::filesystem::exists(get_class_filename()))
			{
                throw std::logic_error(get_class_filename() + " not found");
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
					if ((std::string)result["created_by"] != _grant.user_name) {
						result = jp.create_object();
					}
                }
                else if (_grant.get_grant == class_grants::grant_team) {
					if ((std::string)result["team"] != _grant.team_name) {
						result = jp.create_object();
					}
                }
				else if (_grant.get_grant == class_grants::grant_teamorown) {
					if (((std::string)result["team"] != _grant.team_name) || (std::string)result["created_by"] != _grant.user_name) {
						result = jp.create_object();
					}
				}

				result.put_member("class_color", class_color);
			}
			else 
			{
				_exists = false;
			}


			return result;
		}

		virtual void put_objects(corona_database_interface* _db, json& _child_objects, json& _src_list, class_permissions _grant) override
		{
			bool index_ready = true;

			json_parser jp;

			struct index_object_pair {
				std::shared_ptr<index_interface> index;
				json objects_to_add;
				json objects_to_delete;
			};

			std::vector<index_object_pair> index_updates;

			for (auto idx : indexes) 
			{
				index_object_pair iop;
				iop.index = idx.second;
				iop.objects_to_add = jp.create_array();
				iop.objects_to_delete = jp.create_array();
				index_updates.push_back(iop);
			}

			json put_list = jp.create_array();

			for (auto _src_obj : _src_list)
			{

				int64_t parent_object_id = (int64_t)_src_obj[object_id_field];

				bool exists = false;
				json old_object = get_object(_db, parent_object_id, _grant, exists);
				json write_object;

				if (old_object.object()) {
					write_object = old_object.clone();
					write_object.merge(_src_obj);
				}
				else if (not exists)
				{
					write_object = _src_obj;
				}
				else 
				{
					// in this case, exists == true, but old_object is empty. This means that the object
					// was there, but the user did not have permissions to change it.
					continue;
				}

				write_object.erase_member("class_color");

				bool use_write_object = false;

				if (_grant.put_grant == class_grants::grant_any)
				{
					use_write_object = true;
				}
				else if (_grant.put_grant == class_grants::grant_own)
				{
					std::string owner = (std::string)write_object["created_by"];
					if (_grant.user_name == owner) {
						use_write_object = true;
					}
                }
                else if (_grant.put_grant == class_grants::grant_team)
                {
                    std::string team = (std::string)write_object["team"];
                    if (_grant.team_name == team) {
                        use_write_object = true;
                    }
                }
                else if (_grant.put_grant == class_grants::grant_teamorown)
                {
                    std::string owner = (std::string)write_object["created_by"];
                    std::string team = (std::string)write_object["team"];
                    if (_grant.user_name == owner or _grant.team_name == team) {
                        use_write_object = true;
                    }
                }

				auto these_fields = get_fields();

				for (auto& fld : these_fields) {
					if (fld->get_field_type() == field_types::ft_array)
					{
						json array_field = write_object[fld->get_field_name()];
						if (array_field.array() and fld->is_relational_children()) {
							auto bridges = fld->get_bridges();
							if (bridges) {
								for (auto obj : array_field) {
									std::string obj_class_name = obj[class_name_field];
									auto bridge = bridges->get_bridge(obj_class_name);
									if (bridge) {
										bridge->copy(obj, write_object);
									}
									_child_objects.push_back(obj);
								}
								json empty_array = jp.create_array();
								write_object.erase_member(fld->get_field_name());
							}
						}
					}
					else if (fld->get_field_type() == field_types::ft_object)
					{
						json obj = write_object[fld->get_field_name()];
						if (obj.object() and fld->is_relational_children()) {
							std::string obj_class_name = obj[class_name_field];
							auto bridges = fld->get_bridges();
							if (bridges) {
								auto bridge = bridges->get_bridge(obj_class_name);
								if (bridge) {
									bridge->copy(obj, write_object);
								}
								json empty;
								write_object.erase_member(fld->get_field_name());
								_child_objects.push_back(obj);
							}
						}
					}
				}

				if (use_write_object) {
					put_list.array_impl()->elements.push_back(write_object.object_impl());
				}

				if (index_updates.size() > 0)
				{
					int64_t object_id = (int64_t)write_object[object_id_field];
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
							iop.objects_to_add.push_back(obj_to_add);
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
			
			auto tb = get_xtable(_db);

			tb->put_array(put_list);
			tb->commit();

			auto stb = get_stable(_db);
			if (stb) {
				stb->put_array(put_list);
			}

			for (auto& iop : index_updates)
			{
				auto idx_table = iop.index->get_xtable(_db);
				idx_table->erase_array(iop.objects_to_delete);
				idx_table->put_array(iop.objects_to_add);
				idx_table->commit();
			}
		}

		virtual json get_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant)
		{
			// Now, if there is an index set specified, let's go see if we can find one and use it 
			// rather than scanning the table

			json_parser jp;
			json obj;
			obj = jp.create_array();

			if (_key.has_member(object_id_field)) {
				int64_t object_id = (int64_t)_key[object_id_field];
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
					json temp_result = jp.create_array();

 					temp = index_table->select(_key, [](json& _item) -> json {
						return _item;
					});

					for (auto obi : temp) {
						if (_key.compare(obi) != 0) {
							continue;
                        }	
                        int64_t object_id = (int64_t)obi[object_id_field];	
                        json bojdetail = _db->select_object(class_name, object_id, _grant);
                        json detail = bojdetail.get_first_element();
						temp_result.push_back(detail);
					}
					obj = temp_result;
				}
				else
				{
					auto class_data = get_table(_db);
                    obj = class_data->select(_key, [&_key, &_grant](json& _j)-> json
						{
							json result;
							if (_key.compare(_j) == 0
								and (_grant.get_grant == class_grants::grant_any
									or (_grant.get_grant == class_grants::grant_own || _grant.get_grant == class_grants::grant_teamorown
										and (std::string)_j["created_by"] == _grant.user_name
										)
									)
								or (_grant.get_grant == class_grants::grant_team || _grant.get_grant == class_grants::grant_teamorown
									and (std::string)_j["team"] == _grant.team_name
									)
							)
							{
								result = _j;
							}
							return result;
						});
				}
			}

			if (_include_children) {
                auto obj_items = obj.array_impl();
				for (auto item : obj_items->elements)
				{
                    json _src_obj(item);
					for (auto& fpair : fields) {
						auto& fld = fpair.second;

						if (fld->is_relational_children()) {
							if (fld->get_field_type() == field_types::ft_array)
							{
								auto bridges = fld->get_bridges();
								if (bridges) {
									json results = bridges->get_children(_db, _src_obj, _grant);
									_src_obj.share_member(fld->get_field_name(), results);
								}
							}
							else if (fld->get_field_type() == field_types::ft_object)
							{
								auto bridges = fld->get_bridges();
								if (bridges) {
									json results = bridges->get_children(_db, _src_obj, _grant);
									json first = results.get_first_element();
									_src_obj.share_member(fld->get_field_name(), first);
								}
							}
						}
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

					if (index_table)
					{
						ob_found = index_table->select(key, [this, &backing_table](json& _item) -> json {
							int64_t object_id = (int64_t)_item[object_id_field];
							auto objfound = backing_table->get(object_id);
							return objfound;
						});
					}
					else
					{
						ob_found = backing_table->select(key, [&key](json& _j)
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
							object_id = (int64_t)old_value[object_id_field];
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
				table->commit();
			}

			for (auto ob : obj)
			{
				ob.put_member("class_color", class_color);
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
					or ((_permission.delete_grant == class_grants::grant_own || _permission.delete_grant == class_grants::grant_teamorown) and (std::string)_src_obj["created_by"] == _permission.user_name)
					or ((_permission.delete_grant == class_grants::grant_team || _permission.delete_grant == class_grants::grant_teamorown) and (std::string)_src_obj["team"] == _permission.team_name))
					{
					tb->erase(_src_obj);

					for (auto& fpair : fields) {
						auto& fld = fpair.second;
						if (not fld->is_relational_children()) {
							continue;
						}
						auto bridges = fld->get_bridges();
						bridges->delete_children(_db, _src_obj, _permission);
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
	
	class corona_database_header_data
	{
	public:
		int64_t object_id;
	};

	class corona_database : public corona_database_interface
	{
		shared_lockable allocation_lock,
						class_lock,
						database_lock,
						key_lock;

		json schema;

		std::map<class_method_key, json_function_function> functions;

		crypto crypter;

		bool watch_polling;

        const std::string auth_general = "auth-general"; // this is the general user, which is used for general operations
        const std::string auth_system = "auth-system"; // this is the system user, which is used for system operations
		const std::string auth_self = "auth-self"; // this is the self user, which is used for the case when a user wants his own record

		
		long import_batch_size = 10000;
		/*
		* authorizations in tokens, methods and progressions
		* 
		create_user->login_user
		login_user->send_confirmation_code, send_password_reset_code
		send_login_confirmation_code->receive_login_confirmation_code
		receive_login_confirmation_code->user - connected
		send_password_reset_code->receive_reset_password_code
		receive_reset_password_code->user - connected

		connected can:
		edit_object
		get_classes
		get_class
		put_class
		query_class
		create_object
		put_object
		get_object
		copy_object
		delete_object
		*/

		std::shared_ptr<xtable> classes;
		bool trace_check_class = false;

		void log_errors(std::vector<validation_error>& _errors)
		{
			for (auto err : _errors) {
				std::string msg = std::format("{0}.{1}: {2} @({3},{4})", err.class_name, err.field_name, err.message, err.filename, err.line_number);
				system_monitoring_interface::active_mon->log_warning(msg);
			}
		}

	public:

		virtual std::shared_ptr<class_interface> get_class_impl(activity* _activity, std::string _class_name)
		{
			std::shared_ptr<class_interface> cd;
			json_parser jp;

			class_cache.try_get(_class_name, cd);

			if (not cd) {
				std::shared_ptr<class_implementation> cdimp = std::make_shared<class_implementation>();
				json key = jp.create_object();
				key.put_member(class_name_field, _class_name);
				json class_def = classes->select(key, [&key](json& _target)-> json {
					if (key.compare(_target) == 0) {
						return _target;
					}
					json jx;
					return jx;
					});
				if (class_def.array() and class_def.size()>0) {
					activity get_activity;
					get_activity.db = this;
					try {
						cdimp->open(&get_activity, class_def.get_first_element(), -1);
					}
                    catch (std::exception& ex) {
                        system_monitoring_interface::active_mon->log_warning(std::format("Exception {0} on opening class {1}", ex.what(), _class_name), __FILE__, __LINE__);
                        return nullptr;
                    }
					cd = cdimp;
					class_cache.insert(_class_name, cd);
				}
			}
			return cd;
		}

		virtual std::shared_ptr<class_interface> put_class_impl(activity* _activity, json& _class_definition)
		{
			std::string class_name = _class_definition[class_name_field];
			auto ci = get_class_impl(_activity, class_name);
			if (ci) {
				activity activio;
				activio.db = this;
				ci->update(&activio, _class_definition);
			}
			else 
			{
				activity activio;
				activio.db = this;
				ci = std::make_shared<class_implementation>();
				ci->create(&activio, _class_definition);
			}
            save_class(ci.get());
			class_cache.insert(class_name, ci);
			return ci;
		}

		json create_database()
		{
			json result;
			timer method_timer;

			json created_classes;
			json_parser jp;

			date_time start_time = date_time::now();
			timer tx;

			using namespace std::literals;

			system_monitoring_interface::active_mon->log_job_start("create_database", "start", start_time, __FILE__, __LINE__);
		
			std::shared_ptr<xtable_header> class_data_header = std::make_shared<xtable_header>();

			class_data_header->key_members.columns[1] = { field_types::ft_string, 1, "class_name" };
			class_data_header->object_members.columns[2] = { field_types::ft_string, 2, "class_description" };
			class_data_header->object_members.columns[3] = { field_types::ft_string, 3, "base_class_name" };
			class_data_header->object_members.columns[4] = { field_types::ft_array, 4, "parents" };
			class_data_header->object_members.columns[5] = { field_types::ft_object, 5, "fields" };
			class_data_header->object_members.columns[6] = { field_types::ft_object, 6, "indexes" };
			class_data_header->object_members.columns[7] = { field_types::ft_array, 7, "ancestors" };
			class_data_header->object_members.columns[8] = { field_types::ft_array, 8, "descendants" };
			class_data_header->object_members.columns[9] = { field_types::ft_object, 9, "sql" };

			classes = std::make_shared<xtable>("classes.coronatbl", class_data_header);
			classes->commit();

			created_classes = jp.create_object();

			json response =  create_class(R"(
{	
	"class_name" : "sys_object",
	"class_description" : "Object",
	"class_color": "#bcbcbc",
    "grid_template_rows": "60px 60px 60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
    "display":"optional",
	"fields" : {			
			"object_id" : { 
				"field_type":"int64",
				"read_only": true,	
				"label": "Object Id",	
				"grid_row": "2",		
				"grid_column": "2"		
			},
			"class_name" : { 
				"field_type":"string",
				"read_only": true,	
				"label": "Class",	
				"grid_row": "2",		
				"grid_column": "1"		
			},
			"created" : {
				"field_type":"datetime",
				"read_only": true,	
				"label": "Created",	
				"grid_row": "3",		
				"grid_column": "1"		
			},
			"created_by" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Created By",	
				"grid_row": "3",		
				"grid_column": "2"		
			},
			"updated": {
				"field_type":"datetime",
				"read_only": true,	
				"label": "Updated",	
				"grid_row": "4",		
				"grid_column": "1"		
			},
 			"updated_by" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Updated By",	
				"grid_row": "4",		
				"grid_column": "2"		
			},
			"team" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Team",	
				"grid_row": "5",		
				"grid_column": "1"		
			}
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			json test =  classes->get(R"({"class_name":"sys_object"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_object after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_object", true);

			response = create_class(R"(
{	
	"class_name" : "sys_error",
	"class_description" : "Error",
	"base_class_name" : "sys_object",
	"class_color": "#d80000",
    "grid_template_rows": "60px 60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
    "display":"default",
	"fields" : {			
			"system" : {
				"field_type":"string",
				"read_only": true,
				"label": "System Origin",
				"grid_row":"2",
				"grid_column":"1"
			},
			"message" :{
				"field_type":"string",
				"read_only": true,
				"label": "Message",
				"grid_row":"3",
				"grid_column":"1"
			},
			"body": {
				"field_type":"string",
				"read_only": true,
				"label": "Details",
				"grid_row":"4",
				"grid_column":"1",
				"display": "json"	
			},
			"file" : {
				"field_type":"string",
				"read_only": true,
				"label": "Source File",
				"grid_row":"5",
				"grid_column":"1"				
			},
			"line": {
				"field_type":"number",
				"read_only": true,
				"label": "Source Line #",
				"grid_row":"5",
				"grid_column":"2"
			},
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_error"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_error after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_error", true);


			response = create_class(R"(
{	
	"class_name" : "sys_server",
	"class_description" : "Servers",
	"base_class_name" : "sys_object",
	"class_color": "#bcbcbc",
    "grid_template_rows": "60px 60px 120px 60px 60px",
	"grid_template_columns": "1fr 1fr",
    "display":"default",
	"fields" : {
			"server_name" : {
				"field_type":"string",
				"read_only": true,
				"label": "Server Name",
				"grid_row":"2",
				"grid_column":"1"
			},
			"server_description" : {
				"field_type":"string",
				"read_only": true,
				"label": "Description",
				"grid_row":"3",
				"grid_column":"1",
				"display":"markdown"
			},
			"server_url" : {
				"field_type":"string",
				"read_only": true,
				"label": "Url",
				"grid_row":"4",
				"grid_column":"1",
				"display": "url"	
			},
			"server_version": {
				"field_type":"string",
				"read_only": true,
				"label": "Version",
				"grid_row":"5",
				"grid_column":"1"
			}
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("sys_server put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_server"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_server after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_server", true);



			response = create_class(R"(
{	
	"class_name" : "sys_command",
	"class_description" : "Command",
	"base_class_name" : "sys_object",
	"class_color": "#bcbcbc",
	"fields" : {
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_command"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_command after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_command", true);



			response = create_class(R"(
{
	"class_name" : "sys_grant",
	"base_class_name" : "sys_object",
	"class_description" : "Grant",
	"parents" : [ "sys_team" ],
	"class_color": "#bcbcbc",
	"display":"default",
	"grid_template_rows": "60px 120px",
	"grid_template_columns": "1fr 1fr 1fr",
	"fields" : {
			"grant_classes" : "[string]",
			"get" : {
				"field_type":"string",
				"field_name":"get",
				"enum" : [ "any", "none", "own", "team", "teamorown" ],
				"display" : "dropdown",
				"grid_row":"2",
				"grid_column":"1"
			},
			"put" : {
				"field_type":"string",
				"field_name":"put",
				"enum" : [ "any", "none", "own", "team", "teamorown" ],
				"display" : "dropdown",	
				"grid_row":"3",
				"grid_column":"2"
			},
			"delete" : {
				"field_type":"string",
				"field_name":"delete",
				"enum" : [ "any", "none", "own", "team", "teamorown" ],
				"display" : "dropdown",
				"grid_row":"4",
				"grid_column":"3"
			},
			"alter" : {
				"field_type":"string",
				"field_name":"alter",
				"enum" : [ "any", "none", "own", "team", "teamorown" ],
				"display" : "dropdown",
				"grid_row":"5",
				"grid_column":"1"
			},
			"derive" : {
				"field_type":"string",
				"field_name":"derive",
				"enum" : [ "any", "none", "own", "team", "teamorown" ],
				"display" : "dropdown",
				"grid_row":"6",
				"grid_column":"2"
			},
			"class_colors": "object"
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_grant put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_grant"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_grant after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_grant", true);

			response = create_class(R"(
{
	"class_name" : "sys_status",
	"base_class_name" : "sys_object",
	"class_description" : "Ticket Status",
	"parents" : [ "sys_ticket" ],
	"class_color": "#bcbcbc",
	"display":"default",
	"grid_template_rows": "60px 120px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {
			"status_name" :{
				"field_type":"string",
				"label": "Status Name",
				"grid_row":"2",
				"grid_column":"1"
			},
			"status_description" : {
				"field_type":"string",
				"label": "Status Description",
				"display": "markdown",	
				"grid_row": "3",
				"grid_column": "1"
			},
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_status put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_status"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_status after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_status", true);

			response = create_class(R"(
{
	"class_name" : "sys_ticket",
	"base_class_name" : "sys_object",
	"class_description" : "Ticket",
	"parents" : [ "sys_team" ],
	"class_color": "#bcbcbc",
	"display":"default",
	"grid_template_rows": "60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {
			"ticket_name" :{
				"field_type":"string",
				"label": "Ticket Name",
				"display": "url",
				"grid_row": "2",
				"grid_column":"1"
			},
			"ticket_description" : {
				"field_type":"string",
				"label": "Ticket Name",
				"display": "markdown",	
				"grid_row": "3",
				"grid_column":"1"
			},
			"history" : "[sys_status]"
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_ticket put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_ticket"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_ticket after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_ticket", true);

			response = create_class(R"(
{
	"class_name" : "sys_workflow",
	"base_class_name" : "sys_object",
	"class_description" : "Workflow",
	"parents" : [ "sys_team" ],
	"class_color": "#bcbcbc",
	"grid_template_rows": "60px 120px 60px 60px 60px 120px 60px",
	"grid_template_columns": "1fr 1fr",
	"display":"default",
	"fields" : {
			"workflow_name" : {
				"field_type":"string",
				"label": "Workflow Name",
				"grid_row": "2",
				"grid_column":"1"
			},
			"workflow_description" : {
				"field_type":"string",
				"label": "Workflow Description",
				"grid_row": "3",
				"grid_column":"1"
			},
			"workflow_schedule_type" : {
				"field_type":"string",
				"display": "dropdown",
				"grid_row": "4",
				"grid_column":"1",
				"enum" : [ "Week", "Month" ],
			},
			"workflow_schedule_days" : {
				"field_type":"array",
				"fundamental_type":"number",
				"label": "Days to Run",
				"display": "days",
				"grid_row": "4",
				"grid_column":"2",
			},
			"workflow_schedule_hour" : {
				"field_type":"number",
				"label": "Hour to run",
				"display": "hour",
				"min_value" : 0,
				"max_value" : 23,
				"grid_row": "4",
				"grid_column":"3",
			},
			"ticket_class_name" : {
				"field_type":"string",
				"label": "Create Ticket",
				"display": "dropdown:sys_ticket",	
				"grid_row": "5",
				"grid_column":"1"
			},
			"ticket_name" : {
				"field_type":"string",
				"label": "Ticket Name",
				"grid_row": "6",
				"grid_column":"1"
			},
			"ticket_description" : {
				"field_type":"string",
				"label": "Ticket Description",
				"display": "markdown",
				"grid_row": "7",
				"grid_column":"1"
			},
			"last_ran" : {
				"field_type":"number",
				"label": "Last Run Day",
				"grid_row": "8",
				"grid_column":"1"
			},
			"last_result" : {
				"field_type":"string",
				"label": "Result",
				"grid_row": "8",
				"grid_column":"2"
			}
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_workflow put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_workflow"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_workflow after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_workflow", true);

			response = create_class(R"(
{
	"class_name" : "sys_team",
	"base_class_name" : "sys_object",
	"class_description" : "Team",
	"class_color": "#bcbcbc",
	"display":"default",
	"grid_template_rows": "60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {
			"team_name" :  {
				"field_name:" : "team_name",
				"field_type" : "string",
				"grid_row":"2",
				"grid_column":"1",
				"label":"Team Name"
			},
			"team_description" :  {
				"field_name:" : "team_description",
				"field_type" : "string",		
				"grid_row":"2",
				"grid_column":"2",
				"label":"Team Description"
			},
			"team_domain" : {
				"field_name:" : "team_domain",
				"field_type" : "string",		
				"format:" : "regexp",
				"grid_row":"3",
				"grid_column":"3",
				"label":"Team Domain"
			},
			"permissions" : "[ sys_grant ]",
			"inventory_classes" : "[ string ]",
			"allowed_teams" : "[ string ]",
			"tickets" : "[ sys_ticket ]",
			"workflow" : "[ sys_workflow ]",
			"items" : "[ sys_item ]"
	},
	"indexes" : {
        "sys_team_name": {
          "index_keys": [ "team_name" ]
        }
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_team put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_team"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_team after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_team", true);

			response = create_class(R"(
{
	"class_name" : "sys_dataset",
	"base_class_name" : "sys_object",
	"class_description" : "DataSet",
	"class_color": "#bcbcbc",
	"parents" : [ "sys_schema" ],
	"grid_template_rows": "60px 60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {
			"dataset_name" : {
				"field_name:" : "dataset_name",
				"field_type" : "string",		
				"grid_row":"2",
				"grid_column":"1",
				"label":"Name"
			},
			"dataset_description" : {
				"field_name:" : "dataset_description",
				"field_type" : "string",		
				"grid_row":"3",
				"grid_column":"1",
				"label":"Description"
			},
			"dataset_version" : {
				"field_name:" : "dataset_version",
				"field_type" : "string",		
				"grid_row":"2",
				"grid_column":"2",
				"label":"Version"
			},
			"dataset_author" : {
				"field_name:" : "dataset_author",
				"field_type" : "string",		
				"grid_row":"4",
				"grid_column":"1",
				"label":"Authors"
			},
			"dataset_source" : {
				"field_name:" : "dataset_source",
				"field_type" : "string",		
				"grid_row":"4",
				"grid_column":"2",
				"label":"Source"
			},
			"completed" : {
				"field_name:" : "completed",
				"field_type" : "datetime",		
				"grid_row":"5",
				"grid_column":"1",
				"label":"Completed"
			},
			"run_on_change": {
				"field_name:" : "run_on_change",
				"field_type" : "string",		
				"grid_row":"5",
				"grid_column":"2",
				"label":"Run On Change"
			},	
			"objects" : "array",
			"import" : "object"
	},
	"indexes" : {
		"sys_dataset_dataset_name": {
		  "index_keys": [ "dataset_name", "dataset_version" ]
		}	
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_dataset put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_dataset"})"_jobject);
			if (test.empty() or test.is_member("class_name", "SysParseError")) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_dataset after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_dataset", true);

			response =  create_class(R"(
{
	"class_name" : "sys_schema",
	"base_class_name" : "sys_object",
	"class_description" : "Schema",
	"class_color": "#bcbcbc",
	"grid_template_rows": "60px 60px 60px 60px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {		
			"schema_name" : {
				"field_name:" : "schema_name",
				"field_type" : "string",		
				"grid_row":"2",
				"grid_column":"1",
				"label":"Name"
			},
			"schema_description" : {
				"field_name:" : "schema_description",
				"field_type" : "string",		
				"grid_row":"3",
				"grid_column":"1",
				"label":"Description"
			},
			"schema_version" : {
				"field_name:" : "schema_version",
				"field_type" : "string",		
				"grid_row":"3",
				"grid_column":"2",
				"label":"Version"
			},
			"schema_authors" : {
				"field_name:" : "schema_author",
				"field_type" : "string",		
				"grid_row":"4",
				"grid_column":"1",
				"label":"Authors"
			},
			"classes" : "[object]",
			"users" : "[object]",
			"datasets" : [ "sys_dataset" ]
		}
	}
}
)");

			created_classes.put_member("sys_schema", true);

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_schema"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_schema after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);

				return result;
			}

			response = create_class(R"(
{	
	"base_class_name" : "sys_object",
	"class_name" : "sys_item",
	"class_description" : "Item",
	"parents": [ "sys_user", "sys_item", "sys_team" ],
	"class_color": "#bcbcbc",
	"display":"none",
	"fields" : {			
			
        }
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_item put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_item"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_item after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);

				return result;
			}
			created_classes.put_member("sys_item", true);

			response =  create_class(R"(
{	
	"base_class_name" : "sys_object",
	"class_name" : "sys_user",
	"class_description" : "User",
	"class_color": "#bcbcbc",
	"grid_template_rows": "60px 60px 60px 60px 60px 120px 60px",
	"grid_template_columns": "1fr 1fr",
	"fields" : {			
			"first_name" : {
				"field_type":"string",
				"field_name":"first_name",
				"label": "First Name",
				"required" : true,
				"max_length" : 50,
				"match_pattern": "[a-zA-Z0-9/s]+",
				"grid_row": "2",
				"grid_column":"1"
			},
			"last_name" : {
				"field_type":"string",
				"field_name":"last_name",
				"label": "Last Name",
				"required" : true,
				"max_length" : 50,
				"match_pattern": "[a-zA-Z0-9/s]+",
				"grid_row": "2",
				"grid_column":"2"
			},
			"user_name" : {
				"field_type":"string",
				"field_name":"user_name",
				"label": "User Name",	
				"required" : true,
				"max_length" : 100,
				"grid_row": "3",
				"grid_column":"1"
			},
			"email" : {
				"field_type":"string",
				"field_name":"email",
				"label": "E-Mail",
				"required" : true,
				"max_length" : 100,
				"match_pattern": "(/w+)(/.|_)?(/w*)@(/w+)(/.(/w+))+",
				"grid_row": "3",
				"grid_column": "2"
			},
			"mobile" : {
				"field_type":"string",
				"field_name":"mobile",
				"label": "E-Mail",
				"format":"tel",
				"required" : true,
				"max_length" : 15,
				"match_pattern": "^(1/s?)?(/d{3}|(/d{3}/))[/s/-]?/d{3}[/s/-]?/d{4}$",	
				"grid_row": "4",
				"grid_column": "1"
			},
			"street1" : {
				"field_type":"string",
				"field_name":"street1",
				"format":"street",
				"label": "Street Address",
				"required" : true,
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+",
				"grid_row": "5",
				"grid_column": "1"
			},
			"street2" : {
				"field_type":"string",
				"field_name":"street2",
				"required" : true,
				"label": "Street Address 2",
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+",	
				"grid_row": "6",
				"grid_column": "1"
			},
			"city" : {
				"field_type":"string",
				"field_name":"city",
				"required" : true,
				"label": "City",
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+",	
				"grid_row": "7",
				"grid_column": "1"
			},
			"state" : {
				"field_type":"string",
				"field_name":"state",
				"required" : true,
				"max_length" : 50,
				"label": "State",
				"match_pattern": "[a-zA-Z0-9_/-/s]+",	
				"grid_row": "8",
				"grid_column": "2"
			},
			"zip" : {
				"field_type":"string",
				"field_name":"zip",
				"required" : true,
				"max_length" : 15,
				"match_pattern": "^/d{5}(?:[-/s]/d{4})?$",
				"label": "Zip",
				"grid_row": "9",
				"grid_column": "3"
			},
			"password" : { 
				"field_type":"string",
				"field_name":"password",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"confirmed_code": { 
				"field_type":"bool",
				"field_name":"confirmed_code",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"validation_code" : { 
				"field_type":"string",
				"field_name":"validation_code",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"home_team_name" :{ 
				"field_type":"string",
				"field_name":"home_team_name",
				"read_only" : true,	
			},
			"team_name" :{ 
				"field_type":"string",
				"field_name":"team_name",
				"read_only" : true
			},
			"picture" :{ 
				"field_type":"string",
				"field_name":"picture"
			},
			"inventory" : "[sys_item]"
        }
	}
}
)");

			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_user put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_user"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_schema after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);

				return result;
			}
			created_classes.put_member("sys_user", true);

			json gc = jp.create_object();
			json gcr = create_system_request( gc );

			json classes_array_response =  get_classes(gcr);
			json classes_array = classes_array_response[data_field];
			json classes_grouped = classes_array.group([](json& _item) -> std::string {
				return (std::string)_item[class_name_field];
				});

			auto members = created_classes.get_members();
			json missing_classes = jp.create_array();
			for (auto created_class : members) {
				const std::string& created_class_name = created_class.first;
				if (not classes_grouped.has_member(created_class_name)) {
					missing_classes.push_back(created_class_name);
				}
			}

			if (missing_classes.size() > 0) {
				system_monitoring_interface::active_mon->log_warning("system classes not saved", __FILE__, __LINE__);

				for (auto mc : missing_classes) {
					system_monitoring_interface::active_mon->log_information(mc, __FILE__, __LINE__);
				}

				for (auto mc : classes_array) {
					system_monitoring_interface::active_mon->log_json(mc);
				}

				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			json new_user_request;
			json new_user_data;

			new_user_data = jp.create_object();
			new_user_data.put_member(class_name_field, "sys_user"sv);
			new_user_data.put_member(user_name_field, default_user);
			new_user_data.put_member(user_email_field, default_email_address);
			new_user_data.put_member("password1", default_password);
			new_user_data.put_member("password2", default_password);

			new_user_request = create_system_request(new_user_data);
			json new_user_result =  create_user(new_user_request);
			bool success = (bool)new_user_result[success_field];
			std::vector<validation_error> errors;

			if (success) {
				json new_user = new_user_result[data_field];
				json user_return = create_response(new_user_request, true, "Ok", new_user, errors, method_timer.get_elapsed_seconds());
				response = create_response(new_user_request, true, "Database Created", user_return, errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
				system_monitoring_interface::active_mon->log_warning("system user create failed", __FILE__, __LINE__);
				log_errors(errors);
				json temp = jp.create_object();
				response = create_response(new_user_request, false, "Database user create failed.", temp, errors, method_timer.get_elapsed_seconds());
			}
			classes->commit();
			classes = nullptr;
			classes = std::make_shared<xtable>("classes.coronatbl");

			system_monitoring_interface::active_mon->log_job_stop("create_database", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			return response;
		}

private:

		json create_class(std::string _text)
		{
			json_parser jp;

			json jclass_def = jp.parse_object(_text);

			json sys_request = create_system_request(jclass_def);

			json response = put_class(sys_request);

			if (response.error() or response.empty() or (bool)response[success_field] == false)
			{
				system_monitoring_interface::active_mon->log_warning("Error creating class", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(response);
			}

			return response;
		}

		json check_single_object(date_time &current_date, read_class_sp& class_data, json& _object_definition, const class_permissions& _permission, std::vector<validation_error>& validation_errors)
		{
			json_parser jp;
			using namespace std::literals;

			json object_definition = _object_definition.clone();

			json result = jp.create_object();

			object_definition.erase_member("class_color");

			if (not object_definition.object())
			{
				json warning = jp.create_object();
				validation_error ve;

				ve.class_name = class_data->get_class_name();
				ve.field_name = trim(object_definition.to_json_typed(), 50);
				ve.filename = get_file_name(__FILE__);
				ve.line_number = __LINE__;
				ve.message = "Not an object";
				validation_errors.push_back(ve);
			}
			else {

				if (not object_definition.has_member(class_name_field))
				{
					json warning = jp.create_object();
					validation_error ve;

					ve.class_name = trim(object_definition.to_json_typed(), 50);
					ve.field_name = class_name_field;
					ve.filename = get_file_name(__FILE__);
					ve.line_number = __LINE__;
					ve.message = "Missing class";
					validation_errors.push_back(ve);
				}

				db_object_id_type object_id = -1;

				if (object_definition.has_member(object_id_field))
				{
					object_id = object_definition[object_id_field];
					bool exists;
					auto existing_object = class_data->get_object(this, object_id, _permission, exists);

					if (existing_object.object()) {
						existing_object.erase_member("class_color");
						existing_object.merge(object_definition);
						object_definition = existing_object;
					}
					else 
					{
						object_definition.put_member("created", current_date);
						object_definition.put_member("created_by", _permission.user_name);
					}

					object_definition.put_member("updated", current_date);
					object_definition.put_member("updated_by", _permission.user_name);

				}
				else
				{
					object_id = class_data->get_next_object_id();
					object_definition.put_member_i64(object_id_field, object_id);
					object_definition.put_member("created", current_date);
					object_definition.put_member("created_by", _permission.user_name);
					object_definition.put_member("team", _permission.team_name);
				}

				// if the user was a stooser and saved the query results with the object,
				// blank that out here because we will run that on load
				class_data->clear_queries(object_definition);

				// check the object against the class definition for correctness
				// first we see which fields are in the class not in the object

				for (auto fld : class_data->get_fields()) {
					if (object_definition.has_member(fld->get_field_name())) {
						auto obj_typex = object_definition[fld->get_field_name()];
						if (obj_typex.empty()) {
							continue;
                        }
						auto obj_type = obj_typex->get_field_type();
						auto member_type = fld->get_field_type();
						if (member_type != obj_type) {
							object_definition.change_member_type(fld->get_field_name(), member_type);
						}
					}
				}

				// check to make sure that we have all the fields 
				// for the index

				for (auto& idx : class_data->get_indexes()) {
					std::vector<std::string> missing;
					std::vector<std::string>& idx_keys = idx->get_index_keys();
					if (not object_definition.has_members(missing, idx_keys))
					{
						for (auto& missed : missing) {
							validation_error ve;
							ve.field_name = missed;
							ve.class_name = class_data->get_class_name();
							ve.filename = get_file_name(__FILE__);
							ve.line_number = __LINE__;
							ve.message = std::format("Missing field required for index '{0}'", idx->get_index_name());
							validation_errors.push_back(ve);
						}
					}
				}

				// then we see which fields are in the object that are not 
				// in the class definition.
				auto object_members = object_definition.get_members();
				for (auto om : object_members) {
					auto fld = class_data->get_field(om.first);
					if (fld) {
						fld->accepts(this, validation_errors, class_data->get_class_name(), om.first, om.second);
					}
					else
					{
						json warning = jp.create_object();
						validation_error ve;
						ve.class_name = class_data->get_class_name();
						ve.field_name = om.first;
						ve.filename = get_file_name(__FILE__);
						ve.line_number = __LINE__;
						ve.message = "Field not found in class definition";
						validation_errors.push_back(ve);
					}
				}
			}

			if (validation_errors.size() > 0) {
				json warnings = jp.create_array();
				std::string msg = std::format("Object '{0}' has problems", class_data->get_class_name());
				for (auto& ve : validation_errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					warnings.push_back(jve);
				}
				result.put_member(message_field, msg);
				result.put_member(success_field, 0);
				result.put_member("errors", warnings);
				result.share_member(data_field, object_definition);
			}
			else {
				result.put_member(message_field, "Ok"sv);
				result.put_member(success_field, 1);
				result.share_member(data_field, object_definition);
			}

			return result;
		}

		json check_object(json object_load, std::string _user_name, std::vector<validation_error>& validation_errors, std::string _authorization)
		{
			timer method_timer;
			json_parser jp;
			date_time current_date = date_time::now();
			using namespace std::literals;
			json response; 			

			response = jp.create_object();

			json object_definition,
				object_list,
				result_list;

			if (object_load.array()) 
			{
				object_list = object_load;
			}
			else if (object_load.object()) 
			{
				object_list = jp.create_array();
				object_list.push_back(object_load);
			}
			else
			{
				response.put_member(success_field, false);
				response.put_member(message_field, "not an object"sv);
				return response;
			}

			json classes_group = object_list.group([](json _item) -> std::string {
				return _item[class_name_field];
				});

			auto class_list = classes_group.get_members();

			bool all_objects_good = true;

			for (auto class_pair : class_list)
			{
				if (class_pair.first.empty()) {
					response.put_member(success_field, false);
					response.put_member(message_field, "empty class name"sv);
					return response;
				}
				auto cd = read_lock_class(class_pair.first);

				if (cd) {
					auto permission = get_class_permission(_user_name, class_pair.first);
					cd->init_validation(this, permission);
				}
				else 
				{
					response.put_member(success_field, false);
					response.put_member(message_field, class_pair.first + " invalid class name");
					return response;
				}
			}

			for (auto class_pair : class_list)
			{
				std::string class_name = class_pair.first;

				json class_object_list = classes_group[class_name];
				read_class_sp class_data = read_lock_class(class_name);

				if (not class_data) {
					continue;
				}

				class_permissions permission;

				if (_authorization != auth_system) 
				{
					permission = get_class_permission(_user_name, class_pair.first);

					if (permission.put_grant == class_grants::grant_none) {
						response.put_member(success_field, false);
						response.put_member(message_field, "check_object denied"sv);
						return response;
					}
				}
				else {
					permission.user_name = _user_name;
                    permission.put_grant = class_grants::grant_any;	
				}

				result_list = jp.create_array();

				for (auto item_definition : class_object_list)
				{
					json result = check_single_object(current_date, class_data, item_definition, permission, validation_errors);
					result_list.push_back(result);

					if (not result[success_field]) 
					{
						all_objects_good = false;
					}
				}

				// move_member here is to prevent something stupid happening because this is in a loop 
				// and the scope of result_list is outside of it.
				// fixing it is out of scope for this particular thing.
				// TODO: fix result_list
				classes_group.move_member(class_name, result_list);
			}

			response.put_member(success_field, all_objects_good);
			response.put_member(message_field, "Objects processed"sv);
			response.share_member(data_field, classes_group);
			return response;
		}

		std::string get_pass_phrase()
		{
			return "This is a test pass phrase";
		}

		std::string get_iv()
		{
			return "This is a test iv";
		}

		json create_response(std::string _user_name, std::string _authorization, bool _success, std::string _message, json _data, std::vector<validation_error>& _errors, double _seconds)
		{
			json_parser jp;

			json payload = jp.create_object();
			json token = jp.create_object();
			token.put_member(user_name_field, _user_name);
			token.put_member(authorization_field, _authorization);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string );

			if (_success) {
				payload.put_member(token_field, base64_token_string);
			}
			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.put_member(data_field, _data);
			payload.put_member(seconds_field, _seconds);

			if (_errors.size()) {
				json errors_array = jp.create_array();
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
				payload.share_member("errors", errors_array);
			}


			return payload;
		}

		json create_user_response(json _request, bool _success, std::string _message, json _data, std::vector<validation_error>& _errors, double _seconds)
		{
			json_parser jp;
			json payload;
			json errors_array = jp.create_array();

			if (_errors.size()) {
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
			}
			payload = create_user_response(_request, _success, _message, _data, errors_array, _seconds);

			return payload;
		}

		json create_user_response(json _request, bool _success, std::string _message, json _data, json _errors, double _seconds)
		{
			json_parser jp;
			json payload = jp.create_object();

			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.put_member(data_field, _data);
			payload.put_member_double(seconds_field, _seconds);

			if (_errors.size()) {
				payload.put_member("errors", _errors);
			}

			return payload;
		}

		json create_response(json _request, bool _success, std::string _message, json _data, std::vector<validation_error>& _errors, double _seconds)
		{
			json_parser jp;
			json payload = jp.create_object();
			json token = jp.create_object();
			json src_token = _request[token_field];
			
			token.copy_member(user_name_field, src_token);
			token.copy_member(authorization_field, src_token);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);
			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.share_member(data_field, _data);
			if (_errors.size()) {
				json errors_array = jp.create_array();
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
				payload.share_member("errors", errors_array);
			}
			payload.put_member_double(seconds_field, _seconds);
			return payload;
		}

		json check_token(std::string _encoded_token, std::vector<std::string> _authorizations)
		{
			json_parser jp;
			json empty;
			std::string decoded_token = base64_decode(_encoded_token);
			json token;

			try 
			{
				token = jp.parse_object(decoded_token);
			}
			catch (std::exception)
			{
				return empty;
			}

			if (not token.object())
			{
				return empty;
			}

			if (not token.has_member(signature_field))
			{
				return empty;
			}

			std::string signature = token[signature_field];
			token.erase_member(signature_field);

			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());

			if (cipher_text != signature) {
				return empty;
			}

			token.put_member(signature_field, signature);

			date_time current = date_time::utc_now();
			date_time expiration = (date_time)token[token_expires_field];

			if (current > expiration)
			{
				return empty;
			}

			if (cipher_text != signature)
			{
				return empty;
			}

			if (not token.has_member(authorization_field))
			{
				return empty;
			}

			std::string authorization = token[authorization_field];
			std::string user = token[user_name_field];

			for (auto _authorization : _authorizations)
			{
				if (authorization == _authorization) /* perhaps a tad loose used to be  and user == default_user*/
				{
					return token;
				}
				else if (authorization == auth_system)
				{
					return token;
				}
			}
			return empty;
		}

		public:

		virtual bool check_message(json& _message, std::vector<std::string> _authorizations, std::string& _user_name, std::string& _token_authorization) override
		{
			std::string token = _message[token_field];

			json result = check_token(token, _authorizations);
			bool is_ok = not result.empty();

			if (is_ok) {
				_user_name = result[user_name_field];
                _token_authorization = result[authorization_field];
			}

			return is_ok;
		}

		virtual json get_openapi_schema(std::string user_name) override
		{
			json schema;
			json_parser jp;

			if (user_name.empty()) {
				user_name = default_user;
            }

			json all_classes = classes->select(R"({"class_name":"sys_class"})"_jobject, [](json& _item) -> json {
                return _item;
				});

			json result_list = jp.create_array();
			if (all_classes.array()) {
				for (auto cls : all_classes) {
					std::string class_name = cls[class_name_field];
					auto permission = get_class_permission(user_name, class_name);
					if (permission.get_grant != class_grants::grant_none)
					{
						result_list.push_back(cls);
					}
                }
			}

			schema = jp.create_object();
			for (auto items : result_list)
			{
				std::string class_name = items[class_name_field];
				auto ptr = read_lock_class(class_name);
				if (ptr) {
					json class_schema = ptr->get_openapi_schema(this);
					schema.share_member(class_name, class_schema);
				}
			}

			return schema;
		}

		protected:

		json select_object(std::string _class_name, int64_t _object_id, class_permissions _permissions)
		{
			json_parser jp;
			json obj = jp.create_array();

			read_class_sp classd = read_lock_class(_class_name);
			if (not classd)
				return obj;

			auto perm = get_class_permission(_permissions, _class_name);
			bool exists = false;
			json oneobj= classd->get_object(this, _object_id, _permissions, exists);
			if (oneobj.object())
                obj.push_back(oneobj);

			return obj;
		}


		json select_object(json _key, bool _children, class_permissions _permission)
		{
			json_parser jp;
			json obj = jp.create_array();

			_key.set_natural_order();

			std::string class_name = _key[class_name_field];

			read_class_sp classd = read_lock_class(class_name);
			if (not classd)
				return obj;

			auto perm = get_class_permission(_permission, class_name);

			obj = classd->get_objects(this, _key, _children, perm);

			return obj;
		}

		json select_single_object(json _key, bool _children, class_permissions _permission)
		{
			json result = select_object(_key, _children, _permission);
			result = result.get_first_element();
			return result;
		}

		void put_user(json _user)
		{
			json_parser jp;

			_user.erase_member("team");
			_user.erase_member("home_team");
			_user.erase_member("allowed_teams");
			_user.erase_member("create_options");
			_user.erase_member("class_color");
			json children = jp.create_array();
			json items = jp.create_array();
			items.push_back(_user);
			json request = create_system_request(items);
			put_object(request);			
		}

		void put_error(std::string _system, std::string _message, json& _body, std::string _file, int _line)
		{
			json_parser jp;
			json error = jp.create_object();
			error.put_member(class_name_field, std::string("sys_error"));
			error.put_member("system", _system);
			error.put_member("message", _message);
			error.put_member("file", _file);
			error.put_member("line", _line);
			error.share_member("body", _body);
            json create_object_request = create_system_request(error);
			json response = put_object(error);
			if (not response[success_field]) {
				system_monitoring_interface::active_mon->log_warning("put_error failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
			}
		}

		json get_user(std::string _user_name, class_permissions _permission)
		{
			json_parser jp;

			json key = jp.create_object();
			key.put_member(user_name_field, _user_name);

			auto classd = read_get_class("sys_user");
			if (not classd)
				return jp.create_array();

			json users = classd->get_objects(this, key, true, _permission);

			json user = users.get_first_element();
			if (user.object()) {

				std::map<std::string, bool> allowed_teams;
				json jallowed_teams = jp.create_array();

				std::string team_name = user["team_name"];
				if (not team_name.empty()) {
                    json team_data = run_team(team_name, _permission);
					if (team_data.object()) {
						user.share_member("team", team_data);
						json jallowed_teams = team_data["allowed_teams"];
						if (jallowed_teams.array()) {
							for (json jteam_name : jallowed_teams) {
								std::string atm = (std::string)jteam_name;
								allowed_teams[atm] = true;
								jallowed_teams.push_back(atm);
							}
						}
					}
				}
				team_name = user["home_team_name"];
				if (not team_name.empty()) {
					json team_data = run_team(team_name, _permission);
					if (team_data.object()) {
						user.share_member("home_team", team_data);
						json jallowed_teams = team_data["allowed_teams"];
						if (jallowed_teams.array()) {
							for (json jteam_name : jallowed_teams) {
								std::string atm = (std::string)jteam_name;
								if (not allowed_teams.contains(atm)) {
									allowed_teams[atm] = true;
									jallowed_teams.push_back(atm);
								}
							}
						}
					}
				}
				user.share_member("allowed_teams", jallowed_teams);
			}

			return user;
		}

		json get_team_by_email(std::string _domain, class_permissions _permission)
		{
			json_parser jp;

			json key = jp.create_object();
			json query_details = jp.create_object();
            query_details.put_member("class_name", std::string("sys_team"));	
			query_details.put_member("filter", key);
			json all_teams = query_class(default_user, query_details, jp.create_object());
			json teams = jp.create_array();

			if (all_teams[success_field]) {
                all_teams = all_teams[data_field];
				if (all_teams.array()) {
					for (int ix = 0; ix < all_teams.size(); ix++) {
						json team = all_teams.get_element(ix);
						std::string team_name = (std::string)team["team_name"];
						std::string domains = (std::string)team["team_domain"];
						if (domains.size() == 0 || domains == "NONE")
							continue;
						try {
							std::regex domain_matcher(domains);
							if (std::regex_match(_domain, domain_matcher)) {
								system_monitoring_interface::active_mon->log_warning(std::format("Matched domain '{0}' for team_domain '{1}' for '{2}'", _domain, domains, team_name), __FILE__, __LINE__);
                                json actual_team = get_team(team_name, _permission);
								teams.push_back(actual_team);
							}
							else {
								system_monitoring_interface::active_mon->log_warning(std::format("Not matched domain '{0}' for team_domain '{1}' for '{2}'", _domain, domains, team_name), __FILE__, __LINE__);
							}
						}
						catch (std::exception exc)
						{
							system_monitoring_interface::active_mon->log_warning(std::format("Invalid regexp '{0}' for team_domain '{1}' for '{2}'", _domain, domains, team_name ), __FILE__, __LINE__);
						}
					}
				}
			}

			return teams;
		}

		json get_team(std::string _team_name, class_permissions _permission)
		{
			json_parser jp;

			json key = jp.create_object();
            key.put_member("team_name", _team_name);
			json query_details = jp.create_object();
			query_details.put_member("class_name", std::string("sys_team"));
			query_details.put_member("filter", key);
			json all_teams = query_class(default_user, query_details, jp.create_object());
			json teams = jp.create_array();
			if (all_teams[success_field]) {
				all_teams = all_teams[data_field];
				if (all_teams.array()) {
					for (int ix = 0; ix < all_teams.size(); ix++) {
						json team = all_teams.get_element(ix);
						json key = team.extract({ class_name_field, object_id_field });
						std::string names = (std::string)team["team_name"];
						if (names != _team_name)
							continue;
                        std::string team_class_name = (std::string)key[class_name_field];
						auto classd = read_lock_class(team_class_name);
						if (classd) {
							json full_teams = classd->get_objects(this, key, true, _permission);
                            json full_team = full_teams.get_first_element();
//                          system_monitoring_interface::active_mon->log_warning(std::format("Found team '{0}'", names), __FILE__, __LINE__);
//							system_monitoring_interface::active_mon->log_json(full_team);
							teams.push_back(full_team);
						}
					}
				}
			}
			return teams.get_first_element();
		}

		json run_team(std::string _team_name, class_permissions _permission)
		{
			json_parser jp;

            json team = get_team(_team_name, _permission);
			if (team.object()) 
			{
				int64_t team_id = (int64_t)team[object_id_field];

				json permissions = team["permissions"];

				json class_colors = jp.create_object();
				if (permissions.array()) 
				{
					for (auto grant : permissions) 
					{
						json granted_all = jp.create_object();
						auto class_list = grant["grant_classes"];
						if (class_list.array()) 
						{
							for (auto cls : class_list) 
							{
								std::string class_name = (std::string)cls;
								auto classd = read_lock_class(class_name);
								json descendants = jp.create_array();
								if (classd) 								
								{
									class_colors.put_member(class_name, classd->get_class_color());
									auto desc = classd->get_descendants();
									for (auto d : desc) 
									{
										descendants.push_back(d.first);
									}
								}
								granted_all.put_member(cls, descendants);
							}
						}
						grant.put_member("all_granted_classes", granted_all);
						grant.put_member("class_colors", class_colors);
					}
				}

				json workflows = team["workflow"];
				if (workflows.array()) 
				{
					for (auto wf : workflows) 
					{
						// read the ticket job
						int64_t workflow_object_id = (int64_t)wf[object_id_field];
						std::string workflow_name = (std::string)wf["workflow_name"];
						std::string workflow_description = (std::string)wf["workflow_description"];

						std::string workflow_schedule_type = (std::string)wf["workflow_schedule_type"];
						json workflow_schedule_days = wf["workflow_schedule_days"];
						int hour = (int64_t)wf["workflow_schedule_hour"];

						SYSTEMTIME system_time;
						::GetSystemTime(&system_time);

						bool matched_set = false;
						int matched_day = 0;

						if (workflow_schedule_type == "Month") {
							for (auto day : workflow_schedule_days) {
								int d = (int64_t)day;
								if (d == system_time.wDay) {
									matched_day = d;
									matched_set = true;
									break;
								}
							}
						}
						else if (workflow_schedule_type == "Week") 
						{
							for (auto day : workflow_schedule_days) 
							{
								int d = (int64_t)day;
								if (d == system_time.wDayOfWeek)
								{
									matched_day = d;
									matched_set = true;
									break;
								}
							}
						}

						double last_ran = (double)wf["last_ran"];

						if (matched_set && (system_time.wHour >= hour && last_ran < matched_day)) 
						{
							wf.put_member("last_ran", (double)system_time.wDay);
						}
						else
						{
							system_monitoring_interface::active_mon->log_information(std::format("Not scheduled time for workflow '{0}' for team '{1}'", workflow_name, _team_name), __FILE__, __LINE__);
							continue;
						}

						std::string ticket_class_name = (std::string)wf["ticket_class_name"];
						std::string ticket_name = (std::string)wf["ticket_name"];
						std::string ticket_description = (std::string)wf["ticket_description"];

						if (workflow_name.empty() or ticket_class_name.empty()) 
						{
							continue;
						}
						system_monitoring_interface::active_mon->log_information(std::format("Running workflow '{0}' for team '{1}'", workflow_name, _team_name), __FILE__, __LINE__);

						json new_object = jp.create_object();
						new_object.put_member(class_name_field, ticket_class_name);
						new_object.put_member("sys_team", team_id);
						new_object.put_member("ticket_name", ticket_name);
						new_object.put_member("ticket_description", ticket_description);
						json por = create_system_request(new_object);
						json porresp = put_object(por);
						if (porresp["success"]) 
						{
							new_object = porresp[data_field];
							wf.put_member("last_result", (std::string)porresp["message"]);
							por = create_system_request(wf);
							porresp = put_object(por);
							if (not porresp["success"]) 
							{
								system_monitoring_interface::active_mon->log_warning(std::format("Could not update workflow '{0}' for team '{1}'", workflow_name, _team_name), __FILE__, __LINE__);
								system_monitoring_interface::active_mon->log_json(porresp);
							}
						}
						else 
						{
							system_monitoring_interface::active_mon->log_warning(std::format("Could not create ticket for workflow '{0}' for team '{1}'", workflow_name, _team_name), __FILE__, __LINE__);
							system_monitoring_interface::active_mon->log_json(porresp);
						}
					}
				}
			}
			return team;
		}

		json get_schema(std::string schema_name, std::string schema_version, class_permissions _permission)
		{
			json_parser jp;
			json key = jp.create_object();

			key.put_member("schema_name", schema_name);
			key.put_member("schema_version", schema_version);

			auto classd = read_get_class("sys_schema");
			if (not classd) {
				return jp.create_array();
			}
			json data = classd->get_objects(this, key, true, _permission);

			return data.get_first_element();
		}

		json get_dataset(std::string dataset_name, std::string dataset_version, class_permissions _permission)
		{
			json_parser jp;
			json key = jp.create_object();
			key.put_member("dataset_name", dataset_name);
			key.put_member("dataset_version", dataset_version);

			auto classd = read_get_class("sys_dataset");
			if (not classd) {
				return json();
			}
			json data = classd->get_objects(this, key, true, _permission);

			return data.get_first_element();
		}

		virtual class_permissions get_system_permission()
		{
			class_permissions grants;
			grants.user_name = default_user;
            grants.team_name = "systemonlyfans";
			grants.alter_grant = class_grants::grant_any;
			grants.put_grant = class_grants::grant_any;
			grants.get_grant = class_grants::grant_any;
			grants.delete_grant = class_grants::grant_any;
			return grants;
		}

		virtual class_permissions get_class_permission(
			const class_permissions &_perm,
			std::string _class_name) override
		{
			return get_class_permission(_perm.user_name, _class_name);
		}

		virtual class_permissions get_team_permissions(std::string user_name, std::string team_name, std::string _class_name)
		{

			bool granted;
			auto sys_perm = get_system_permission();
			json jteam = get_team(team_name, sys_perm);

			class_permissions grants;

			grants.user_name = user_name;
			grants.alter_grant = class_grants::grant_none;
			grants.put_grant = class_grants::grant_own;
			grants.get_grant = class_grants::grant_own;
			grants.delete_grant = class_grants::grant_none;

			if (jteam.object()) {
				json jpermissions = jteam["permissions"];
				if (jpermissions.array()) {
					for (auto jperm : jpermissions) {
						json class_array = jperm["grant_classes"];
						std::vector<std::string> granted_classes;
						if (class_array.array())
						{
							for (auto jcls : class_array) {
								granted_classes.push_back((std::string)jcls);
							}
						}
						else {
							granted_classes.push_back((std::string)class_array);
						}
						bool granted = false;
						for (std::string& jclass : granted_classes) {
							auto permclass = read_lock_class(jclass);
							if (permclass) {
								if (permclass->get_descendants().contains(_class_name)) {
									std::string permission = jperm[class_permission_get];
									if (permission == "any")
										grants.get_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.get_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.get_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_put];
									if (permission == "any")
										grants.put_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.put_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.put_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_delete];
									if (permission == "any")
										grants.delete_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.delete_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.delete_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_alter];
									if (permission == "any")
										grants.alter_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.alter_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.alter_grant = class_grants::grant_teamorown;

									granted = true;
									break;
								}
								else {
									system_monitoring_interface::active_mon->log_warning(std::format("Team {1}: Class '{0}' not granted permission", jclass, team_name), __FILE__, __LINE__);
								}
							}
							else {
								system_monitoring_interface::active_mon->log_warning(std::format("Team {1}: Class '{0}' not found for permission", jclass, team_name), __FILE__, __LINE__);
							}
						}
					}
				}
			}
			return grants;
		}

		virtual class_permissions get_class_permission(
			std::string _user_name,
			std::string _class_name) override
		{

			class_permissions grants;

			grants.user_name = _user_name;
			grants.alter_grant = class_grants::grant_none;
			grants.put_grant = class_grants::grant_own;
			grants.get_grant = class_grants::grant_own;
			grants.delete_grant = class_grants::grant_none;

			// extract the user key from the token and get the user object
			if (_user_name == default_user) 
			{
				grants = get_system_permission();
				return grants;
			}

			auto sys_perm = get_system_permission();
			json user =  get_user(_user_name, sys_perm);

			if (not user.empty()) 
			{			
				auto home_grants = grants;
				std::string team_name = (std::string)user["home_team_name"];
				home_grants.team_name = team_name;
				home_grants = get_team_permissions(_user_name, team_name, _class_name);

				auto team_grants = grants;
				team_name = (std::string)user["team_name"];
				team_grants.team_name = team_name;
				team_grants = get_team_permissions(_user_name, team_name, _class_name);

				grants = home_grants | team_grants | grants;
				grants.team_name = team_name;
				grants.user_name = _user_name;
			}

			return grants;
		}

		thread_safe_map<std::string, std::shared_ptr<class_interface>> class_cache;

		int64_t maximum_record_cache_size_bytes = giga_to_bytes(1);

		bool send_user_confirmation(json user_info, std::string email_template)
		{
			bool success = false;
			try {
				std::string new_code = get_random_code();

				user_info.put_member("validation_code", new_code);
				user_info.put_member("confirmed_code", 0);
				auto sys_perm = get_system_permission();
				put_user(user_info);

				sendgrid_client sc_client;
				sc_client.sender_email = sendgrid_sender_email;
				sc_client.sender_name = sendgrid_sender;
				sc_client.api_key = connections.get_connection("sendgrid");

				if (sc_client.api_key.empty()) {
                    json_parser jp;
					json blank = jp.create_object();
					system_monitoring_interface::active_mon->log_warning("Sendgrid configuration is missing Api Key.  Be sure to set the CONNECT_SENDGRID environment variable.", __FILE__, __LINE__);
					put_error("SendGrid", "Sendgrid configuration is missing Api Key.  Be sure to set the CONNECT_SENDGRID environment variable.", blank, __FILE__, __LINE__);
				}

				if (email_template.empty()) {
					email_template = R"(<html><body><h2>$EMAIL_TITLE$</h2><p>Username is $USERNAME$</p><p>Validation code <span style="background:grey;border:1px solid black;padding 8px;">$CODE$</p></body></html>)";
				}

				std::string user_name = user_info[user_name_field];

				std::string email_body = replace(email_template, "$CODE$", new_code);
				email_body = replace(email_body, "$USERNAME$", user_name);
				email_body = replace(email_body, "$EMAIL_TITLE$", user_confirmation_title);
				auto sg_response = sc_client.send_email(user_info, user_confirmation_title, email_body, "text/html");
				if (sg_response.response.http_status_code > 299 or sg_response.response.http_status_code < 200) {
					system_monitoring_interface::active_mon->log_warning(std::format("Send email {} to {}", user_confirmation_title, user_name), __FILE__, __LINE__);
					if (sg_response.response.response_body.is_safe_string()) {
						system_monitoring_interface::active_mon->log_warning(sg_response.response.response_body.get_ptr(), __FILE__, __LINE__);
					}
					else {
						system_monitoring_interface::active_mon->log_warning("Response body is not a string", __FILE__, __LINE__);
					}
				}
				success = true;
			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_warning(exc.what(), __FILE__, __LINE__);
			}
			return success;
		}

	public:

		std::string default_user;
		std::string default_password;
		std::string default_email_address;
		std::string default_guest_team;
		std::string default_onboard_email_filename;
		std::string default_recovery_email_filename;
		std::string default_onboard_email;
		std::string default_recovery_email;

		time_span token_life;

		std::string default_api_title;
		std::string default_api_description;
		std::string default_api_version;
		std::string default_api_author;

		std::string sendgrid_sender;
		std::string sendgrid_sender_email;
		std::string user_confirmation_title;

		// constructing and opening a database

		corona_database() 
		{
			token_life = time_span(1, time_models::hours);	
		}

		virtual ~corona_database()
		{
			
		}

		void apply_config(json _config)
		{
			date_time start;
			start = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("apply_config", "start", start, __FILE__, __LINE__);

			if (_config.has_member("SendGrid"))
			{
				json send_grid = _config["SendGrid"];
				std::string send_grid_api_key = send_grid["ApiKey"];
				sendgrid_sender = send_grid["SenderName"];
				sendgrid_sender_email = send_grid["SenderEmail"];
				user_confirmation_title = send_grid["UserConfirmationTitle"];
				this->connections.set_connection("sendgrid", (std::string)send_grid_api_key);
			}

			if (_config.has_member("Connections"))
			{
				json connections = _config["Connections"];
				auto members = connections.get_members();
				for (auto member : members) {
					this->connections.set_connection(member.first, (std::string)member.second);
				}
			}

			if (_config.has_member("Server"))
			{
				json server = _config["Server"];
				default_user = server[sys_user_name_field];
				default_password = server[sys_user_password_field];
				default_email_address = server[sys_user_email_field];
				default_guest_team = server[sys_default_team_field];
				default_api_title = server[sys_default_api_title_field];
				default_api_description = server[sys_default_api_description_field];
				default_api_version = server[sys_default_api_version_field];
				default_api_author = server[sys_default_api_author_field];
                default_onboard_email_filename = server[sys_default_onboard_email_template];
				default_recovery_email_filename = server[sys_default_recovery_email_template];
				default_onboard_email = read_all_string(default_onboard_email_filename);
				default_recovery_email = read_all_string(default_recovery_email_filename);

				if (server.has_member(sys_record_cache_field)) {
					maximum_record_cache_size_bytes = (int64_t)server[sys_record_cache_field];
				}
			}

			system_monitoring_interface::active_mon->log_job_stop("apply_config", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

		}


		virtual std::string get_random_code()
		{
			std::string s_confirmation_code = "";
			int confirmation_code_digits = 8;
			char confirmation_code[32] = {};

			int rc = 0;
			int lc = 0;
			int i = 0;
			int random_number;

			std::random_device rd; // Seed generator
			std::mt19937 gen(rd()); // Mersenne Twister engine
			std::uniform_int_distribution<> dist('A', 'A' + 26); // Range [1, 100]

			while (i < confirmation_code_digits and i < sizeof(confirmation_code))
			{
				do
				{
					random_number = dist(gen);
					rc = random_number % 26 + 'A';
					confirmation_code[i] = rc;
				} while (rc == lc);
				lc = rc;
				i++;
			}
			confirmation_code[i] = 0;
			s_confirmation_code = confirmation_code;
			return s_confirmation_code;
		}

		virtual std::shared_ptr<class_interface> read_get_class(const std::string& _class_name) 
		{
			std::shared_ptr<class_interface> cd;

			activity act;
			act.db = this;
			cd = get_class_impl(&act, _class_name);

			return cd;
		}

		virtual read_class_sp read_lock_class(const std::string& _class_name) override
		{
			std::shared_ptr<class_interface> cd = read_get_class(_class_name);
			return read_class_sp(cd);
		}

		virtual write_class_sp write_lock_class(const std::string& _class_name) override
		{
			std::shared_ptr<class_interface> cd = read_get_class(_class_name);
			return write_class_sp(cd);
		}

		virtual json save_class(class_interface *_class_to_save)
		{
			json_parser jp;
			json class_def;

			class_def = jp.create_object();
			_class_to_save->get_json(class_def);
            classes->put(class_def);
			classes->commit();
			return class_def;
		}

		void log_error_array(json put_result)
		{
			auto result_items = put_result[data_field];
			result_items.for_each_member([](const std::string& _member_name, json _member) {
				if (_member.array()) {
					_member.for_each_element([](json& _item) {
						if (not _item[success_field]) {
							std::string msg = std::format("{0}:", (std::string)_item[message_field]);
							system_monitoring_interface::active_mon->log_warning(msg);
							if (_item.has_member("errors"))
							{
								json errors = _item["errors"];
								errors.for_each_element([](json& _msg) {
									std::string msg = std::format("{0}.{1} {2}", (std::string)_msg[class_name_field], (std::string)_msg["field_name"], (std::string)_msg[message_field]);
									system_monitoring_interface::active_mon->log_information(msg);
									});
							}
						}
						});
				}
				});
		}

		virtual json apply_schema(json _schema)
		{
			date_time start_schema = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("apply_schema", "Applying schema file", start_schema, __FILE__, __LINE__);
			using namespace std::literals;

			if (not _schema.has_member("schema_name"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema name");
			}

			if (not _schema.has_member("schema_version"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema version");
			}

			if (not _schema.has_member("schema_authors"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema author");
			}

			auto sys_perm = get_system_permission();

			json_parser jp;

			json schema_key = jp.create_object();
			schema_key.copy_member("schema_name", _schema);
			schema_key.copy_member("schema_version", _schema);
			schema_key.put_member(class_name_field, "sys_schema"sv);
			schema_key.set_compare_order({ "schema_name", "schema_version" });

			json jschema_array =  select_object(schema_key, false, sys_perm);
            json jschema = jschema_array.get_first_element();
			int64_t schema_id;

			if (jschema.object()) 
			{
                jschema.merge(_schema);
				schema_id = (int64_t)_schema["object_id"];
			}
			else 
			{
                json create_object_body = jp.create_object();
				create_object_body.put_member(class_name_field, "sys_schema"sv);
                json create_object_request = create_system_request(create_object_body);
                json result = create_object(create_object_request);
				if (result.has_member(data_field)) 
				{
					json new_schema = result[data_field];
                    new_schema.merge(_schema);
					jschema = new_schema;
					schema_id = (int64_t)jschema["object_id"];
				}
			}

			if (jschema.has_member("classes"))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Classes", start_section, __FILE__, __LINE__);

				json class_array = jschema["classes"];
				if (class_array.array())
				{
					for (int i = 0; i < class_array.size(); i++)
					{
						date_time start_class = date_time::now();
						timer txc;

						json class_definition = class_array.get_element(i);

						try {

							if constexpr (debug_teams)
							{
								if ((std::string)class_definition[base_class_name_field] == "sys_team")
								{
									DebugBreak();
								}
							}

							json put_class_request = create_system_request(class_definition);
							json class_result =  put_class(put_class_request);

							if (class_result.error()) 
							{
								system_monitoring_interface::active_mon->log_warning(class_result, __FILE__, __LINE__);
							}
						}
						catch (std::exception exc)
						{
							system_monitoring_interface::active_mon->log_exception(exc);
						}
					}
				}
				system_monitoring_interface::active_mon->log_job_section_stop("", "Classes", txsect.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			else
			{
				system_monitoring_interface::active_mon->log_warning("classes not found in schema");
			}

			if (jschema.has_member("users"))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Users", start_section, __FILE__, __LINE__);
				json user_array = jschema["users"];
				if (user_array.array())
				{
					for (int i = 0; i < user_array.size(); i++)
					{
						date_time start_user = date_time::now();
						timer txu;

						json user_definition = user_array.get_element(i);
						system_monitoring_interface::active_mon->log_function_start("put user", user_definition[user_name_field], start_user, __FILE__, __LINE__);
						json put_user_request = create_system_request(user_definition);
						create_user(put_user_request);
					    system_monitoring_interface::active_mon->log_function_stop("put user", user_definition[user_name_field], txu.get_elapsed_seconds(), __FILE__, __LINE__);
					}
				}
				system_monitoring_interface::active_mon->log_job_section_stop("", "Users", txsect.get_elapsed_seconds(), __FILE__, __LINE__);
			}
	
			if (jschema.has_member("datasets"))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Datasets", start_section, __FILE__, __LINE__);
				using namespace std::literals;
				json dataset_array = jschema["datasets"];
				if (dataset_array.array())
				{
					for (int i = 0; i < dataset_array.size(); i++)
					{
						date_time start_dataset = date_time::now();
						timer txs;

						json new_dataset = dataset_array.get_element(i);
						new_dataset.put_member(class_name_field, "sys_dataset"sv);
						new_dataset.put_member_i64("sys_schema", (int64_t)jschema[object_id_field]);
						std::string dataset_name = new_dataset["dataset_name"];
						std::string dataset_version = new_dataset["dataset_version"];

						system_monitoring_interface::active_mon->log_job_section_start("DataSet", dataset_name + " Start", start_dataset, __FILE__, __LINE__);

						bool run_on_change = (bool)new_dataset["run_on_change"];
						json existing_dataset = get_dataset(dataset_name, dataset_version, sys_perm);

						if (not (run_on_change or existing_dataset.empty())) {
							system_monitoring_interface::active_mon->log_job_section_stop("DataSet", dataset_name + " Already Done", tx.get_elapsed_seconds(), __FILE__, __LINE__);
							continue;
						}

						if (new_dataset.object()) {

							if (new_dataset.has_member("import"))
							{
								new_dataset.put_member_i64("sys_schema", (int64_t)jschema[object_id_field]);
								json import_spec = new_dataset["import"];
								std::vector<std::string> missing;

								if (not import_spec.has_members(missing, { "target_class", "type" })) {
									system_monitoring_interface::active_mon->log_warning("Import missing:");
									std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
										system_monitoring_interface::active_mon->log_warning(s);
										});
									system_monitoring_interface::active_mon->log_information("the source json is:");
									system_monitoring_interface::active_mon->log_json<json>(import_spec, 2);
									continue;
								}

								std::string target_class = import_spec["target_class"];
								std::string import_type = import_spec["type"];
								date_time import_datatime = import_spec["import_datatime"];

								if (import_type == "csv") {

									if (not import_spec.has_members(missing, { "filename", "delimiter" })) {
										system_monitoring_interface::active_mon->log_warning("Import CSV missing:");
										std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
											system_monitoring_interface::active_mon->log_warning(s);
											});
										system_monitoring_interface::active_mon->log_information("the source json is:");
										system_monitoring_interface::active_mon->log_json<json>(import_spec, 2);
										continue;
									}

									std::string filename = import_spec["filename"];
									std::string delimiter = import_spec["delimiter"];
									if (filename.empty() or delimiter.empty()) {
										system_monitoring_interface::active_mon->log_warning("filename and delimiter can't be blank.");
									}

									if (std::filesystem::exists(filename))
									{

										auto file_modified = std::filesystem::last_write_time(filename);
										const auto system_file_modified = std::chrono::clock_cast<std::chrono::system_clock>(file_modified);
										const auto file_modified_time = std::chrono::system_clock::to_time_t(system_file_modified);

										date_time file_modified_dt = date_time(file_modified_time);
										if (file_modified_dt > import_datatime) {

											json column_map = import_spec["column_map"];

											FILE* fp = nullptr;
											int error_code = fopen_s(&fp, filename.c_str(), "rS");

											if (fp) {
												// Buffer to store each line of the file.
												char line[8182];
												json datomatic = jp.create_array();

												// create template object
												json codata = jp.create_object();
												codata.put_member(class_name_field, target_class);
												json cor = create_system_request(codata);
												json new_object_response = create_object(cor);

												if (new_object_response[success_field]) {
													json new_object_template = new_object_response[data_field];

													// Read each line from the file and store it in the 'line' buffer.
													int64_t total_row_count = 0;
													while (fgets(line, sizeof(line), fp)) {
														// Print each line to the standard output.
														json new_object = new_object_template.clone();
														new_object.erase_member(object_id_field);
														jp.parse_delimited_string(new_object, column_map, line, delimiter[0]);
														datomatic.push_back(new_object);
														if (datomatic.size() > import_batch_size) {
															int batch_size = (int)datomatic.size();
															total_row_count += datomatic.size();
															json request(datomatic);
															json cor = create_system_request(request);

															put_object_sync(cor, [this, total_row_count, batch_size](json& put_result, double _exec_time) {
																if (put_result[success_field]) {
																	double x = batch_size / _exec_time;
																	std::string msg = std::format("{0} objects, {1:.2f} / sec, {2} rows total", batch_size, x, total_row_count);
																	system_monitoring_interface::active_mon->log_activity(msg, _exec_time, __FILE__, __LINE__);
																}
																else
																{
																	log_error_array(put_result);
																}
																});

															datomatic = jp.create_array();
														}
													}

													if (datomatic.size() > 0) {
														int batch_size = (int)datomatic.size();
														total_row_count += batch_size;
														json request(datomatic);
														json cor = create_system_request(request);
														put_object_sync(cor, [this, total_row_count, batch_size](json& put_result, double _exec_time) {
															if (put_result[success_field]) {
																double x = batch_size / _exec_time;
																std::string msg = std::format("{0} objects, {1:.2f} / sec, {2} rows total", batch_size, x, total_row_count);
																system_monitoring_interface::active_mon->log_activity(msg, _exec_time, __FILE__, __LINE__);
															}
															else
															{
																log_error_array(put_result);
															}
															});
														datomatic = jp.create_array();
													}

												}

												// Close the file stream once all lines have been read.
												fclose(fp);
												import_spec.put_member("import_datatime", file_modified_dt);
												new_dataset.share_member("import", import_spec);
												json dataset_request = create_system_request(new_dataset);
												json result = put_object(dataset_request);
												if (result[success_field]) {
													std::string msg = std::format("imported from {0}", filename);
													system_monitoring_interface::active_mon->log_information(msg, __FILE__, __LINE__);
												}
												else {
													system_monitoring_interface::active_mon->log_warning(result[message_field], __FILE__, __LINE__);

												}
											}
											else {
												char error_buffer[256] = {};
												strerror_s(
													error_buffer,
													std::size(error_buffer),
													error_code
												);
												std::string msg = std::format("could not open file {0}:{1}", filename, error_buffer);
												system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
												char directory_name[MAX_PATH] = {};
												char* result = _getcwd(directory_name, std::size(directory_name));
												if (result) {
													msg = std::format("cwd is {0}", result);
													system_monitoring_interface::active_mon->log_information(msg, __FILE__, __LINE__);
												}
											}
										}
									}
									else {
										std::string msg = std::format("file {0} doesn't exist", filename);
										system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
									}
								}
							}

							if (new_dataset.has_member("objects")) {
								json object_list = new_dataset["objects"];
								if (object_list.array()) {
									for (int j = 0; j < object_list.size(); j++) {
										json object_definition = object_list.get_element(j);
										json put_object_request = create_system_request(object_definition);
										json create_result = put_object(put_object_request);
										if (not create_result[success_field]) {
											system_monitoring_interface::active_mon->log_warning(create_result[message_field]);
										}
										else {
                                            json result = create_result[data_field];
											for (auto class_result : result.get_members()) {
                                                json items = class_result.second;
												if (items.array()) {
													for (auto item : items) {
														if (not item[success_field]) {
															system_monitoring_interface::active_mon->log_warning(item[message_field], __FILE__, __LINE__);
														}
														else {
                                                            json item_data = item[data_field];
															std::string new_class_name = (std::string)item_data[class_name_field];
															int64_t object_id = item_data[object_id_field];
															std::string object_created = std::format("object {0} {1} saved", new_class_name, object_id);
															system_monitoring_interface::active_mon->log_information(object_created);
														}
													}
												}
											}
										}
									}
								}
							}
							date_time completed_date = date_time::now();
							new_dataset.put_member("completed", completed_date);
							json put_script_request = create_system_request(new_dataset);
							json save_script_result = put_object(put_script_request);
							if (not save_script_result[success_field]) {
								system_monitoring_interface::active_mon->log_warning(save_script_result[message_field]);
							}
							else
								system_monitoring_interface::active_mon->log_information(save_script_result[message_field]);
						}

						system_monitoring_interface::active_mon->log_job_section_stop("DataSet", dataset_name + " Finished", txs.get_elapsed_seconds(), __FILE__, __LINE__);
					}
					system_monitoring_interface::active_mon->log_job_section_stop("DataSets", "", txsect.get_elapsed_seconds(), __FILE__, __LINE__);
				}
			}

			json put_schema_request = create_system_request(jschema);
			// in corona, creating an object doesn't actually persist anything 
			// but a change in identifier.  It's a clean way of just getting the 
			// "new chumpy" item for ya.  
			json put_schema_result =  put_object(put_schema_request);
			if (put_schema_result[success_field]) {
				std::string message = put_schema_result[message_field];
				system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
			}
			else 
			{
				system_monitoring_interface::active_mon->log_warning(put_schema_result[message_field], __FILE__, __LINE__);
			}

			system_monitoring_interface::active_mon->log_job_stop("apply_schema", "schema applied", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			json temp = R"({ "success" : true, "message" : "Everything Ok, situation normal."})"_jobject;

			return temp;
		}

		virtual relative_ptr_type open_database()
		{
			write_scope_lock my_lock(database_lock);
			json_parser jp;

			timer method_timer;
			date_time start_schema = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("open_database", "Open database", start_schema, __FILE__, __LINE__);

			activity act;
			act.db = this;

			json class_definition = jp.parse_object(class_definition_string);
			if (class_definition.error())
				throw std::exception("Class Definition Parse Error");

			classes = std::make_shared<xtable>("classes.coronatbl");

            std::vector<std::string> check_classes = { "sys_object", "sys_user", "sys_team", "sys_grant", "sys_server", "sys_error", "sys_schema", "sys_dataset" };

			for (auto check_class : check_classes) {
                std::string class_key = std::format(R"({{"class_name":"{0}"}})", check_class);
                json key = jp.parse_object(class_key);
				json test = classes->get(key);
				if (test.empty() or test.error()) {
					system_monitoring_interface::active_mon->log_warning("could not find class '" + check_class + "' after open.", __FILE__, __LINE__);
					system_monitoring_interface::active_mon->log_job_stop("open_database", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					abort();
				}
			}

			system_monitoring_interface::active_mon->log_job_stop("open_database", "Open database", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return true;
		}

		class password_metrics
		{
		public:
			int punctuation_count = 0;
			int digit_count = 0;
			int alpha_count = 0;
			int char_count = 0;

			password_metrics(const std::string& _check)
			{
				for (auto c : _check)
				{
					if (std::ispunct(c) or c == ' ') {
						punctuation_count++;
					}
					else if (std::isdigit(c)) {
						digit_count++;
					}
					else if (std::isalpha(c))
					{
						alpha_count++;
					}

					char_count++;
				}

			}

			struct password_policy {
				int min_length = 10;
				int min_punctuation = 1;
				int min_digits = 1;
                int min_alpha = 1;
			};

			std::string is_stupid()
			{
				password_policy policy;

				std::string plural = "s";

				std::string corrections = "";
				int c;
				if ((c = policy.min_punctuation - punctuation_count) > 0) {
                    std::string x = c > 1 ? "s" : "";
					corrections += std::format( "Add {0} character{1}", c, x);
                }
				if ((c = policy.min_punctuation - punctuation_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} punctuation marks, like '!'", policy.min_length - char_count);
				}
				if ((c = policy.min_digits - digit_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} digits, like '0123'", policy.min_length - char_count);
				}
				if ((c = policy.min_alpha - alpha_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} letters, like 'Abc'", policy.min_alpha - alpha_count);
				}

				return corrections;
			}
		};

		/// <summary>
		/// Removes references to fields in object_to_scrub that are marked server only.
		/// This prevents clients from updating server only data,
		/// and also guards against leaking them.
		/// </summary>
		/// <param name="object_to_scrub"></param>
		virtual void scrub_object(json& object_to_scrub)
		{
			
			if (auto obj_impl = object_to_scrub.object_impl()) {
				std::string class_name = object_to_scrub[class_name_field];
				auto rsp = read_lock_class(class_name);
                auto member_set = object_to_scrub.get_members();
				for (auto member : member_set)
				{
					if (rsp and not rsp->is_server_only(member.first)) {
						obj_impl->members.erase(member.first);
					}
                    json child(member.second);
					if (child.object() or child.array()) {
						scrub_object(child);
					}
				}
			}
			else if (auto array_impl = object_to_scrub.array_impl()) {
				for (int i = 0; i < array_impl->elements.size(); i++) {
					json item(array_impl->elements[i]);
					if (item.object()) {
						scrub_object(item);
					}
				}
			}
		}

		date_time last_checked;
		json google_keys;

		//https://www.googleapis.com/oauth2/v3/certs

		json check_jwt_token(std::string _token)
		{

			json_parser jp;
			json result;

			std::vector<std::string> parts = split(_token, '.');

			date_time current = date_time::now();

			if (google_keys.empty() or current.hour() != last_checked.hour()) {
				write_scope_lock check_lock(key_lock);

				last_checked = current;
				http_client google_api;
				http_params google_response = google_api.get("www.googleapis.com", 443, "/oauth2/v3/certs");
				system_monitoring_interface::active_mon->log_warning("Google auth keys", __FILE__, __LINE__);
				if (google_response.response.response_body.is_safe_string()) 
				{
					char* body = google_response.response.response_body.get_ptr();
					system_monitoring_interface::active_mon->log_warning(body, __FILE__, __LINE__);
					google_keys = jp.parse_object(body);
				}
				else 
				{
					system_monitoring_interface::active_mon->log_warning("Response body is not a string", __FILE__, __LINE__);
				}
			}

			if (google_keys.array()) {
				std::string header = base64_decode(parts[0]);
				json jheader = jp.parse_object(header);

				std::string payload = base64_decode(parts[1]);
				json jpayload = jp.parse_object(payload);

				std::string kid = jheader["kid"];
				json public_key;
                for (auto key : google_keys) {
					std::string jkid = key["kid"];
                    if (jkid == kid) {
                        public_key = key;
                        break;
                    }
                }
                std::string plain_text = parts[0] + "." + parts[1];
				std::string cipher_text = crypter.encrypt(plain_text, public_key);
				std::string cipher64 = base64_encode(cipher_text);
				result = jpayload;
			}

			return result;
		}

		virtual json login_user_sso(json _sso_user_request) override
		{
			timer method_timer;
			json_parser jp;
			json response;

			std::vector<validation_error> errors;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("login_user_sso", "start", start_time, __FILE__, __LINE__);
			
			json data = _sso_user_request[data_field];
            std::string access_code = data["code"];
			std::string user_name = data[user_name_field];
			std::string user_email = data[user_email_field];

			if (access_code.empty())
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Invalid access_code (sso integration botched)";
				errors.push_back(err);				
				response = create_user_response(_sso_user_request, false, "access_code is required to validate.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string client_secret = connections.get_connection("googleclientsecret");
			std::string client_id = connections.get_connection("googleclientid");

			/*
			POST /token HTTP/1.1
Host: oauth2.googleapis.com
Content-Type: application/x-www-form-urlencoded

code=4/P7q7W91a-oMsCeLvIaQm6bTrgtp7&
client_id=your_client_id&
client_secret=your_client_secret&
redirect_uri=https%3A//oauth2.example.com/code&
grant_type=authorization_code
			*/

			json jpost_body = jp.create_object();
            jpost_body.put_member("code", access_code);
			jpost_body.put_member("client_id", client_id);
			jpost_body.put_member("client_secret", client_secret);
			jpost_body.put_member("redirect_uri", std::string("postmessage"));
			jpost_body.put_member("grant_type", std::string("authorization_code"));

			http_client google_api;
			http_params google_response1 = google_api.post("oauth2.googleapis.com", 443, "token", jpost_body);

			if (not google_response1.response.response_body.is_safe_string())
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_token";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Could not convert Google code to token";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(_sso_user_request, false, "could not convert google code.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			json token_body = jp.parse_object(google_response1.response.response_body.get_ptr());
            std::string access_token = token_body["access_token"];

            std::string header = std::format("Authorization: Bearer {}\n", access_token);

			http_params google_response = google_api.get("www.googleapis.com", 443, "/oauth2/v3/userinfo", header.c_str());
			system_monitoring_interface::active_mon->log_warning("Google auth keys", __FILE__, __LINE__);

            if (not google_response.response.response_body.is_safe_string())
			{
                std::vector<validation_error> errors;
                validation_error err;
                err.class_name = "sys_user";
                err.field_name = "access_token";
                err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "Google didn't like the sign on";
                errors.push_back(err);
                system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
                response = create_user_response(_sso_user_request, false, "access_token is invalid.", data, errors, method_timer.get_elapsed_seconds());
                return response;
			}

            json user_body = jp.parse_object(google_response.response.response_body.get_ptr());

            user_name = user_body["email"];
            user_email = user_body["email"];

            std::string user_first = user_body["given_name"];
            std::string user_last = user_body["family_name"];
            std::string user_picture = user_body["picture"];

			data.put_member(user_name_field, user_name);
			data.put_member(user_email_field, user_email);

			auto sys_perm = get_system_permission();

			json existing_user = get_user(user_name, sys_perm);

			if (not existing_user.object())
			{
				std::string user_password1;
				user_password1 = get_random_code() + get_random_code() + get_random_code();
				std::string hashed_pw = crypter.hash(user_password1);

				existing_user = jp.create_object();
				existing_user.put_member(class_name_field, std::string("sys_user"));
				existing_user.put_member(user_name_field, user_name);
				existing_user.put_member(user_email_field, user_email);
				existing_user.put_member(user_password_field, hashed_pw);
				existing_user.put_member(user_first_name_field, user_first);
				existing_user.put_member(user_last_name_field, user_last);
				existing_user.put_member("picture", user_picture);
				existing_user.put_member("confirmed_code", 1);
				json teams = get_team_by_email(user_email, sys_perm);
				for (json team : teams)
				{
					existing_user.put_member("home_team_name", (std::string)team["team_name"]);
					existing_user.put_member("team_name", (std::string)team["team_name"]);
				}

				apply_user_team(existing_user);

				json create_object_request = create_system_request(existing_user);
				json user_result = put_object(create_object_request);
				// we have to confirm if this guy did his job.
				json jerrors = user_result["errors"];
				if (user_result[success_field]) {
					existing_user = get_user(user_name, sys_perm);
				} 				
				else 				
				{
					std::string message = user_result[message_field];
					system_monitoring_interface::active_mon->log_warning(std::format("Could not create user '{}': {}", user_name, message), __FILE__, __LINE__);
					system_monitoring_interface::active_mon->log_json(user_result);
					response = create_user_response(_sso_user_request, false, "User not created", existing_user, jerrors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return response;
				}
			}
			else {
				json teams = get_team_by_email(user_email, sys_perm);
				for (json team : teams)
				{
					existing_user.put_member("home_team_name", (std::string)team["team_name"]);
					existing_user.put_member("team_name", (std::string)team["team_name"]);
				}
				apply_user_team(existing_user);
				put_user(existing_user);
				existing_user = get_user(user_name, sys_perm);
			}

			response = create_response(user_name, auth_general, true, "Ok", existing_user, errors, method_timer.get_elapsed_seconds());
			system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}

		virtual json create_user(json create_user_request, bool _trusted_user = false, bool _system_user = false)
		{
			timer method_timer;
			json_parser jp;
			json response;

			read_scope_lock my_lock(database_lock);

			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("create_user", "start", start_time, __FILE__, __LINE__);

			json data = create_user_request[data_field];

			std::string user_name = data[user_name_field];
			std::string user_email = data[user_email_field];

			if (user_name.empty() and not user_email.empty()) {
				user_name = user_email;
			}
			else if (not user_name.empty() and user_email.empty()) {
				user_email = user_name;
            }
			else if (user_name.empty() and user_email.empty()) {
				std::vector<validation_error> errors;
				validation_error err;
                err.class_name = "sys_user";
                err.field_name = user_name_field;	
                err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "email/username is required";
                errors.push_back(err);
				err.field_name = user_email_field;
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, "An email is required.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			data.put_member(user_name_field, user_name);
			data.put_member(user_email_field, user_email);
			std::string user_password1 = data["password1"];
			std::string user_password2 = data["password2"];
			std::string user_class = "sys_user";		

			if (user_password1 != user_password2)
			{
				std::vector<validation_error> errors;
				validation_error err;
                err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "Passwords don't match";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, "Passwords don't match", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}
			// password complexity check

			password_metrics pm1(user_password1);
            std::string reason = pm1.is_stupid();

			if (not reason.empty())
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password1";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple";
				errors.push_back(err);
				err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple";
				errors.push_back(err);

				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, std::format("Password too simple:{0}", reason), data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			bool user_exists = true;
			int attempt_count = 0;

			auto sys_perm = get_system_permission();

			json existing_user = get_user(user_name, sys_perm);

			if (existing_user.object())
			{
				json_parser jp;
                json existing_errors = jp.create_array();
				response = create_user_response(create_user_request, false, "User already exists.", existing_user, existing_errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string hashed_pw = crypter.hash(user_password1);

			json create_user_params = jp.create_object();
			create_user_params.put_member(class_name_field, user_class);
			create_user_params.put_member(user_name_field, user_name);
			create_user_params.put_member(user_email_field, user_email);
			create_user_params.put_member(user_password_field, hashed_pw);
			create_user_params.copy_member(user_first_name_field, data);
			create_user_params.copy_member(user_last_name_field, data);
			create_user_params.copy_member(user_email_field, data);
			create_user_params.copy_member(user_mobile_field, data);
			create_user_params.copy_member(user_street1_field, data);
			create_user_params.copy_member(user_street2_field, data);
			create_user_params.copy_member(user_state_field, data);
			create_user_params.copy_member(user_zip_field, data);

			// TODO: SYSTEM USER CONFIRMATION POLICY
			// the default system user doesn't need a confirmation process
			// this may change or at least be allowed as an option 
			// if in the future you really wanted to lock this down.

			bool is_system_user = _system_user;
			if (is_system_user) {
				is_system_user = user_name == user_name;
			}

			create_user_params.put_member("confirmed_code", is_system_user ? 1 : 0);

			std::string new_code = get_random_code();
			create_user_params.put_member("validation_code", new_code);

			json create_object_request = create_system_request(create_user_params);
			json user_result =  put_object(create_object_request);
			// we have to confirm if this guy did his job.
			json jerrors = user_result["errors"];
			if (user_result[success_field]) {
				json new_user_wrapper = user_result[data_field]["sys_user"].get_element(0);
				new_user_wrapper = new_user_wrapper[data_field];
				if (not is_system_user)
				{
					send_user_confirmation(new_user_wrapper, default_onboard_email);
				}
				new_user_wrapper.erase_member("password");
				new_user_wrapper.erase_member("confirmed_code");
				new_user_wrapper.erase_member("validation_code");
				response = create_user_response(create_user_request, true, "User created", new_user_wrapper, jerrors, method_timer.get_elapsed_seconds());
			}
			else
			{
                std::string message = user_result[message_field];
				system_monitoring_interface::active_mon->log_warning(std::format("Could not create user '{}': {}", user_name, message), __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(user_result);
				response = create_user_response(create_user_request, false, "User not created", create_user_params, jerrors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("create_user", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}


		virtual json user_send_code(json validation_code_request) override
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			auto sys_perm = get_system_permission();

			system_monitoring_interface::active_mon->log_function_start("send_validation_code", "start", start_time, __FILE__, __LINE__);

			std::string user_name = validation_code_request[user_name_field];

			if (user_name.empty()) {
				json data = validation_code_request[data_field];
			
				if (data.object()) {
					user_name = data[user_name_field];
				}
			}

			if (user_name.empty()) {
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = user_name_field;
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "User required";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(validation_code_request, false, "user_name (typically an email address) is required", validation_code_request[data_field], errors, tx.get_elapsed_seconds());
				return response;
			}

			json user_info = get_user(user_name, sys_perm);

			std::string message = "Code not sent.";

			if (user_info.object()) {
				message = "Code sent";
				send_user_confirmation(user_info, recovery_email);
			}
			else 
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = user_name_field;
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = std::format("User '{}' not found", user_name);
				errors.push_back(err);
				message = "User not found.";
				system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				response = create_user_response(validation_code_request, false, message, validation_code_request[data_field], errors, tx.get_elapsed_seconds());
                return response;
			}

			json errors = jp.create_array();
			response = create_user_response(validation_code_request, true, message, validation_code_request[data_field], errors, tx.get_elapsed_seconds());

			system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}

		virtual void apply_user_team(json& user, json& team)
		{
			json_parser jp;
			std::string user_name = user["user_name"];
			json user_inventory = user["inventory"];
			json user_inventory_classes = team["inventory_classes"];
			if (not user_inventory.array())
			{
				user_inventory = jp.create_array();
			}
			std::map<std::string, bool> existing_classes;
			for (auto inv_item : user_inventory) {
				std::string class_name = (std::string)inv_item[class_name_field];
				existing_classes[class_name] = true;
			}
			// workflow classes lets you create editable objects for a user
			// whose methods are search
			if (user_inventory_classes.array()) {
				for (auto wf_class : user_inventory_classes) {
					std::string class_name = (std::string)wf_class;
					if (existing_classes.contains(class_name))
						continue;
					json create_req = jp.create_object();
					create_req.put_member(class_name_field, class_name);
					json sys_create_req = create_system_request(create_req);
					json result = create_object(sys_create_req);
					if (result[success_field]) {
						json obj = result[data_field];
						obj.put_member("created_by", user_name);
						user_inventory.push_back(obj);
					}
				}
			}
			user.share_member("inventory", user_inventory);
		}

		virtual void apply_user_team(json user)
		{
			json_parser jp;
            json team = get_team(user["team_name"], get_system_permission());
			if (not team.empty())
				apply_user_team(user, team);

			team = get_team(user["home_team_name"], get_system_permission());
			if (not team.empty())
				apply_user_team(user, team);
		}

		// this allows a user to login
		virtual json user_confirm_code(json _confirm_request)
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			json data;
			std::string user_name = _confirm_request[user_name_field];
			std::string user_code = _confirm_request["validation_code"];
			if (user_name.empty()) {
				data = _confirm_request[data_field];
				if (data.object()) {
					user_name = data[user_name_field];
					user_code = data["validation_code"];
				}
            }

			auto sys_perm = get_system_permission();

			json user = get_user(user_name, sys_perm);

			if (user.empty()) {

				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = std::format("User '{}' not found", user_name);
				errors.push_back(err);

				response = create_user_response(_confirm_request, false, err.message, data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string sys_code = user["validation_code"];

			if (sys_code == user_code)
			{
				user.put_member("confirmed_code", 1);
				std::string email = user[user_email_field];
				json teams = get_team_by_email(email, sys_perm);
				for (json team : teams) 
				{
					user.put_member("home_team_name", (std::string)team["team_name"]);
					user.put_member("team_name", (std::string)team["team_name"]);					
				}
				apply_user_team(user);
				put_user(user);
				user = get_user(user_name, sys_perm);

				response = create_response(user_name, auth_general, true, "Ok", user, errors, method_timer.get_elapsed_seconds());
			}
			else
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "validation_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Incorrect validation code.";
				errors.push_back(err);

				response = create_user_response(_confirm_request, false, "Incorrect validation code.  Check your e-mail or send another one.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("confirm", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}

		virtual json user_home(json _user_home_request)
		{
			timer method_timer;
			json_parser jp;

			json result;
			json result_list;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("user_home", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(_user_home_request, { auth_self }, user_name, user_auth))
			{
				result = create_response(_user_home_request, false, "User home denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

            json user_details = get_user(user_name, get_system_permission());

			system_monitoring_interface::active_mon->log_function_stop("user_home", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
            std::vector<validation_error> empty_errors;
			result = create_response(_user_home_request, true, "Ok", user_details, empty_errors, method_timer.get_elapsed_seconds());

			return result;

		}

		// and user set password
		virtual json set_user_password(json _password_request)
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			read_scope_lock my_lock(database_lock);
			std::vector<validation_error> errors;

			json data = _password_request[data_field];
			std::string user_name, user_auth;
			std::string user_code = data["validation_code"];
			std::string requested_user_name = data[user_name_field];

			auto sys_perm = get_system_permission();

			json user = get_user(requested_user_name, sys_perm);

			if (user.empty()) {

				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Incorrect user_name.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "User not found", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string sys_code = user["validation_code"];

			if (user_code.size()>0 and sys_code.size()>0 and sys_code == user_code)
			{
				user.put_member("validation_code", std::string(""));
				user_name = requested_user_name;

			}
			else if (not check_message(_password_request, { auth_general }, user_name, user_auth))
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Denied.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "Set password denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			if (requested_user_name != user_name)
			{
				response = create_response(_password_request, false, "Set Password denied.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			std::string user_password1 = data["password1"];
			std::string user_password2 = data["password2"];

			if (user_password1 != user_password2)
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Passwords don't match.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "Passwords do not match.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			password_metrics pm1(user_password1);

            std::string reason = pm1.is_stupid();
			if (not reason.empty())
			{
				std::vector<validation_error> errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password1";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple.";
				errors.push_back(err);


				response = create_user_response(_password_request, false, std::format( "Password fails complexity test: {0}", reason), jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			std::string encrypted_password = crypter.hash(user_password1);
			user.put_member("password", encrypted_password);

			put_user(user);
			json jerrors;
			response = create_user_response(_password_request, true, "Ok", data, jerrors, method_timer.get_elapsed_seconds());

			system_monitoring_interface::active_mon->log_function_stop("confirm", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}

		virtual json user_set_team(json _user_set_team_request)
		{
			timer method_timer;
			json_parser jp;

			json result;
			json result_list;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("user_set_team", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(_user_set_team_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(_user_set_team_request, false, "User set team denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

			bool team_set = false;
			std::string ok_message;
			json user_details = get_user(user_name, get_system_permission());
			if (user_details.object()) {
				json jteam = user_details["home_team"];
				if (jteam.object()) {
					json jallowed_teams = jteam["allowed_teams"];
					if (auto ata = jallowed_teams.array_impl()) {
						if (std::any_of(ata->elements.begin(), ata->elements.end(), [&](json _item) {
							return (std::string)_item == (std::string)_user_set_team_request["team_name"];
							})) {
							user_details.put_member("team_name", (std::string)_user_set_team_request["team_name"]);
							team_set = true;
                            ok_message = "Selected " + (std::string)_user_set_team_request["team_name"];
						}
					}
				}
			}

			system_monitoring_interface::active_mon->log_function_stop("user_set_team", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			std::vector<validation_error> empty_errors;

			if (team_set) {
				apply_user_team(user_details);
				put_user(user_details);
                user_details = get_user(user_name, get_system_permission());
				result = create_response(_user_set_team_request, true, ok_message, user_details, empty_errors, method_timer.get_elapsed_seconds());
			}
			else
			{
				// clear user details to avoid any kind of leak of info
				user_details = jp.create_object();
				result = create_response(_user_set_team_request, false, "False", user_details, empty_errors, method_timer.get_elapsed_seconds());
			}

			return result;

		}

		// this starts a login attempt
		virtual json login_user(json _login_request)
		{
			timer method_timer;
			json_parser jp;
			json response;

			std::vector<validation_error> errors;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);

			system_monitoring_interface::active_mon->log_function_start("login_user", "start", start_time, __FILE__, __LINE__);

			json data = _login_request;
			std::string user_name = data[user_name_field];
			std::string user_password = data["password"];
			std::string hashed_user_password;

			std::string hashed_pw = crypter.hash(user_password);
			auto sys_perm = get_system_permission();

			json user = get_user(user_name, sys_perm);
			std::string pw = user["password"];

			if (pw == hashed_pw)
			{
				bool confirm = (bool)user["confirmed_code"];

				json workflow = user["inventory"];
				json navigation_options = jp.create_object();

				if (user_name == default_user and default_user.size() > 0) 
				{					
					json result = user.clone();
					result.erase_member("password");
					result.erase_member("confirmed_code");
					result.erase_member("validation_code");

					response = create_response(user_name, auth_system, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
				}
				else if (confirm)
				{
					json result = user.clone();
					result.erase_member("password");
					result.erase_member("confirmed_code");
					result.erase_member("validation_code");

					response = create_response(user_name, auth_general, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
				}
				else
				{
					response = create_response(user_name, auth_general, false, "Need confirmation", data, errors, method_timer.get_elapsed_seconds());
				}
			}
			else
			{
				response = create_response(_login_request, false, "Failed", jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}
			system_monitoring_interface::active_mon->log_function_stop("login_user", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return response;
		}

		virtual json run_object(json _run_object_request)
		{
			std::string user_name, user_auth;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			std::vector<validation_error> errors;

			if (not check_message(_run_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(_run_object_request, false, "run object denied", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("run_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			json data = _run_object_request[data_field];
			json result = put_object(_run_object_request);
			if (result.error())
			{
				system_monitoring_interface::active_mon->log_function_stop("run_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}
			json key = jp.create_object();
			key.copy_member(class_name_field, data);
			key.copy_member(object_id_field, data);
			json get_object_request = create_request(user_name, auth_general, key);
			get_object_request.put_member("include_children", true);
			response = get_object(get_object_request);
			return response;
		}

		virtual json edit_object(json _edit_object_request) override
		{
			timer method_timer;
			json_parser jp;
			json result;
			json jedit_object;

			read_scope_lock my_lock(database_lock);

			date_time start_time = date_time::now();
			timer tx;
			std::string user_name, user_auth;

			bool include_children = (bool)_edit_object_request["include_children"];
			std::vector<validation_error> errors;

			if (not check_message(_edit_object_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(_edit_object_request, false, "edit object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("edit_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			system_monitoring_interface::active_mon->log_function_start("edit_object", "start", start_time, __FILE__, __LINE__);

			json token = _edit_object_request[token_field];
			json key = _edit_object_request.extract({ class_name_field, object_id_field });
			int64_t object_id = (int64_t)key[object_id_field];
			std::string class_name = key[class_name_field];

			class_permissions perms = get_class_permission(user_name, class_name);
			class_permissions sys_perms = get_system_permission();

			json user = get_user(user_name, sys_perms);
			auto edit_class = read_lock_class(class_name);

			if (edit_class) {

				if (object_id > 0) {
					jedit_object = edit_class->get_single_object(this, key, true, perms);
				}
				else {
					json create_object_body = jp.create_object();
					json create_object_data = jp.create_object();
					create_object_data.put_member(class_name_field, class_name);
					create_object_body.put_member(data_field, create_object_data);					
					json create_object_request = create_system_request(create_object_body);
					json result = create_object(create_object_request);
					jedit_object = result[data_field];
				}
				result = jp.create_object();
				result.share_member("object", jedit_object);
				bool attempted = false;
				json jclasses = jp.create_object();

				while (edit_class)
				{
					auto current_class_name = edit_class->get_class_name();
					edit_class->init_validation(this, perms); // this might be a bad idea
					json jedit_object = edit_class->get_single_object(this, key, include_children, perms);
					if (not jedit_object.empty() and include_children)
					{
						std::string token = _edit_object_request[token_field];
						edit_class->run_queries(this, token, class_name, jedit_object);
					}
					json jedit_class = jp.create_object();
					edit_class->get_json(jedit_class);
					jclasses.share_member(current_class_name, jedit_class);
                    std::string edit_class_base = edit_class->get_base_class_name();
					if (edit_class_base.empty()) {
						edit_class.reset();
					}
					else 
					{
                        edit_class = read_lock_class(edit_class_base);
					}
				}

				result.share_member("classes", jclasses);

			}
			else 
			{
				system_monitoring_interface::active_mon->log_function_stop("edit_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return create_response(_edit_object_request, false, "Invalid class:" + class_name, jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("edit_object", "success", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			return create_response(_edit_object_request, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
		}

		virtual json get_classes(json get_classes_request)
		{
			timer method_timer;
			json_parser jp;

			json result;
			json result_list;
			json object_list;
			json data;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("get_classes", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(get_classes_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_classes_request, false, "get_classes denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

			object_list = classes->select(jp.create_object(), [](json& _item)->json {
				return _item;
				});
            result_list = jp.create_array();

			// put on the afterburners.  my ai said this was faster...
			if (object_list.array()) {
				for (auto& item : object_list.array_impl()->elements) {
                    std::shared_ptr<json_object> jo = dynamic_pointer_cast<json_object>(item);	
					if (jo) {
						std::string class_name = jo->members[class_name_field]->to_string();
						auto permission = get_class_permission(user_name, class_name);

						if (permission.get_grant != class_grants::grant_none)
						{
							auto classd = read_lock_class(class_name);
							json item = jp.create_object();
							if (classd) {
								classd->get_json(item);
								json class_info = classd->get_info(this);
								item.share_member("info", class_info);
                                result_list.push_back(item);
							}
						}
					}
				}
			}

			data = jp.create_object();
			data.share_member("class", result_list);

			system_monitoring_interface::active_mon->log_function_stop("get_classes", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			result = create_response(get_classes_request, true, "Ok", result_list, errors, method_timer.get_elapsed_seconds());

			return result;
		}

		virtual json get_class(json get_class_request)
		{
			timer method_timer;
			json_parser jp;
			json result;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);

			system_monitoring_interface::active_mon->log_function_start("get_class", "start", start_time, __FILE__, __LINE__);

			std::vector<std::string> missing_elements;
			std::vector<validation_error> errors;


			if (not get_class_request.has_members(missing_elements, { token_field, class_name_field })) {
				std::string error_message;
				error_message = "get_class missing elements:";
				std::string comma = "";
				for (auto m : missing_elements) {
					error_message.append(comma);
					error_message.append(m);
				}
				json response = create_response(get_class_request, false, error_message, jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			std::string user_name, user_auth;

			if (not check_message(get_class_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_class_request, false, "get_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			std::string class_name = get_class_request[class_name_field];

			auto permission = get_class_permission( user_name, class_name);

			json key = jp.create_object(class_name_field, class_name);
			key.set_natural_order();

			if (permission.get_grant != class_grants::grant_none) {

				json class_definition = classes->get(key);

				auto classd = read_lock_class(class_name);
				if (classd) {
					json class_info = classd->get_info(this);

					result = jp.create_object();
					result.share_member("class", class_definition);
					result.share_member("info", class_info);

					result = create_response(get_class_request, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("get_class", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
				else {
					result = create_response(get_class_request, false, "Missing Class", key, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
			}
			else {
				result = create_response(get_class_request, false, "get_class denied", key, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			return result;
		}

		virtual json put_class(json put_class_request)
		{
			timer method_timer;
			json result;
			json_parser jp;

			std::vector<validation_error> errors;

			date_time start_time = date_time::now();
			timer tx;

			std::string pc_name = "put_class";
			std::string pc_msg = (std::string)put_class_request[data_field][class_name_field];
			std::string pc_start = pc_msg + " start";
			std::string pc_stop = pc_msg + " stop";
			std::string pc_failed = pc_msg + " failed";

			system_monitoring_interface::active_mon->log_function_start(pc_name, pc_start, start_time, __FILE__, __LINE__);

			std::vector<std::string> missing_elements;
			if (not put_class_request.has_members(missing_elements, { token_field })) {
				std::string error_message;
				error_message = "create_class missing elements:";
				std::string comma = "";
				for (auto m : missing_elements) {
					error_message.append(comma);
					error_message.append(m);
				}
				json response = create_response(put_class_request, false, error_message, jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			std::string user_name, user_auth;

			if (not check_message(put_class_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(put_class_request, false, "put_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), __FILE__, __LINE__);

				return result;
			}

			json token = put_class_request[token_field];
			json jclass_definition = put_class_request[data_field];
			std::string class_name = jclass_definition[class_name_field];

			if (jclass_definition.error())
			{
				result = create_response(put_class_request, false, "Invalid class", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			if (class_name.empty()) {
				result = create_response(put_class_request, false, "No class name", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}

			auto permission = get_class_permission(
				user_name,
				class_name);

			if (permission.put_grant != class_grants::grant_any) {
				result = create_response(put_class_request, false, "put_class denied", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), __FILE__, __LINE__);

				return result;
			}

			activity pcactivity;
			pcactivity.db = this;

			auto pclass = put_class_impl(&pcactivity, jclass_definition);

			if (pclass) 
			{				
				result = create_response(put_class_request, true, "Ok", jclass_definition, pcactivity.errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
                log_errors(pcactivity.errors);
				result = create_response(put_class_request, false, "errors", jclass_definition, pcactivity.errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_stop, tx.get_elapsed_seconds(), __FILE__, __LINE__);
			return result;
		}


		private:

		json query_class(std::string _user_name, json query_details, json update_json)
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("update", "start", start_time, __FILE__, __LINE__);

			bool include_children = (bool)query_details["include_children"];

			json base_class_name = query_details[class_name_field];
			if (base_class_name.empty()) {
				response = create_response(_user_name, auth_general, false, "class_name not specified", query_details, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("update", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			auto permission =  get_class_permission(_user_name, base_class_name);
			if (permission.get_grant == class_grants::grant_none)
			{
				response = create_response(_user_name, auth_general, false, "query_class denied", query_details, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("update", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			auto class_def = read_lock_class(base_class_name);

			json object_list = jp.create_array();
			if (not class_def) {
				response = create_response(_user_name, auth_general, false, "class not found", query_details, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("update", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			json filter = query_details["filter"];
			if (filter.empty()) {
				filter = jp.create_object();
			}

			for (auto class_pair : class_def->get_descendants())
			{
				read_class_sp classd = read_lock_class(class_pair.first);
				if (classd) {
					json objects = classd->get_objects(this, filter, include_children, permission);

					if (objects.array()) {
						for (auto obj : objects) {
							if (permission.get_grant == class_grants::grant_any or
								(permission.get_grant == class_grants::grant_own 
									and (std::string)obj["created_by"] == _user_name)) {
								object_list.push_back(obj);
							}
						}
					}
				}
			}

			response = create_response(_user_name, auth_general, true, "completed", object_list, errors, method_timer.get_elapsed_seconds());

			system_monitoring_interface::active_mon->log_function_stop("update", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			return response;
		}

		public:

		virtual json query(json query_request)
		{
			timer tx;
			read_scope_lock my_lock(database_lock);
			json_parser jp;
			json jx; // not creating an object, leaving it empty.  should work with empty objects
			// or with an object that has no members.
			json response;

			std::string user_name, user_auth;
			std::vector<validation_error> errors;

			if (not check_message(query_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(query_request, false, "query denied", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("query", "denied", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			query_context context;

			context.froms_preloaded = true;

			json from_classes = query_request["from"];
			if (from_classes.array())
			{
				json stages = query_request["stages"];

				if (not stages.array() or stages.size() == 0)
				{
					response = create_response(query_request, false, "query has no stages", jp.create_object(), errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "query has no stages", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return response;
				}

				for (auto from_class : from_classes)
				{
					std::string from_class_name = from_class[class_name_field];
					std::string from_name = from_class["name"];
					if (from_name.empty())
					{
						response = create_response(query_request, false, "from with no name", jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "from with no name", tx.get_elapsed_seconds(), __FILE__, __LINE__);
						return response;
					}
					if (from_class_name.empty())
					{
						response = create_response(query_request, false, "from with no class", jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "from with no class", tx.get_elapsed_seconds(), __FILE__, __LINE__);
						return response;
					}
					auto cd = read_lock_class(from_class_name);
					if (not cd) {
						std::string message = std::format("from class '{0}' not found", from_class_name);
						response = create_response(query_request, false, message, jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "from class not found", tx.get_elapsed_seconds(), __FILE__, __LINE__);
						return response;
					}
				}

				from_join fj;

				// these run in order to allow for dependencies
				for (auto from_class : from_classes)
				{
					std::string from_class_name = from_class[class_name_field];
					std::string filter_source_name = from_class["name"];
					std::string filter_class_name = from_class[class_name_field];
					json data = from_class[data_field];
					json objects;

					// allow the query to have inline objects
					if (data.object())
					{
						objects = jp.create_array();
						objects.push_back(data);
						fj.add_data(from_class, objects);
					}
					else if (data.array())
					{
						objects = data;
						fj.add_data(from_class, objects);
					}
					else if (data.empty()) {
						objects = jp.create_array();
						json from_classes = jp.create_array();
						json class_filter = from_class["filter"];
						auto filter_class = read_lock_class(filter_class_name);

						if (class_filter.object()) {
							auto members = class_filter.get_members();
							for (auto member : members)
							{
								auto fld = filter_class->get_field(member.first);
								if (not fld) {
									context.add_error(filter_source_name, member.first, "Invalid field for filter", __FILE__, __LINE__);
								}
								if (member.second.is_string())
								{
									std::string value = (std::string)member.second;
									if (value.starts_with("$"))
									{
										std::string path(value.substr(1));
										std::vector<std::string> split_path = split(path, '.');
										if (split_path.size() == 2) {
											std::string& source_from_name = split_path[0];
											std::string& source_from_member = split_path[1];
											fj.add_join(source_from_name, source_from_member, filter_source_name, member.first);
										}
										else
										{
											response = create_response(query_request, false, "Bad $ reference in query.", class_filter, errors, tx.get_elapsed_seconds());
											system_monitoring_interface::active_mon->log_function_stop("query", "bad query data", tx.get_elapsed_seconds(), __FILE__, __LINE__);
											return response;
										}
									}
								}
							}
						}
						json filters = fj.get_filters(from_class);
						if (filters.size() > 0) {
							for (auto from_filter : filters)
							{
								json query_class_response = query_class(user_name, from_filter, jx);
								json temp_objects = query_class_response[data_field];
								if (temp_objects.array()) {
									bool include_children = (bool)from_class["include_children"];
									if (include_children)
									{
										auto edit_class = read_lock_class(filter_class_name);
										if (edit_class) {
											std::string token = query_request[token_field];
											for (auto obj : temp_objects) {
												edit_class->run_queries(this, token, filter_class_name, obj);
											}
										}
									}
									for (auto obj : temp_objects)
									{

										objects.push_back(obj);
									}
									fj.add_data(from_filter, objects);
								}
							}							
						}
						else
						{
							std::string message = std::format("Missing filters for {0}", filter_class_name);
							response = create_response(query_request, false, message, jp.create_object(), errors, tx.get_elapsed_seconds());
							system_monitoring_interface::active_mon->log_function_stop("query", "filter error", tx.get_elapsed_seconds(), __FILE__, __LINE__);
							return response;
						}
					}
					else {
						response = create_response(query_request, true, "query data is not an object or an array", jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "bad query data",  tx.get_elapsed_seconds(), __FILE__, __LINE__);
						return response;
					}
				}

				for (auto from_class : from_classes)
				{
					std::string from_name = from_class["name"];
					json data = fj.get_data(from_name);
					context.set_data_source(from_name, data);
				}

				if (context.is_error()) 
				{
					json query_errors = context.get_errors();
					response = create_response(query_request, false, "errors", query_errors, errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return response;
				}

				// so now, we've loaded up our context, we can extract the stages
				context.put_json(query_request);

				// and, we can run the thing.
				json query_results = context.run();
				if (context.is_error()) {
					json query_errors = context.get_errors();
					response = create_response(query_request, false, "errors", query_errors, errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
				else 
				{
					json query_errors = context.get_errors();
					response = create_response(query_request, true, "completed", query_results, errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				}
			}
			else 
			{
				response = create_response(query_request, true, "query has no froms", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("query", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			return response;
		}


		virtual json create_object(json create_object_request)
		{
			timer method_timer;
			json_parser jp;
			using namespace std::literals;
			date_time start_time = date_time::now();
			timer tx;
			read_scope_lock my_lock(database_lock);
			system_monitoring_interface::active_mon->log_function_start("create_object", "start", start_time, __FILE__, __LINE__);

			json token = create_object_request[token_field];
			json data = create_object_request[data_field];
			std::string class_name = data[class_name_field];
			json response;

			std::string user_name, user_auth;
			std::vector<validation_error> errors;

			if (not check_message(create_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(create_object_request, false, "create_object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			auto permission =  get_class_permission(user_name, class_name);
			if (permission.put_grant == class_grants::grant_none) {
				validation_error ve;
				ve.message = "put access denied";
				ve.class_name = class_name;
				ve.field_name = "n/a";
				ve.filename = __FILE__;
				ve.line_number = __LINE__;
				errors.push_back(ve);
				json result = create_response(create_object_request, false, "create_object denied", data, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			auto class_def = read_lock_class(class_name);

			if (class_def) {

				if (not class_def->ready()) {
					validation_error ve;
					ve.message = "class not ready or failed definition (check base as well)";
					ve.class_name = class_name;
					ve.field_name = "n/a";
					ve.filename = __FILE__;
					ve.line_number = __LINE__;
					errors.push_back(ve);
					json result = create_response(create_object_request, false, "create_object failed", data, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return result;
				}

				json new_object = jp.create_object();
				new_object.put_member(class_name_field, class_name);

				auto fields = class_def->get_fields();

				for (auto& field : fields)
				{
					std::string field_name = field->get_field_name();

					if (field_name == "created_by" or field_name == "created" or field_name == "updated_by" or field_name == "updated")
						continue;

					if (data.has_member(field_name))
					{
						new_object.copy_member(field_name, data);
						continue;
					}

					if (field_name == class_name_field) {
						new_object.put_member(field_name, class_name);
					}
					else if (field->get_field_type() == field_types::ft_object)
					{
						new_object.put_member_object(field_name);
					}
					else if (field->get_field_type() == field_types::ft_array or field->get_field_type() == field_types::ft_query)
					{
						new_object.put_member_array(field_name);
					}
					else if (field->get_field_type() == field_types::ft_double)
					{
						new_object.put_member(field_name, 0.0);
					}
					else if (field->get_field_type() == field_types::ft_string)
					{
						new_object.put_member(field_name, ""sv);
					}
					else if (field->get_field_type() == field_types::ft_int64)
					{
						new_object.put_member_i64(field_name, 0);
					}
					else if (field->get_field_type() == field_types::ft_datetime)
					{
						date_time dt;
						new_object.put_member(field_name, dt);
					}
					else if (field->get_field_type() == field_types::ft_function)
					{
						auto key = std::make_tuple(class_name, field_name);
						if (functions.contains(key)) {
							new_object.put_member_function(field_name, functions[key]);
						}
						else 
						{
							std::string err_message = std::format("function {0} {1} not defined", class_name, field_name);
							new_object.put_member(field_name, err_message);
						}
					}
				}
				int64_t new_id = class_def->get_next_object_id();
				new_object.put_member_i64("object_id", new_id);
				commit();
				response = create_response(create_object_request, true, "Object created", new_object, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			else {
				std::string msg = std::format("create_object failed because the class '{0}' was never found.", class_name);
				system_monitoring_interface::active_mon->log_warning(msg);
				response = create_response(create_object_request, false, "Couldn't find class", create_object_request, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}

			return response;
			
		}

		virtual json put_object_nl(json put_object_request)
		{
			timer method_timer;
			json_parser jp;
			json token;
			json object_definition;
			json result;

			try {

				date_time start_time = date_time::now();
				timer tx;
				std::vector<validation_error> errors;

				system_monitoring_interface::active_mon->log_function_start("put_object", "start", start_time, __FILE__, __LINE__);

				object_definition = put_object_request[data_field];
				std::string user_name;

				if (object_definition.object()) {
					object_definition.erase_member("class_color");
				}
				else if (object_definition.array()) {
					object_definition.for_each_element([](json& _item) {
						_item.erase_member("class_color");
						});
				}

				std::string authority = auth_general;
				std::string token_authority = auth_general;

				if (not check_message(put_object_request, { authority }, user_name, token_authority))
				{
					result = create_response(put_object_request, false, "Put object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("put_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return result;
				}

				result = check_object(object_definition, user_name, errors, token_authority);

				json grouped_by_class_name = result[data_field];

				if (result[success_field])
				{
					auto classes_and_data = grouped_by_class_name.get_members();

					json child_objects = jp.create_array();

					for (auto class_pair : classes_and_data)
					{
						auto cd = read_lock_class(class_pair.first);
						if (cd) {

							// now that we have our class, we can go ahead and open the storage for it

							json data_list = jp.create_array();
							for (const auto& item : class_pair.second) {
								data_list.push_back(item[data_field]);
							}

							auto perms = get_system_permission();

							cd->put_objects(this, child_objects, data_list, perms);
						}
					}

					if (child_objects.size() > 0) {
						put_object_request.put_member(data_field, child_objects);
						result = put_object(put_object_request);
					}

					result = create_response(put_object_request, true, "Object(s) created", grouped_by_class_name, errors, method_timer.get_elapsed_seconds());
				}
				else
				{
					result = create_response(put_object_request, false, result[message_field], grouped_by_class_name, errors, method_timer.get_elapsed_seconds());
					log_errors(errors);
				}
				system_monitoring_interface::active_mon->log_function_stop("put_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

				commit();
			}
			catch (std::exception exc)
			{
                result = jp.create_object();
                result.put_member(success_field, false);
                result.put_member(message_field, "Exception: " + (std::string)exc.what());
			}
			return result;
		}

		virtual int64_t commit()
		{
			
			return 1;
		}

		virtual json put_object(json put_object_request)
		{
			read_scope_lock my_lock(database_lock);

			return put_object_nl(put_object_request);
		}

		virtual void put_object_async(json put_object_request, std::function<void(json& result, double exec_time)> on_complete)
		{
			global_job_queue->submit_job([this, put_object_request, on_complete]() {
				timer tx;
				// this is a lambda that will run in the job queue
				// it will call the put_object_nl method, which will
				// lock the database and do the work.
				read_scope_lock my_lock(database_lock);
				json result = this->put_object_nl(put_object_request);
				if (on_complete)
				{
					on_complete(result, tx.get_elapsed_seconds());
				}
			}, NULL);
		}

		virtual void put_object_sync(json put_object_request, std::function<void(json& result, double exec_time)> on_complete)
		{
			timer tx;
				// lock the database and do the work.
			read_scope_lock my_lock(database_lock);
			json result = this->put_object_nl(put_object_request);
			if (on_complete)
			{
				on_complete(result, tx.get_elapsed_seconds());
			}
		}

		virtual json get_object(json get_object_request)
		{
			timer method_timer;
			json_parser jp;
			json result;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("get_object", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;
			json object_key = get_object_request.extract({ class_name_field, object_id_field });

			std::vector<validation_error> errors;
			if (not check_message(get_object_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			std::string class_name = object_key[class_name_field];

			auto permission =  get_class_permission(user_name, class_name);
			if (permission.get_grant == class_grants::grant_none) {
				json result = create_response(get_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return result;
			}

			bool include_children = (bool)get_object_request["include_children"];
			
			json obj =  select_single_object(object_key, include_children, permission);
			if (obj.object()) 
			{
				if (include_children)
				{
					auto edit_class = read_lock_class(class_name);
					if (edit_class) {
						std::string token = get_object_request[token_field];
						edit_class->run_queries(this, token, class_name, obj);
					}
				}

				result = create_response(get_object_request, true, "Ok", obj, errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
				result = create_response(get_object_request, false, "Not found", object_key, errors, method_timer.get_elapsed_seconds());
			}
			system_monitoring_interface::active_mon->log_function_stop("get_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

			return result;
		}


		virtual json delete_object(json delete_object_request)
		{
			timer method_timer;
			json response;
			json_parser jp;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();			
			timer tx;

			std::vector<validation_error> errors;

			system_monitoring_interface::active_mon->log_function_start("delete_object", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;
			json object_key = delete_object_request.extract({ class_name_field, object_id_field });

			if (not check_message(delete_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(delete_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			std::string class_name = object_key[class_name_field];

			auto permission = get_class_permission(user_name, class_name);
			if (permission.delete_grant == class_grants::grant_none) {
				response = create_response(delete_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			auto cd = read_lock_class(class_name);
			if (cd) {

				cd->delete_objects(this, object_key, true, permission);

				response = create_response(delete_object_request, true, "Ok", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}
			else 
			{
				response = create_response(delete_object_request, true, "class not found", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			}


			return response;
		}

		virtual json copy_object(json copy_request)
		{
			timer method_timer;
			json_parser jp;
			read_scope_lock my_lock(database_lock);
			json response;

			date_time start_time = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_function_start("copy_object", "start", start_time, __FILE__, __LINE__);

			std::vector<validation_error> errors;
			std::string user_name, user_auth;

			if (not check_message(copy_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(copy_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			json source_spec = copy_request["from"];
			std::string source_class = source_spec[class_name_field];			
			std::string source_path = source_spec["path"];
			json source_key = source_spec.extract({ class_name_field, object_id_field });

			json dest_spec = copy_request["to"];
			std::string dest_class = dest_spec[class_name_field];
			std::string dest_path = dest_spec["path"];
			json dest_key = dest_spec.extract({ class_name_field, object_id_field });

			json transform_spec = copy_request["transform"];
			std::string transform_class = transform_spec[class_name_field];

			class_permissions src_permission, dest_permission, trans_permission;

			src_permission = get_class_permission(user_name, source_class);
			if (src_permission.get_grant == class_grants::grant_none) {
				response = create_response(copy_request, false, "Denied", source_spec, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			dest_permission = get_class_permission(user_name, dest_class);
			if (dest_permission.put_grant == class_grants::grant_none) {
				response = create_response(copy_request, false, "Denied", dest_spec, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
				return response;
			}

			if (not transform_class.empty()) {
				trans_permission = get_class_permission(user_name, transform_class);
				if (trans_permission.get_grant == class_grants::grant_none) {
					response = create_response(copy_request, false, "Denied", transform_spec, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), __FILE__, __LINE__);
					return response;
				}
			}

			// load the complete object
			json object_source_array = select_object(source_key, true, src_permission);
			json object_source = object_source_array.get_first_element();

			json object_to_copy_result;
			json source_object;

			if (not object_source.empty())
			{
				// if the source object is not empty, then, walk the source path to extract the child object we want
				object_to_copy_result = object_source.query(source_path);
				source_object = object_to_copy_result["value"];

				// if that worked, then let's go for the copy
				if (source_object.object()) 
				{
					json new_object;
					// we transform the object into a new class while copying, if there is a 
					// transform specification

					if (not transform_class.empty()) {
						// so we create a new object of the class
						json transform_key = transform_spec.extract({ class_name_field });
						json cor = create_request(user_name, auth_general, transform_spec);
						json new_object_result = create_object(cor);

						// then we can copy the fields over
						if (new_object_result[success_field]) {
							new_object = new_object_result[data_field];
							auto new_object_fields = new_object.get_members();
							for (auto nof : new_object_fields) {
								if (source_object.has_member(nof.first)) {
									new_object.copy_member(nof.first, source_object);
								}
							}
						}
					}
					else 
					{
						new_object = source_object.clone();
						new_object.erase_member(object_id_field);
					}

					//
					json object_dest = select_single_object(dest_key, true, dest_permission);

					// now we are going to put this object in our destination
					json object_dest_result = object_dest.query(dest_path);

					if (object_dest_result.object()) {
						json update_obj;
						json target = object_dest_result["target"];
						std::string name = object_dest_result["name"];
						update_obj = object_dest_result["object"];
						if (target.array())
						{
							target.push_back(new_object);
						}
						else if (target.object())
						{
							target.put_member(name, new_object);
						}
						json por = create_request(user_name, auth_general, update_obj);
						response = put_object(por);
					}
					else {
						response = create_response(copy_request, false, "could not find dest object", object_dest, errors, method_timer.get_elapsed_seconds());;
					}
				}
				else {
					response = create_response(copy_request, false, "could not find source object", object_source, errors, method_timer.get_elapsed_seconds());;
				}
			}
			else {
				response = create_response(copy_request, false, "source object not specified", object_source, errors, method_timer.get_elapsed_seconds());;;
			}

			system_monitoring_interface::active_mon->log_function_stop("copy_object", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
			return response;
		}

		private:

		json create_system_request(json _data)
		{
			json_parser jp;

			json payload;

			payload = jp.create_object();

			json token = jp.create_object();
            std::string user_name = _data[user_name_field];
			if (user_name.empty()) {
				user_name = default_user;
            }
			token.put_member(user_name_field, user_name);
			token.put_member(authorization_field, auth_system);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);

			payload.put_member(data_field, _data);

			return payload;
		}


		json create_request(std::string _user_name, std::string _authorization, json _data)
		{
			json_parser jp;

			json payload;

			if (_data.object())
			{
				payload = jp.create_object();
			}

			json token = jp.create_object();
			token.put_member(user_name_field, _user_name);
			token.put_member(authorization_field, _authorization);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);
			payload.share_member(data_field, _data);

			return payload;
		}

	};

	////

	class_interface* activity::get_class(std::string _class_name)
	{
		auto fi = classes.find(_class_name);

		if (fi != classes.end()) {
			return fi->second.get();
		}

		return nullptr;
	}

	class_interface* activity::create_class(std::string _class_name)
	{
		auto fi = classes.find(_class_name);

		if (fi != classes.end()) {
			return fi->second.get();
		}

		std::shared_ptr<class_implementation> class_impl = std::make_shared<class_implementation>();
		std::shared_ptr<class_interface> iclass = std::dynamic_pointer_cast<class_interface>(class_impl);
		classes.insert_or_assign(_class_name, iclass);
		return get_class(_class_name);
	}


	bool test_database_engine(json& _proof, std::shared_ptr<application> _app)
	{
		bool success = true;
		using namespace std::literals;
		date_time st = date_time::now();
		timer tx;
		system_monitoring_interface::active_mon->log_function_start("table proof", "start", st, __FILE__, __LINE__);

		std::shared_ptr<file> f = _app->create_file_ptr(FOLDERID_Documents, "ig_test.cdb");

		json_parser jp;
		json proof_assertion = jp.create_object();

		proof_assertion.put_member("test_name", "database"sv);

		json dependencies = jp.parse_object(R"( 
{ 
	"create" : [  "table.get", "table.put", "table.create", "table.select" ],
	"login" : [  "table.get", "table.put",  "table.select" ],
    "class" : [ "table.get", "table.put", "table.create", "table.select" ],
	"class_index" : [ "table.get", "table.put", "table.create", "table.select" ],
	"create" : [ "table.get", "table.put", "table.select" ],
	"put" : [ "table.get", "table.put", "table.select" ],
	"put_index" : [ "table.get", "table.put", "table.select" ],
	"get" : [ "table.get" ],
	"get_index" : [ "table.get", "table.select" ],
	"delete" : [ "table.erase" ],
	"team_class" : [ "table.get", "table.put", "table.create", "table.select" ],
	"team_get" : [ "table.get", "table.select" ],
	"team_put" : [ "table.get", "table.put", "table.select" ],
	"team_delete" : [ "table.get", "table.put", "table.create", "table.select" ]
}
)");

		bool login_success;

		date_time start_schema = date_time::now();
		system_monitoring_interface::active_mon->log_job_start("test_database_engine", "start", start_schema, __FILE__, __LINE__);

		corona_database db;

		proof_assertion.put_member("dependencies", dependencies);
		json db_config = jp.create_object();
		json server_config = jp.create_object();
		server_config.put_member(sys_user_name_field, "todd"sv);
		server_config.put_member(sys_user_password_field, "randomite"sv);
		server_config.put_member(sys_user_email_field, "todd.bandrowsky@gmail.com"sv);
		server_config.put_member(sys_default_team_field, "GuestTeam"sv);
		db_config.put_member("Server", server_config);

		db.apply_config(db_config);

		relative_ptr_type database_location = db.create_database();

		login_success = true;

		json login_positive_request = R"(
{
	"user_name" : "todd",
	"password" : "randomite"
}
)"_jobject;

		json login_negative_request = R"(
{
	"user_name" : "todd",
	"password" : "reachio"
}
)"_jobject;

		json login_result;

		login_result = db.login_user(login_negative_request);

		if (login_result[success_field]) {
			login_success = false;
			system_monitoring_interface::active_mon->log_warning("able to login with bad account", __FILE__, __LINE__);
		}

		login_result = db.login_user(login_positive_request);

		if (not login_result[success_field]) {
			login_success = false;
			system_monitoring_interface::active_mon->log_warning("can't with good account", __FILE__, __LINE__);
		}


		system_monitoring_interface::active_mon->log_job_stop("test_database_engine", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);

		return success;
	}

	bool collection_tests()
	{

		try {

			application app;

			std::shared_ptr<dynamic_box> box = std::make_shared<dynamic_box>();
			box->init(1 << 21);

			jschema schema;
			relative_ptr_type schema_id;

			schema = jschema::create_schema(box, 20, true, schema_id);

			relative_ptr_type quantity_field_id = null_row;
			relative_ptr_type last_name_field_id = null_row;
			relative_ptr_type first_name_field_id = null_row;
			relative_ptr_type birthday_field_id = null_row;
			relative_ptr_type count_field_id = null_row;
			relative_ptr_type title_field_id = null_row;
			relative_ptr_type institution_field_id = null_row;

			schema.bind_field("quantity", quantity_field_id);
			schema.bind_field("lastName", last_name_field_id);
			schema.bind_field("firstName", first_name_field_id);
			schema.bind_field("birthday", birthday_field_id);
			schema.bind_field("count", count_field_id);
			schema.bind_field("title", title_field_id);
			schema.bind_field("institutionName", institution_field_id);

			jcollection_ref ref;
			ref.data = std::make_shared<dynamic_box>(1 << 20);
			ref.max_objects = 50;
			ref.collection_size_bytes = 1 << 19;

			init_collection_id(ref.collection_id);

			jcollection people = schema.create_collection(&ref);

			put_class_request person;

			person.class_name = "person";
			person.class_description = "a person";
			person.member_fields = { last_name_field_id, first_name_field_id, birthday_field_id, count_field_id, quantity_field_id };
			relative_ptr_type person_class_id = schema.put_class(person);

			if (person_class_id == null_row)
			{
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			int birthdaystart = 1941;
			int countstart = 12;
			double quantitystart = 10.22;
			int increment = 5;

			relative_ptr_type people_object_id;

			auto sl = people.create_object(person_class_id, people_object_id);
			auto last_name = sl.get_string(0);
			auto first_name = sl.get_string(1);
			auto birthday = sl.get_time(2);
			auto count = sl.get_int64(3);
			auto qty = sl.get_double(4);
			last_name = "last 1";
			first_name = "first 1";
			birthday = birthdaystart + increment * 0;
			count = countstart + increment * 0;
			qty = quantitystart + increment * 0;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 2";
			first_name = "first 2";
			birthday = birthdaystart + increment * 1;
			count = countstart + increment * 1;
			qty = quantitystart + increment * 1;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 3";
			first_name = "first 3";
			birthday = birthdaystart + increment * 2;
			count = countstart + increment * 2;
			qty = quantitystart + increment * 2;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 4";
			first_name = "first 4";
			birthday = birthdaystart + increment * 3;
			count = countstart + increment * 3;
			qty = quantitystart + increment * 3;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 5 really long test 12345 abcde 67890 fghij 12345 klmno 67890 pqrst";
			first_name = "first 5 really long test 12345 abcde 67890 fghij 12345 klmno 67890 pqrst";
			birthday = birthdaystart + increment * 4;
			count = countstart + increment * 4;
			qty = quantitystart + increment * 4;

			int inc_count = 0;

			for (auto sl : people)
			{
				last_name = sl.item.get_string(0);
				if (not last_name.starts_with("last")) {
					std::cout << __LINE__ << ":last name failed" << std::endl;
					return false;
				}
				first_name = sl.item.get_string(1);
				if (not first_name.starts_with("first")) {
					std::cout << __LINE__ << ":first name failed" << std::endl;
					return false;
				}
				birthday = sl.item.get_time(2);
				count = sl.item.get_int64(3);
				qty = sl.item.get_double(4);

				if (birthday != birthdaystart + increment * inc_count) {
					std::cout << __LINE__ << ":birthday failed" << std::endl;
					return false;
				}

				if (count != countstart + increment * inc_count) {
					std::cout << __LINE__ << ":count failed" << std::endl;
					return false;
				}

				if (qty != quantitystart + increment * inc_count) {

					std::cout << __LINE__ << ":qty failed" << std::endl;
					return false;
				}

				inc_count++;
			}

			return true;
		}
		catch (std::exception exc)
		{
			system_monitoring_interface::active_mon->log_exception(exc);
			return false;
		}

	}

	bool array_tests()
	{

		try {
			std::shared_ptr<dynamic_box> box = std::make_shared<dynamic_box>();
			box->init(1 << 21);
			application app;

			jschema schema;
			relative_ptr_type schema_id;

			schema = jschema::create_schema(box, 50, true, schema_id);

			put_class_request sprite_frame_request;

			sprite_frame_request.class_name = "spriteframe";
			sprite_frame_request.class_description = "sprite frame";
			sprite_frame_request.member_fields = { "shortName", "object_x", "object_y", "object_w", "object_h", "color" };
			relative_ptr_type sprite_frame_class_id = schema.put_class(sprite_frame_request);

			if (sprite_frame_class_id == null_row) {
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			put_class_request sprite_class_request;
			sprite_class_request.class_name = "sprite";
			sprite_class_request.class_description = "sprite";
			sprite_class_request.member_fields = { "shortName", "object_x", "object_y", "object_w", "object_h" , member_field(sprite_frame_class_id) };
			relative_ptr_type sprite_class_id = schema.put_class(sprite_class_request);

			if (sprite_class_id == null_row) {
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			collection_id_type colid;

			init_collection_id(colid);

			relative_ptr_type classesb[2] = { sprite_class_id, null_row };

			jcollection_ref ref;
			ref.data = std::make_shared<dynamic_box>(1 << 20);
			ref.max_objects = 50;
			ref.collection_size_bytes = 1 << 19;

			init_collection_id(ref.collection_id);

			jcollection sprites = schema.create_collection(&ref);

			for (int i = 0; i < 10; i++) {

				relative_ptr_type new_sprite_id;
				auto slice = sprites.create_object(sprite_class_id, new_sprite_id);
				auto image_name = slice.get_string(0);


				auto bx = slice.get_int32("object_x");
				auto by = slice.get_int32("object_y");
				auto bw = slice.get_int32("object_w");
				auto bh = slice.get_int32("object_h");

				rectangle image_rect;

				image_name = std::format("{} #{}", "image", i);
				bx = 0;
				by = 0;
				bw = 1000;
				bh = 1000;

#if _DETAIL
				std::cout << "before:" << image_name << std::endl;
				std::cout << image_rect->w << " " << image_rect->h << " " << image_rect->x << " " << image_rect->y << std::endl;
#endif

			}

			int scount = 0;

			for (auto slice : sprites)
			{
				auto image_name = slice.item.get_string(0);
				auto bx = slice.item.get_int32("object_x");
				auto by = slice.item.get_int32("object_y");
				auto bw = slice.item.get_int32("object_w");
				auto bh = slice.item.get_int32("object_h");

#if _DETAIL
				std::cout << image_name << std::endl;
				std::cout << image_rect->w << " " << image_rect->h << " " << image_rect->x << " " << image_rect->y << std::endl;
#endif

				if (bw != 1000 or bh != 1000) {

					std::cout << __LINE__ << ":array failed" << std::endl;
					return false;
				}

			}

			return true;
		}
		catch (std::exception& exc)
		{
			system_monitoring_interface::active_mon->log_exception(exc);
			return false;
		}
	}

}

#endif
