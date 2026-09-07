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

        void get_json(json& _dest)
        {
            json_parser jp;
            _dest.put_member("field_name", field_name);
            _dest.put_member("units", units);
        }

        void put_json(json& _src)
        {
            field_name = _src["field_name"].as_string();
            units = _src["units"].as_string();
        }
	};

	class chart_xy_series
	{
	public:
		chart_series x_source;
		chart_series y_source;

		void get_json(json& _dest)
		{
			json_parser jp;
			json jx = jp.create_object();
			json jy = jp.create_object();

			x_source.put_json(jx);
			y_source.put_json(jy);
		}

		void put_json(json& _src)
		{
			json jx = _src["x_source"];
			json jy = _src["y_source"];
			x_source.put_json(jx);
			y_source.put_json(jy);
		}
	};

	class time_chart_specification : public chart_specification
	{
	public:
		chart_series time_series;
		std::vector<chart_series> values_fields;

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

	class chart_control : public draw_control
	{
        std::vector<std::shared_ptr<game::frame>>			frames;

		bool set_items(json_array _data);
		double elapsed_seconds = 0.0;

        std::vector<std::shared_ptr<chart_specification>>	chart_options;

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
			for (auto anim : animations) {
				anim->set_time(_time);
			}
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

		virtual void create_time_chart(std::shared_ptr<direct2dContext>& _context, time_chart_specification& _spec, rectangle* _ctx)
		{
			std::map<std::string, std::shared_ptr<game::vector_frame>> lines;

		}

		virtual void create_xy_chart(std::shared_ptr<direct2dContext>& _context, xy_chart_specification& _spec, rectangle* _ctx)
		{
			std::map<std::string, std::shared_ptr<game::vector_frame>> lines;

		}

		virtual void create_pie_chart(std::shared_ptr<direct2dContext>& _context, rectangle* _ctx)
		{
			std::map<std::string, std::shared_ptr<game::vector_frame>> pies;
			;
		}

		virtual void create_bar_chart(std::shared_ptr<direct2dContext>& _context, rectangle* _ctx)
		{
			std::map<std::string, std::shared_ptr<game::vector_frame>> bars;
			;
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