#pragma once

namespace corona
{
	class chart_specification : public json_serializable
	{

	public:

		std::string class_name;
		std::string title;
		std::string description;

        virtual void get_json(json& _dest)
        {
            json_parser jp;
            json j = jp.create_object();
            _dest.put_member("class_name", class_name);
			_dest.put_member("title", title);
            _dest.put_member("description", description);
        }

		virtual void put_json(json& _src)
		{
            class_name = _src["class_name"].as_string();
			title = _src["title"].as_string();
            description = _src["description"].as_string();
		}
	};

	class chart_series
	{
	public:
		std::string field_name;
		std::string units;
        generalBrushRequest fill;
        generalBrushRequest stroke;
        double stroke_width = 1.0;

        void get_json(json& _dest)
        {
            json_parser jp;
            _dest.put_member("field_name", field_name);
            _dest.put_member("units", units);
			json jfill = jp.create_object();
			fill.get_json(jfill);
			_dest.put_member("fill", jfill);
			json jstroke = jp.create_object();
			stroke.get_json(jstroke);
			_dest.put_member("stroke", jstroke);
			_dest.put_member("stroke_width", stroke_width);
        }

        void put_json(json& _src)
        {
            field_name = _src["field_name"].as_string();
            units = _src["units"].as_string();
			json jfill = _src["fill"];
			fill.put_json(jfill);
			json jstroke = _src["stroke"];
			stroke.put_json(jstroke);
			stroke_width = _src["stroke_width"].as_double();
			if (!*fill.get_name()) {
				fill.set_name("chart_series_" + field_name + "_" + units);
			}
			if (!*stroke.get_name()) {
				stroke.set_name("chart_series_" + field_name + "_" + units);
			}

        }
	};

	class chart_xy_series
	{
	public:
		std::string xfield_name;
		std::string yfield_name;
		std::string x_units;
		std::string y_units;
		generalBrushRequest fill;
		generalBrushRequest stroke;
		double stroke_width = 1.0;

		void get_json(json& _dest)
		{
			json_parser jp;
			_dest.put_member("xfield_name", xfield_name);
			_dest.put_member("yfield_name", yfield_name);
			_dest.put_member("x_units", x_units);
			_dest.put_member("y_units", y_units);
			json jfill = jp.create_object();
			fill.get_json(jfill);
			_dest.put_member("fill", jfill);
			json jstroke = jp.create_object();
			stroke.get_json(jstroke);
			_dest.put_member("stroke", jstroke);
			_dest.put_member("stroke_width", stroke_width);
		}

		void put_json(json& _src)
		{
			xfield_name = _src["xfield_name"].as_string();
			yfield_name = _src["yfield_name"].as_string();
			x_units = _src["x_units"].as_string();
			y_units = _src["y_units"].as_string();
			json jfill = _src["fill"];
			fill.put_json(jfill);
			json jstroke = _src["stroke"];
			stroke.put_json(jstroke);
			stroke_width = _src["stroke_width"].as_double();
			if (!*fill.get_name()) {
				fill.set_name("chart_series_" + xfield_name + "_" + yfield_name + "_" + x_units + "_" + y_units);
			}
			if (!*stroke.get_name()) {
				stroke.set_name("chart_series_" + xfield_name + "_" + yfield_name + "_" + x_units + "_" + y_units);
			}

		}
	};

	class time_chart_specification : public chart_specification
	{
	public:
		chart_series				time_series;
		std::vector<chart_series>	values_fields;

        void get_json(json& _dest)
        {
            json_parser jp;
            chart_specification::get_json(_dest);
            json jtime = jp.create_object();
            time_series.put_json(jtime);
            _dest.put_member("time_series", jtime);
            json jvalues = jp.create_array();
            for (auto fld : values_fields) {
                json jfld = jp.create_object();
                fld.put_json(jfld);
                jvalues.push_back(jfld);
            }
            _dest.put_member("series", jvalues);
        }

        void put_json(json& _src)
        {
            json jtime = _src["time_series"];
            time_series.put_json(jtime);
            json jvalues = _src["series"];
            for (auto jfld : jvalues) {
                chart_series fld;
                fld.put_json(jfld);
                values_fields.push_back(fld);
            }
        }
	};

	class xy_chart_specification : public chart_specification
	{
	public:		
		
