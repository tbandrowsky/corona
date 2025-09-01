#pragma once

#ifndef CORONA_EXPRESS_RECORD_HPP
#define CORONA_EXPRESS_RECORD_HPP

/// <summary>
/// 
/// </summary>
namespace corona
{


	const int packed_field_type_size = 1;

	class xcolumn
	{
	public:
		field_types				field_type;
		int32_t					field_id;
		istring<256>			field_name;
	};

	class xtable_columns
	{
	public:
		std::map<int32_t, xcolumn>	columns;
	};

	class xfield
	{
	public:
		int32_t					record_offset;
		int32_t                 size_bytes;
		int32_t                 field_id;
		field_types	            field_type;
	};

	typedef int32_t field_pair_type;
	field_pair_type make_field_pair(field_types fta, field_types ftb)
	{
		return ((int32_t)fta << 8) | (int32_t)ftb;
	}

	static field_types allowed_field_type_operations[65536] = { ft_string };

	void init_field_comparisons()
	{
		allowed_field_type_operations[make_field_pair(field_types::ft_double, field_types::ft_double)] = field_types::ft_double;
		allowed_field_type_operations[make_field_pair(field_types::ft_double, field_types::ft_string)] = field_types::ft_double;
		allowed_field_type_operations[make_field_pair(field_types::ft_double, field_types::ft_int64)] = field_types::ft_int64;

		allowed_field_type_operations[make_field_pair(field_types::ft_int64, field_types::ft_int64)] = field_types::ft_int64;
		allowed_field_type_operations[make_field_pair(field_types::ft_int64, field_types::ft_double)] = field_types::ft_int64;
		allowed_field_type_operations[make_field_pair(field_types::ft_int64, field_types::ft_string)] = field_types::ft_int64;

		allowed_field_type_operations[make_field_pair(field_types::ft_string, field_types::ft_double)] = field_types::ft_double;
		allowed_field_type_operations[make_field_pair(field_types::ft_string, field_types::ft_int64)] = field_types::ft_int64;

		allowed_field_type_operations[make_field_pair(field_types::ft_datetime, field_types::ft_datetime)] = field_types::ft_datetime;
	}

	struct xrecord
	{
	protected:
		std::vector<xfield> field_data;
		std::vector<char> record_data;

		// and field stuff

		int32_t get_last_id()
		{
			int32_t last_id = 0;
			if (field_data.size() == 0)
				return last_id;

			xfield& f = field_data.back();
			return f.field_id;
		}

		int32_t get_end()
		{
			int32_t offset = 0;
			if (field_data.size() == 0)
				return offset;

			xfield& f = field_data.back();
			return f.record_offset + f.size_bytes;
		}

		template<typename T> void add_poco(int32_t _field_id, T _d, field_types _field_type)
		{
			xfield f;
            if (_field_id <= get_last_id())
                throw std::logic_error("field ids must be added in ascending order");
			f.field_id = _field_id;
			f.record_offset = get_end();
			f.size_bytes = sizeof(d);
			f.field_type = _field_type;
			field_data.push_back(f);
			record_data.resize(record_data.size() + f.size_bytes);
			char* I = (char*)&_d;
			std::copy(I, I + _length, record_data.begin() + f.record_offset);
		}

		std::strong_ordering strong_compare(double a, double b) const
		{
			if (std::isnan(a) && std::isnan(b)) return std::strong_ordering::equal;
			if (std::isnan(a)) return std::strong_ordering::less;
			if (std::isnan(b)) return std::strong_ordering::greater;
			if (a < b) return std::strong_ordering::less;
			if (a > b) return std::strong_ordering::greater;
			return std::strong_ordering::equal;
		}

