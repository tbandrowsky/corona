/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved


MIT License

About this File
This is the base class(es) for a control in a presentation.

Notes

For Future Consideration
*/


#pragma once

namespace corona
{
	class applicationBase;

	class row_layout;
	class column_layout;
	class absolute_layout;
	class frame_layout;
	class row_view_layout;
	class grid_view;
	class absolute_view_layout;
	class single_column_form;
	class double_column_form;

	class title_control;
	class subtitle_control;
	class chaptertitle_control;
	class chaptersubtitle_control;
	class paragraph_control;
	class code_control;
	class label_control;
	class image_control;

	class static_control;
	class pushbutton_control;
	class pressbutton_control;
	class checkbox_control;
	class radiobutton_control;
	class linkbutton_control;
	class listbox_control;
	class combobox_control;
	class edit_control;
	class scrollbar_control;
	class listview_control;
	class treeview_control;
	class header_control;
	class toolbar_control;
	class statusbar_control;
	class hotkey_control;
	class animate_control;
	class richedit_control;
	class draglistbox_control;
	class comboboxex_control;
	class datetimepicker_control;
	class monthcalendar_control;
	class control_base;

	class minimize_button_control;
	class maximize_button_control;
	class close_button_control;
	class menu_button_control;

	class caption_bar_control;
	class status_bar_control;

	class camera_control;

	template <typename T> class change_monitored_property
	{
		T last_value;
		T current_value;
		bool is_changed;

	public:

		change_monitored_property()
		{
			current_value = {};
			last_value = {};
			is_changed = false;
		}

		change_monitored_property(T _default_value)
		{
			current_value = _default_value;
			last_value = _default_value;
			is_changed = false;
		}

		virtual ~change_monitored_property()
		{
			;
		}

		operator T ()
		{
			return current_value;
		}

		T operator = (T _new_value)
		{
			is_changed = (_new_value != current_value);
			last_value = current_value;
			current_value = _new_value;
			return current_value;
		}

		void reset(T _value)
		{
			is_changed = false;
			last_value = current_value;
			current_value = _value;
			return current_value;
		}

		T& value()
		{
			return current_value;
		}

		bool changed_from(T _from_value)
		{
			bool really_changed = (is_changed &&
				_from_value == current_value);
			return really_changed;
		}

		bool changed_to(T _to_value)
		{
			bool really_changed = (is_changed &&
				_to_value == current_value);
			return really_changed;
		}

		bool changed(T _from_value, T _to_value)
		{
			bool really_changed = (is_changed &&
				_from_value == last_value &&
				_to_value == current_value);
			return really_changed;
		}
	};

	class container_control_base : public json_serializable
	{
	public:
		container_control_base()
		{
			;
		}

		virtual ~container_control_base()
		{
			;
		}

		virtual control_base* get_control() = 0;
		virtual LRESULT get_nchittest() = 0;
	};

	template <typename T> std::shared_ptr<control_base> clone(T* _src, container_control_base* _parent)
	{
		std::shared_ptr<T> cb = std::make_shared<T>(*_src);
		return cb;
	}

	class control_base : public container_control_base
	{


	protected:

		rectangle				arrange_extent = {};
		rectangle				bounds = {};
		rectangle				inner_bounds = {};
		point					margin_amount = {};
		point					padding_amount = {};

		change_monitored_property<bool> mouse_over;
		change_monitored_property<bool> mouse_left_down;
		change_monitored_property<bool> mouse_right_down;
		point		 mouse_relative_position = {};
		std::string  name = {};

		void copy(const control_base& _src)
		{
			id = _src.id;
			bounds = _src.bounds;
			inner_bounds = _src.inner_bounds;
			margin_amount = _src.margin_amount;
			padding_amount = _src.padding_amount;
			class_name = _src.class_name;
			name = _src.name;

			mouse_over = _src.mouse_over;
			mouse_left_down = _src.mouse_left_down;
			mouse_right_down = _src.mouse_right_down;
			mouse_relative_position = _src.mouse_relative_position;

			arrange_extent = _src.arrange_extent;
			box = _src.box;
			margin = _src.margin;
			padding = _src.padding;
			tooltip_text = _src.tooltip_text;

			children.clear();
			for (auto child : _src.children) {
				auto new_child = child->clone();
				children.push_back(new_child);
			}
			name = _src.name;
			wrap_break = _src.wrap_break;
            json_field_name = _src.json_field_name;
		}

	public:

		friend class draw_control;
		friend class row_layout;
		friend class column_layout;
		friend class absolute_layout;
		friend class frame_layout;
		friend class tab_view_control;
		friend class grid_view;

