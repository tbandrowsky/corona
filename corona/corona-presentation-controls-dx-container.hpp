/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved


MIT License

About this File
This is for controls using direct2d to draw their own contexts and are containers

Notes

For Future Consideration
*/


#pragma once

namespace corona
{

	template <typename DATA> class contents_generator
	{
	public:
		DATA data;
		std::function<void(DATA& data, control_base* generator)> generator;

		void operator() ( control_base* _this )
		{
			generator(data, _this);
		}
	};

	class container_control : public draw_control
	{

	public:

		layout_rect				item_box = {};
		measure					item_margin = {};
		point					remaining = {};

		visual_alignment		content_alignment = visual_alignment::align_near;
		visual_alignment		content_cross_alignment = visual_alignment::align_near;

		container_control()
		{
			id = id_counter::next();
		}

		container_control(const container_control& _src) = default;

		container_control(control_base *_parent, int _id)
		{
			id = _id;
			if (_parent and get_nchittest() == HTCLIENT) {
				set_nchittest(_parent->get_nchittest());
			}
		}

		virtual void create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host) override
		{
			draw_control::create(_context, _host);
		}

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<container_control>(*this);
			return tv;
		}

		virtual ~container_control()
		{
			;
		}

		virtual void set_contents(control_base *_parent, std::function<void(control_base* _page)> _contents)
		{
			_contents(this);

			arrange(_parent, &bounds);
		}

		virtual void set_contents(control_base *_parent, page_base *_contents)
		{
			auto new_root = _contents->root;
			children.clear();

			for (auto srcchild : _contents->root->children)
			{
				auto new_child = srcchild->clone();
				children.push_back(new_child);
			}

			remaining = { bounds.w, bounds.h, 0.0 };

			for (auto child : children) {
				auto sz = child->get_size(this);
				point item_origin = {};
				point item_position = child->get_position(this);
				item_origin.x += item_position.x;
				item_origin.y += item_position.y;
				rectangle item_bounds = { item_origin.x, item_origin.y, sz.x, sz.y };
				child->arrange(this, &item_bounds);
			}
		}

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			for (auto child : children) {
				child->on_subscribe(_presentation, _page);
			}
		}

		container_control& set_content_align(visual_alignment _new_alignment)
		{
			content_alignment = _new_alignment;
			return *this;
		}

		container_control& set_content_cross_align(visual_alignment _new_alignment)
		{
			content_cross_alignment = _new_alignment;
			return *this;
		}

		container_control& set_item_origin(measure _x, measure _y)
		{
			item_box.x = _x;
			item_box.y = _y;
			return *this;
		}

		container_control& set_item_size(measure _width, measure _height)
		{
			if (_width.amount > 0)
				item_box.width = _width;
			if (_height.amount > 0)
				item_box.height = _height;
			return *this;
		}

		container_control& set_item_position(layout_rect _new_layout)
		{
			item_box = _new_layout;
			return *this;
		}

		container_control& set_item_margin(measure _item_margin)
		{
			item_margin = _item_margin;
			return *this;
		}


		container_control& set_origin(measure _x, measure _y)
		{
			box.x = _x;
			box.y = _y;
			return *this;
		}

		container_control& set_size(measure _width, measure _height)
		{
			box.width = _width;
			box.height = _height;
			return *this;
		}


		container_control& set_position(layout_rect _new_layout)
		{
			box = _new_layout;
			return *this;
		}

		container_control& set_margin(measure _item_space)
		{
			margin = _item_space;
			return *this;
		}

		// default implementation for composed controls
		virtual void arrange(control_base* _parent, rectangle* _ctx) override
		{
			set_bounds(_parent, *_ctx);
			for (auto child : children) {
				child->arrange(this, _ctx);
			}
		}

		virtual void get_json(json& _dest)
		{
			draw_control::get_json(_dest);

			corona::get_json(_dest, "content_alignment", content_alignment);
			corona::get_json(_dest, "content_cross_alignment", content_cross_alignment);
		}

		virtual void put_json(json& _src)
		{
			draw_control::put_json(_src);

			corona::put_json(content_alignment, _src, "content_alignment" );
			corona::put_json(content_cross_alignment, _src, "content_cross_alignment");
		}
	};

	class absolute_layout :
		public container_control
	{
	public:
		absolute_layout() { ; }
		absolute_layout(const absolute_layout& _src) = default;
		absolute_layout(control_base*_parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~absolute_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<absolute_layout>(*this);
			return tv;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override;
	};

	class column_layout :
		public container_control
	{
		virtual void calculate_remaining();
		virtual void arrange_near(control_base* _parent, rectangle* _ctx);
		virtual void arrange_center(control_base* _parent, rectangle* _ctx);
		virtual void arrange_far(control_base* _parent, rectangle* _ctx);

	public:
		layout_rect item_size;
		measure item_start_space;
		measure item_next_space;
		bool	wrap = false;

		column_layout() { ; }
		column_layout(const column_layout& _src) : container_control(_src) 
		{
			item_size = _src.item_size;
			item_start_space = _src.item_start_space;
			item_next_space = _src.item_next_space;
			wrap = _src.wrap;
		}
		column_layout(control_base* _parent, int _id) : container_control(_parent, _id), wrap(true) { ; }

		virtual ~column_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<column_layout>(*this);
			return tv;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override;
		virtual point get_remaining(control_base* _parent) override;

		virtual void get_json(json& _dest)
		{
			container_control::get_json(_dest);

			json_parser jp;

			json temp = jp.create_object();
			corona::get_json(temp, item_start_space );
			_dest.put_member("item_start_space", temp);

			temp = jp.create_object();
			corona::get_json(temp, item_next_space);
			_dest.put_member("item_next_space", temp);

			_dest.put_member("wrap", wrap);
		}

		virtual void put_json(json& _src)
		{
			container_control::put_json(_src);

			json temp = _src["item_start_space"];
			corona::put_json(item_start_space, temp);

			temp = _src["item_next_space"];
			corona::put_json(item_next_space, temp);

			wrap = _src["wrap"].as_bool();
		}

	};

	class row_layout :
		public container_control
	{
	protected:

        virtual void calculate_remaining();
		virtual void arrange_near(control_base* _parent, rectangle *_ctx);
		virtual void arrange_center(control_base* _parent, rectangle *_ctx);
		virtual void arrange_far(control_base* _parent, rectangle *_ctx);

	public:

		measure item_start_space;
		measure item_next_space;

		layout_rect item_size;
		bool		wrap = false;

		row_layout() { ; }
		row_layout(const row_layout& _src) = default;
		row_layout(control_base* _parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~row_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<row_layout>(*this);
			return tv;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override;
		virtual point get_remaining(control_base* _parent) override;
		virtual void get_json(json& _dest)
		{
			container_control::get_json(_dest);

			json_parser jp;

			json temp = jp.create_object();
			corona::get_json(temp, item_start_space);
			_dest.put_member("item_start_space", temp);

			temp = jp.create_object();
			corona::get_json(temp, item_next_space);
			_dest.put_member("item_next_space", temp);

			_dest.put_member("wrap", wrap);
		}

		virtual void put_json(json& _src)
		{
			container_control::put_json(_src);

			json temp = _src["item_start_space"];
			corona::put_json(item_start_space, temp);

			temp = _src["item_next_space"];
			corona::put_json(item_next_space, temp);

			wrap = _src["wrap"].as_bool();
		}

	};

	class frame_layout :
		public container_control
	{
	protected:
		void arrange_children();
		std::string hit_words;

	public:

		frame_layout() { ; }
		frame_layout(const frame_layout& _src) = default;
		frame_layout(control_base* _parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~frame_layout() { ; }

		virtual void set_hit_word(std::string _words) override
		{
			hit_words = _words;
		}

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<frame_layout>(*this);
			return tv;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override;
		virtual point get_remaining(control_base* _parent) override;

		virtual void set_contents(presentation_base *_presentation, page_base *_parent_page, page_base* _contents);

		virtual void set_contents(control_base* _parent, std::function<void(control_base* _page)> _contents)
		{
			container_control::set_contents(_parent, _contents);
		}

		virtual void create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host) override
		{
			container_control::create(_context, _host);
		}
	};

	class items_view_row
	{
	public:
		int item_id;
		int page_index;
		json object_data;
		rectangle bounds;
		std::shared_ptr<control_base> control;
	};

	class items_view_page
	{
	public:
		int page_index;
		double start_y;
		double stop_y;
		int start_index;
		int stop_index;
	};

	class items_view : public draw_control
	{
		json data;
		std::shared_ptr<corona_class_page_map>					sources;
        std::map<std::string, std::shared_ptr<control_base>>	page_controls;
		std::vector<items_view_row>								rows;
		std::map<int, items_view_page>							pages;
		solidBrushRequest										selection_border;
		solidBrushRequest										focused_border;

		solidBrushRequest										scroll_well;
		solidBrushRequest										scroll_well_border;
		solidBrushRequest										scroll_knob;
		solidBrushRequest										scroll_knob_border;
		solidBrushRequest										scroll_knob_selected;
		solidBrushRequest										scroll_knob_border_selected;
		rectangle                                               scroll_well_bounds;

		viewStyleRequest										text_style;
		std::string												empty_text;

        bool													keep_position_on_set_data = true;

		// we keep the set of controls here on the back end, because they are small as they are not dragging around any 
		// back end bitmaps or windows.  (arranging doesn't create the assets on a control, create does)
		rectangle view_port;

		int selected_page_index;
		int selected_item_index;

		std::shared_ptr<corona_bus_command> select_command;
		std::shared_ptr<corona_bus_command> empty_command;

		void update_selection();

		void check_scroll()
		{
			if (rows.size()==0)
			{
				selected_item_index = 0;
				selected_page_index = 0;
				return;
            }

			if (selected_item_index >= rows.size() - 1)
			{
				selected_item_index = rows.size() - 1;
			}

			if (selected_item_index < 0)
			{
				selected_item_index = 0;
			}

			selected_page_index = rows[selected_item_index].page_index;
			int selected_item_page_index = pages[selected_page_index].start_index;
			view_port.y = rows[selected_item_page_index].bounds.y;
			update_selection();
			std::string msg;
			msg = std::format("selected_page_index '{0}' selected_item_index {1}, y:{2} ", selected_page_index, selected_item_page_index, view_port.y);
			system_monitoring_interface::active_mon->log_information(msg);
		}

		void set_scroll_well(solidBrushRequest _brushFill)
		{
			scroll_well = _brushFill;
			scroll_well.name = typeid(*this).name();
			scroll_well.name += "_scroll_well";
        }

        void set_scroll_well_border(solidBrushRequest _brushFill)
		{
			scroll_well_border = _brushFill;
			scroll_well_border.name = typeid(*this).name();
			scroll_well_border.name += "_scroll_well_border";
		}

		void set_scroll_knob(solidBrushRequest _brushFill)
		{
			scroll_knob = _brushFill;
			scroll_knob.name = typeid(*this).name();
			scroll_knob.name += "_scroll_knob";
		}

		void set_scroll_knob_border(solidBrushRequest _brushFill)
		{
			scroll_knob_border = _brushFill;
			scroll_knob_border.name = typeid(*this).name();
			scroll_knob_border.name += "_scroll_knob_border";
		}

		void set_scroll_knob_selected(solidBrushRequest _brushFill)
		{
			scroll_knob_selected = _brushFill;
			scroll_knob_selected.name = typeid(*this).name();
			scroll_knob_selected.name += "_scroll_knob_selected";
		}

		void set_scroll_knob_border_selected(solidBrushRequest _brushFill)
		{
			scroll_knob_border_selected = _brushFill;
			scroll_knob_border_selected.name = typeid(*this).name();
			scroll_knob_border_selected.name += "_scroll_knob_border_selected";
		}

		void set_scroll_well(std::string _brushFill)
		{
			scroll_well.brushColor = toColor(_brushFill);
			scroll_well.name = typeid(*this).name();
			scroll_well.name += "_scroll_well";
		}

		void set_scroll_well_border(std::string _brushFill)
		{
			scroll_well.brushColor = toColor(_brushFill);
			scroll_well_border.name = typeid(*this).name();
			scroll_well_border.name += "_scroll_well_border";
		}

		void set_scroll_knob(std::string _brushFill)
		{
			scroll_knob.brushColor = toColor(_brushFill);
			scroll_knob.name = typeid(*this).name();
			scroll_knob.name += "_scroll_knob";
		}

		void set_scroll_knob_border(std::string _brushFill)
		{
			scroll_knob_border.brushColor = toColor(_brushFill);
			scroll_knob_border.name = typeid(*this).name();
			scroll_knob_border.name += "_scroll_knob_border";
		}

		void set_scroll_knob_selected(std::string _brushFill)
		{
			scroll_knob_selected.brushColor = toColor(_brushFill);
			scroll_knob_selected.name = typeid(*this).name();
			scroll_knob_selected.name += "_scroll_knob_selected";
		}

		void set_scroll_knob_border_selected(std::string _brushFill)
		{
			scroll_knob_border_selected.brushColor = toColor(_brushFill);
			scroll_knob_border_selected.name = typeid(*this).name();
			scroll_knob_border_selected.name += "_scroll_knob_border_selected";
		}

		void set_focused_border(solidBrushRequest _brushFill)
		{
			focused_border = _brushFill;
			focused_border.name = typeid(*this).name();
			focused_border.name += "_focused";
		}

		void set_focused_border(std::string _color)
		{
			focused_border.brushColor = toColor(_color.c_str());
			focused_border.name = typeid(*this).name();
			focused_border.name += "_focused";
		}

		void set_selection_border(solidBrushRequest _brushFill)
		{
			selection_border = _brushFill;
			selection_border.name = typeid(*this).name();
			selection_border.name +="_selection";
		}

		void set_selection_border(std::string _color)
		{
			selection_border.brushColor = toColor(_color.c_str());
			selection_border.name = typeid(*this).name();
			selection_border.name += "_selection";
		}

		void init()
		{

			auto st = presentation_style_factory::get_current();
			auto view_style = st->get_style()->EmptyListStyle;
            text_style = *view_style;

            empty_text = "No items in list";

			view_port = {};
			selected_item_index = 0;
			selected_page_index = 0;

			set_selection_border("#c0c0c0");
			set_focused_border("#708090");
			set_scroll_knob("#101025");
			set_scroll_knob_border("#303050");
			set_scroll_knob_selected("#606080");
			set_scroll_knob_border_selected("#505060");
			set_scroll_well("#202030");
			set_scroll_well_border("#303050");

			on_create = [this](std::shared_ptr<direct2dContext>& _context, draw_control *_src)
				{
					_context->setSolidColorBrush(&selection_border);
					_context->setSolidColorBrush(&focused_border);
					_context->setSolidColorBrush(&scroll_knob);
					_context->setSolidColorBrush(&scroll_knob_border);
					_context->setSolidColorBrush(&scroll_knob_selected);
					_context->setSolidColorBrush(&scroll_knob_border_selected);
					_context->setSolidColorBrush(&scroll_well);
					_context->setSolidColorBrush(&scroll_well_border);
			};
		}

		void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			_page->on_mouse_wheel(this, [this, _presentation, _page](mouse_wheel_event evt)
				{
					int step = evt.delta / WHEEL_DELTA;
					int start = step < 0 ? step : 0;
                    int stop = step > 0 ? step : 0;
					
					if (step < 0) {
						this->line_down();
					}
					else {
						this->line_up();
					}

				});
			_page->on_mouse_click(this, [this, _presentation, _page](mouse_click_event evt)
				{
					auto bounds = evt.control->get_inner_bounds();
					bounds.x = bounds.right() - 16;
					if (bounds.x > 0 && rectangle_math::contains(bounds, evt.absolute_point.x, evt.absolute_point.y)) {
						int t_selected_page_index = find_page_index(evt.absolute_point);
						if (t_selected_page_index >= 0) {
							selected_item_index = pages[t_selected_page_index].start_index;
							check_scroll();
						}
					}
					else 
					{
						int temp = find_row_index(evt.relative_point);
						if (temp != -1) {
							selected_item_index = temp;
							check_scroll();
						}
					}
				});
		}

		int find_page_index(point pt)
		{
			int index = -1;
			if (rows.size() == 0) {
				return index;
			}
			double scroll_scale = inner_bounds.h / rows.back().bounds.bottom();
			for (auto& pg : pages) {
				double start = inner_bounds.y + scroll_scale * pg.second.start_y;
				double stop = inner_bounds.y + scroll_scale * pg.second.stop_y;

				if (pt.y >= start and pt.y < stop) {
					index = pg.second.page_index;
					break;
                }
			}
			return index;
		}

		// finds the selected row index based on a point clicked on the page.
		int find_row_index(point pt)
		{
			if (selected_page_index < 0 || selected_page_index >= pages.size()) {
				return -1;
            }

            int start_row_index = pages[selected_page_index].start_index;
            if (start_row_index < 0 || start_row_index >= rows.size()) {
				return -1;
			}
			pt.y += rows[start_row_index].bounds.y;

			while (start_row_index < rows.size()) {
                auto& row = rows[start_row_index];
                if (row.bounds.y <= pt.y && row.bounds.bottom() > pt.y) {
					return start_row_index;
				}
				start_row_index++;
			}

			return -1;
		}

		virtual void call_on_draw(std::shared_ptr<direct2dContext>& _context)
		{
			_context->setSolidColorBrush(&selection_border);
			_context->setSolidColorBrush(&focused_border);
			_context->setSolidColorBrush(&scroll_knob);
			_context->setSolidColorBrush(&scroll_knob_border);
			_context->setSolidColorBrush(&scroll_knob_selected);
			_context->setSolidColorBrush(&scroll_knob_border_selected);
			_context->setSolidColorBrush(&scroll_well);
			_context->setSolidColorBrush(&scroll_well_border);

			auto draw_bounds = inner_bounds;

			point offset = { view_port.x, view_port.y };

			auto& context = _context;

			if (is_focused) {
				_context->drawRectangle(&bounds, focused_border.name, 2, "");
			}

			if (rows.size() == 0) {
				_context->drawText(empty_text, &inner_bounds, text_style.text_style.name, text_style.shape_fill_brush.get_name(), "");
				return;
			}

			if (pages.size() <= selected_page_index) {
				std::string msg;
				msg = std::format("selected_page_index '{0}' not found", selected_page_index);
				system_monitoring_interface::active_mon->log_warning(msg);
				return;
			}

			int idx = pages[selected_page_index].start_index;
			int start_idx = idx;

			while (idx < rows.size())
			{
				auto& row = rows[idx];

				auto rect_bounds = row.bounds;
				rect_bounds.x = rect_bounds.x - offset.x + draw_bounds.x;
				rect_bounds.y = rect_bounds.y - offset.y + draw_bounds.y;

				if (rect_bounds.bottom() <= bounds.bottom() || idx == start_idx)
				{
					if (row.control)
					{
						row.control->arrange(this, &rect_bounds);
						row.control->render(_context);
						if (selected_item_index == idx) {
							_context->drawRectangle(&rect_bounds, selection_border.name, 2, "");
						}
					}
				}
				else
				{
					break;
				}

				idx++;
			};

			_context->drawRectangle(&scroll_well_bounds, scroll_well_border.name, 2, scroll_well.name);

			if (inner_bounds.h > 0) {

				double scroll_scale = inner_bounds.h / rows.back().bounds.bottom();

				for (int cpi = 0; cpi < pages.size(); cpi++)
				{
					auto& page = pages[cpi];

					rectangle scroll_knob_bounds = {};

					scroll_knob_bounds.x = scroll_well_bounds.x + 2;
					scroll_knob_bounds.y = inner_bounds.y + scroll_scale * page.start_y;
					scroll_knob_bounds.w = scroll_well_bounds.w - 4;
					scroll_knob_bounds.h = scroll_scale * (page.stop_y - page.start_y);

					if (page.page_index == selected_page_index)
					{
						_context->drawRectangle(&scroll_knob_bounds, scroll_knob_selected.name, 2, scroll_knob_border_selected.name);
					}
					else
					{
						_context->drawRectangle(&scroll_knob_bounds, scroll_knob.name, 2, scroll_knob_border.name);
					}
				}
			}

		}

		std::string hit_words;

	public:

		items_view()
		{
			init();
		}

		items_view(control_base* _parent, int _id) : draw_control(_parent, _id)
		{
			init();
		}

		items_view(const items_view& _src) = default;

		void create_controls();

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<items_view>(*this);
			return tv;
		}

		virtual ~items_view()
		{
			;
		}

		virtual bool captures_keyboard_focus() { return true; }

		virtual void set_hit_word(std::string _words) override
		{
			hit_words = _words;
        }

		virtual void arrange(control_base* _parent, rectangle* _bounds)
		{
			control_base::arrange(_parent, _bounds);

			view_port.w = bounds.w;
			view_port.h = bounds.h;

			scroll_well_bounds.w = 16;
			scroll_well_bounds.h = inner_bounds.h;
			scroll_well_bounds.x = inner_bounds.right() - 16;
			scroll_well_bounds.y = inner_bounds.y;

			if (bounds.h == 0 or bounds.w == 0) 
			{
				return;
			}

			double last_page_y = 0;
			double y = 0;

			int page_index = 0;
			pages.clear();

			int sz = rows.size();
			int i;

			items_view_page& ivp = pages[page_index];
			ivp.start_index = 0;
			ivp.start_y = y;
			ivp.page_index = 0;


			for (i = 0; i < sz; i++)
			{
				auto& r = rows[i];
				r.bounds.x = 0;
				r.bounds.y = y;
				r.bounds.w = 0;
				r.bounds.h = 0;
				if (r.control) {
					auto sz = r.control->get_size(this);
					r.bounds.w = sz.x;
					r.bounds.h = sz.y;
				}
				pages[page_index].stop_index = i;
				pages[page_index].stop_y = y;
				double pgy = r.bounds.bottom() - last_page_y;
                if (pgy > bounds.h)
				{
					page_index++;
					pages[page_index].page_index = page_index;
					pages[page_index].start_index = i;
					pages[page_index].start_y = y;
                    pages[page_index].page_index = page_index;
					last_page_y = y;
				}
				r.page_index = page_index;
				y += r.bounds.h;
			}
			pages[page_index].stop_index = i;
			pages[page_index].stop_y = y;
		}

		virtual json set_data(json _data) override
		{
			if (!keep_position_on_set_data) {
				selected_item_index = 0;
				selected_page_index = 0;
			}
			if (_data.array()) {
				set_items(_data);
			}
			else if (_data.has_member(json_field_name)) {
                set_items(_data[json_field_name]);
			}
			else {
				data = _data;
				arrange(this, &bounds);
				check_scroll();
				update_selection();
			}
			return data;
		}

		virtual bool set_items(json _data)
		{
			data = _data;
			rows.clear();

			create_controls();

			int i;
			rectangle item_bounds;
			item_bounds.x = 0;
			item_bounds.y = 0;
			item_bounds.w = 0;
			item_bounds.h = 0;

			for (i = 0; i < data.size(); i++)
			{
				items_view_row gvr;
				gvr.page_index = 0;
				gvr.bounds = item_bounds;
				gvr.item_id = i;
                gvr.object_data = data.get_element(i);
                gvr.control = nullptr;
				std::string class_name = gvr.object_data[class_name_field].as_string();
				if (page_controls.contains(class_name))
				{
					gvr.control = page_controls[class_name]->clone();
                    gvr.control->set_hit_word(hit_words);
					gvr.control->set_data(gvr.object_data);
				}
				else {
                    system_monitoring_interface::active_mon->log_warning(std::format("items_view: control for class '{0}' not found", class_name), __FILE__, __LINE__);
				}
				rows.push_back(gvr);
			}

			arrange(this, &bounds);
			if (keep_position_on_set_data == false) {
				selected_item_index = 0;
			}
			check_scroll();
			create_controls();
	
			return true;
		}

		virtual json get_items()
		{
			return data;
		}

		virtual void key_down(int _key)
		{
			switch (_key) {
			case VK_BACK:
				break;
			case VK_UP:
				line_up();
				break;
			case VK_DOWN:
				line_down();
				break;
			case VK_PRIOR:
				page_up();
				break;
			case VK_NEXT:
				page_down();
				break;
			case VK_HOME:
				home();
				break;
			case VK_END:
				end();
				break;
			case VK_DELETE:
				delete_selected();
				break;
			case VK_RETURN:
				navigate_selected();
				break;
			}
		}

		void line_down()
		{
			selected_item_index++;
			check_scroll();
		}

		void line_up()
		{
			selected_item_index--;
			check_scroll();
		}

		void page_up()
		{
			selected_page_index--;
			if (selected_page_index < 0)
				selected_page_index = 0;
			selected_item_index = pages[selected_page_index].start_index;
			check_scroll();
		}

		void page_down()
		{
			selected_page_index++;
			if (selected_page_index >= pages.size())
				selected_page_index = pages.size() - 1;
			selected_item_index = pages[selected_page_index].start_index;
			check_scroll();
		}

		void home()
		{
			selected_item_index = 0;
			check_scroll();
		}

		void end()
		{
			selected_item_index = data.size() - 1;
			check_scroll();
		}

		void navigate_selected()
		{
			;
		}

		void delete_selected()
		{
			;
		}

		virtual void key_up(key_up_event evt) override
		{

		}

		virtual void key_down(key_down_event evt) override
		{
            key_down(evt.key);
		}

		virtual void key_press(key_press_event evt) override
		{

		}

		virtual json get_selected_object()
		{
			json j;
			if (selected_item_index >= 0 && data.array()) {
				j = data.get_element(selected_item_index);
			}
			return j;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			draw_control::get_json(_dest);

			if (sources) {
				sources->get_json(_dest);
			}

			if (select_command) {
				json jcommand = jp.create_object();
				corona::get_json(jcommand, select_command);
				_dest.put_member("select_command", jcommand);
			}

			if (empty_command) {
				json jcommand = jp.create_object();
				corona::get_json(jcommand, empty_command);
				_dest.put_member("empty_command", jcommand);
			}

			corona::get_json("selection_border", _dest, selection_border);
			corona::get_json("focused_border", _dest, focused_border);
			corona::get_json("scroll_knob", _dest, scroll_knob);
			corona::get_json("scroll_knob_border", _dest, scroll_knob_border);
			corona::get_json("scroll_knob_selected", _dest, scroll_knob_selected);
			corona::get_json("scroll_knob_border_selected", _dest, scroll_knob_border_selected);
			corona::get_json("scroll_knob_well", _dest, scroll_knob);
			corona::get_json("scroll_knob_well_border", _dest, scroll_knob_selected);
			_dest.put_member("empty_text", empty_text);

		}

		virtual void put_json(json& _src)
		{
			draw_control::put_json(_src);

            empty_text = _src["empty_text"].as_string();

			if (_src.has_member("sources")) {
				sources = std::make_shared<corona_class_page_map>();
				sources->put_json(_src);
			}

			json command = _src["select_command"];
			if (command.object()) {
				// select command is loaded through the reference.
				corona::put_json(select_command, command);
			}

			command = _src["empty_command"];
			if (command.object()) {
				// select command is loaded through the reference.
				corona::put_json(empty_command, command);
			}

			if (_src.has_member("selection_border")) {
				corona::put_json("selection_border", selection_border, _src);
			}

			if (_src.has_member("focused_border")) {
				corona::put_json("focused_border", focused_border, _src);
			}

			if (_src.has_member("scroll_knob")) {
				corona::put_json("scroll_knob", scroll_knob, _src);
			}

			if (_src.has_member("scroll_knob_border")) {
				corona::put_json("scroll_knob_border", scroll_knob_border, _src);
			}

			if (_src.has_member("scroll_knob_selected")) {
				corona::put_json("scroll_knob_selected", scroll_knob_selected, _src);
			}

			if (_src.has_member("scroll_knob_border_selected")) {
				corona::put_json("scroll_knob_border_selected", scroll_knob_border_selected, _src);
			}

			if (_src.has_member("scroll_knob_well")) {
				corona::put_json("scroll_knob_well", scroll_knob, _src);
			}

			if (_src.has_member("scroll_knob_well_border")) {
				corona::put_json("scroll_knob_well_border", scroll_knob_selected, _src);
			}

		}

	};

	class row_view_layout :
		public row_layout
	{
	protected:
	public:
		row_view_layout() { ; }
		row_view_layout(const row_view_layout& _src) = default;
		row_view_layout(control_base* _parent, int _id) : row_layout(_parent, _id) { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<row_view_layout>(*this);
			return tv;
		}

		virtual ~row_view_layout() { ; }
	};

	class absolute_view_layout :
		public absolute_layout
	{
	protected:
	public:
		absolute_view_layout() { ; }
		absolute_view_layout(const absolute_view_layout& _src) = default;
		absolute_view_layout(control_base* _parent, int _id) : absolute_layout(_parent, _id) { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<absolute_view_layout>(*this);
			return tv;
		}

		virtual ~absolute_view_layout() { ; }
	};

	point row_layout::get_remaining(control_base* _parent)
	{
		return remaining;
	}

	point column_layout::get_remaining(control_base* _parent)
	{
		return remaining;
	}

	void absolute_layout::arrange(control_base* _parent, rectangle* _bounds)
	{
		control_base::arrange(_parent, _bounds);

		point origin = { bounds.x, bounds.y, 0.0 };
		remaining = { bounds.w, bounds.h, 0.0 };

		for (auto child : children) {
			auto sz = child->get_size(this);
            point item_origin = origin;
			point item_position = child->get_position(this);
            item_origin.x += item_position.x;
            item_origin.y += item_position.y;
            rectangle item_bounds = { item_origin.x, item_origin.y, sz.x, sz.y };
			child->arrange(this, &item_bounds);
		}
	}

	void row_layout::arrange(control_base* _parent, rectangle* _bounds)
	{
		if (!children.size())
			return;

		control_base::arrange(_parent, _bounds);

		switch (content_alignment)
		{
			case visual_alignment::align_far:
				arrange_far(_parent, &bounds);
                break;
            case visual_alignment::align_center:
				arrange_center(_parent, &bounds);
                break;
            case visual_alignment::align_near:	
				arrange_near(_parent, &bounds);
				break;
		}
	}

	void row_layout::calculate_remaining()
	{
		int start = children.size() - 1;
		int end = 0;

		if (wrap) {
			remaining = { inner_bounds.w, inner_bounds.h, 0.0 };
		}
		else
		{
			point temp_remaining = { inner_bounds.w, inner_bounds.h, 0.0 };

			for (int i = start; i >= end; i--)
			{
				auto child = children[i];
				if (child->box.width.units != measure_units::percent_remaining) {
					auto child_size = child->get_size(this);
					temp_remaining.x -= child_size.x;
				}
			}

            if (temp_remaining.x < 0)
			{
				temp_remaining.x = 0;
			}

			remaining = temp_remaining;
		}
	}

	void row_layout::arrange_far(control_base* _parent, rectangle* _bounds)
	{
		point current_position = { inner_bounds.right(), inner_bounds.y, 0.0 };

		int start = children.size() - 1;
		int end = 0;
		double max_height = 0.0;

		calculate_remaining();

		for (int i = start; i >= end; i--)
		{
			auto child = children[i];
			auto child_size = child->get_size(this);

			if (wrap and (current_position.x < inner_bounds.x))
			{
				current_position.x = inner_bounds.right();
				current_position.y += max_height + to_pixels_y(this, item_next_space);
				max_height = 0.0;
			}

			current_position.x -= child_size.x;

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

            if (child_size.y > max_height)
			{
				max_height = child_size.y;
			}

			child->arrange(this, &child_bounds);
			current_position.x -= to_pixels_x(this, item_start_space);
		}

	}

	void row_layout::arrange_center(control_base* _parent, rectangle* _bounds)
	{
		int start = 0;
		int end = children.size() - 1;

		calculate_remaining();

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		int start_row_index = 0;
		int last_index = 0;
		double max_height = 0.0;

		for (int i = start; i <= end; i++)
		{
			auto child = children[i];
			auto child_size = child->get_size(this);

			if (wrap and (current_position.x > inner_bounds.right()))
			{
				double last_x = inner_bounds.x;

                last_x = children[last_index]->get_bounds().right();

                double line_center = (inner_bounds.right() - last_x) / 2.0;

                for (int j = start_row_index; j <= i; j++)
				{
                    auto row_child = children[j];
					rectangle child_bounds = row_child->get_bounds();
                    child_bounds.x += line_center;
					row_child->arrange(_parent, &child_bounds);
				}

				current_position.x = inner_bounds.x;
				current_position.y += max_height + to_pixels_y(this, item_next_space);
				last_index = i;
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->arrange(this, &child_bounds);
			current_position.x += child_size.x;
			current_position.x += to_pixels_x(this, item_start_space);
		}

		if (last_index <= end)
		{
			double last_x = inner_bounds.x;

			last_x = children[end]->get_bounds().right();
			double line_center = (inner_bounds.right() - last_x) / 2.0;

			for (int j = last_index; j <= end; j++)
			{
				auto row_child = children[j];
				rectangle child_bounds = row_child->get_bounds();
				child_bounds.x += line_center;
				row_child->arrange(_parent, &child_bounds);
			}
		}
	}

	void row_layout::arrange_near(control_base* _parent, rectangle *_bounds)
	{
		control_base::arrange(_parent, _bounds);

		calculate_remaining();

        point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		double max_height = 0;

        for (auto child : children) 
		{
			auto child_size = child->get_size(this);

			// if the element is too big
			if (wrap and current_position.x > inner_bounds.right()) 
			{
				current_position.x = inner_bounds.x;
				current_position.y += max_height + to_pixels_y(this, item_next_space);
			}

			rectangle child_bounds = { 
				current_position.x, 
				current_position.y, 
				child_size.x, 
				child_size.y 
            };

			if (child_size.y > max_height)
			{
				max_height = child_size.y;
			}

            child->arrange(this, &child_bounds);
			current_position.x += child_size.x;
			current_position.x += to_pixels_x(this, item_start_space);
		}
	}

	void column_layout::arrange(control_base *_base, rectangle* _bounds)
	{
		control_base::arrange(_base, _bounds);

		if (!children.size())
			return;

		switch (content_alignment)
		{
		case visual_alignment::align_far:
			arrange_far(_base, &bounds);
			break;
		case visual_alignment::align_center:
			arrange_center(_base, &bounds);
			break;
		case visual_alignment::align_near:
			arrange_near(_base, &bounds);
			break;
		}
	}

	void column_layout::calculate_remaining()
	{
		int start = children.size() - 1;
		int end = 0;

		if (wrap) {
			remaining = { inner_bounds.w, inner_bounds.h, 0.0 };
		}
		else
		{
			point temp_remaining = { inner_bounds.w, inner_bounds.h, 0.0 };

			for (int i = start; i >= end; i--)
			{
				auto child = children[i];
				if (child->box.height.units != measure_units::percent_remaining) {
					auto child_size = child->get_size(this);
					temp_remaining.y -= child_size.y;
				}
			}

			if (temp_remaining.y < 0)
			{
				temp_remaining.y = 0;
			}

			remaining = temp_remaining;
		}
	}

	void column_layout::arrange_far(control_base *_parent, rectangle* _bounds)
	{
		point current_position = { inner_bounds.x, inner_bounds.bottom(), 0.0 };

		int start = children.size() - 1;
		int end = 0;

		calculate_remaining();

		double max_width = 0.0;

		for (int i = start; i >= end; i--)
		{
			auto child = children[i];

			auto child_size = child->get_size(this);

			if (wrap and (current_position.x < inner_bounds.x))
			{
				current_position.x += max_width + to_pixels_x(this, item_next_space);
				current_position.y = inner_bounds.bottom();
				max_width = 0;
			}

            current_position.y -= child_size.y;

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

            if (child_size.x > max_width)
			{
				max_width = child_size.x;
			}

			child->arrange(this, &child_bounds);
			current_position.y -= to_pixels_y(this, item_start_space);
		}

	}

	void column_layout::arrange_center(control_base *_parent, rectangle* _bounds)
	{
		int start = 0;
		int end = children.size() - 1;

		calculate_remaining();

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		int start_row_index = 0;
		int last_index = 0;

		for (int i = start; i <= end; i++)
		{
			auto child = children[i];
			auto child_size = child->get_size(this);

			if (wrap and (current_position.y > inner_bounds.bottom()))
			{
				double last_y = inner_bounds.y;

				if (last_index >= 0)
				{
					last_y = children[last_index]->get_bounds().bottom();
				}

				double line_center = (inner_bounds.bottom() - last_y) / 2.0;

				for (int j = start_row_index; j < i; j++)
				{
					auto row_child = children[j];
					rectangle child_bounds = row_child->get_bounds();
					child_bounds.y += line_center;
					row_child->arrange(this, &child_bounds);
				}

				current_position.y = inner_bounds.y;
				current_position.y += to_pixels_y(this, item_next_space);
				last_index = i;
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->arrange(this, &child_bounds);
			current_position.y += child_size.y;
			current_position.y += to_pixels_y(this, item_start_space);
		}

		if (last_index <= end)
		{
			double last_y = children[last_index]->get_bounds().bottom();

			double line_center = (inner_bounds.bottom() - last_y) / 2.0;

			for (int j = last_index; j <= end; j++)
			{
				auto row_child = children[j];
				rectangle child_bounds = row_child->get_bounds();
				child_bounds.y += line_center;
				row_child->arrange(this, &child_bounds);
			}
		}
	}

	void column_layout::arrange_near(control_base* _parent, rectangle* _bounds)
	{
		control_base::arrange(_parent, _bounds);

		calculate_remaining();

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		for (auto child : children)
		{
			auto child_size = child->get_size(this);

			if (wrap and (current_position.x + child_size.x > inner_bounds.right()))
			{
				current_position.y = inner_bounds.y;
				current_position.y += child_size.y + to_pixels_y(this, item_next_space);
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->arrange(this, &child_bounds);
			current_position.y += child_size.y;
			current_position.y += to_pixels_y(this, item_start_space);
		}
	}

	point frame_layout::get_remaining(control_base* _parent) 
	{
		return remaining;
	}

	void frame_layout::arrange(control_base* _parent, rectangle* _ctx)
	{
		control_base::arrange(_parent, _ctx);
        remaining.x = inner_bounds.w;
        remaining.y = inner_bounds.h;
		arrange_children();
	}

	void frame_layout::arrange_children()
	{
		remaining = { inner_bounds.w, inner_bounds.h, 0.0 };
		for (auto child : children) {
			auto sz = child->get_size(this);
			point item_origin = { bounds.x, bounds.y, 0.0 };
			point item_position = child->get_position(this);
			item_origin.x += item_position.x;
			item_origin.y += item_position.y;
			rectangle item_bounds = { item_origin.x, item_origin.y, sz.x, sz.y };
			child->arrange(this, &item_bounds);
		}
	}

}