		std::strong_ordering compare_field(int32_t this_field_idx, int32_t that_field_idx, const xrecord& _other) const
		{

			std::strong_ordering comparison;

			auto& this_field = field_data[this_field_idx];
			auto& that_field = _other.field_data[that_field_idx];

			const char* this_data = &record_data[this_field.record_offset];
			const char* that_data = &_other.record_data[that_field.record_offset];

			field_types op_type = allowed_field_type_operations[make_field_pair(this_field.field_type, that_field.field_type)];

			if (op_type == field_types::ft_string)
			{
				std::string stringthis, stringthat;

				if (this_field.field_type == field_types::ft_string)
					stringthis = std::string(this_data, this_field.size_bytes);
				else if (this_field.field_type == field_types::ft_double)
					stringthis = std::to_string(*((double*)this_data));
				else if (this_field.field_type == field_types::ft_int64)
					stringthis = std::to_string(*((int64_t*)this_data));

				if (that_field.field_type == field_types::ft_string)
					stringthat = std::string(that_data, that_field.size_bytes);
				else if (that_field.field_type == field_types::ft_double)
					stringthat = std::to_string(*((double*)that_data));
				else if (that_field.field_type == field_types::ft_int64)
					stringthat = std::to_string(*((int64_t*)that_data));

				comparison = stringthis <=> stringthat;
			}
			else if (op_type == field_types::ft_double)
			{
				double doublethis, doublethat;

				if (this_field.field_type == field_types::ft_double)
					doublethis = *((double*)this_data);
				else if (this_field.field_type == field_types::ft_string)
					doublethis = std::strtod(std::string(this_data, this_field.size_bytes).c_str(), nullptr);
				else if (this_field.field_type == field_types::ft_int64)
					doublethis = (double)(*((int64_t*)this_data));
				else if (this_field.field_type == field_types::ft_bool)
					doublethis = (double)(*((bool*)this_data));
				else if (this_field.field_type == field_types::ft_array)
					doublethis = std::strtod(std::string(this_data, this_field.size_bytes).c_str(), nullptr);
				else if (this_field.field_type == field_types::ft_object)
					doublethis = std::strtod(std::string(this_data, this_field.size_bytes).c_str(), nullptr);

				if (that_field.field_type == field_types::ft_double)
					doublethat = *((double*)that_data);
				else if (that_field.field_type == field_types::ft_string)
					doublethat = std::strtod(std::string(that_data, that_field.size_bytes).c_str(), nullptr);
				else if (that_field.field_type == field_types::ft_int64)
					doublethat = (double)(*((int64_t*)that_data));
				else if (that_field.field_type == field_types::ft_bool)
					doublethat = (double)(*((bool*)that_data));
				else if (that_field.field_type == field_types::ft_array)
					doublethat = std::strtod(std::string(that_data, that_field.size_bytes).c_str(), nullptr);
				else if (that_field.field_type == field_types::ft_object)
					doublethat = std::strtod(std::string(that_data, that_field.size_bytes).c_str(), nullptr);

				comparison = strong_compare(doublethis, doublethat);
			}
			else if (op_type == field_types::ft_int64)
			{
				int64_t int64this, int64that;

				if (this_field.field_type == field_types::ft_int64)
					int64this = *((int64_t*)this_data);
				else if (this_field.field_type == field_types::ft_string)
					int64this = std::strtoll(std::string(this_data, this_field.size_bytes).c_str(), nullptr, 10);
				else if (this_field.field_type == field_types::ft_double)
					int64this = (int64_t)(*((double*)this_data));

				if (that_field.field_type == field_types::ft_int64)
					int64that = *((int64_t*)that_data);
				else if (that_field.field_type == field_types::ft_string)
					int64that = std::strtoll(std::string(that_data, that_field.size_bytes).c_str(), nullptr, 10);
				else if (that_field.field_type == field_types::ft_double)
					int64that = (int64_t)(*((double*)that_data));

				comparison = int64this <=> int64that;
			}
			else if (op_type == field_types::ft_datetime)
			{
				date_time datetimethis, datetimethat;

				if (this_field.field_type == field_types::ft_datetime)
					datetimethis = *((date_time*)this_data);

				if (that_field.field_type == field_types::ft_datetime)
					datetimethat = *((date_time*)that_data);

				comparison = datetimethis <=> datetimethat;
			}
		}

	public:

