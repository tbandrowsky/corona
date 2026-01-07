#pragma once


namespace corona
{
	class validation_error
	{
	public:
		std::string class_name;
		std::string field_name;
		std::string message;
		std::string filename;
		int			line_number;
		std::string comments;
		int			count;

		validation_error()
		{
			count = 1;
		}

		validation_error(const validation_error& _src) = default;
		validation_error(validation_error&& _src) = default;
		validation_error& operator = (const validation_error& _src) = default;
		validation_error& operator = (validation_error&& _src) = default;

		validation_error(std::string _field_name, std::string _message, std::string _filename, int _line_number)
		{
			count = 1;
			class_name = "N/A";
			field_name = _field_name;
			message = _message;
			filename = _filename;
			line_number = _line_number;
		}

		virtual void get_json(json& _dest) const
		{
			_dest.put_member(class_name_field, class_name);
			_dest.put_member("field_name", field_name);
			_dest.put_member(message_field, message);
			_dest.put_member("filename", filename);
			_dest.put_member("line_number", line_number);
			_dest.put_member("comments", line_number);
			_dest.put_member("count", count);
		}

		virtual void put_json(json& _src)
		{
			class_name = _src[class_name_field].as_string();
			field_name = _src["field_name"].as_string();
			message = _src[message_field].as_string();
			filename = _src["filename"].as_string();
			line_number = _src["line_number"].as_int();
			comments = _src["comments"].as_string();
			count = _src["count"].as_int();
		}

		bool operator < (const validation_error& _src) const
		{
			return std::tie(class_name, field_name, filename, line_number) < std::tie(_src.class_name, _src.field_name, _src.filename, _src.line_number);
		}
	};

	class validation_error_collection
	{
		std::set<validation_error> errors;

	public:
		validation_error_collection() = default;

		void push_back(std::string _field_name, std::string _message, std::string _filename, int _line_number)
		{
			validation_error ve(_field_name, _message, _filename, _line_number);
			auto it = errors.find(ve);
			if (it != errors.end())
			{
				validation_error existing = *it;
				existing.count += ve.count;
				errors.erase(it);
				errors.insert(existing);
			}
			else
			{
				ve.count = 1;
				errors.insert(ve);
			}
		}

		void push_back(const validation_error& _ve)
		{
			auto it = errors.find(_ve);
			if (it != errors.end())
			{
				validation_error existing = *it;
				existing.count += _ve.count;
				errors.erase(it);
				errors.insert(existing);
			}
			else
			{
				errors.insert(_ve);
			}
		}

		auto begin() { return errors.begin(); }
		auto end() { return errors.end(); }
		auto size() { return errors.size(); }

        void clear() { errors.clear(); }
	};

}
