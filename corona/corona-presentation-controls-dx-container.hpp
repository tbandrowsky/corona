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
			parent = nullptr;
			id = id_counter::next();
		}

		container_control(const container_control& _src) = default;

		container_control(container_control_base *_parent, int _id)
		{
			parent = _parent;
			id = _id;
			if (parent and get_nchittest() == HTCLIENT) {
				set_nchittest(parent->get_nchittest());
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

		virtual void set_contents(std::function<void(control_base* _page)> _contents)
		{
			_contents(this);

			for (auto child : children) {
				child->parent = this;
			}

			arrange(&bounds);
		}

		virtual void set_contents(page_base *_contents)
		{
			auto new_root = _contents->root;
			children.clear();

			for (auto srcchild : _contents->root->children)
			{
				auto new_child = srcchild->clone();
				children.push_back(new_child);
			}

			arrange(&bounds);
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
		virtual void arrange(rectangle* _ctx)
		{
			set_bounds(*_ctx);
			for (auto child : children) {
				child->arrange(_ctx);
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
		absolute_layout(container_control_base *_parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~absolute_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<absolute_layout>(*this);
			return tv;
		}

		virtual void arrange(rectangle* _ctx);
	};

	class column_layout :
		public container_control
	{
		virtual void calculate_remaining();
		virtual void arrange_near(rectangle* _ctx);
		virtual void arrange_center(rectangle* _ctx);
		virtual void arrange_far(rectangle* _ctx);

	public:
		layout_rect item_size;
		measure item_start_space;
		measure item_next_space;
		bool	wrap;

		column_layout() : wrap(true) { ; }
		column_layout(const column_layout& _src) : container_control(_src) 
		{
			item_size = _src.item_size;
			item_start_space = _src.item_start_space;
			item_next_space = _src.item_next_space;
			wrap = _src.wrap;
		}
		column_layout(container_control_base* _parent, int _id) : container_control(_parent, _id), wrap(true) { ; }

		virtual ~column_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<column_layout>(*this);
			return tv;
		}

		virtual void arrange(rectangle* _ctx);
		virtual point get_remaining();

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
		virtual void arrange_near(rectangle *_ctx);
		virtual void arrange_center(rectangle *_ctx);
		virtual void arrange_far(rectangle *_ctx);

	public:

		measure item_start_space;
		measure item_next_space;

		layout_rect item_size;
		bool		wrap;

		row_layout() { ; }
		row_layout(const row_layout& _src) = default;
		row_layout(container_control_base* _parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~row_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<row_layout>(*this);
			return tv;
		}

		virtual void arrange(rectangle* _ctx);
		virtual point get_remaining();
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
	public:

		frame_layout() { ; }
		frame_layout(const frame_layout& _src) = default;
		frame_layout(container_control_base* _parent, int _id) : container_control(_parent, _id) { ; }
		virtual ~frame_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<frame_layout>(*this);
			return tv;
		}

		virtual void arrange(rectangle* _ctx);
		virtual point get_remaining();

		virtual void set_contents(presentation_base *_presentation, page_base *_parent_page, page_base* _contents);
		virtual void set_contents(std::function<void(control_base* _page)> _contents)
		{
			container_control::set_contents(_contents);
		}

		virtual void create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host) override
		{
			container_control::create(_context, _host);
		}


	};

	class grid_view_row
	{
	public:
		int item_id;
		int page_index;
		rectangle bounds;
	};

	class grid_view : public draw_control
	{
		array_data_source items_source;
		std::vector<grid_view_row> rows;
		std::map<int,int> page_to_item_index;
		solidBrushRequest selection_border;

		// we keep the set of controls here on the back end, because they are small as they are not dragging around any 
		// back end bitmaps or windows.  (arranging doesn't create the assets on a control, create does)
		rectangle view_port;

		int selected_page_index;
		int selected_item_index;

		void check_scroll()
		{
			if (selected_item_index >= rows.size() - 1)
			{
				selected_item_index = rows.size() - 1;
			}
			if (selected_item_index < 0)
			{
				selected_item_index = 0;
			}
			selected_page_index = rows[selected_item_index].page_index;
			int selected_item_page_index = page_to_item_index[selected_page_index];
			view_port.y = rows[selected_item_page_index].bounds.y;
			std::string msg;
			msg = std::format("selected_page_index '{0}' selected_item_index {1}, y:{2} ", selected_page_index, selected_item_page_index, view_port.y);
			system_monitoring_interface::active_mon->log_information(msg);
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
			on_create = [this](std::shared_ptr<direct2dContext>& _context, draw_control *_src)
				{
					if (items_source.assets)
					{
						items_source.assets(_context, this, bounds);
					}

					_context->setSolidColorBrush(&selection_border);
				};

			on_draw = [this](std::shared_ptr<direct2dContext>& _context, control_base* _item)
				{
					_context->setSolidColorBrush(&selection_border);

					auto draw_bounds = inner_bounds;

					draw_bounds.x = inner_bounds.x;
					draw_bounds.y = inner_bounds.y;

					point offset = { view_port.x, view_port.y };

					auto& context = _context;

					if (not page_to_item_index.contains(selected_page_index )) {
						std::string msg;
						msg = std::format("selected_page_index '{0}' not found", selected_page_index);
						system_monitoring_interface::active_mon->log_warning(msg);
						return;
					}

					int idx = page_to_item_index[ selected_page_index ];

					while (idx < rows.size())
					{
						auto& row = rows[idx];

						auto rect_bounds = row.bounds;
						rect_bounds.x -= offset.x;
						rect_bounds.y -= offset.y;

						if (rect_bounds.y < bounds.bottom()) 
						{
							items_source.draw_item(_context, this, idx, items_source.data, rect_bounds);
							if (selected_item_index == idx) 
							{
								context->drawRectangle(&rect_bounds, selection_border.name, 4, nullptr);
							}
							context->drawText("Test", &rect_bounds, "" ,selection_border.name);
						}
						else 
						{
							break;
						}

						idx++;
					}
				};

		}

	public:

		grid_view()
		{
			view_port = {};
			selected_item_index = 0;
			selected_page_index = 0;
			set_selection_border("#000000");
			init();
		}

		grid_view(container_control_base* _parent, int _id) : draw_control(_parent, _id)
		{
			view_port = {};
			selected_item_index = 0;
			selected_page_index = 0;
			set_selection_border("#000000");
			init();
		}

		grid_view(const grid_view& _src) = default;

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<grid_view>(*this);
			return tv;
		}

		virtual ~grid_view()
		{
			;
		}

		virtual void arrange(rectangle* _bounds)
		{
			control_base::arrange(_bounds);

			view_port.w = _bounds->w;
			view_port.h = _bounds->h;

			if (_bounds->h == 0 or _bounds->w == 0) 
			{
				return;
			}

			double h = _bounds->h;
			double y = 0;

			int page_index;
			page_to_item_index.clear();

			int sz = rows.size();
			int i;
			for (i = 0; i < sz; i++)
			{
				json temp;
				auto isz = items_source.size_item(this, i, temp, *_bounds);
				auto& r = rows[i];
				r.bounds.x = 0;
				r.bounds.y = y;
				r.bounds.w = _bounds->w;
				r.bounds.h = isz.y;
				page_index = (r.bounds.bottom() / h);
				r.page_index = page_index;
				if (not page_to_item_index.contains(page_index))
				{
					page_to_item_index[page_index] = i;
				}
				y += isz.y;
			}
		}

		void set_item_source(array_data_source _item_source)
		{
			items_source = _item_source;
			rows.clear();

			int i;
			rectangle item_bounds;
			item_bounds.x = 0;
			item_bounds.y = 0;
			item_bounds.w = 0;
			item_bounds.h = 0;

			for (i = 0; i < items_source.data.size(); i++)
			{
				grid_view_row gvr;
				gvr.page_index = 0;
				gvr.bounds = item_bounds;
				gvr.item_id = i;
				rows.push_back(gvr);
			}

			arrange(&bounds);

			if (rows.size() <= selected_item_index)
			{
				selected_item_index = 0;
				check_scroll();
			}

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
			selected_item_index = page_to_item_index[selected_page_index];
			check_scroll();
		}

		void page_down()
		{
			selected_page_index++;
			if (selected_page_index >= page_to_item_index.size())
				selected_page_index = page_to_item_index.size() - 1;
			selected_item_index = page_to_item_index[selected_page_index];
			check_scroll();
		}

		void home()
		{
			selected_item_index = 0;
			check_scroll();
		}

		void end()
		{
			selected_item_index = items_source.data.size() - 1;
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

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			_page->on_key_down(id, [](key_down_event evt)
				{
					grid_view* gv = dynamic_cast<grid_view *>(evt.control);
					if (gv) {
						gv->key_down(evt.key);
					}
				});
			for (auto child : children) {
				child->on_subscribe(_presentation, _page);
			}
		}

		virtual json get_selected_object()
		{
			json j;
			if (selected_item_index >= 0) {
				j = items_source.data.get_element(selected_item_index);
			}
			return j;
		}

		virtual bool set_items(json _data)
		{
			items_source.data = _data;
			set_item_source(items_source);
			return true;
		}


	};

	class row_view_layout :
		public row_layout
	{
	protected:
	public:
		row_view_layout() { ; }
		row_view_layout(const row_view_layout& _src) = default;
		row_view_layout(container_control_base* _parent, int _id) : row_layout(_parent, _id) { ; }

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
		absolute_view_layout(container_control_base* _parent, int _id) : absolute_layout(_parent, _id) { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<absolute_view_layout>(*this);
			return tv;
		}

		virtual ~absolute_view_layout() { ; }
	};

	point row_layout::get_remaining()
	{
		return remaining;
	}

	point column_layout::get_remaining()
	{
		return remaining;
	}

	void absolute_layout::arrange(rectangle* _bounds)
	{
		control_base::arrange(_bounds);

		point origin = { bounds.x, bounds.y, 0.0 };
		remaining = { bounds.w, bounds.h, 0.0 };

		for (auto child : children) {
			auto sz = child->get_size();
            point item_origin = origin;
			point item_position = child->get_position();
            item_origin.x += item_position.x;
            item_origin.y += item_position.y;
            rectangle item_bounds = { item_origin.x, item_origin.y, sz.x, sz.y };
			child->arrange(&item_bounds);
		}
	}

	void row_layout::arrange(rectangle* _bounds)
	{
		control_base::arrange(_bounds);

		switch (content_alignment)
		{
			case visual_alignment::align_far:
				arrange_far(_bounds);
                break;
            case visual_alignment::align_center:
				arrange_center(_bounds);
                break;
            case visual_alignment::align_near:	
				arrange_near(_bounds);
				break;
		}
	}

	void row_layout::calculate_remaining()
	{
		int start = children.size() - 1;
		int end = 0;

		if (wrap) {
			remaining = { inner_bounds.h, inner_bounds.w, 0.0 };
		}
		else
		{
			point temp_remaining = { inner_bounds.w, inner_bounds.h, 0.0 };

			for (int i = start; i >= end; i--)
			{
				auto child = children[i];
				auto child_size = child->get_size();
				temp_remaining.x -= child_size.x;
			}

			remaining = temp_remaining;
		}
	}

	void row_layout::arrange_far(rectangle* _bounds)
	{
		point current_position = { inner_bounds.right(), inner_bounds.y, 0.0 };

		int start = children.size() - 1;
		int end = 0;
		double max_height = 0.0;

		calculate_remaining();

		for (int i = start; i >= end; i--)
		{
			auto child = children[i];
			auto child_size = child->get_size();

			if (wrap and (current_position.x < inner_bounds.x))
			{
				current_position.x = inner_bounds.right();
				current_position.y += max_height + to_pixels_y(item_next_space);
				max_height = 0.0;
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

			child->set_bounds(child_bounds);
			current_position.x -= child_size.x;
			current_position.x -= to_pixels_x(item_start_space);
		}

	}

	void row_layout::arrange_center(rectangle* _bounds)
	{
		int start = 0;
		int end = children.size() - 1;

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		int start_row_index = 0;
		int last_index = 0;
		double max_height = 0.0;

		for (int i = start; i < end; i++)
		{
			auto child = children[i];
			auto child_size = child->get_size();

			if (wrap and (current_position.x > inner_bounds.right()))
			{
				double last_x = inner_bounds.x;

				if (last_index >= 0) 
				{
                    last_x = children[last_index]->get_bounds().right();
				}

                double line_center = (inner_bounds.right() - last_x) / 2.0;

                for (int j = start_row_index; j < i; j++)
				{
                    auto row_child = children[j];
					rectangle child_bounds = row_child->get_bounds();
                    child_bounds.x += line_center;
					row_child->set_bounds(child_bounds);
				}

				current_position.x = inner_bounds.x;
				current_position.y += max_height + to_pixels_y(item_next_space);
				last_index = i;
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->set_bounds(child_bounds);
			current_position.x += child_size.x;
			current_position.x += to_pixels_x(item_start_space);
		}

		if (wrap && last_index < end)
		{
			double last_x = inner_bounds.x;

			if (last_index >= 0)
			{
				last_x = children[last_index]->get_bounds().right();
			}

			double line_center = (inner_bounds.right() - last_x) / 2.0;

			for (int j = last_index; j < end; j++)
			{
				auto row_child = children[j];
				rectangle child_bounds = row_child->get_bounds();
				child_bounds.x += line_center;
				row_child->set_bounds(child_bounds);
			}
		}
	}

	void row_layout::arrange_near(rectangle *_bounds)
	{
		control_base::arrange(_bounds);

        point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		double max_height = 0;

        for (auto child : children) 
		{
			auto child_size = child->get_size();

			// if the element is too big
			if (wrap and current_position.x > inner_bounds.right()) 
			{
				current_position.x = inner_bounds.x;
				current_position.y += max_height + to_pixels_y(item_next_space);
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

            child->set_bounds(child_bounds);
			current_position.x += child_size.x;
			current_position.x += to_pixels_x(item_start_space);
		}
	}

	void column_layout::arrange(rectangle* _bounds)
	{
		control_base::arrange(_bounds);

		switch (content_alignment)
		{
		case visual_alignment::align_far:
			arrange_far(_bounds);
			break;
		case visual_alignment::align_center:
			arrange_center(_bounds);
			break;
		case visual_alignment::align_near:
			arrange_near(_bounds);
			break;
		}
	}

	void column_layout::calculate_remaining()
	{
		int start = children.size() - 1;
		int end = 0;

		if (wrap) {
			remaining = { inner_bounds.h, inner_bounds.w, 0.0 };
		}
		else
		{
			point temp_remaining = { inner_bounds.w, inner_bounds.h, 0.0 };

			for (int i = start; i >= end; i--)
			{
				auto child = children[i];
				auto child_size = child->get_size();
				temp_remaining.y -= child_size.y;
			}

			remaining = temp_remaining;
		}
	}

	void column_layout::arrange_far(rectangle* _bounds)
	{
		point current_position = { inner_bounds.x, inner_bounds.bottom(), 0.0 };

		int start = children.size() - 1;
		int end = 0;

		calculate_remaining();

		for (int i = start; i >= end; i--)
		{
			auto child = children[i];

			auto child_size = child->get_size();

			if (wrap and (current_position.x < inner_bounds.x))
			{
				current_position.x += child_size.x + to_pixels_x(item_next_space);
				current_position.y = inner_bounds.bottom();
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->set_bounds(child_bounds);
			current_position.y += child_size.y;
			current_position.y += to_pixels_y(item_start_space);
		}

	}

	void column_layout::arrange_center(rectangle* _bounds)
	{
		int start = 0;
		int end = children.size() - 1;

		calculate_remaining();

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		int start_row_index = 0;
		int last_index = 0;

		for (int i = start; i < end; i++)
		{
			auto child = children[i];
			auto child_size = child->get_size();

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
					row_child->set_bounds(child_bounds);
				}

				current_position.y = inner_bounds.y;
				current_position.x += child_size.x + to_pixels_x(item_next_space);
				last_index = i;
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->set_bounds(child_bounds);
			current_position.y += child_size.y;
			current_position.y += to_pixels_y(item_start_space);
		}

		if (wrap && last_index < end)
		{
			double last_x = inner_bounds.x;

			if (last_index >= 0)
			{
				last_x = children[last_index]->get_bounds().right();
			}

			double line_center = (inner_bounds.right() - last_x) / 2.0;

			for (int j = last_index; j < end; j++)
			{
				auto row_child = children[j];
				rectangle child_bounds = row_child->get_bounds();
				child_bounds.y += line_center;
				row_child->set_bounds(child_bounds);
			}
		}
	}

	void column_layout::arrange_near(rectangle* _bounds)
	{
		control_base::arrange(_bounds);

		calculate_remaining();

		point current_position = { inner_bounds.x, inner_bounds.y, 0.0 };

		for (auto child : children)
		{
			auto child_size = child->get_size();

			if (wrap and (current_position.x + child_size.x > inner_bounds.right()))
			{
				current_position.y = inner_bounds.y;
				current_position.y += child_size.y + to_pixels_y(item_next_space);
			}

			rectangle child_bounds = {
				current_position.x,
				current_position.y,
				child_size.x,
				child_size.y
			};

			child->set_bounds(child_bounds);
			current_position.y += child_size.x;
			current_position.y += to_pixels_x(item_start_space);
		}
	}

	point frame_layout::get_remaining()
	{
		return remaining;
	}
}