		xrecord() = default;
		xrecord(const xrecord& _xrecord) = default;
		xrecord(xrecord&& _xrecord) = default;
		xrecord &operator =(const xrecord& _xrecord) = default;
		xrecord &operator =(xrecord&& _xrecord) = default;
		bool operator ==(const xrecord& _other) const = default;

        void add(int32_t _field_id, const char* _data, size_t _length, field_types _field_type)
        {
            xfield f;
			if (_field_id <= get_last_id())
				throw std::logic_error("field ids must be added in ascending order");
			f.field_id = _field_id;
			f.record_offset = get_end();
            f.size_bytes = _length;
            f.field_type = _field_type;
            field_data.push_back(f);
            record_data.resize(record_data.size() + _length);
            std::copy(_data, _data + _length, record_data.begin() + f.record_offset);
        }

		void add(int32_t _field_id, double _d)
		{
            add_poco<double>(_field_id, _d, field_types::ft_double);
		}

		void add(int32_t _field_id, date_time _d)
		{
			add_poco<date_time>(_field_id, _d, field_types::ft_datetime);
		}

		void add(int32_t _field_id, int64_t _d)
		{
			add_poco<int64_t>(_field_id, _d, field_types::ft_int64);
		}

		void add(int32_t _field_id, const std::string& _d)
		{
			add(_field_id, _d.c_str(), _d.size(), field_types::ft_string);
		}

		virtual char* before_read(int32_t _size)  
		{
			char* bytes = new char[_size + 10];
			return bytes;
		}

		virtual void after_read(char* _bytes, int32_t _size) 
		{
			field_data.clear();
			record_data.clear();

			int32_t field_count = *((int *)(_bytes));
            _bytes += sizeof(int32_t);

            int32_t record_size = 0;
            auto pfield_data = (xfield *)_bytes;
			for (auto i = 0; i < field_count; i++) {
                field_data.push_back(*pfield_data);
				record_size += pfield_data->size_bytes;
				pfield_data++;
			}
            _bytes = (char*)pfield_data;

			std::copy(_bytes, _bytes + record_size, record_data);
		}

		virtual char* before_write(int32_t* _size) 
		{
			int32_t header_size = sizeof(int32_t) + sizeof(xfield) * field_data.size();
			int32_t record_size = 0, record_count = 0;

			for (auto& fd : field_data) {
				record_size += fd.size_bytes;
				record_count++;
			}

            char* data = new char[header_size + record_data.size() + 10];	
			char* cdata = data;

            int32_t* count = (int32_t*)data;
            *count = record_count;
            cdata += sizeof(int32_t);
            auto pfield = (xfield*)data;
			for (auto& fd : field_data) {
				*pfield = fd;
				pfield++;
				cdata += sizeof(xfield);
			}
            std::copy(record_data.begin(), record_data.end(), cdata);
			return data;
		}

		virtual void after_write(char* _t) 
		{

		}

		virtual void finished_io(char* _bytes) 
		{
			if (_bytes)
				delete[] _bytes;
		}

		// end implements xblock storable

		void put_json(xtable_columns *_xt, json& _j)
		{
			field_data.clear();
			record_data.clear();

			int index = 0;
			double empty = 0;

			for (auto& ycolumn : _xt->columns)
			{
				auto& acol = ycolumn.second;

                json m = _j[acol.field_name.c_str()];
                if (m.empty())
                    continue;

				switch (acol.field_type)
				{
				case field_types::ft_string:
				{
					std::string s = (std::string)m;
					add(acol.field_id, s);
				}
				break;
				case field_types::ft_array:
				{
					std::string s;
					std::string s = m.to_json_typed();
					add(acol.field_id, s);
				}
				break;
				case field_types::ft_object:
				{
					std::string s;
					std::string s = m.to_json_typed();
					add(acol.field_id, s);
				}
				break;
				case field_types::ft_double:
					{
						double s = (double)m;
						add(acol.field_id, s);
					}
					break;
				case field_types::ft_datetime:
					{
						date_time s = (date_time)m;
						add(acol.field_id, s);
					}
					break;
				case field_types::ft_int64:
					{
						int64_t s = (int64_t)m;
						add(acol.field_id, s);
					}
					break;
				}
			}
		}