		std::vector<chart_xy_series> values_fields;

        void get_json(json& _dest)
        {
            json_parser jp;
            chart_specification::get_json(_dest);
            json jvalues = jp.create_array();
            for (auto fld : values_fields) {
                json jfld = jp.create_object();
                fld.get_json(jfld);
                jvalues.push_back(jfld);
            }
            _dest.put_member("series", jvalues);
        }

        void put_json(json& _src)
        {
            json jvalues = _src["series"];
            for (auto jfld : jvalues) {
                chart_xy_series fld;
                fld.put_json(jfld);
                values_fields.push_back(fld);
            }
        }

	};

	class bar_chart_specification : public chart_specification
	{
	public:
		std::vector<chart_series> value_fields;

        void get_json(json& _dest)
        {
            json_parser jp;
            chart_specification::get_json(_dest);
            json jvalues = jp.create_array();
            for (auto fld : value_fields) {
                json jfld = jp.create_object();
                fld.put_json(jfld);
                jvalues.push_back(jfld);
            }
            _dest.put_member("series", jvalues);
        }

        void put_json(json& _src)
        {
            json jvalues = _src["series"];
            for (auto jfld : jvalues) {
                chart_series fld;
                fld.put_json(jfld);
                value_fields.push_back(fld);
            }
        }
	};

	class pie_chart_specification : public chart_specification
	{
	public:
		std::vector<chart_series> value_fields;

		void get_json(json& _dest)
		{
			json_parser jp;
			chart_specification::get_json(_dest);
			json jvalues = jp.create_array();
			for (auto fld : value_fields) {
				json jfld = jp.create_object();
				fld.put_json(jfld);
				jvalues.push_back(jfld);
			}
			_dest.put_member("series", jvalues);
		}

		void put_json(json& _src)
		{
			json jvalues = _src["series"];
			for (auto jfld : jvalues) {
				chart_series fld;
				fld.put_json(jfld);
				value_fields.push_back(fld);
			}
		}
	};

    class program_chart_specification : public chart_specification
	{
	public:
		std::string				 attachment_point_field;
		std::string				 limit_field;
		std::string				 deductible_field;
		std::string				 blanket_deductible_field;
        std::string              policy_field;
        std::string              coverage_field;
		std::vector<std::string> y_value_fields;
	};

	class unit_frame {
	public:
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
		double sum = 0.0;
        double count = 0.0;

        unit_frame() { ; }
        unit_frame	(double _min, double _max) : min(_min), max(_max) { ; }
		
        double accumulate(double _value)
        {
            if (_value < min) {
                min = _value;
            }
            if (_value > max) {
                max = _value;
            }
            sum += _value;
            count += 1.0;
            return sum;
        }

        double scale(double _value, double _scale_min, double _scale_max)
        {
            if (max == min) {
                return (_scale_min + _scale_max) / 2.0;
            }
            double scale = (_value - min) / (max - min);
            return _scale_min + scale * (_scale_max - _scale_min);
        }
	};

	class axis_frame {
	public:
		unit_frame units;
		rectangle  bounds;
		std::vector<double> ticks;
	};

	class chart_frame {
	public:
		rectangle					ux_bounds;
		rectangle					chart_bounds;
		
        textStyleRequest			axis_text;
		generalBrushRequest			axis_major;
		generalBrushRequest		    axis_minor;

		const double y_axis_width = 100;
		const double x_axis_height = 75;

		chart_frame()
		{
			axis_major.setColor("#000000");
            axis_major.set_name("axis_major");
			axis_minor.setColor("#404040");
			axis_minor.set_name("axis_minor");
			axis_text.name = "Axis Text";
            axis_text.fontName = "Arial";
			axis_text.fontSize = 12;
		}

		virtual void draw_chart(std::shared_ptr<direct2dContext>& _context)
		{
			;
		}


	protected:

		virtual void create_assets(std::shared_ptr<direct2dContext>& _context)
		{
			_context->setBrush(&axis_major);
			_context->setBrush(&axis_minor);
			_context->setTextStyle(&axis_text);
		}