		int						id = -1;

		layout_rect				box = {};
		measure					margin = {};
		measure					padding = {};
		std::string				tooltip_text = {};
		bool					is_focused = false;
		std::string				json_field_name = {};
		std::string				class_name = {};
		bool					wrap_break = false;

		std::vector<std::shared_ptr<control_base>> children;

		LRESULT hittest = HTCLIENT;

		virtual LRESULT get_nchittest() { return hittest; }
		virtual void set_nchittest(LRESULT _hittest) { hittest = _hittest; }

		control_base() 
		{
			id = id_counter::next();
			is_focused = false;
			wrap_break = false;
		}

		control_base(int _id) 
		{
			if (_id < 0) {
				id = id_counter::next();
			}
			else {
				id = _id;
			}
			is_focused = false;
			wrap_break = false;
		}

		control_base(const control_base& _src)
		{
			copy(_src);
		}

		virtual std::string form_name() { return "no form"; }

		virtual point get_size(control_base* _parent);
		virtual point get_remaining(control_base* _parent);
		virtual point get_position(control_base* _parent);

		double to_pixels_x(control_base* _parent, measure _margin);
		double to_pixels_y(control_base* _parent, measure _margin);
		virtual void on_resize();

		bool is_mouse_over()
		{
			return mouse_over;
		}

		bool is_mouse_left_down()
		{
			return mouse_left_down;
		}

		bool is_wrap_break()
		{
			return wrap_break;
		}

		point get_relative_mouse_position()
		{
			return mouse_relative_position;
		}

		int get_id() { return id;  }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<control_base>(*this);
			return tv;
		}

		virtual ~control_base()
		{
			;
		}

		virtual bool gets_real_focus() 
		{ 
			return false; 
		}

		virtual bool set_focus()
		{
			is_focused = true;
			return false;
		}
		 
		virtual bool kill_focus()
		{
			is_focused = false;
			return false;
		}

		virtual bool is_control_message(int _key)
		{
			return false;
		}

		template <typename control_type> std::shared_ptr<control_type> create(int _id)
		{
			std::shared_ptr<control_type> temp;
			temp = std::make_shared<control_type>(this, _id);
			if (temp) {
				children.push_back(temp);
//				std::cout << " " << typeid(*this).name() << " ->create:" << typeid(control_type).name() << std::endl;
			}
			apply_item_sizes(*temp);
			return temp;
		}

		virtual control_base* get_control()
		{
			return this;
		}

		virtual json get_data()
		{
			json_parser jp;
			json data = jp.create_object();
			for (auto child : children) {
				json child_data = child->get_data();
				if (not child_data.empty()) {
					data.copy_member(child->json_field_name, child_data);
				}
			}
			return data;
		}

		virtual bool set_items(json _data)
		{
			return false;
		}

		virtual json set_data(json _data)
		{
			for (auto child : children) {
				if (child->json_field_name.empty()) 
				{
					child->set_data(_data);
				}
				else 
				{
					child->set_data(_data);
				}
			}
			return _data;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;

			json jbox, jmargin, jpadding, jbounds, jinner_bounds, jcomputed, jissues;

			jbox = jp.create_object();
			jmargin = jp.create_object();
			jpadding = jp.create_object();

			jbounds = jp.create_object();
			jinner_bounds = jp.create_object();
			jcomputed = jp.create_object();
			jissues = jp.create_array();

			corona::get_json(jbox, box);
			corona::get_json(jpadding, padding);
			corona::get_json(jmargin, margin);

			corona::get_json(jinner_bounds, inner_bounds);
			corona::get_json(jbounds, bounds);
			jcomputed.put_member("inner_bounds", jinner_bounds);
			jcomputed.put_member("bounds", jbounds);

			if (bounds.w == 0 or bounds.h == 0) {
				jissues.push_back("item sized empty");
			}

			_dest.put_member("name", name);
			_dest.put_member("class_name", class_name);
			_dest.put_member("id", id );
			_dest.put_member("box", jbox );
			_dest.put_member("padding", jpadding);
			_dest.put_member("margin", jmargin);
			_dest.put_member("tooltip_text", tooltip_text);
			_dest.put_member("json_field_name", json_field_name);
			_dest.put_member("computed", jcomputed);
			_dest.put_member("issues", jissues);
			_dest.put_member("wrap_break", wrap_break);

		}