		void get_json(xtable_columns* _xt, json& _dest) const
		{
			json_parser jp;

			for (auto& field : field_data)
			{
                auto xcolumnit = _xt->columns.find(field.field_id);
                if (xcolumnit == _xt->columns.end())
                    continue;
				auto acol = xcolumnit->second;
				const char* s = &record_data[field.record_offset];

				switch (acol.field_type)
				{
				case field_types::ft_string:
				{
                    _dest.put_member(acol.field_name.c_str(), std::string(s, field.size_bytes));
				}
				break;
				case field_types::ft_array:
				{
					std::string t = s;
					json result = jp.parse_array(t);
                    _dest.put_member(acol.field_name.c_str(), result);	
				}
				break;
				case field_types::ft_object:
				{
					std::string t = s;
					json result = jp.parse_object(t);
					_dest.put_member(acol.field_name.c_str(), result);
				}
				break;
				case field_types::ft_double:
				{
					double t = *((double *)s);
					_dest.put_member(acol.field_name.c_str(), t);
				}
				break;
				case field_types::ft_datetime:
				{
					date_time t = *((date_time*)s);
					_dest.put_member(acol.field_name.c_str(), t);
				}
				break;
				case field_types::ft_int64:
				{
					int64_t t = *((int64_t*)s);
					_dest.put_member(acol.field_name.c_str(), t);
				}
				break;
				}
			}
		}

		std::string to_string() const
		{
			std::string temp;
			return temp;
		}


        std::strong_ordering operator<=>(const xrecord& _other) const
        {
			int32_t this_idx = 0;
			int32_t that_idx = 0;
			int32_t comparison_count = 0;

			while (this_idx < field_data.size() && that_idx < _other.field_data.size())
			{
				int32_t this_field_id = field_data[this_idx].field_id;
				int32_t that_field_id = _other.field_data[that_idx].field_id;

				if (this_field_id == that_field_id) {
					auto ordering = compare_field(this_idx, that_idx, _other);
					if (ordering != std::strong_ordering::equal)
						return ordering;
					that_idx++;
					this_idx++;
					comparison_count++;
                }
                else if (this_field_id < that_field_id) {
                    this_idx++;
                }
				else if (this_field_id > that_field_id) {
					that_idx++;
				}
			}

			std::strong_ordering ordering;

            if (comparison_count == _other.field_data.size() or comparison_count == field_data.size())
				ordering = std::strong_ordering::equal;
			else if (_other.field_data.size() > field_data.size())
				ordering = std::strong_ordering::greater;
			else 
				ordering = std::strong_ordering::less;
			return ordering;
        }

		void clear()
		{
            field_data.clear();
            record_data.clear();
		}
	};


