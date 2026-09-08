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
            _dest.put_member("values_fields", jvalues);
        }

        void put_json(json& _src)
        {
            json jtime = _src["time_series"];
            time_series.put_json(jtime);
            json jvalues = _src["values_fields"];
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
            _dest.put_member("values_fields", jvalues);
        }

        void put_json(json& _src)
        {
            json jvalues = _src["values_fields"];
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
            _dest.put_member("value_fields", jvalues);
        }

        void put_json(json& _src)
        {
            json jvalues = _src["value_fields"];
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
			_dest.put_member("value_fields", jvalues);
		}

		void put_json(json& _src)
		{
			json jvalues = _src["value_fields"];
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

	class chart_frame {
	public:
		rectangle					ux_bounds;
		rectangle					chart_bounds;
	};

	class axis_frame {
	public:
        unit_frame units;
        rectangle  bounds;
	};

	class program_chart_frame : public chart_frame {
	public:
		program_chart_specification specs;
        std::vector<axis_frame>		y_axes;
	};

	class time_chart_frame : public chart_frame {
	public:
		time_chart_specification specs;
		std::map<std::string, unit_frame> x_units;
		std::map<std::string, unit_frame> y_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	lines;
		std::vector<axis_frame>		y_axes;
		axis_frame					x_axes;
		rectangle					chart_bounds;
	};

	class xy_chart_frame : public chart_frame {
	public:
		xy_chart_specification specs;
		std::map<std::string, unit_frame> x_units;
		std::map<std::string, unit_frame> y_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	lines;
		std::vector<axis_frame>		y_axes;
		std::vector<axis_frame>		x_axes;
	};

	class pie_chart_frame : public chart_frame {
	public:
		pie_chart_specification specs;
        unit_frame				pie_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	slices;
	};

	class bar_chart_frame : public chart_frame {
	public:
		bar_chart_specification specs;
		unit_frame				bar_units;
		std::map<std::string, std::shared_ptr<game::vector_frame>>	bars;
		std::vector<axis_frame>		y_axes;
	};

	class chart_control : public draw_control
	{
        std::vector<std::shared_ptr<game::frame>>					frames;

		bool set_items(json_array _data);
		double elapsed_seconds = 0.0;

        std::vector<std::shared_ptr<chart_specification>>			chart_options;
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
			json charts = jp.create_array();
            for (auto chart : chart_options) {
                json jchart;
                chart->get_json(jchart);				
                charts.push_back(jchart);
            }
			_dest.put_member("charts", charts);
		}

		virtual void put_json(json& _src)
		{
			;
		}

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<animations_control>(*this);
			return tv;
		}

		virtual void on_update(double _time)
		{
			elapsed_seconds = _time;

			for (auto child : children) {
				child->on_update(_time);
			}
		}

        virtual void create_xaxis(std::shared_ptr<direct2dContext>& _context, rectangle* _ctx)
        {
            ;
        }

		virtual void create_yaxis(std::shared_ptr<direct2dContext>& _context, rectangle* _ctx)
		{
			;
		}

		virtual std::shared_ptr<time_chart_frame> create_time_chart(std::shared_ptr<direct2dContext>&_context, time_chart_specification & _spec, rectangle * _ctx)
		{
            std::shared_ptr<time_chart_frame> result = std::make_shared<time_chart_frame>();

            result->ux_bounds = *_ctx;
			
			result->specs = _spec;
			result->x_units[_spec.time_series.units] = unit_frame();
			auto& f2 = result->x_units[_spec.time_series.units];

            for (auto fld : _spec.values_fields) {
                std::string field_name = fld.field_name;
                std::string units = fld.units;
                std::shared_ptr<game::vector_frame> line_frame = std::make_shared<game::vector_frame>();
				line_frame->fill = fld.fill;
				line_frame->stroke = fld.stroke;
				line_frame->stroke_width = fld.stroke_width;
                result->y_units[units] = unit_frame();
                result->lines[field_name] = line_frame;
            }

			for (auto item : slice_array) {
				json jitem = item;
				double time_value = jitem[_spec.time_series.field_name].as_double();
				f2.accumulate(time_value);
				for (auto fld : _spec.values_fields) {
					std::string field_name = fld.field_name;
					double y_value = jitem[field_name].as_double();
					auto& f = result->y_units[fld.units];
					f.accumulate(y_value);
				}
			}

            result->chart_bounds.x = _ctx->x + xaxis_width * result->y_units.size();
			result->chart_bounds.y = _ctx->y;
			result->chart_bounds.h = _ctx->h - yaxis_height;
            result->chart_bounds.w = _ctx->right() - result->chart_bounds.x;
            result->x_axes.bounds.x = result->chart_bounds.x;
            result->x_axes.bounds.w = result->chart_bounds.w;
            result->x_axes.bounds.y = result->chart_bounds.bottom();
            result->x_axes.bounds.h = yaxis_height;

			int index = 0;
			for (auto &unit : result->y_units) {
				axis_frame axis;
                axis.units = unit.second;
                axis.bounds.x = _ctx->x + xaxis_width * index;
                axis.bounds.w = xaxis_width;
				axis.bounds.y = _ctx->y;
                axis.bounds.h = _ctx->h - yaxis_height;
				result->y_axes.push_back(axis);
				index++;
			}

            for (auto item : slice_array) {
                json jitem = item;
                double time_value = jitem[_spec.time_series.field_name].as_double();

				for (auto fld : _spec.values_fields) {
                    std::string field_name = fld.field_name;
                    double y_value = jitem[field_name].as_double();
					auto& y_units = result->y_units[fld.units];
					auto& x_units = result->x_units[_spec.time_series.units];
					auto& line_frame = result->lines[field_name];
                    double y_scaled = result->chart_bounds.bottom()- y_units.scale(y_value, result->chart_bounds.y, result->chart_bounds.bottom());
                    double x_scaled = x_units.scale(time_value, result->chart_bounds.x, result->chart_bounds.right());
                    line_frame->path.addLineTo(x_scaled, y_scaled);
                }
            }
            return result;
		}

		virtual std::shared_ptr<xy_chart_frame> create_xy_chart(std::shared_ptr<direct2dContext>& _context, xy_chart_specification& _spec, rectangle* _ctx)
		{
			std::shared_ptr<xy_chart_frame> result = std::make_shared<xy_chart_frame>();

			result->specs = _spec;
			result->ux_bounds = *_ctx;

			for (auto fld : _spec.values_fields) {
				std::string xname = fld.xfield_name;
                std::string yname = fld.yfield_name;
				result->x_units[fld.x_units] = unit_frame();
				result->y_units[fld.y_units] = unit_frame();

				std::shared_ptr<game::vector_frame> line_frame = std::make_shared<game::vector_frame>();
				std::string line_name = fld.xfield_name + "_" + fld.yfield_name;
				result->lines[line_name] = line_frame;
				line_frame->fill = fld.fill;
				line_frame->stroke = fld.stroke;
				line_frame->stroke_width = fld.stroke_width;
			}

			result->chart_bounds.x = _ctx->x + xaxis_width * result->y_units.size();
			result->chart_bounds.y = _ctx->y;
			result->chart_bounds.h = _ctx->h - yaxis_height * result->x_units.size();
			result->chart_bounds.w = _ctx->right() - result->chart_bounds.x;

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.values_fields) {
					double x = jitem[fld.xfield_name].as_double();
					double y = jitem[fld.yfield_name].as_double();
					auto& f = result->x_units[fld.x_units];
					f.accumulate(x);
					auto& f2 = result->y_units[fld.y_units];
					f2.accumulate(y);
				}
			}

			int index = 0;
			for (auto& unit : result->y_units) {
				axis_frame axis;
				axis.units = unit.second;
				axis.bounds.x = _ctx->x + xaxis_width * index;
				axis.bounds.w = xaxis_width;
				axis.bounds.y = _ctx->y;
				axis.bounds.h = _ctx->h - yaxis_height;
				result->y_axes.push_back(axis);
				index++;
			}

			index = 0;
			for (auto& unit : result->x_units) {
				axis_frame axis;
				axis.units = unit.second;
				axis.bounds.x = result->chart_bounds.x;
				axis.bounds.w = result->chart_bounds.w;
				axis.bounds.y = result->chart_bounds.bottom() + yaxis_height * index;
				axis.bounds.h = yaxis_height;
				result->x_axes.push_back(axis);
				index++;
			}

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.values_fields) {
					double x = jitem[fld.xfield_name].as_double();
					double y = jitem[fld.yfield_name].as_double();
					auto& f = result->x_units[fld.x_units];
					auto& f2 = result->y_units[fld.y_units];
					std::string line_name = fld.xfield_name + "_" + fld.yfield_name;
					auto& line_frame = result->lines[line_name];
                    double x_scaled = f.scale(x, result->chart_bounds.x, result->chart_bounds.right());
                    double y_scaled = result->chart_bounds.bottom() - f2.scale(y, result->chart_bounds.y, result->chart_bounds.bottom());
					line_frame->path.addLineTo(x_scaled, y_scaled);
				}
			}
			return result;
		}

		virtual std::shared_ptr<pie_chart_frame> create_pie_chart(std::shared_ptr<direct2dContext>& _context, pie_chart_specification& _spec, rectangle* _ctx)
		{
			std::shared_ptr<pie_chart_frame> result = std::make_shared<pie_chart_frame>();

			result->specs = _spec;
			result->ux_bounds = *_ctx;

			for (auto fld : _spec.value_fields) {
				std::string name = fld.field_name;
				std::shared_ptr<game::vector_frame> pie_frame = std::make_shared<game::vector_frame>();
				std::string slice_name = fld.field_name;
				result->slices[slice_name] = pie_frame;
				pie_frame->fill = fld.fill;
				pie_frame->stroke = fld.stroke;
				pie_frame->stroke_width = fld.stroke_width;
			}

			for (auto item : slice_array) {
				json jitem = item;
				for (auto fld : _spec.value_fields) {
					double x = jitem[fld.field_name].as_double();
					auto& f = result->pie_units;
					f.accumulate(x);
				}
			}

            double xradius = _ctx->w / 2.0;
            double yradius = _ctx->h / 2.0;
            double radius = std::min(xradius, yradius);

			point center = rectangle_math::center(result->chart_bounds);

			for (auto item : slice_array) {
				json jitem = item;
				double start_angle = 0.0;
				for (auto fld : _spec.value_fields) {
					double x = jitem[fld.field_name].as_double();
                    auto& f = result->pie_units;
                    double angle = (x / f.sum) * 360.0;
                    auto& slice_frame = result->slices[fld.field_name];
					slice_frame->path.addLineTo(center.x, center.y);
					slice_frame->path.addPathArc(center.x + cos(start_angle) * radius, center.y +sin(start_angle) * radius, radius, radius, start_angle);
					slice_frame->path.addLineTo(center.x, center.y);
					start_angle += angle;
				}
			}

			return result;
		}

		virtual std::shared_ptr<bar_chart_frame> create_bar_chart(std::shared_ptr<direct2dContext>& _context, bar_chart_specification& _spec, rectangle* _ctx)
		{
			std::shared_ptr<bar_chart_frame> result = std::make_shared<bar_chart_frame>();

			result->specs = _spec;
			result->ux_bounds = *_ctx;

            double bar_width = (_ctx->w - xaxis_width) / slice_array.size();

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
			axis.bounds.w = xaxis_width;
			axis.bounds.y = _ctx->y;
			axis.bounds.h = _ctx->h - yaxis_height;
			result->y_axes.push_back(axis);

			result->chart_bounds.x = _ctx->x + xaxis_width;
			result->chart_bounds.y = _ctx->y;
			result->chart_bounds.h = _ctx->h - yaxis_height;
			result->chart_bounds.w = _ctx->w - xaxis_width;

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

			return result;
		}

		virtual std::shared_ptr<program_chart_frame> create_program_chart(std::shared_ptr<direct2dContext>& _context, rectangle* _ctx)
		{
			std::shared_ptr<program_chart_frame> result = std::make_shared<program_chart_frame>();
			result->ux_bounds = *_ctx;
			return result;
		}

		virtual void on_draw(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{

			animation_border = solidBrushRequest("animation_border", "00C000");
			frame_border = solidBrushRequest("frame_border", "850095");

			_context->setBrush(&animation_border);
			_context->setBrush(&frame_border);

			std::string border_name = animation_border.get_name();

			for (auto& anim_rect : animation_rectangles) {
				if (anim_rect.object) {
					auto rect = anim_rect.rect;
					DirectX::XMVECTOR location = to_point(rect);
					_context->drawRectangle(&rect, border_name, 2, "");
				}
			}

			border_name = frame_border.get_name();

			for (auto& frame_rect : frame_rectangles) {
				if (frame_rect.object) {
					auto rect = frame_rect.rect;
					DirectX::XMVECTOR location = to_point(rect);
					_context->drawRectangle(&rect, border_name, 8, "");
				}
			}

			for (auto& anim_rect : animation_rectangles) {
				if (anim_rect.object) {
					auto rect = anim_rect.rect;
					anim_rect.object->draw(*_context, &rect);
				}
			}

			for (auto& frame_rect : frame_rectangles) {
				if (frame_rect.object) {
					auto rect = frame_rect.rect;
					frame_rect.object->draw(*_context, &rect);
				}
			}
		}

		virtual void on_create(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{
			set_default_styles();
			for (auto anim : animations) {
				anim->create_assets(*_context);
			}
		}

		void init()
		{
			set_origin(0.0_px, 0.0_px);
			set_size(1.0_container, 1.2_fontgr);
			animation_border = solidBrushRequest("animation_border", "400040");
			frame_border = solidBrushRequest("frame_border", "400035");
		}

		virtual void set_default_styles()
		{
			;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override
		{
			draw_control::arrange(_parent, _ctx);

			double num_frames_x = 4;
			double num_frames_y = 2;
			double num_animations_x = 2;
			double num_animations_y = 4;

			point total_size = rectangle_math::size(_ctx);

			double animation_width = total_size.x * 0.3;
			double animation_height = animation_width;
			double frame_width = animation_width * .75;
			double frame_height = animation_height * .75;

			rectangle animation_list_rect = { _ctx->x, _ctx->y, animation_width, total_size.y };
			rectangle frame_list_rect = { _ctx->x + animation_width, _ctx->y + total_size.y - frame_height, total_size.x - animation_width, frame_height };
			rectangle current_animation_rect = { _ctx->x + animation_width, _ctx->y, total_size.x - animation_width, total_size.y - frame_height };

			point base;
			current_animation.rect = current_animation_rect;

			auto ianim = animations.begin();

			rectangle r;

			r.x = animation_list_rect.x;
			r.y = animation_list_rect.y;
			r.w = animation_width;
			r.h = animation_height;

			while (ianim != std::end(animations))
			{
				animation_rectangles.push_back({ r, *ianim });

				if (!current_animation.object) {
					current_animation.object = *ianim;
				}

				r.y += animation_height;
				ianim++;
			}

			r.x = frame_list_rect.x;
			r.y = frame_list_rect.y;
			r.w = frame_width;
			r.h = frame_height;

			if (current_animation.object) {
				auto iframe = current_animation.object->frames.begin();

				while (iframe != std::end(current_animation.object->frames)) {
					corona_frame_rectangle fr;
					fr.rect = r;
					fr.object = iframe->second;
					frame_rectangles.push_back(fr);
					if (!current_frame.object) {
						current_frame.object = iframe->second;
					}
					r.x += frame_width;
					iframe++;
				}
			}
		}


		virtual json_object set_data(json_object _data) override
		{
			draw_control::set_data(_data);
			set_items(slice_array);
			return _data;
		}

		virtual double get_font_size() { return view_style ? view_style->text_style.fontSize : 14; }
	};

}