		virtual void put_json(json& _src)
		{
			json jbox, jmargin, jpadding;

			std::vector<std::string> missing;

			name = _src["name"].as_string();
			class_name = _src["class_name"].as_string();

			if (not _src.has_members(missing, { "box" })) {
				system_monitoring_interface::active_mon->log_warning(std::format( "control '{0}/{1}' is missing:", class_name, name), __FILE__, __LINE__);
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				return;
			}

			jbox = _src["box"];
			jmargin = _src["margin"];
			jpadding = _src["padding"];

			corona::put_json(box, jbox);
			corona::put_json(margin, jmargin);
			corona::put_json(padding, jpadding);

			tooltip_text = _src["tooltip_text"].as_string();
			json_field_name = _src["json_field_name"].as_string();

			wrap_break = _src["wrap_break"].as_bool();

		}

		virtual json get_selected_object()
		{
			json j;
			get_json(j);
			return j;
		}

		rectangle& get_bounds() { return bounds; }
		rectangle& get_inner_bounds() { return inner_bounds; }

		point& get_margin_amount() { return margin_amount; }
		point& get_padding_amount() { return padding_amount; }

		rectangle& set_bounds(control_base *_parent, rectangle& _bounds, bool _clip_children = true);

		virtual void arrange(control_base *_parent, rectangle* _ctx);
		bool contains(point pt);

		control_base* find(int _id);
		control_base* find(std::string _name);
		control_base* find(point p);
		control_base* get(control_base* _root, int _id);
		std::pair<control_base*, control_base*> get_pair(int _id);
		control_base* get(control_base* _root, std::string _name);

		template <typename control_type> std::shared_ptr<control_type> find_by_id(int _id)
		{
			std::shared_ptr<control_type> ptr;

			control_base* root = (control_base*)this;
			for (auto cntrl : children) {
				if (cntrl->id == _id)
				{
					ptr = std::dynamic_pointer_cast<control_type>(cntrl);
				}
				else
				{
					ptr = cntrl->find_by_id<control_type>(_id);
				}
				if (ptr) {
					return ptr;
				}
			}
			return ptr;
		}

		virtual double get_font_size() { return 14; }

		template <typename control_type> control_type& find(int _id)
		{
			control_base* temp = find(_id);
			if (not temp) {
				throw std::invalid_argument(std::format("Control id {0} not found", _id));
			}
			control_type* citem = dynamic_cast<control_type *>(temp);
			if (not citem) {
				throw std::invalid_argument(std::format("Control id {0} is a {1}.", _id, typeid(*temp).name()));
			}
			return *citem;
		}

		template <typename control_type> control_type& get(control_base* _root, int _id)
		{
			control_base* temp = get(_root, _id);
			control_type* citem = dynamic_cast<control_type *>(temp);
			return *citem;
		}

		void foreach(std::function<void(control_base* _root)> _item);
		std::pair<control_base*, control_base *> find_if(std::function<bool(control_base* _root)> _item);

