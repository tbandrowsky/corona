
/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This is the core database engine fields for the corona database server.

*/

#pragma once

const bool debug_teams = false;
const std::string reference_field = "$reference";

template <typename T>
constexpr bool is_convertible_to_int = std::is_convertible_v<T, int>;

namespace corona
{
	static bool corona_db_read_only = false;

	const std::string class_definition_string = R"(
{	
	"class_name" : "sys_class",
	"class_description" : "Class definitions",
	"fields" : {			
			"class_name":"string",
			"class_description":"string",
			"class_author": "string",
			"class_version": "string",
			"base_class_name":"string",	
			"parents":"[ string ]",
			"full_text":"[ string ]",
			"ancestors":"[ string ]",
			"descendants":"[ string ]",
			"fields" : "object",
			"indexes" : "object",
			"sql" : "object",
			"field_order": "[ string ]"
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
		grant_anyorown = 3,
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
		bool                     is_reference;
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

			cod.is_reference = false;

			if ((not _src) or (*_src == 0))
			{
				return cod;
			}

			_src = pb.eat_white(_src);
			if (*_src == '[')
			{
				_src++;
				cod.is_array = true;
				_src = pb.eat_white(_src);
			}
			else if (_src[0] == '-' && _src[1] == '>')
			{
				_src += 2;
				cod.fundamental_type = field_types::ft_reference;
				cod.is_undefined = false;
				auto child_class = std::make_shared<child_object_class>();
				child_class->class_name = _src;
				child_class->copy_values[_default_target_field] = reference_field;
				cod.child_classes.push_back(child_class);
				cod.is_reference = true;
				return cod;
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
								new_class->copy_values.insert_or_assign(_default_target_field, reference_field);
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
							new_class->copy_values.insert_or_assign(dest_field, reference_field);
						}
						else if (*_src == ';')
						{
							_src++;
							status = parsing_class_name;
							new_class->copy_values.insert_or_assign(dest_field, reference_field);
							cod.child_classes.push_back(new_class);
							new_class = std::make_shared<child_object_class>();
						}
						else if (*_src == 0 || *_src == ']')
						{
							new_class->copy_values.insert_or_assign(dest_field, reference_field);
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

		const char* case1 = "class1";
		cd = child_object_definition::parse_definition(case1, "XOXO");
		result = not cd.is_undefined and cd.child_classes.size() == 1 and cd.child_classes[0]->class_name == case1 and cd.child_classes[0]->copy_values.size() == 1 and cd.child_classes[0]->copy_values.contains("XOXO");
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

		const char* case4 = "class1:target1;class2:target2 = src2;";
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

		system_monitoring_interface::active_mon->log_function_stop("parse_child_field", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
	}

	class chest_field
	{
	public:
		std::map<object_reference, chest_item> items;
		std::shared_ptr<chest_field_options_interface> options;

		chest_field() = default;
		chest_field(const chest_field& _src) = default;
		chest_field(chest_field&& _src) = default;
		chest_field& operator = (const chest_field& _src) = default;
		chest_field& operator = (chest_field&& _src) = default;

		chest_field(std::shared_ptr<chest_field_options_interface> _options, json _src) : options(_options)
		{
			put_json(_src);
		}

		chest_item* use(int _quantity)
		{
			chest_item* ci = nullptr;

			// you'll have to clean up the inventory at some point
			// to remove zero items, but not now.
			for (auto& item : items) {
				if (item.second.quantity >= _quantity) {
					item.second.quantity -= _quantity;
					ci = &item.second;
					break;
				}
			}
			return ci;
		}

		bool match(std::vector<chest_item>& _matches, std::string _class_name, std::string _item_type, double _quantity) const
		{
			if (!options) return false;
			double found_quantity = 0.0;

			for (auto& item : items) {
				if ((_class_name.empty() || (_class_name == item.first.class_name)) && (_item_type.empty() || item.second.item_type == _item_type))
				{
					chest_item temp = item.second;
					double new_found_quantity = found_quantity + item.second.quantity;
					if (new_found_quantity > _quantity) {
						temp.quantity = new_found_quantity - item.second.quantity;
					}
					_matches.push_back(temp);
				}
			}

			return found_quantity >= _quantity;
		}

		chest_item* find(const object_reference& _ref)
		{
			if (!options) return nullptr;
			auto iter = items.find(_ref);
			if (iter != items.end()) {
				auto& cf = iter->second;
				return &cf;
			}
			return nullptr;
		}

		void add_part(const chest_item& _item)
		{
			if (!options) return;

			auto iter = items.find(_item.reference);
			if (iter != items.end()) {
				iter->second.quantity += _item.quantity;
			}
			else
			{
				items.insert_or_assign(_item.reference, _item);
			}
		}

		bool remove_part(const chest_item& _item)
		{
			if (!options) return false;

			auto iter = items.find(_item.reference);
			if (iter != items.end()) {
				if (iter->second.quantity < _item.quantity) {
					return false;
				}
				iter->second.quantity -= _item.quantity;
				if (iter->second.quantity == 0) {
					items.erase(iter);
				}
			}
			else
			{
				items.insert_or_assign(_item.reference, _item);
			}
		}

		bool move_part(chest_field& _dest, const chest_item& _item)
		{
			if (!options) return false;

			if (remove_part(_item)) {
				_dest.add_part(_item);
				return true;
			}
			return false;
		}

		bool loot(chest_field& _dest)
		{
			if (!options) return false;

			for (auto& item : items) {
				if (remove_part(item.second)) {
					_dest.add_part(item.second);
				}
			}
			return true;
		}

		virtual void get_json(json& _dest)
		{
			if (!options) return;

			json_parser jp;
			_dest = jp.create_array();

			for (auto& item : items) {
				json jitem = jp.create_object();
				item.second.get_json(jitem);
				_dest.push_back(jitem);
			}

			_dest.put_member("items", _dest);
		}

		virtual void put_json(json& _src)
		{
			if (!options) return;

			json_parser jp;

			items.clear();

			if (_src.array()) {
				for (int i = 0; i < _src.size(); i++) {
					json jitem = _src.get_element(i);
					chest_item item;
					item.put_json(jitem);
					items.insert_or_assign(item.reference, item);
				}
			}
		}
	};

	class selection_field
	{
	public:

		std::shared_ptr<selection_field_options_interface> options;
		std::map<object_reference, bool> selections;

		selection_field(std::shared_ptr<selection_field_options_interface> _options, json _src) : options(_options)
		{
			put_json(_src);
		}

		selection_field() = default;
		selection_field(const selection_field& _src) = default;
		selection_field(selection_field&& _src) = default;
		selection_field& operator=(const selection_field& _src) = default;
		selection_field& operator=(selection_field&& _src) = default;


		void clear()
		{
			selections.clear();
		}

		void create(const object_reference& _item)
		{
			selections.clear();
			selections[_item] = true;
		}

		void extend(const object_reference& _item)
		{
			selections[_item] = true;
		}

		void remove(const object_reference& _item)
		{
			selections.erase(_item);
		}

		virtual bool is_selected(chest_item* _item)
		{
			return selections.contains(_item->reference);
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			_dest = jp.create_array();

			for (const auto& sel : selections) {
				json jitem = jp.from_reference(sel.first);
				_dest.push_back(jitem);
			}
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;

			selections.clear();
			if (_src.array()) {
				for (int i = 0; i < _src.size(); i++) {
					json jitem = _src.get_element(i);
					auto ref = jitem.as_object_reference();
					selections[ref] = true;
				}
			}
		}
	};

	namespace selection_commands
	{

		class requirement
		{
		public:
			std::string name;
			std::string class_name;
			std::string item_type;
			double		quantity;

			requirement() = default;
			requirement(const requirement& _src) = default;
			requirement(requirement&& _src) = default;
			requirement& operator = (const requirement& _src) = default;
			requirement& operator = (requirement&& _src) = default;
		};

		class match
		{
		public:
			std::shared_ptr<requirement> requirement;
			std::vector<chest_item> matches;

			operator bool()
			{
				return requirement && matches.size() > 0;
			}

			chest_item first()
			{
				if (matches.size() > 0) {
					return matches[0];
				}
				return chest_item();
			}
		};

		class command
		{
		public:
			std::string command_name;
			std::vector<std::shared_ptr<requirement>> requirements;

			bool matches(std::vector<match>& _matched_items, chest_field& _src, selection_field& _selection) const
			{
				for (auto& req : requirements)
				{
					match mx;
					bool matches = _src.match(mx.matches, req->class_name, req->item_type, req->quantity);
					if (!matches) {
						_matched_items.clear();
						return false;
					}
					_matched_items.push_back(mx);
				};
				return true;
			}

			bool matches(chest_field& _src, selection_field& _selection) const
			{
				std::vector<match> dummy_matches;
				return matches(dummy_matches, _src, _selection);
			}

			virtual match get_match(std::vector<match>& _matched_items, std::shared_ptr<requirement> _req)
			{
				match m;
				auto found = std::find_if(_matched_items.begin(), _matched_items.end(), [&_req](const match& _match)->bool {
					return _req == _match.requirement;
					});
				if (found != std::end(_matched_items)) {
					m = *found;
				}
				return m;
			}
		};

		class command_collection
		{
			std::map<std::string, std::shared_ptr<command>> commands;

		public:

			command_collection() = default;
			command_collection(const command_collection& _src) = default;
			command_collection(command_collection&& _src) = default;
			command_collection& operator = (const command_collection& _src) = default;
			command_collection& operator = (command_collection&& _src) = default;

			void put(std::shared_ptr<command> _command)
			{
				commands[_command->command_name] = _command;
			}

			std::shared_ptr<command> get(std::string _name)
			{
				return commands[_name];
			}

			std::vector<std::shared_ptr<command>> get_commands_for_selection(chest_field& _all_items, selection_field& _selected_items)
			{
				std::vector<std::shared_ptr<command>> matching_commands;

				for (auto& command : commands) {
					if (command.second->matches(_all_items, _selected_items)) {
						matching_commands.push_back(command.second);
					}
				}

				return matching_commands;
			}

		};

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
			cp.user_name = user_name;
			cp.team_name = team_name;
			return cp;
		}
	};

	class corona_database_interface;

	class child_relation_interface
	{
	public:
		virtual std::string get_class_name() = 0;
		virtual void set_class_name(std::string _class_name) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(json& _src) = 0;
		virtual void copy(json& _dest, json& _src) = 0;
		virtual json get_key(json& src) = 0;
		virtual json get_copy_assignment() = 0;
	};

	class child_relations_interface
	{
	public:
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(json& _src) = 0;

		virtual std::shared_ptr<child_relation_interface> get_relation(std::string _class_name) = 0;

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual json get_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) = 0;
		virtual json delete_children(corona_database_interface* _db, json _parent_object, class_permissions _permissions) = 0;

		virtual std::vector<std::string> get_allowed_classes() = 0;
	};

	class field_options_interface
	{
	public:

		bool		required;
		std::string format;
		std::string input_mask;
		std::string label;
		std::string description;
		std::string tab_index;
		bool		read_only;
		bool		server_only;

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual json run_method(corona_database_interface* _db, std::string _method_name, std::string& _token, std::string& _class_name, json& _object) = 0;
		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json& _object) = 0;
		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) = 0;
		virtual std::shared_ptr<child_relations_interface> get_relations() = 0;
		virtual bool is_relational_children() = 0;
		virtual json get_openapi_schema(corona_database_interface* _db) = 0;
		virtual bool is_required() = 0;
		virtual bool is_server_only() = 0;
		virtual std::vector<std::string> get_allowed_classes() = 0;