	void test_xrecord(std::shared_ptr<test_set> _tests, std::shared_ptr<application> _app)
	{
		timer tx;
		date_time start = date_time::now();

		system_monitoring_interface::active_mon->log_function_start("xrecord", "start", start, __FILE__, __LINE__);
		using namespace std::literals;

		xrecord comp1, comp2, comp3, test_in, test_out;

		init_field_comparisons();

		json_parser jp;
        json test_obj = jp.create_object();
        json test_inner_obj = jp.create_object();
		json test_inner_array = jp.create_array();
		test_inner_obj.put_member("string_value", std::string("hello"));
		test_inner_obj.put_member("int64_value", 42i64);
		test_inner_array.push_back(std::string("one"));
		test_inner_array.push_back(2i64);
		test_inner_array.push_back(3.0);
		test_obj.put_member("obj_value", test_inner_obj);
		test_obj.put_member("arr_value", test_inner_array);
		test_obj.put_member("amount", 4.0);
		test_obj.put_member("prompt", std::string("hello"));
        test_obj.put_member("object_id", 42i64);
		
		xtable_columns columns;
        columns.columns[1] = { field_types::ft_double, 1, "amount" };
        columns.columns[2] = { field_types::ft_string, 2, "prompt" };
        columns.columns[3] = { field_types::ft_int64, 3, "object_id" };
        columns.columns[4] = { field_types::ft_object, 4, "obj_value" };
        columns.columns[5] = { field_types::ft_array, 5, "arr_value" };

		test_in.put_json(&columns, test_obj);

		json jtest_out = jp.create_object();
		test_in.get_json(&columns, jtest_out);

		comp1.add(1, 4.0);
		comp1.add(2, "hello");
		comp1.add(3, 42i64);

		comp2.add(1, 4.0);
		comp2.add(2, "hello");
		comp2.add(3, 42i64);

		bool result;
		result = comp1 == comp2;
		_tests->test({ "xr =", result, __FILE__, __LINE__ });

		comp3.add(1, 2.0);
		result = comp3 < comp1;
		_tests->test({ "xr <", result, __FILE__, __LINE__ });

		result = not (comp1 < comp3);
		_tests->test({ "xr < swo", result, __FILE__, __LINE__ });

		result = comp1 > comp3;
		_tests->test({ "xr >", result, __FILE__, __LINE__ });

		// partial keys are equal.
		comp3.clear();
		comp3.add(1, 4.0);
		result = comp3 == comp1;
		_tests->test({ "xr == key 1", result, __FILE__, __LINE__ });

		comp3.add(2, "hello");
		result = comp3 == comp1;
		_tests->test({ "xr == key 2", result, __FILE__, __LINE__ });

		comp3.add(3, 42i64);
		result = comp3 == comp1;
		_tests->test({ "xr == key 3", result, __FILE__, __LINE__ });

		comp3.clear();
		comp3.add(1, 2.0);
		result = comp3 < comp1;
		_tests->test({ "xr < key 1", result, __FILE__, __LINE__ });

		comp3.add(2, "hello");
		result = comp3 < comp1;
		_tests->test({ "xr < key 2", result, __FILE__, __LINE__ });

		result = comp1 >= comp3;
		_tests->test({ "xr >= key 3", result, __FILE__, __LINE__ });

		result = not (comp1 < comp3);
		_tests->test({ "xr < key 3, swo", result, __FILE__, __LINE__ });

		comp3.add(3, 42i64);
		result = comp3 < comp1;
		_tests->test({ "xr < key 3", result, __FILE__, __LINE__ });

		result = not (comp1 < comp3);
		_tests->test({ "xr < key 3, swo", result, __FILE__, __LINE__ });

		// implied equality
		comp3.clear();
		comp3.add(1, 4.0);
		result = comp3 < comp1;
		_tests->test({ "xr < key 2.1", result, __FILE__, __LINE__ });

		result = not (comp1 < comp3);
		_tests->test({ "xr < key 2.1 swo", result, __FILE__, __LINE__ });


		comp3.add(2, "hello");
		result = comp3 < comp1;
		_tests->test({ "xr < key 2.2", result, __FILE__, __LINE__ });

		result = not (comp1 < comp3);
		_tests->test({ "xr < key 2.2 swo", result, __FILE__, __LINE__ });


		comp3.add(3, 43i64);
		result = not(comp3 < comp1);
		_tests->test({ "xr < key 2.3", result, __FILE__, __LINE__ });

		result = (comp1 < comp3);
		_tests->test({ "xr > key 2.3 swo", result, __FILE__, __LINE__ });


		comp3.clear();
		comp3.add(1, 4.0);
		comp3.add(2, "ahello");
		result = comp3 < comp1;
		_tests->test({ "xr < key 3.1", result, __FILE__, __LINE__ });
		result = not(comp1 < comp3);
		_tests->test({ "xr > key 3.1", result, __FILE__, __LINE__ });

		// keys, more partial tests

		// partial keys are equal.
		comp3.clear();
		comp3.add(1, 4.0);
		result = comp3 == comp1;
		_tests->test({ "xr == key 1", result, __FILE__, __LINE__ });

		comp3.add(2, "hello");
		result = comp3 == comp1;
		_tests->test({ "xr == key 2", result, __FILE__, __LINE__ });

		comp3.add(3, 42i64);
		result = comp3 == comp1;
		_tests->test({ "xr == key 3", result, __FILE__, __LINE__ });

		comp3.clear();
		comp3.add(1, 2.0);
		result = not (comp3 == comp1);
		_tests->test({ "xr == key 1", result, __FILE__, __LINE__ });

		comp3.add(2, "hello");
		result = not (comp3 == comp1);
		_tests->test({ "xr == key 2", result, __FILE__, __LINE__ });

		comp3.add(3, 42i64);
		result = not (comp3 == comp1);
		_tests->test({ "xr == key 3", result, __FILE__, __LINE__ });

		// and now json tests.
		json jsrc = jp.create_object();
		jsrc.put_member("Name", "Bill"sv);
		jsrc.put_member("Age", "41"sv);
		jsrc.put_member_i64("Atoms", 124);
		jsrc.put_member("Today", date_time::now());

		xtable_columns columns2;
		columns2.columns[1] = { field_types::ft_string, 1, "Name" };
		columns2.columns[2] = { field_types::ft_string, 2, "Age" };
		columns2.columns[3] = { field_types::ft_int64, 3, "Atoms" };
		columns2.columns[4] = { field_types::ft_datetime, 4, "Today" };

		xrecord compj;
		json jdst = jp.create_object();
		compj.get_json(&columns2, jdst);

		result = (std::string)jsrc["Name"] == (std::string)jdst["Name"];
		_tests->test({ "rt name", result, __FILE__, __LINE__ });

		result = (double)jsrc["Age"] == (double)jdst["Age"];
		_tests->test({ "rt age", result, __FILE__, __LINE__ });

		result = (int64_t)jsrc["Atoms"] == (int64_t)jdst["Atoms"];
		_tests->test({ "rt atoms", result, __FILE__, __LINE__ });

		result = (date_time)jsrc["Today"] == (date_time)jdst["Today"];
		_tests->test({ "rt today", result, __FILE__, __LINE__ });

		// and with the json
		jdst = jp.create_object();
		xrecord xsrc(keys, jsrc);
		xrecord readin = xrecord(xsrc.data(), xsrc.size());
		readin.get_json(jdst, keys);

		// and finally, checking our matches after the round trip
		result = (std::string)jsrc["Name"] == (std::string)jdst["Name"];
		_tests->test({ "rts name", result, __FILE__, __LINE__ });

		result = (double)jsrc["Age"] == (double)jdst["Age"];
		_tests->test({ "rts age", result, __FILE__, __LINE__ });

		result = (_int64)jsrc["Atoms"] == (_int64)jdst["Atoms"];
		_tests->test({ "rts atoms", result, __FILE__, __LINE__ });

		result = (date_time)jsrc["Today"] == (date_time)jdst["Today"];
		_tests->test({ "rts today", result, __FILE__, __LINE__ });

		// and now, unit tests that simulate our indexes.
		comp1.clear();
		comp1.add(1, "Roger");
		comp1.add(2, 13i64);

		comp2.clear();
		comp2.add(1, "Sam");
		comp2.add(2, 14i64);

		comp3.clear();
		comp3.add(1, "Xavier");
		comp3.add(2, 15i64);

		result = comp1 < comp2;
		_tests->test({ "index <", result, __FILE__, __LINE__ });

		result = not (comp2 < comp1);
		_tests->test({ "index < swo", result, __FILE__, __LINE__ });

		result = comp2 < comp3;
		_tests->test({ "index 2 <", result, __FILE__, __LINE__ });

		result = not (comp3 < comp2);
		_tests->test({ "index 2 < swo", result, __FILE__, __LINE__ });

		result = comp1 < comp3;
		_tests->test({ "index 3 <", result, __FILE__, __LINE__ });

		result = not (comp3 < comp1);
		_tests->test({ "index 3 < swo", result, __FILE__, __LINE__ });

		system_monitoring_interface::active_mon->log_function_stop("xrecord", "complete", tx.get_elapsed_seconds(), __FILE__, __LINE__);
	}

}

#endif