		virtual void draw_x_axis(std::shared_ptr<direct2dContext>& _context, axis_frame& _axis)
		{
			point start, stop;
            start.x = _axis.bounds.x;
            start.y = _axis.bounds.y;
            stop.x = _axis.bounds.right();
            stop.y = _axis.bounds.y;
			_context->drawLine(&start, &stop, axis_major.get_name(), 2);
			std::set<int> tick_positions;

            for (auto i = 0; i < _axis.ticks.size(); i++) {
                double tick_value = _axis.ticks[i];
                double tick_position = _axis.units.scale(tick_value, _axis.bounds.x, _axis.bounds.right());
				start.x = tick_position;
				start.y = chart_bounds.y;
				stop.x = tick_position;
				stop.y = _axis.bounds.bottom();
				_context->drawLine(&start, &stop, axis_minor.get_name(), 1);
				int tick_pos = static_cast<int>(tick_position / 150);
				if (tick_positions.find(tick_pos) == tick_positions.end()) 
				{
					tick_positions.insert(tick_pos);
                    rectangle text_bounds;
                    text_bounds.x = tick_position - 8;
                    text_bounds.y = _axis.bounds.y + 5;
					text_bounds.w = 125;
					text_bounds.h = _axis.bounds.h - 5;
                    _context->drawText(std::to_string(tick_value), &text_bounds, axis_text.name, axis_minor.get_name(), "");
				}
            }
		}

        virtual void draw_x_axis_time(std::shared_ptr<direct2dContext>& _context, axis_frame& _axis)
        {
			point start, stop;
			start.x = _axis.bounds.x;
			start.y = _axis.bounds.y;
			stop.x = _axis.bounds.right();
			stop.y = _axis.bounds.y;
			_context->drawLine(&start, &stop, axis_major.get_name(), 2);
			std::set<int> tick_positions;

			for (auto i = 0; i < _axis.ticks.size(); i++) {
				double tick_value = _axis.ticks[i];
				double tick_position = _axis.units.scale(tick_value, _axis.bounds.x, _axis.bounds.right());
				start.x = tick_position;
				start.y = chart_bounds.y;
				stop.x = tick_position;
				stop.y = _axis.bounds.bottom();
				_context->drawLine(&start, &stop, axis_minor.get_name(), 1);
				int tick_pos = static_cast<int>(tick_position / 150);
				if (tick_positions.find(tick_pos) == tick_positions.end())
				{
					tick_positions.insert(tick_pos);
					rectangle text_bounds;
					text_bounds.x = tick_position - 8;
					text_bounds.y = _axis.bounds.y + 5;
					text_bounds.w = 125;
					text_bounds.h = _axis.bounds.h - 5;
					date_time dt = date_time(tick_value);
                    std::string tick_label = dt.format("%Y-%m-%d");
					_context->drawText(tick_label, &text_bounds, axis_text.name, axis_minor.get_name(), "");
				}
			}
        }

        virtual void draw_y_axis(std::shared_ptr<direct2dContext>& _context, axis_frame& _axis)
        {
			point start, stop;
			start.x = _axis.bounds.right();
			start.y = _axis.bounds.y;
			stop.x = _axis.bounds.right();
			stop.y = _axis.bounds.bottom();
			_context->drawLine(&start, &stop, axis_major.get_name(), 2);

			std::set<int> tick_positions;

			for (auto i = 0; i < _axis.ticks.size(); i++) {
				double tick_value = _axis.ticks[i];
				double tick_position = _axis.units.scale(tick_value, _axis.bounds.y, _axis.bounds.bottom());
				start.x = _axis.bounds.x;
				start.y = tick_position;
				stop.x = _axis.bounds.right();
				stop.y = tick_position;
				_context->drawLine(&start, &stop, axis_minor.get_name(), 1);
				int tick_pos = static_cast<int>(tick_position / 30);
				if (tick_positions.find(tick_pos) == tick_positions.end())
				{
					tick_positions.insert(tick_pos);
					rectangle text_bounds;
					text_bounds.x = _axis.bounds.x;
					text_bounds.y = tick_position - 8;
					text_bounds.w = _axis.bounds.w;
					text_bounds.h = 20;
					_context->drawText(std::to_string(tick_value), &text_bounds, axis_text.name, axis_minor.get_name(), "");
				}
			}
		}

	};

	class program_chart_frame : public chart_frame {
	public:
		program_chart_specification specs;
	};