		virtual void create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host);
		virtual void destroy();
		virtual void draw(std::shared_ptr<direct2dContext>& _context);
		virtual void render(std::shared_ptr<direct2dContext>& _context);

		// these all need to trigger a layout from the parent

		control_base& set_origin(measure _x, measure _y)
		{
			box.x = _x;
			box.y = _y;
			return *this;
		}

		control_base& set_size(std::string _str, measure _height)
		{
			box.width = measure( _str.size(), measure_units::text );
			box.height = _height;
			return *this;
		}

		control_base& set_size(measure _width, measure _height)
		{
			box.width = _width;
			box.height = _height;
			return *this;
		}

		control_base& set_size(layout_rect _new_layout)
		{
			box.width = _new_layout.width;
			box.height = _new_layout.height;
			return *this;
		}

		control_base& set_box(layout_rect _new_layout)
		{
			box = _new_layout;
			return *this;
		}

		control_base& set_padding(measure _space)
		{
			padding = _space;
			return *this;
		}

		control_base& set_margin(measure _space)
		{
			margin = _space;
			return *this;
		}

		template <typename control_class> control_class& set_spacing(measure _spacing)
		{
			control_class* r = dynamic_cast<control_class>(this);
			r->margin = _spacing;
			return *this;
		}

		virtual void apply_item_sizes(control_base& _ref);

		virtual bool set_mouse(point _position,
			bool* _left_down,
			bool* _right_down,
			std::function<void(control_base* _item)> _left_click,
			std::function<void(control_base* _item)> _right_click
		);

		virtual void on_unsubscribe(presentation_base* _presentation, page_base* _page)
		{
			_page->clear_events(id);
			for (auto child : children) {
				child->on_unsubscribe(_presentation, _page);
			}
		}

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			for (auto child : children) {
				child->on_subscribe(_presentation, _page);
			}
		}

		virtual void on_update(double _time)
		{
			for (auto child : children) {
				child->on_update(_time);
			}
		}

		virtual void hardware_scan()
		{
			for (auto child : children) {
				child->hardware_scan();
			}
		}

		virtual void dump(int _indent = {4})
		{
			json_parser jp;
			json control_json = jp.create_object();
			std::string sindent(_indent, ' ');
			std::string fmt;
			fmt = std::format("{0} {1} debug clicked", typeid(*this).name(), name);
			system_monitoring_interface::active_mon->log_information(fmt);
			get_json(control_json);
			system_monitoring_interface::active_mon->log_json<json>(control_json);
		}

	};

	control_base* control_base::find(point p)
	{
		control_base* result = nullptr;

		if (rectangle_math::contains(inner_bounds, p.x, p.y))
		{
			result = this;
			for (auto child : children)
			{
				auto temp = child->find(p);
				if (temp) {
					return temp;
				}
			}
		}

		return result;
	}

	control_base* control_base::find(int _id)
	{
		control_base* root = (control_base *)this;
		control_base* result = control_base::get(root, _id);
		return result;
	}

	control_base* control_base::find(std::string _name)
	{
		control_base* root = (control_base*)this;
		control_base* result = control_base::get(root, _name);
		return result;
	}

	std::pair<control_base *, control_base *> control_base::get_pair(int _id)
	{
		std::pair<control_base*, control_base*> result;
		result = find_if([_id](control_base* c) { return c->id == _id; });
		return result;
	}

	control_base* control_base::get(control_base* _root, std::string _name)
	{
		control_base* result = _root->find_if([_name](control_base* c) { return c->name == _name; }).second;
		return result;
	}

	control_base* control_base::get(control_base* _root, int _id)
	{
		control_base* result = _root->find_if([_id](control_base* c) { return c->id == _id; }).second;
		return result;
	}

	std::pair<control_base*, control_base *> control_base::find_if(std::function<bool(control_base* _root)> _item)
	{
		std::pair<control_base*, control_base*> result;

        result.first = nullptr;
		result.second = nullptr;

		if (_item(this)) {
			result.second = this;
			return result;
		}
		for (auto child : children) {
			auto result = child->find_if(_item);
			if (result.second)
				return result;
		}
		return result;
	}

	void control_base::foreach(std::function<void(control_base* _root)> _item)
	{
		_item(this);
		for (auto child : children) {	
			if (child) {
				child->foreach(_item);
			}
		}
	}

	void control_base::create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host)
	{
		for (auto child : children) {
			try {
				child->create(_context, _host);
			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_exception(exc);
			}
		}
	}

	void control_base::destroy()
	{
		for (auto child : children) {
			child->destroy();
		}
	}

	void control_base::draw(std::shared_ptr<direct2dContext>& _context)
	{
		//std::cout << typeid(*this).name() << " control_base::draw" << std::endl;
		for (auto child : children) 
		{
			try 
			{
				child->draw(_context);
			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_exception(exc);
			}
		}
	}

	void control_base::render(std::shared_ptr<direct2dContext>& _context)
	{
		for (auto child : children) {
			try
			{
				child->render(_context);
			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_exception(exc);
			}
		}
	}

	void control_base::apply_item_sizes(control_base& _ref)
	{

	}

	bool control_base::set_mouse(point _position,
		bool* _left_down,
		bool* _right_down,
		std::function<void(control_base* _item)> _left_click,
		std::function<void(control_base* _item)> _right_click
	)
	{
		mouse_relative_position = {};
		mouse_over = rectangle_math::contains(bounds, _position.x, _position.y);

		if (mouse_over)
		{
			mouse_relative_position = _position;
			mouse_relative_position.x -= bounds.x;
			mouse_relative_position.y -= bounds.y;
			if (_left_down)
			{
				mouse_left_down = *_left_down;
			}
			if (_right_down)
			{
				mouse_right_down = *_right_down;
			}
		}
		else
		{
			mouse_left_down = false;
			mouse_right_down = false;
		}

		if (mouse_left_down.changed_to(false) and _left_click) {
			_left_click(this);
		}

		if (mouse_right_down.changed_to(false) and _right_click) {
			_right_click(this);
		}

		for (auto child : children)
		{
			child->set_mouse(_position, _left_down, _right_down, _left_click, _right_click);
		}
		return mouse_over;
	}

	double control_base::to_pixels_x(control_base* _parent, measure length)
	{
		double sz = 0.0;

		if (length.units == measure_units::pixels)
		{
			sz = length.amount;
		}
		else if (length.units == measure_units::percent_container)
		{
			double parent_width = 0.0;
			if (_parent)
			{
				auto pparent = dynamic_cast<control_base*>(_parent);
				if (pparent)
				{
					parent_width = pparent->inner_bounds.w;
				}
			}
			sz = length.amount * parent_width;
		}
		else if (length.units == measure_units::percent_remaining)
		{
			double parent_width = 0.0;
			if (_parent)
			{
				auto pparent = dynamic_cast<control_base*>(_parent);
				if (pparent)
				{
					parent_width = pparent->get_remaining(this).x;
				}
			}
			sz = length.amount * parent_width;
		}
		else if (length.units == measure_units::font)
		{
			double font_height = 12.0;
			sz = font_height * box.width.amount;
		}
		else if (length.units == measure_units::font_golden_ratio)
		{
			double font_height = 12.0;
			sz = font_height * box.width.amount / 1.618;
		}
		else if (length.units == measure_units::text)
		{

		}

		return sz;
	}

	double control_base::to_pixels_y(control_base* _parent, measure length)
	{
		double sz = 0.0;

		if (length.units == measure_units::pixels)
		{
			sz = length.amount;
		}
		else if (length.units == measure_units::percent_container)
		{
			double parent_height = 0.0;
			if (_parent)
			{
				auto pparent = dynamic_cast<control_base*>(_parent);
				if (pparent)
				{
					parent_height = pparent->inner_bounds.h;
				}
			}
			sz = length.amount * parent_height;
		}
		else if (length.units == measure_units::percent_remaining)
		{
			double parent_height = 0.0;
			if (_parent)
			{
				auto pparent = dynamic_cast<control_base*>(_parent);
				if (pparent)
				{
					parent_height = pparent->get_remaining(this).y;
				}
			}
			sz = length.amount * parent_height;
		}
		else if (length.units == measure_units::font)
		{
			double font_height = 12.0;
			sz = font_height * box.height.amount;
		}
		else if (length.units == measure_units::font_golden_ratio)
		{
			double font_height = 12.0;
			sz = font_height * box.height.amount / 1.618;
		}
		else if (length.units == measure_units::text)
		{

		}

		return sz;
	}
 
	point control_base::get_size(control_base* _parent)
	{
		point sz;

        sz.x = to_pixels_x(_parent, box.width);
        sz.y = to_pixels_y(_parent, box.height);

		return sz;
	}

	point control_base::get_position(control_base* _parent)
	{
		point pt;

        pt.x = to_pixels_x(_parent, box.x);
		pt.y = to_pixels_y(_parent, box.y);

		return pt;
	}

	point control_base::get_remaining(control_base* _parent)
	{
		return { 0.0, 0.0 };
	}

	bool control_base::contains(point pt)
	{
		return rectangle_math::contains(bounds, pt.x, pt.y);
	}

	rectangle& control_base::set_bounds(control_base* _parent, rectangle& _bounds, bool _clip_children)
	{
		bounds = _bounds;

		margin_amount.x = to_pixels_x(_parent, margin);
		margin_amount.y = to_pixels_y(_parent, margin);
		padding_amount.x = to_pixels_x(_parent, padding);
		padding_amount.y = to_pixels_y(_parent, padding);

		if (_parent)
		{
			auto pparent = dynamic_cast<control_base *>(_parent);
			if (pparent and _clip_children) {
				auto pbounds = pparent->get_inner_bounds();
				if (bounds.x < pbounds.x)
					bounds.x = pbounds.x;
				if (bounds.y < pbounds.y)
					bounds.y = pbounds.y;
				if (bounds.right() > pbounds.right())
				{
					bounds.w -= (bounds.right() - pbounds.right());
				}
				if (bounds.bottom() > pbounds.bottom())
				{
					bounds.h -= (bounds.bottom() - pbounds.bottom());
				}
				if (bounds.w < 0) bounds.w = 0;
				if (bounds.h < 0) bounds.h = 0;
			}
		}

		inner_bounds = bounds;

		inner_bounds.x += padding_amount.x;
		inner_bounds.y += padding_amount.y;
		inner_bounds.w -= (padding_amount.x * 2.0);
		inner_bounds.h -= (padding_amount.y * 2.0);

		if (inner_bounds.w < 0) inner_bounds.w = 0;
		if (inner_bounds.h < 0) inner_bounds.h = 0;
	
		on_resize();
		return bounds;
	}

	void control_base::arrange(control_base* _parent, rectangle *_bounds)
	{
		set_bounds(_parent, *_bounds);
	}

	void control_base::on_resize()
	{
		auto ti = typeid(*this).name();
	}


}

