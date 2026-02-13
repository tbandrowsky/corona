
/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved


MIT License

About this File
this is a query engine, that you can drop sources into for json, and then query 
used in database enngine annd in ui.

Notes

For Future Consideration
*/

#pragma once
namespace corona
{
	class source_item
	{
	public:
		std::string name;
        json		data;

		source_item() 
		{
			json_parser jp;
            name = "";
            data = jp.create_array();
		}
	};

	class query_context_base
	{
		json froms;
		std::map<std::string, std::shared_ptr<source_item>> sources;
		validation_error_collection errors;
		 
	public:

		bool froms_preloaded = false;

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "query_context"sv);
			_dest.put_member("froms", froms);
		}

		virtual void put_json(json& _src)
		{
			if (froms_preloaded)
				return;

			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "class_name", "froms" })) {
				system_monitoring_interface::active_mon->log_warning("query_context missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			froms = _src["froms"];
			sources.clear();
		}

		void add_error(const std::string& _class_name, const std::string& _field_name, const std::string& _message, const std::string& _file, int _line_number)
		{
			validation_error ve;
			ve.class_name = _class_name;
			ve.field_name = _field_name;
			ve.message = _message;
			ve.filename = _file;
			ve.line_number = _line_number;
			errors.push_back(ve);
		}

		void clear_errors()
		{
			errors.clear();
		}

		bool is_error()
		{
			return errors.size() > 0;
		}

		json get_errors_json()
		{
			json_parser jp;
			json results = jp.create_array();
			for (auto err : errors) {
				json jerr = jp.create_object();
				err.get_json(jerr);
				results.push_back(jerr);
			}
			return results;
		}

		validation_error_collection& get_errors()
		{
			return errors;
        }

		virtual bool get_data(json& _dest, std::string _query) 
		{
			json_parser jp;
			json query_s = jp.parse_query(_query);
			json j;

			std::string query_si = query_s["source_name"].as_string();
			std::string query_p = query_s["query_path"].as_string();

			if (query_si.empty()) 
			{
				std::string msg = std::format("{0} query data request does not have a source_name: prefix", _query);
				add_error("get_data", _query, msg, __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_warning(msg);
				return false;
			}
			else if (not sources.contains(query_si))
			{
				std::string msg = std::format("{0}({1}) query data source not found", _query, query_si);
				add_error("get_data", _query, msg, __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_warning(msg);
				return false;
			}
			else 
			{
				_dest = sources[query_si]->data;
				_dest = _dest.query(query_p);
				_dest = _dest["value"];
			}

			return true;
		}

		virtual void set_data_source(std::string _name, json _data)
		{
			std::shared_ptr<source_item> new_si = std::make_shared<source_item>();
			new_si->name = _name;
            new_si->data = _data;
            sources[_name] = new_si;
		}

		virtual void fill_data_sources(std::function<json(std::string _name)> _filler)
		{
            for (auto pair : sources) {
                pair.second->data = _filler(pair.first);
            }
		}
	};

	class query_stage
	{
	public:
		std::string stage_name;
		json		stage_output;
		double		execution_time_seconds;

		virtual std::string term_name() { return "stage"; }
		virtual json process(query_context_base *_src) { return stage_output; }

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "query_stage"sv);
			_dest.put_member("name", stage_name);
			_dest.put_member("output", stage_output);
			_dest.put_member(seconds_field, execution_time_seconds);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "class_name", "name" })) {
				system_monitoring_interface::active_mon->log_warning("query_stage missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			stage_name = _src["name"].as_string();
			stage_output = _src["output"];
		}

	};

	void get_json(json& _dest, std::shared_ptr<query_stage>& _src);
	void put_json(std::shared_ptr<query_stage>& _dest, json _src);

	class query_context : public query_context_base
	{
	public:

		std::vector<std::shared_ptr<query_stage>> stages;

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_context_base::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "query_context"sv);
			json stage_array = jp.create_array();
			for (auto st : stages) {
				json stj = jp.create_object();
				st->get_json(stj);
				stage_array.push_back(stj);
			}
			_dest.put_member("stages", stage_array);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			stages.clear();

			if (not _src.has_members(missing, { "stages" })) {
				system_monitoring_interface::active_mon->log_warning("query_context missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_context_base::put_json(_src);

			json jstgs = _src["stages"];
			if (jstgs.array()) {
				for (auto item : jstgs) {
					std::shared_ptr<query_stage> new_query;
					corona::put_json(new_query, item);
					if (new_query) {
						stages.push_back(new_query);
					}
				}
			}
			else if (jstgs.object()) {
				std::shared_ptr<query_stage> new_query;
				corona::put_json(new_query, jstgs);
				if (new_query) {
					stages.push_back(new_query);
				}
			}
		}

		virtual bool get_data(json& _dest, std::string _data_name) override {
			json_parser jp;
			json result = jp.create_array();
			bool found = false;

			auto found_it = std::find_if(
				stages.begin(),
				stages.end(),
				[_data_name](std::shared_ptr<query_stage>& _item) ->bool {
					return _item->stage_name == _data_name;
				});
			if (found_it != std::end(stages)) {
				result = found_it->get()->stage_output;
				found = true;
			}
			else {
				found = query_context_base::get_data(result, _data_name);
			}
			return found;
		}

		virtual json run()
		{
			json output_op;
			for (auto stage : stages) {
				output_op = stage->process(this);
			}
			return output_op;
		}
	};

	class query_condition
	{
	public:
		virtual std::string term_name() { return "condition"; }
		virtual bool accepts(query_context_base *_qcb, json _src) { return true; }

		virtual void get_json(json& _dest)
		{
		}

		virtual void put_json(json& _src)
		{
		}

	};

	void put_json(std::shared_ptr<query_condition>& _dest, json _src);

	class query_filter : public query_stage
	{
	public:

		std::string stage_input_name;

		std::shared_ptr<query_condition> condition;
		virtual std::string term_name() { return "filter"; }

		virtual json process(query_context_base* _src) {
			timer tx;
			json_parser jp;
			json result = jp.create_array();

            stage_output = jp.create_array();

			if (stage_input_name.empty())
			{
				_src->add_error("filter", "source", "missing property 'source' for stage.", __FILE__, __LINE__);
				return result;
			}

			json stage_input;
			
			bool found = _src->get_data(stage_input, stage_input_name);
			if (!found) {
				return stage_output;
			} 
			else if (stage_input.object()) {
				if (condition) {
					if (condition->accepts(_src, stage_input)) {
						result.push_back(stage_input);
						stage_output = result;
						return stage_output;
					}
				}
				else {
					result.push_back(stage_input);
					stage_output = result;
					return stage_output;
				}
			}
			else if (stage_input.array()) {
				if (condition) {
					for (auto item : stage_input) {
						if (condition->accepts(_src, item)) {
							result.push_back(item);
						}
					}
				}
				else 
				{
					result = stage_input;
				}
				stage_output = result;
				return result;
			}
			execution_time_seconds = tx.get_elapsed_seconds();
			return stage_output;  
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_stage::get_json(_dest);
			json jcondition = jp.create_object();
			if (condition) {
				condition->get_json(jcondition);
				_dest.put_member("condition", jcondition);
			}
			using namespace std::literals;
			_dest.put_member("class_name", "filter"sv);
		}

		virtual void put_json(json& _src)
		{
			query_stage::put_json(_src);

			json jcondition = _src["condition"];
			if (not jcondition.empty()) {
				corona::put_json(condition, jcondition);
			}
		}

	};

	class query_result : public query_stage
	{
	public:

		std::string stage_input_name;
		virtual std::string term_name() { return "result"; }

		virtual json process(query_context_base* _src) {
			timer tx;
			json_parser jp;
			stage_output = jp.create_array();			

			if (stage_input_name.empty())
			{
				_src->add_error("result", "source", "missing property 'source' for stage.", __FILE__, __LINE__);
				return stage_output;
			}

			json stage_input;
			bool found;

			found = _src->get_data(stage_input, stage_input_name);
            if (!found) {
				return stage_output;
			}
			else if (stage_input.object()) {
				stage_output.push_back(stage_input);
				return stage_output;
			}
			else if (stage_input.array()) {
				stage_output = stage_input;
				return stage_output;
			}
			execution_time_seconds = tx.get_elapsed_seconds();
			return stage_output;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_stage::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "result"sv);
			_dest.put_member("source", stage_input_name);
		}

		virtual void put_json(json& _src)
		{
			query_stage::put_json(_src);

		}

	};

	class query_join : public query_stage
	{
	public:

		std::string resultname1;
		std::string resultname2;
		std::string source1;
		std::string source2;
		std::vector<std::string> keys;

		virtual std::string term_name() { return "join"; }

		virtual json process(query_context_base* _src) {
			timer tx;
			json_parser jp;
			stage_output = jp.create_array();

			json query1;
			json query2;

			bool found = _src->get_data(query1, source1);
			if (!found) {
				return stage_output;
			}

			found = _src->get_data(query2, source2);
			if (!found) {
				return stage_output;
			}

			if (query1.object()) {
				json t = jp.create_array();
				t.push_back(query1);
				query1 = t;
			}
		
			if (query2.object()) {
				json t = jp.create_array();
				t.push_back(query2);
				query2 = t;
			}

			stage_output = jp.create_array();

			if (query1.array() and query2.array()) {
				for (auto item1 : query1) {
					json key1 = item1.extract(keys);
					for (auto item2 : query2) {
						json key2 = item2.extract(keys);
						if (key1.compare(key2) == 0) {
							json new_item = jp.create_object();
							new_item.merge(key1);
							new_item.put_member(resultname1, item1);
							new_item.put_member(resultname2, item2);
							stage_output.push_back(new_item);
						}
					}
				}
			}
			execution_time_seconds = tx.get_elapsed_seconds();

			return stage_output;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_stage::get_json(_dest);
			_dest.put_member("class_name", "join"sv);
			_dest.put_member("resultname1", resultname1);
			_dest.put_member("resultname2", resultname2);
			_dest.put_member("source1", source1);
			_dest.put_member("source2", source2);
			std::string skeys = join(keys, ",");
			_dest.put_member("keys", skeys);
			_dest.put_member(seconds_field, execution_time_seconds);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "class_name", "resultname1", "resultname2", "source1", "source2", "keys" })) {
				system_monitoring_interface::active_mon->log_warning("join missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}
			query_stage::put_json(_src);

			resultname1 = _src["resultname1"].as_string();
			resultname2 = _src["resultname2"].as_string();
			source1 = _src["source1"].as_string();
			source2 = _src["source2"].as_string();
			std::string skeys = _src["keys"].as_string();
			keys = split(skeys, ',');
			stage_output = _src["output"];
		}

	};

	class filter_contains : public query_condition
	{
	public:

		std::string value;
		std::string valuepath;
		virtual std::string term_name() { return "contains"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{			
			json s = _src.query(valuepath);
			std::string v = s["value"].as_string();
			bool result = v.find(value) != std::string::npos;
			return result;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "contains"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"].as_string();
		}
	};

	class filter_gt : public query_condition
	{
	public:

		std::string valuepath;
		json value;

		virtual std::string term_name() { return "gt"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json s = _src.query(valuepath);
			return s["value"].gt(value);
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "gt"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"];
		}

	};

	class filter_lt : public query_condition
	{
	public:

		std::string valuepath;
		json value;

		virtual std::string term_name() { return "lt"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json s = _src.query(valuepath);
			return s["value"].lt(value);
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "lt"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"];
		}

	};

	class filter_eq : public query_condition
	{
	public:

		std::string valuepath;
		json value;

		virtual std::string term_name() { return "eq"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json s = _src.query(valuepath);
			return s["value"].eq(value);
		}

		virtual void get_json(json& _dest)
		{
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "eq"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"];
		}
	};

	class filter_gte : public query_condition
	{
	public:

		std::string valuepath;
		json value;

		virtual std::string term_name() { return "gte"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json s = _src.query(valuepath);
			return s["value"].gt(value) or s["value"].eq(value);
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "gte"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"];
		}
	};

	class filter_lte: public query_condition
	{
	public:

		std::string valuepath;
		json value;

		virtual std::string term_name() { return "lte"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json s = _src.query(valuepath);
			return s["value"].gt(value) or s["value"].eq(value);
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "lte"sv);
			_dest.put_member("valuepath", valuepath);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "valuepath", "value" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
			valuepath = _src["valuepath"].as_string();
			value = _src["value"];
		}
	};

	class filter_between : public query_condition
	{
	public:

		std::shared_ptr<filter_gte> start;
		std::shared_ptr<filter_lt> stop;

		virtual std::string term_name() { return "between"; }

		virtual bool accepts(query_context_base* _srcx, json _src)
		{
			return	start->accepts(_srcx, _src) and stop->accepts(_srcx, _src);
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "between"sv);

			json jstart = jp.create_object();
			if (start) {
				start->get_json(jstart);
				_dest.put_member("start", jstart);
			}

			json jstop = jp.create_object();
			if (stop) {
				stop->get_json(jstop);
				_dest.put_member("stop", jstop);
			}
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "start", "stop" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);

			json jstart = _src["start"];
			start = std::make_shared<filter_gte>();
			start->put_json(jstart);

			json jstop = _src["stop"];
			stop = std::make_shared<filter_lt>();
			stop->get_json(jstop);

		}

	};

	class filter_in : public query_condition
	{
	public:

		std::string src_path;
		std::string items_path;

		virtual std::string term_name() { return "in"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			json tsrcc = _src.query(src_path);
			json srcc = tsrcc["value"];
			json items;
			bool has_stuff = _qcb->get_data(items, items_path);
			if (has_stuff) {
				for (auto item : items) {
					if (item.compare(srcc) == 0)
						return true;
				}
			}
			return false;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "in"sv);
			_dest.put_member("src_path", src_path);
			_dest.put_member("items_path", items_path);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "src_path", "items_path" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
		}

	};



	class filter_all : public query_condition
	{
	public:

		virtual std::string term_name() { return "all"; }
		std::vector<std::shared_ptr<query_condition>> conditions;

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			return std::all_of(conditions.begin(), conditions.end(), [_qcb, _src](std::shared_ptr<query_condition>& _condition)-> bool
				{
					return _condition->accepts(_qcb, _src);
				});
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "in"sv);
			json jconditions = jp.create_array();
			for (auto cond : conditions) {
				json jcond = jp.create_object();
				cond->get_json(jcond);
			}
			_dest.put_member("conditions", jconditions);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "conditions" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);

			json_parser jp;
			json jconditions = _src["conditions"];
			conditions.clear();

			for (auto jcondition : jconditions)
			{
				std::shared_ptr<query_condition> new_condition;
				corona::put_json(new_condition, jcondition);
				conditions.push_back(new_condition);
			}
		}
	};

	class filter_any : public query_condition
	{
	public:
		virtual std::string term_name() { return "any"; }

		std::vector<std::shared_ptr<query_condition>> conditions;

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			if (conditions.size() == 0)
				return true;
			return std::any_of(conditions.begin(), conditions.end(), [_qcb, _src](std::shared_ptr<query_condition>& _condition) -> bool
				{
					return _condition->accepts(_qcb, _src);
				});
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "any"sv);
			json jconditions = jp.create_array();
			for (auto cond : conditions) {
				if (cond) {
					json jcond = jp.create_object();
					cond->get_json(jcond);
					jconditions.push_back(jcond);
				}
			}
			_dest.put_member("conditions", jconditions);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "conditions" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);

			json_parser jp;
			json jconditions = _src["conditions"];
			conditions.clear();

			for (auto jcondition : jconditions)
			{
				std::shared_ptr<query_condition> new_condition;
				corona::put_json(new_condition, jcondition);
				if (new_condition) {
					conditions.push_back(new_condition);
				}
			}
		}
	};

	class filter_none : public query_condition
	{
	public:

		virtual std::string term_name() { return "none"; }
		std::vector<std::shared_ptr<query_condition>> conditions;

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			return not std::any_of(conditions.begin(), conditions.end(), [_qcb, _src](std::shared_ptr<query_condition>& _condition)-> bool
				{
					return _condition->accepts(_qcb, _src);
				});
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			query_condition::get_json(_dest);
			using namespace std::literals;
			_dest.put_member("class_name", "none"sv);
			json jconditions = jp.create_array();
			for (auto cond : conditions) {
				json jcond = jp.create_object();
				cond->get_json(jcond);
			}
			_dest.put_member("conditions", jconditions);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "conditions" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);

			json_parser jp;
			json jconditions = _src["conditions"];
			conditions.clear();

			for (auto jcondition : jconditions)
			{
				std::shared_ptr<query_condition> new_condition;
				corona::put_json(new_condition, jcondition);
				conditions.push_back(new_condition);
			}
		}
	};

	class filter_allow_all : public query_condition
	{
	public:


		virtual std::string term_name() { return "allow_all"; }

		virtual bool accepts(query_context_base* _qcb, json _src)
		{
			return true;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			query_condition::get_json(_dest);
			_dest.put_member("class_name", "allow_all"sv);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name" })) {
				system_monitoring_interface::active_mon->log_warning(std::format("filter '{}' missing:", term_name()));
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			query_condition::put_json(_src);
		}
	};

	class query_project : public query_stage {
	public:

		std::string source_name;
		json		projection;

		virtual std::string term_name() { return "project"; }

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "project"sv);
			_dest.put_member("source", source_name);
			_dest.put_member("projection", projection);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			source_name = _src["source"].as_string();
			projection = _src["projection"];

			stage_name = _src["name"].as_string();
			if (stage_name.empty()) {
				stage_name = std::format("project_{0}", source_name);
			}
			stage_output = _src["output"];
		}

		virtual json process(query_context_base* _src) {
			timer tx;
			json_parser jp;
			auto members = projection.get_members();
			stage_output = jp.create_array();

			// if there is no source, the projection is really just an object
            // composed of multiple queries that are executed against the main source. 
			// This allows for composition of multiple queries into a single result object 
			// without having to have a source object to query against.
			if (source_name.empty()) {
                json temp_object = jp.create_object();
				for (auto member : members) {
					std::string path = member.second.as_string();
					json data;
                    if (_src->get_data(data, path)) {
						data = data["value"];
						temp_object.put_member(member.first, data);
					}
					else 
					{
						std::string msg = std::format("'{0}' that does not have a source. Egs source_name:path.path.path", member.first, path);
						_src->add_error("projection", member.first, msg, __FILE__, __LINE__);
						comm_bus_app_interface::global_bus->log_warning(msg, __FILE__, __LINE__);
					}
				}
                stage_output.push_back(temp_object);
			}
            // but, if there is a source, then, the projection is executed against the source, 
			// and each member of the projection is a query executed against the source, 
			// and the results are composed into the output object/array.
			else 
			{
				json projection_source;
				bool found = _src->get_data(projection_source, source_name);

				if (!found) {
					return stage_output;
                }
				else if (projection_source.object())
				{
                    json temp_object = projection_source;
                    projection_source = jp.create_array();
                    projection_source.push_back(temp_object);
				}

				if (projection_source.array())
				{
					stage_output = jp.create_array();
					for (auto arr_item : projection_source) {
						json new_item = jp.create_object();
						// look at our projection object.
						// for each member, 
                        // if it is a string, 
						// 
						// take it as a query of the arr_item
						// try it as a path and put the result in the output object under the member name.
						// 
						// if it is a non-string, apply the member directly to the object
						for (auto member : members) {
							if (member.second.is_string()) {

								std::string path = member.second.as_string();
								if (path.starts_with("."))
								{
									path = path.substr(1);
									json data = arr_item.query(path);
									data = data["value"];
									new_item.put_member(member.first, data);
								}
								else if (path.starts_with("$")) {
									path = path.substr(1);
									json data = jp.create_array();
									if (_src->get_data(data, path)) {
										data = data["value"];
									}
									new_item.put_member(member.first, data);
								}
								else
								{
									new_item.put_member(member.first, member.second);
								}
								json data;
							}
							else 
							{
								new_item.put_member(member.first, member.second);
							}
						}
						stage_output.push_back(new_item);
					}
				}
				else 
				{
					stage_output = jp.create_object();
				}
			}

			execution_time_seconds = tx.get_elapsed_seconds();
			return stage_output; 
		}
	};

	class query_promote : public query_stage {
	public:

		std::string source_name;
		std::string path_name;

		virtual std::string term_name() { return "promote"; }

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "promote"sv);
			_dest.put_member("source", source_name);
			_dest.put_member("path", path_name);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			source_name = _src["source"].as_string();
			path_name = _src["path"].as_string();
			stage_name = _src["name"].as_string();
			if (stage_name.empty()) {
				stage_name = std::format("promote_{0}", source_name);
			}
			stage_output = _src["output"];
		}

		virtual json process(query_context_base* _src) {
			timer tx;
			json_parser jp;

			stage_output = jp.create_array();

			if (source_name.empty()) {
				std::string msg = "Missing source name in query_promote";
				_src->add_error("projection", "source_name", msg, __FILE__, __LINE__);
				comm_bus_app_interface::global_bus->log_warning(msg, __FILE__, __LINE__);
			}
			else
			{
				json projection_source;
				if (_src->get_data(projection_source, source_name)) {
					json process_list = jp.create_array();

					if (projection_source.object())
					{
						process_list.push_back(projection_source);
					}
					else if (projection_source.array())
					{
						process_list = projection_source;
					}

					for (auto item : process_list) {
						json t = item.query(path_name);
						t = t["value"];
						if (t.object()) {
							stage_output.push_back(t);
						}
						else if (t.array())
						{
							for (int i = 0; i < t.size(); i++) {
								json titem = t.get_element(i);
								stage_output.push_back(titem);
							}
						}
					}
				}
			}

			execution_time_seconds = tx.get_elapsed_seconds();
			return stage_output;
		}
	};

	void get_json(json& _dest, std::shared_ptr<query_stage>& _src)
	{
		_src->get_json(_dest);
	}

	void put_json(std::shared_ptr<query_stage>& _dest, json _src)
	{
		std::vector<std::string> missing;

		if (_src.empty())
			return;

		if (not _src.has_members(missing, { "class_name" })) {
			system_monitoring_interface::active_mon->log_warning("query stage missing class_name.");
			if (_src.size()) {
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
			}
			return;
		}

		if (_src.has_member("class_name"))
		{
			std::string class_name = _src["class_name"].as_string();

			if (class_name == "filter")
			{
				_dest = std::make_shared<query_filter>();
				_dest->put_json(_src);
			}
			else if (class_name == "project")
			{
				_dest = std::make_shared<query_project>();
				_dest->put_json(_src);
			}
			else if (class_name == "promote")
			{
				_dest = std::make_shared<query_promote>();
				_dest->put_json(_src);
			}
			else if (class_name == "join")
			{
				_dest = std::make_shared<query_join>();
				_dest->put_json(_src);
			}
			else if (class_name == "result")
			{
				_dest = std::make_shared<query_result>();
				_dest->put_json(_src);
			}
			else {
				std::string msg = std::format("class_name {0} is not a valid query stage.  Use 'filter', 'project', 'promote', 'join', 'replace' or 'result'.",  class_name );
				system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
			}
		}
	}

	void get_json(json& _dest, std::shared_ptr<query_filter>& _src)
	{
		if (_src)
			_src->get_json(_dest);
	}

	void put_json(std::shared_ptr<query_condition>& _dest, json _src)
	{
		std::vector<std::string> missing;

		if (_src.empty())
			return;

		if (not _src.has_members(missing, { "class_name" })) {
			system_monitoring_interface::active_mon->log_warning("query filter missing class_name.");
			if (_src.size()) {
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
			}
			return;
		}

		if (_src.has_member("class_name"))
		{
			std::string class_name = _src["class_name"].as_string();

			if (class_name == "eq")
			{
				_dest = std::make_shared<filter_eq>();
				_dest->put_json(_src);
			}
			else if (class_name == "gt")
			{
				_dest = std::make_shared<filter_gt>();
				_dest->put_json(_src);
			}
			else if (class_name == "gte")
			{
				_dest = std::make_shared<filter_gte>();
				_dest->put_json(_src);
			}
			else if (class_name == "lt")
			{
				_dest = std::make_shared<filter_lt>();
				_dest->put_json(_src);
			}
			else if (class_name == "lte")
			{
				_dest = std::make_shared<filter_lte>();
				_dest->put_json(_src);
			}
			else if (class_name == "any")
			{
				_dest = std::make_shared<filter_any>();
				_dest->put_json(_src);
			}
			else if (class_name == "all")
			{
				_dest = std::make_shared<filter_all>();
				_dest->put_json(_src);
			}
			else if (class_name == "none")
			{
				_dest = std::make_shared<filter_none>();
				_dest->put_json(_src);
			}
			else if (class_name == "contains")
			{
				_dest = std::make_shared<filter_contains>();
				_dest->put_json(_src);
			}
			else if (class_name == "allow_all")
			{
				_dest = std::make_shared<filter_allow_all>();
				_dest->put_json(_src);
            }
            else if (class_name == "between")
            {
                _dest = std::make_shared<filter_between>();
                _dest->put_json(_src);
            }
            else if (class_name == "in")
            {
                _dest = std::make_shared<filter_in>();
                _dest->put_json(_src);
            }
			else {
				std::string msg = std::format("class_name {0} is not a valid query filter.  Use 'eq', 'gt', 'lt', 'gte', 'lte', 'any', 'all', 'none', 'contains', 'between', 'in', 'allow_all'.", class_name);
				system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
			}
		}
	}

}