	class time_chart_frame : public chart_frame {
	public:
		time_chart_specification specs;
		std::map<std::string, unit_frame> x_units;
		std::map<std::string, unit_frame> y_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	lines;
		std::map<std::string, axis_frame>		y_axes;
		axis_frame					x_axes;
		rectangle					chart_bounds;

		time_chart_frame() = default;
		time_chart_frame(const time_chart_frame& _src) = default;
		time_chart_frame(time_chart_frame&& _src) = default;
		time_chart_frame(std::shared_ptr<direct2dContext>& _context, json_array& slice_array, time_chart_specification& _spec, rectangle* _ctx)
		{
			ux_bounds = *_ctx;
			specs = _spec;
			x_units[_spec.time_series.units] = unit_frame();
			auto& f2 = x_units[_spec.time_series.units];

			for (auto fld : _spec.values_fields) {
				std::string field_name = fld.field_name;
				std::string units = fld.units;
				std::shared_ptr<game::vector_frame> line_frame = std::make_shared<game::vector_frame>();
				line_frame->fill = fld.fill;
				line_frame->stroke = fld.stroke;
				line_frame->stroke_width = fld.stroke_width;
				y_units[units] = unit_frame();
				lines[field_name] = line_frame;
			}

			for (auto item : slice_array) {
				json jitem = item;
				double time_value = jitem[_spec.time_series.field_name].as_double();
				f2.accumulate(time_value);
				for (auto fld : _spec.values_fields) {
					std::string field_name = fld.field_name;
					double y_value = jitem[field_name].as_double();
					auto& f = y_units[fld.units];
					f.accumulate(y_value);
				}
			}

			chart_bounds.x = _ctx->x + y_axis_width * y_units.size();
			chart_bounds.y = _ctx->y;
			chart_bounds.h = _ctx->h - x_axis_height;
			chart_bounds.w = _ctx->right() - chart_bounds.x;
			x_axes.bounds.x = chart_bounds.x;
			x_axes.bounds.w = chart_bounds.w;
			x_axes.bounds.y = chart_bounds.bottom();
			x_axes.bounds.h = x_axis_height;

			int index = 0;
			for (auto& unit : y_units) {
				axis_frame axis;
				axis.units = unit.second;
				axis.bounds.x = _ctx->x + y_axis_width * index;
				axis.bounds.w = y_axis_width;
				axis.bounds.y = _ctx->y;
				axis.bounds.h = _ctx->h - x_axis_height;
				y_axes[unit.first] = axis;
				index++;
			}

			for (auto item : slice_array) {
				json jitem = item;
				double time_value = jitem[_spec.time_series.field_name].as_double();
                x_axes.ticks.push_back(time_value);
				for (auto fld : _spec.values_fields) {
					std::string field_name = fld.field_name;
					double y_value = jitem[field_name].as_double();
					auto& y_unit = y_units[fld.units];
                    auto& y_axis = y_axes[fld.units];
					y_axis.ticks.push_back(y_value);
					auto& x_unit = x_units[_spec.time_series.units];
					auto& line_frame = lines[field_name];
					double y_scaled = chart_bounds.bottom() - y_unit.scale(y_value, chart_bounds.y, chart_bounds.bottom());
					double x_scaled = x_unit.scale(time_value, chart_bounds.x, chart_bounds.right());
					line_frame->path.addLineTo(x_scaled, y_scaled);
				}
			}
		}

		virtual void draw_chart(std::shared_ptr<direct2dContext>& _context)
		{
			draw_x_axis_time(_context, x_axes);
			for (auto& y_axis_pair : y_axes) {
				draw_y_axis(_context, y_axis_pair.second);
			}
			for (auto& line : this->lines) 
			{				
				line.second->draw_item(*_context.get());
			}
		}

	};

	class xy_chart_frame : public chart_frame {
	public:
		xy_chart_specification specs;
		std::map<std::string, unit_frame> x_units;
		std::map<std::string, unit_frame> y_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	lines;
		std::map<std::string, axis_frame>		y_axes;
		std::map<std::string, axis_frame>		x_axes;