		virtual void get_json(json& _dest)
		{
			_dest.put_member("required", required);
			_dest.put_member("format", format);
			_dest.put_member("input_mask", input_mask);
			_dest.put_member("label", label);
			_dest.put_member("description", description);
			_dest.put_member("tab_index", tab_index);
			_dest.put_member("read_only", read_only);
			_dest.put_member("server_only", server_only);
		}

		virtual void put_json(json& _src)
		{
			required = _src["required"].as_bool();
			format = _src["format"].as_string();
			input_mask = _src["input_mask"].as_string();
			label = _src["label"].as_string();
			description = _src["description"].as_string();
			tab_index = _src["tab_index"].as_string();
			server_only = _src["server_only"].as_bool();
			read_only = _src["read_only"].as_bool();
		}

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

		virtual void init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;
		virtual bool accepts(corona_database_interface* _db, validation_error_collection& _validation_errors, std::string _class_name, std::string _field_name, json& _object_to_test) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(validation_error_collection& _errors, json& _src, class_interface_base* _owner) = 0;

		virtual const std::string& get_field_name() const
		{
			return field_name;
		}
		virtual const std::string& get_field_class() const
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

		virtual json run_method(corona_database_interface* _db, std::string _method_name, std::string& _token, std::string& _class_name, json& _object) = 0;
		virtual json run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json& _object) = 0;
		virtual std::shared_ptr<child_relations_interface> get_relations() = 0;

		virtual json get_openapi_schema(corona_database_interface* _db) = 0;

		virtual bool is_relational_children() {
			if (options) {
				return options->is_relational_children();
			}
			return false;
		}

		virtual std::vector<std::string> get_allowed_classes()
		{
			if (options) {
				return options->get_allowed_classes();
			}
			return {};
		}

		virtual std::string get_label()
		{
			std::string label;
			if (options) {
				label = options->label;
			}
			if (label.empty()) {
				label = human_case(field_name);
			}
			return label;
		}
	};

	class index_interface
	{
	protected:
	public:
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(validation_error_collection& _errors, json& _src) = 0;

		virtual int64_t									get_index_id() = 0;
		virtual std::string								get_index_name() = 0;
		virtual void									set_index_name(const std::string& _index_name) = 0;
		virtual std::string								get_index_class() = 0;
		virtual void									set_index_class(const std::string& _index_class) = 0;
		virtual std::vector<std::string>& get_index_keys() = 0;
		virtual std::shared_ptr<xtable>					get_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					create_xtable(corona_database_interface* _db, std::map<std::string, std::shared_ptr<field_interface>>& _fields) = 0;
		virtual std::string								get_index_key_string() = 0;
		virtual std::string								get_index_filename(corona_database_interface* _db) = 0;
		virtual int64_t									get_count() = 0;
		virtual std::shared_ptr<index_interface>		clone() = 0;
	};

	class activity;

	class field_section
	{
	public:
		std::string section_name;
		std::string section_description;
		std::vector<std::shared_ptr<field_interface>> section_fields;
	};

}