		xy_chart_frame() = default;
		xy_chart_frame(const xy_chart_frame& _src) = default;
		xy_chart_frame(xy_chart_frame&& _src) = default;
		xy_chart_frame(std::shared_ptr<direct2dContext>& _context, json_array& slice_array, xy_chart_specification& _spec, rectangle* _ctx)
		{
			specs = _spec;
			ux_bounds = *_ctx;

			for (auto fld : _spec.values_fields) {
				std::string xname = fld.xfield_name;
				std::string yname = fld.yfield_name;
				x_units[fld.x_units] = unit_frame();
				y_units[fld.y_units] = unit_frame();

				std::shared_ptr<game::vector_frame> line_frame = std::make_shared<game::vector_frame>();
				std::string line_name = fld.xfield_name + "_" + fld.yfield_name;
				lines[line_name] = line_frame;
				line_frame->fill = fld.fill;
				line_frame->stroke = fld.stroke;
				line_frame->stroke_width = fld.stroke_width;
			}

			chart_bounds.x = _ctx->x + y_axis_width * y_units.size();
			chart_bounds.y = _ctx->y;
			chart_bounds.h = _ctx->h - x_axis_height * x_units.size();
			chart_bounds.w = _ctx->right() - chart_bounds.x;

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.values_fields) {
					double x = jitem[fld.xfield_name].as_double();
					double y = jitem[fld.yfield_name].as_double();
					auto& f = x_units[fld.x_units];
					f.accumulate(x);
					auto& f2 = y_units[fld.y_units];
					f2.accumulate(y);
				}
			}

			int index = 0;
			for (auto& unit : y_units) {
				axis_frame axis;
				axis.units = unit.second;
				axis.bounds.x = _ctx->x + y_axis_width * index;
				axis.bounds.w = y_axis_width;
				axis.bounds.y = _ctx->y;
				axis.bounds.h = _ctx->h - x_axis_height;
				y_axes.insert({unit.first, axis});
				index++;
			}

			index = 0;
			for (auto& unit : x_units) {
				axis_frame axis;
				axis.units = unit.second;
				axis.bounds.x = chart_bounds.x;
				axis.bounds.w = chart_bounds.w;
				axis.bounds.y = chart_bounds.bottom() + y_axis_width * index;
				axis.bounds.h = x_axis_height;
				x_axes.insert({unit.first, axis});
				index++;
			}

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.values_fields) {
					double x = jitem[fld.xfield_name].as_double();
					double y = jitem[fld.yfield_name].as_double();
                    auto& xa = x_axes[fld.x_units];
                    auto& ya = y_axes[fld.y_units];
					xa.ticks.push_back(x);
					ya.ticks.push_back(y);
					std::string line_name = fld.xfield_name + "_" + fld.yfield_name;
					auto& line_frame = lines[line_name];
					double x_scaled = xa.units.scale(x, chart_bounds.x, chart_bounds.right());
					double y_scaled = chart_bounds.bottom() - ya.units.scale(y, chart_bounds.y, chart_bounds.bottom());
					line_frame->path.addLineTo(x_scaled, y_scaled);
				}
			}
		}

		virtual void draw_chart(std::shared_ptr<direct2dContext>& _context)
		{
			for (auto& x_axis_pair : x_axes) {
				draw_x_axis(_context, x_axis_pair.second);
			}
			for (auto& y_axis_pair : y_axes) {
				draw_y_axis(_context, y_axis_pair.second);
			}
			for (auto& line : this->lines)
			{
				line.second->draw_item(*_context.get());
			}
		}

	};


	class pie_chart_frame : public chart_frame {
	public:
		pie_chart_specification specs;
        unit_frame				pie_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	slices;

		pie_chart_frame() = default;
		pie_chart_frame(const pie_chart_frame& _src) = default;
		pie_chart_frame(pie_chart_frame&& _src) = default;
		pie_chart_frame(std::shared_ptr<direct2dContext>& _context, json_array& slice_array, pie_chart_specification& _spec, rectangle* _ctx)
		{
			std::shared_ptr<pie_chart_frame> result = std::make_shared<pie_chart_frame>();

			specs = _spec;
			ux_bounds = *_ctx;

			for (auto fld : _spec.value_fields) {
				std::string name = fld.field_name;
				std::shared_ptr<game::vector_frame> pie_frame = std::make_shared<game::vector_frame>();
				std::string slice_name = fld.field_name;
				slices[slice_name] = pie_frame;
				pie_frame->fill = fld.fill;
				pie_frame->stroke = fld.stroke;
				pie_frame->stroke_width = fld.stroke_width;
			}

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.value_fields) {
					double x = jitem[fld.field_name].as_double();
					auto& f = pie_units;
					f.accumulate(x);
				}
			}

			double xradius = _ctx->w / 2.0;
			double yradius = _ctx->h / 2.0;
			double radius = std::min(xradius, yradius);

			point center = rectangle_math::center(chart_bounds);

			for (auto item : slice_array) {
				json jitem = item;
				double start_angle = 0.0;
				for (auto fld : _spec.value_fields) {
					double x = jitem[fld.field_name].as_double();
					auto& f = pie_units;
					double angle = (x / f.sum) * 360.0;
					auto& slice_frame = slices[fld.field_name];
					slice_frame->path.addLineTo(center.x, center.y);
					slice_frame->path.addPathArc(center.x + cos(start_angle) * radius, center.y + sin(start_angle) * radius, radius, radius, start_angle);
					slice_frame->path.addLineTo(center.x, center.y);
					start_angle += angle;
				}
			}

		}

		virtual void draw_chart(std::shared_ptr<direct2dContext>& _context)
		{
			for (auto& slice : this->slices)
			{
				slice.second->draw_item(*_context.get());
			}
		}
	};

	class bar_chart_frame : public chart_frame {
	public:
		bar_chart_specification specs;
		unit_frame				bar_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	bars;
		std::map<std::string, axis_frame>		y_axes;

		bar_chart_frame() = default;
        bar_chart_frame(const bar_chart_frame& _src) = default;
        bar_chart_frame(bar_chart_frame&& _src) = default;
		bar_chart_frame(std::shared_ptr<direct2dContext>& _context, json_array& slice_array, bar_chart_specification& _spec, rectangle* _ctx)
		{
			std::shared_ptr<bar_chart_frame> result = std::make_shared<bar_chart_frame>();

			result->specs = _spec;
			result->ux_bounds = *_ctx;

			double bar_width = (_ctx->w - y_axis_width) / slice_array.size();

			for (auto fld : _spec.value_fields) {
				std::string name = fld.field_name;
				std::shared_ptr<game::vector_frame> bar_frame = std::make_shared<game::vector_frame>();
				std::string bar_name = fld.field_name;
				result->bars[bar_name] = bar_frame;
				bar_frame->fill = fld.fill;
				bar_frame->stroke = fld.stroke;
				bar_frame->stroke_width = fld.stroke_width;
			}

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.value_fields) {
					double x = jitem[fld.field_name].as_double();
					auto& f = result->bar_units;
					f.accumulate(x);
				}
			}

			axis_frame axis;
			axis.units = result->bar_units;
			axis.bounds.x = _ctx->x;
			axis.bounds.w = y_axis_width;
			axis.bounds.y = _ctx->y;
			axis.bounds.h = _ctx->h - x_axis_height;
			result->y_axes.insert({ "", axis });

			result->chart_bounds.x = _ctx->x + y_axis_width;
			result->chart_bounds.y = _ctx->y;
			result->chart_bounds.h = _ctx->h - x_axis_height;
			result->chart_bounds.w = _ctx->w - y_axis_width;

			for (auto item : slice_array) {
				json jitem = item;
				double x = result->chart_bounds.x;

				for (auto fld : _spec.value_fields) {

					double y = jitem[fld.field_name].as_double();
					auto& f = result->bar_units;
					double height = (y / f.max);
					double width = bar_width - 8;
					if (width < 0) width = 0;

					auto& bar_frame = result->bars[fld.field_name];
					double y_scaled = result->chart_bounds.bottom() - result->bar_units.scale(y, result->chart_bounds.y, result->chart_bounds.bottom());

					result->bars[fld.field_name]->path.addLineTo(x, result->chart_bounds.bottom());
					result->bars[fld.field_name]->path.addLineTo(x + width, result->chart_bounds.bottom());
					result->bars[fld.field_name]->path.addLineTo(x + width, y_scaled);
					result->bars[fld.field_name]->path.addLineTo(x, y_scaled);
					result->bars[fld.field_name]->path.addLineTo(x, result->chart_bounds.bottom());
					x += bar_width;
				}
			}
		}

		virtual void draw_chart(std::shared_ptr<direct2dContext>& _context)
		{
			for (auto& bar : this->bars)
			{
				bar.second->draw_item(*_context.get());
			}
		}

	};

	std::shared_ptr<chart_specification> create_chart_specification(json& jchart)
	{
		std::shared_ptr<chart_specification> current_options;

		std::string class_name = jchart["class_name"].as_string();
		if (class_name == "time_chart") {
			current_options = std::make_shared<time_chart_specification>();
			current_options->put_json(jchart);
		}
		else if (class_name == "xy_chart") {
			current_options = std::make_shared<xy_chart_specification>();
			current_options->put_json(jchart);
		}
		else if (class_name == "bar_chart") {
			current_options = std::make_shared<bar_chart_specification>();
			current_options->put_json(jchart);
		}
		else if (class_name == "pie_chart") {
			current_options = std::make_shared<pie_chart_specification>();
			current_options->put_json(jchart);
		}
		else if (class_name == "program_chart") {
			current_options = std::make_shared<program_chart_specification>();
			current_options->put_json(jchart);
		}
		return current_options;
	}

	class chart_control : public draw_control
	{
		double elapsed_seconds = 0.0;

        std::shared_ptr<chart_specification>						current_options;
		std::shared_ptr<chart_frame>								current_chart;

		double														xaxis_width = 100;
		double														yaxis_height = 50;

	public:

		chart_control(const chart_control& _src) = default;

		chart_control()
		{
			init();
		}

		chart_control(control_base* _parent, int _id)
			: draw_control(_parent, _id)
		{
			init();
		}

		virtual ~chart_control() { ; }

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			draw_control::get_json(_dest);

			if (current_options) {
				json chart = jp.create_object();
				current_options->get_json(chart);
				_dest.put_member("chart", chart);
			}
		}

		virtual void put_json(json& _src)
		{
			json_parser jp;
			draw_control::put_json(_src);
            json chart = _src["chart"];
			if (chart.object()) {
				set_chart(chart);
			}
		}

		virtual void set_chart(json& jchart)
		{
			current_chart = nullptr;
			current_options = create_chart_specification(jchart);
		}

		virtual void set_chart(std::shared_ptr<chart_specification> _chart, json_object& _src, std::string _json_field_name)
		{
			current_chart = nullptr;
			current_options = _chart;
			json_field_name = _json_field_name;
            set_data(_src);
		}

		virtual void set_chart(std::shared_ptr<chart_specification> chart)
		{
			current_chart = nullptr;
			current_options = chart;
		}

		void create_chart(std::shared_ptr<direct2dContext>& _context)
		{
			if (auto tc_options = std::dynamic_pointer_cast<time_chart_specification>(current_options)) {
				current_chart = std::make_shared<time_chart_frame>(_context, slice_array, *tc_options.get(), &inner_bounds);
			}
			else if (auto xyc_options = std::dynamic_pointer_cast<xy_chart_specification>(current_options)) {
				current_chart = std::make_shared<xy_chart_frame>(_context, slice_array, *xyc_options.get(), &inner_bounds);
			}
			else if (auto bc_options = std::dynamic_pointer_cast<bar_chart_specification>(current_options)) {
				current_chart = std::make_shared<bar_chart_frame>(_context, slice_array, *bc_options.get(), &inner_bounds);
			}
			else if (auto pc_options = std::dynamic_pointer_cast<pie_chart_specification>(current_options)) {
				current_chart = std::make_shared<pie_chart_frame>(_context, slice_array, *pc_options.get(), &inner_bounds);
			}
		}

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<chart_control>(*this);
			return tv;
		}

		virtual void on_update(double _time)
		{
			elapsed_seconds = _time;

			for (auto child : children) {
				child->on_update(_time);
			}
		}

		virtual void on_draw(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{
			if (!current_chart) {
				create_chart(_context);
			}
		}

		virtual void on_create(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{
			set_default_styles();
			current_chart = nullptr;
			create_chart(_context);
		}

		void init()
		{
			set_origin(0.0_px, 0.0_px);
		}

		virtual void set_default_styles()
		{
			;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override
		{
			draw_control::arrange(_parent, _ctx);
			current_chart = nullptr;
		}


		virtual json_object set_data(json_object _data) override
		{
			draw_control::set_data(_data);
			return _data;
		}

		virtual double get_font_size() { return view_style ? view_style->text_style.fontSize : 14; }
	};


}