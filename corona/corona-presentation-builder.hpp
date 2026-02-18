/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved


MIT License

About this File
This file contains controls that are compositions, and a more
fluent code style builder for building them.  The fluent code builder
is then used to build up the page collection.

Notes

For Future Consideration
*/

#pragma once

namespace corona
{

	class tab_view_control;
	class caption_bar_control;
	class status_bar_control;
	class checkbox_list_control;
	class radiobutton_list_control;

	class horizontal_field_layout {
	public:
		layout_rect label_box = { 0.0_px, 0.0_px, 0.3_container, 30.0_px };
		layout_rect field_box = { 0.0_px, 0.0_px, 0.3_container, 30.0_px };
		layout_rect status_box = { 0.0_px, 0.0_px, 0.3_container, 30.0_px };
		layout_rect box = { 0.0_px, 0.0_px, 1.0_container, 34.0_px };
	};

	class vertical_field_layout {
	public:
		layout_rect label_box = { 0.0_px, 0.0_px, 1.0_container, 30.0_px };
		layout_rect field_box = { 0.0_px, 0.0_px, 1.0_container, 30.0_px };
		layout_rect status_box = { 0.0_px, 0.0_px, 1.0_container, 30.0_px };
		layout_rect box = { 0.0_px, 0.0_px, 1.0_container, 90.0_px };
	};

	template <typename layout_container, typename field_control, typename layout_strategy> class field_layout : public layout_container
	{
	public:
		std::shared_ptr<edit_label_control>  label;
		std::shared_ptr<field_control>		field;
		std::shared_ptr<status_control>		status;

		layout_rect label_box;
		layout_rect field_box;
		layout_rect status_box;

		field_layout()
		{
			label = std::make_shared<edit_label_control>();
			field = std::make_shared<field_control>();
			status = std::make_shared<status_control>();

			layout_strategy ls;

			label_box = ls.label_box;
			field_box = ls.field_box;
			status_box = ls.status_box;
			control_base::box = ls.box;

			label->box = label_box;
            field->box = field_box;	
            status->box = status_box;
            
			control_base::children.add(label);
			control_base::children.add(field);
			control_base::children.add(status);
		}

		field_layout(const field_layout& _src)
		{
			label = std::dynamic_pointer_cast<edit_label_control>(_src.label->clone());
			field = std::dynamic_pointer_cast<field_control>(_src.field->clone());
			status = std::dynamic_pointer_cast<status_control>(_src.status->clone());

			label_box = _src.label_box;
			field_box = _src.field_box;
			status_box = _src.status_box;
			control_base::box = _src.box;

			label->box = label_box;
			field->box = field_box;
			status->box = status_box;
			
			control_base::children.push_back(label);
			control_base::children.push_back(field);
			control_base::children.push_back(status);
		}

		field_layout(control_base* _parent, int _id) : layout_container(_parent, _id)
		{
			label = std::make_shared<edit_label_control>();
			field = std::make_shared<field_control>();
			status = std::make_shared<status_control>();

			layout_strategy ls;

			label_box = ls.label_box;
			field_box = ls.field_box;
			status_box = ls.status_box;
			control_base::box = ls.box;

			label->box = label_box;
			field->box = field_box;
			status->box = status_box;

			control_base::children.push_back(label);
			control_base::children.push_back(field);
			control_base::children.push_back(status);
		}

		virtual ~field_layout() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<field_layout>(*this);
			return tv;
		}

		virtual void set_error(validation_error& _error)
		{
			status->set_error(_error);
		}
		
		virtual void clear_error()
		{
			status->clear_error();
		}

		virtual void get_json(json& _dest)
		{
			control_base::get_json(_dest);
			if (label) {
				json label_json;
				label->get_json(label_json);
				_dest.put_member("label", label_json);
			}
			if (field) {
				json field_json;
				field->get_json(field_json);
				_dest.put_member("field", field_json);
			}
			if (status) {
				json status_json;
				status->get_json(status_json);
				_dest.put_member("status", status_json);
			}
		}

		virtual void put_json(json& _src)
		{
			control_base::put_json(_src);

			label = std::make_shared<edit_label_control>();
			auto label_json = _src["label"];
			if (!label_json.object()) {
				label->box = label_box;
			}
			else if (!label_json.has_member("box")) {
				label->box = label_box;
			}
			label->put_json(label_json);

			if (_src.has_member("text")) {
				label->set_text(_src["text"].as_string());
            }

			field = std::make_shared<field_control>();
			auto field_json = _src["field"];
			if (!label_json.object() || !field_json.has_member("box")) {
				field->box = field_box;
			}
			field->put_json(field_json);

			if (_src.has_member("json_field_name")) {
				field->json_field_name = _src["json_field_name"].as_string();
			}

			status = std::make_shared<status_control>();
			auto status_json = _src["status"];
			if (!label_json.object() || !field_json.has_member("box")) {
				status->box = status_box;
			}
			status->put_json(status_json);

			control_base::children.clear();

			control_base::children.push_back(label);
			control_base::children.push_back(field);
			control_base::children.push_back(status);

		}
	};

	using default_layout = row_layout;
	using default_field_layout = horizontal_field_layout;

	using edit_field_control = field_layout<default_layout, edit_control, default_field_layout>;
	using readonly_field_control = field_layout<default_layout, readonly_control, default_field_layout>;
	using password_field_control = field_layout<default_layout, password_control, default_field_layout>;
	using number_field_control = field_layout<default_layout, number_control, default_field_layout>;
	using checkbox_field_control = field_layout<default_layout, checkbox_control, default_field_layout>;
	using listview_field_control = field_layout<default_layout, listview_control, default_field_layout>;
	using treeview_field_control = field_layout<default_layout, treeview_control, default_field_layout>;
	using header_field_control = field_layout<default_layout, header_control, default_field_layout>;
	using toolbar_field_control = field_layout<default_layout, toolbar_control, default_field_layout>;
	using statusbar_field_control = field_layout<default_layout, statusbar_control, default_field_layout>;
	using hotkey_field_control = field_layout<default_layout, hotkey_control, default_field_layout>;
	using animate_field_control = field_layout<default_layout, animate_control, default_field_layout>;
	using richedit_field_control = field_layout<default_layout, richedit_control, default_field_layout>;
	using draglistbox_field_control = field_layout<default_layout, draglistbox_control, default_field_layout>;
	using combobox_field_control = field_layout<default_layout, combobox_control, default_field_layout>;
	using comboboxex_field_control	 = field_layout<default_layout, comboboxex_control, default_field_layout>;
	using datetimepicker_field_control = field_layout<default_layout, datetimepicker_control, default_field_layout>;
	using monthcalendar_field_control = field_layout<default_layout, monthcalendar_control, default_field_layout>;
	using radiobutton_list_field_control = field_layout<default_layout, radiobutton_list_control, default_field_layout>;
	using checkbox_list_field_control = field_layout<default_layout, checkbox_list_control, default_field_layout>;
 
	class corona_button_control : public pushbutton_control
	{
	public:
		using control_base::id;

		std::shared_ptr<call_status>	status;

		using windows_control::enable;
		using windows_control::disable;

		corona_button_control(control_base* _parent, int _id) : pushbutton_control(_parent, _id)
		{
			init();
		}

		corona_button_control(const corona_button_control& _src) : pushbutton_control(_src)
		{
			init();
			status = _src.status;
		}

		virtual ~corona_button_control() { ; }

		void init();
		virtual double get_font_size() { return text_style.fontSize; }

		virtual void get_json(json& _dest)
		{
			pushbutton_control::get_json(_dest);
		}
		virtual void put_json(json& _src)
		{
			pushbutton_control::put_json(_src);
		}
	};


	class control_builder
	{
		std::shared_ptr<container_control> root;
		control_builder *parent;

	public:

		comm_bus_app_interface* bus;

		template <typename control_type> std::shared_ptr<control_type> get(int _id)
		{
			std::shared_ptr<control_type> temp;
			auto found = std::find_if(root->children.begin(), root->children.end(), [_id](auto& ch) {
				return ch->id == _id;
				});
			if (found != std::end(root->children)) {
				temp = std::dynamic_pointer_cast<control_type>(*found);
			}
			return temp;
		}

		template <typename control_type> std::shared_ptr<control_type> create(int _id)
		{

			if (_id == 0)
				throw std::logic_error("cannot create a control with id 0");

			auto cp = root.get();
			std::shared_ptr<control_type> temp;
			auto found = std::find_if(root->children.begin(), root->children.end(), [_id](auto& ch) {
				return ch->id == _id;
				});

			// check to see if an element of this id exists
			if (found != std::end(root->children)) {
				// ok, so now we see if it is of the same type
				temp = std::dynamic_pointer_cast<control_type>(*found);

				// and, it is not, so we replace it
				if (!temp) {
					temp = std::make_shared<control_type>(cp, _id);
					if (temp) {
						// here's our new temp, and we copy the children
						// this may not be the best plan but it allows us
						// to easily switch container types.
						temp->children = (*found)->children;
						int found_index = std::distance(found, root->children.begin());
						root->children[found_index] = temp;
					}
				}

				// otherwise, we found an existing control of the same type and we use that.
			}
			else 
			{
				// and, since we found nothing, we go ahead and just shove our new element at the back
				temp = std::make_shared<control_type>(cp, _id);
				if (temp) {
					root->children.push_back(temp);
				}
			}
			return temp;
		}

		template <typename control_type> void apply_item_sizes(control_type _ref)
		{
			if (not _ref) {
				return;
			}

			if (root->item_box.height.amount > 0)
			{
				_ref->box.height = root->item_box.height;
			}

			if (root->item_box.width.amount > 0)
			{
				_ref->box.width = root->item_box.width;
			}

			if (root->item_margin.amount > 0)
			{
				_ref->margin = root->item_margin;
			}
		}

		control_builder() 
		{
			parent = nullptr;
			root = std::make_shared<row_layout>();
		}

		control_builder(std::shared_ptr<container_control>& _root)
		{
			parent = nullptr;
			root = _root;
		}

		control_builder(std::shared_ptr<column_layout> _root)
		{
			parent = nullptr;
			root = std::dynamic_pointer_cast<container_control>(_root);
		}

		control_builder(std::shared_ptr<row_layout> _root)
		{
			parent = nullptr;
			root = std::dynamic_pointer_cast<container_control>(_root);
		}

		control_builder(std::shared_ptr<absolute_layout> _root)
		{
			parent = nullptr;
			root = std::dynamic_pointer_cast<container_control>(_root);
		}

		control_builder(std::shared_ptr<container_control> _root, int _id )
		{
			parent = nullptr;
			root = _root->find_by_id<container_control>(_id);
		}

		control_builder(control_builder* _parent, std::shared_ptr<container_control>& _root)
		{
			parent = _parent;
			root = _root;
		}

		control_builder(const control_builder& _src)
		{
			parent = _src.parent;
			root = _src.root;
		}

		control_builder &operator =(const control_builder& _src)
		{
			parent = _src.parent;
			root = _src.root;
			return *this;
		}

		control_builder(control_builder&& _src)
		{
			parent = std::move(_src.parent);
			root = std::move(_src.root);
		}

		control_builder &operator =(control_builder&& _src)
		{
			parent = std::move(_src.parent);
			root = std::move(_src.root);
			return *this;
		}

		void apply_controls(control_base* _control)
		{
			if (_control == root.get())
				return;
			_control->children.clear();
			for (auto child : root->children) {
				_control->children.push_back(child);
			}
		}

		std::shared_ptr<container_control> get_root()
		{
			return root;
		}

		inline control_builder& title(std::string _text) { return title(_text, nullptr, id_counter::next()); }
		inline control_builder& subtitle(std::string _text) { return subtitle(_text, nullptr, id_counter::next()); }
		inline control_builder& chaptertitle(std::string _text) { return chaptertitle(_text, nullptr, id_counter::next()); }
		inline control_builder& chaptersubtitle(std::string _text) { return chaptersubtitle(_text, nullptr, id_counter::next()); }
		inline control_builder& paragraph(std::string _text) { return paragraph(_text, nullptr, id_counter::next()); }
		inline control_builder& code(std::string _text) { return code(_text, nullptr, id_counter::next()); }
		inline control_builder& label(std::string _text) { return label(_text, nullptr, id_counter::next()); }

		inline control_builder& error(call_status _status) { return error(_status, nullptr, id_counter::next()); }
		inline control_builder& status(call_status _status) { return status(_status, nullptr, id_counter::next()); }
		inline control_builder& success(call_status _status) { return success(_status, nullptr, id_counter::next()); }

		inline control_builder& title(int _id, std::string _text) { return title(_text, nullptr, _id); }
		inline control_builder& subtitle(int _id, std::string _text) { return subtitle(_text, nullptr, _id); }
		inline control_builder& chaptertitle(int _id, std::string _text) { return chaptertitle(_text, nullptr, _id); }
		inline control_builder& chaptersubtitle(int _id, std::string _text) { return chaptersubtitle(_text, nullptr, _id); }
		inline control_builder& paragraph(int _id, std::string _text) { return paragraph(_text, nullptr, _id); }
		inline control_builder& code(int _id, std::string _text) { return code(_text, nullptr, _id); }
		inline control_builder& label(int _id, std::string _text) { return label(_text, nullptr, _id); }
		inline control_builder& error(int _id, std::string _text) { return error(_text, nullptr, _id); }

		inline control_builder& title(int _id, std::function<void(title_control&)> _settings) { return title("", _settings, _id); }
		inline control_builder& subtitle(int _id, std::function<void(subtitle_control&)> _settings) { return subtitle("", _settings, _id); }
		inline control_builder& authorscredit(int _id, std::function<void(authorscredit_control&)> _settings) { return authorscredit("", _settings, _id); }
		inline control_builder& chaptertitle(int _id, std::function<void(chaptertitle_control&)> _settings) { return chaptertitle("", _settings, _id); }
		inline control_builder& chaptersubtitle(int _id, std::function<void(chaptersubtitle_control&)> _settings) { return chaptersubtitle("", _settings, _id); }
		inline control_builder& paragraph(int _id, std::function<void(paragraph_control&)> _settings) { return paragraph("", _settings, _id); }
		inline control_builder& code(int _id, std::function<void(code_control&)> _settings) { return code("", _settings, _id); }
		inline control_builder& label(int _id, std::function<void(label_control&)> _settings) { return label("", _settings, _id); }
		inline control_builder& error(int _id, std::function<void(error_control&)> _settings) { return error("", _settings, id_counter::next()); }

		inline control_builder& title(std::string _text, std::function<void(title_control&)> _settings) { return title(_text, _settings, id_counter::next()); }
		inline control_builder& subtitle(std::string _text, std::function<void(subtitle_control&)> _settings) { return subtitle(_text, _settings, id_counter::next()); }
		inline control_builder& authorscredit(std::string _text, std::function<void(authorscredit_control&)> _settings) { return authorscredit(_text, _settings, id_counter::next()); }
		inline control_builder& chaptertitle(std::string _text, std::function<void(chaptertitle_control&)> _settings) { return chaptertitle(_text, _settings, id_counter::next()); }
		inline control_builder& chaptersubtitle(std::string _text, std::function<void(chaptersubtitle_control&)> _settings) { return chaptersubtitle(_text, _settings, id_counter::next()); }
		inline control_builder& paragraph(std::string _text, std::function<void(paragraph_control&)> _settings) { return paragraph(_text, _settings, id_counter::next()); }
		inline control_builder& code(std::string _text, std::function<void(code_control&)> _settings) { return code(_text, _settings, id_counter::next()); }
		inline control_builder& label(std::string _text, std::function<void(label_control&)> _settings) { return label(_text, _settings, id_counter::next()); }
		inline control_builder& error(std::string _text, std::function<void(error_control&)> _settings) { return error(_text, _settings, id_counter::next()); }

		inline control_builder& command_button(std::string _text, std::function<void(command_button_control&)> _settings) { return command_button(_text, _settings, id_counter::next()); }

		inline control_builder& error(call_status _status, std::function<void(error_control&)> _settings) { return error(_status, _settings, id_counter::next()); }
		inline control_builder& status(call_status _status, std::function<void(status_control&)> _settings) { return status(_status, _settings, id_counter::next()); }
		inline control_builder& success(call_status _status, std::function<void(success_control&)> _settings) { return success(_status, _settings, id_counter::next()); }

		control_builder row_begin(int _id, std::function<void(row_layout&)> _settings)
		{
			auto tc = create<row_layout>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			auto temp = control_builder(this, cc);
			return temp;
		}

		control_builder column_begin(int _id, std::function<void(column_layout&)> _settings)
		{
			auto tc = create<column_layout>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder absolute_begin(int _id, std::function<void(absolute_layout&)> _settings)
		{
			auto tc = create<absolute_layout>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder row_view_begin(int id, std::function<void(row_view_layout&)> _settings)
		{
			auto tc = create<row_view_layout>(id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder items_view_begin(int id, std::function<void(items_view&)> _settings)
		{
			auto tc = create<items_view>(id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder absolute_view_begin(int id, std::function<void(absolute_view_layout&)> _settings)
		{
			auto tc = create<absolute_view_layout>(id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder frame_begin(int _id, std::function<void(frame_layout&)> _settings)
		{
			auto tc = create<frame_layout>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			auto cc = std::dynamic_pointer_cast<container_control>(tc);
			return control_builder(this, cc);
		}

		control_builder end()
		{
			if (parent) {
				return *parent;
			}
			else 
			{
				return *this;
			}
		}

		control_builder& listbox(int _id, std::function<void(listbox_control&)> _settings)
		{
			auto tc = create<listbox_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& combobox(int _id, std::function<void(combobox_control&)> _settings)
		{
			auto tc = create<combobox_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& edit_field(int _id, std::function<void(edit_field_control&)> _settings)
		{
			auto tc = create<edit_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& readonly_field(int _id, std::function<void(readonly_field_control&)> _settings)
		{
			auto tc = create<readonly_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& password_field(int _id, std::function<void(password_field_control&)> _settings)
		{
			auto tc = create<password_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& number_field(int _id, std::function<void(number_field_control&)> _settings)
		{
			auto tc = create<number_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& listview_field(int _id, std::function<void(listview_field_control&)> _settings)
		{
			auto tc = create<listview_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& treeview_field(int _id, std::function<void(treeview_field_control&)> _settings)
		{
			auto tc = create<treeview_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& header_field(int _id, std::function<void(header_field_control&)> _settings)
		{
			auto tc = create<header_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& toolbar_field(int _id, std::function<void(toolbar_field_control&)> _settings)
		{
			auto tc = create<toolbar_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& statusbar_field(int _id, std::function<void(statusbar_field_control&)> _settings)
		{
			auto tc = create<statusbar_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& hotkey_field(int _id, std::function<void(hotkey_field_control&)> _settings)
		{
			auto tc = create<hotkey_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& animate_field(int _id, std::function<void(animate_field_control&)> _settings)
		{
			auto tc = create<animate_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& richedit_field(int _id, std::function<void(richedit_field_control&)> _settings)
		{
			auto tc = create<richedit_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& draglistbox_field(int _id, std::function<void(draglistbox_field_control&)> _settings)
		{
			auto tc = create<draglistbox_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& combobox_field(int _id, std::function<void(combobox_field_control	&)> _settings)
		{
			auto tc = create<combobox_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& comboboxex_field(int _id, std::function<void(comboboxex_field_control&)> _settings)
		{
			auto tc = create<comboboxex_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& datetimepicker_field(int _id, std::function<void(datetimepicker_field_control&)> _settings)
		{
			auto tc = create<datetimepicker_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& monthcalendar_field(int _id, std::function<void(monthcalendar_field_control&)> _settings)
		{
			auto tc = create<monthcalendar_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& radiobutton_list_field(int _id, std::function<void(radiobutton_list_field_control&)> _settings)
		{
			auto tc = create<radiobutton_list_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& checkbox_list_field(int _id, std::function<void(checkbox_list_field_control&)> _settings)
		{
			auto tc = create<checkbox_list_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& checkbox_field(int _id, std::function<void(checkbox_field_control&)> _settings)
		{
			auto tc = create<checkbox_field_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& image(int id, int _control_id, std::function<void(image_control&)> _settings)
		{
			auto tc = create<image_control>(id);
			apply_item_sizes(tc);
			tc->load_from_control(_control_id);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& draw(int id, std::function<void(draw_control&)> _settings)
		{
			auto tc = create<draw_control>(id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& image(int id, std::string _filename, std::function<void(image_control&)> _settings)
		{
			auto tc = create<image_control>(id);
			apply_item_sizes(tc);
			tc->load_from_file(_filename);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& image(std::string _filename, std::function<void(image_control&)> _settings)
		{
			auto tc = create<image_control>(id_counter::next());
			apply_item_sizes(tc);
			tc->load_from_file(_filename);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& camera(std::function<void(camera_control&)> _settings)
		{
			auto tc = create<camera_control>(id_counter::next());
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
	
		control_builder& camera_view(std::function<void(camera_view_control&)> _settings)
		{
			auto tc = create<camera_view_control>(id_counter::next());
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& title(std::string text, std::function<void(title_control&)> _settings, int _id)
		{
			auto tc = create<title_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& subtitle(std::string text, std::function<void(subtitle_control&)> _settings, int _id)
		{
			auto tc = create<subtitle_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& authorscredit(std::string text, std::function<void(authorscredit_control&)> _settings, int _id)
		{
			auto tc = create<authorscredit_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& chaptertitle(std::string text, std::function<void(chaptertitle_control&)> _settings, int _id)
		{
			auto tc = create<chaptertitle_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& chaptersubtitle(std::string text, std::function<void(chaptersubtitle_control&)> _settings, int _id)
		{
			auto tc = create<chaptersubtitle_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& paragraph(std::string text, std::function<void(paragraph_control&)> _settings, int _id)
		{
			auto tc = create<paragraph_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& code(std::string text, std::function<void(code_control&)> _settings, int _id)
		{
			auto tc = create<code_control>(_id);
			apply_item_sizes(tc);
			tc->text = text;
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& label(std::string _text, std::function<void(label_control&)> _settings, int _id)
		{
			auto tc = create<label_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(_text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& error(std::string _text, std::function<void(error_control&)> _settings, int _id)
		{
			auto tc = create<error_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(_text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}


		control_builder& success(call_status _status, std::function<void(success_control&)> _settings, int _id)
		{
			auto tc = create<success_control>(_id);
			apply_item_sizes(tc);
			tc->set_status(_status);
			if (_settings) {
				_settings(*tc);

			}
			return *this;
		}

		control_builder& error(call_status _status, std::function<void(error_control&)> _settings, int _id)
		{
			auto tc = create<error_control>(_id);
			apply_item_sizes(tc);
			tc->set_status(_status);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& status(call_status _status, std::function<void(status_control&)> _settings, int _id)
		{
			auto tc = create<status_control>(_id);
			apply_item_sizes(tc);
			tc->set_status(_status);
			if (_settings) {
				_settings(*tc);

			}
			return *this;
		}

		control_builder& link_button(int _id, std::function<void(linkbutton_control&)> _settings = nullptr)
		{
			auto tc = create<linkbutton_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& link_button(int _id, std::string text, std::function<void(linkbutton_control&) > _settings = nullptr)
		{
			auto tc = create<linkbutton_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}


		control_builder& push_button(int _id, std::function<void(pushbutton_control&)> _settings = nullptr)
		{
			auto tc = create<pushbutton_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& command_button(std::string text, std::function<void(command_button_control&)> _settings, int _id)
		{
			auto tc = create<command_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& push_button(int _id, std::string text, std::function<void(pushbutton_control&)> _settings = nullptr)
		{
			auto tc = create<pushbutton_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& press_button(int _id, std::string text, std::function<void(pressbutton_control&)> _settings = nullptr)
		{
			auto tc = create<pressbutton_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& radio_button(int _id, std::string text, std::function<void(radiobutton_control&)> _settings = nullptr)
		{
			auto tc = create<radiobutton_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& checkbox(int _id, std::string text, std::function<void(checkbox_control&)> _settings = nullptr)
		{
			auto tc = create<checkbox_control>(_id);
			apply_item_sizes(tc);
			tc->set_text(text);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& radio_button(int _id, std::function<void(radiobutton_control&)> _settings = nullptr)
		{
			auto tc = create<radiobutton_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& checkbox(int _id, std::function<void(checkbox_control&)> _settings = nullptr)
		{
			auto tc = create<checkbox_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& radiobutton_list(int _id, std::function<void(radiobutton_list_control&)> _settings = nullptr)
		{
			auto tc = create<radiobutton_list_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& checkbox_list(int _id,  std::function<void(checkbox_list_control&)> _settings = nullptr)
		{
			auto tc = create<checkbox_list_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& edit(int _id, std::function<void(edit_control&)> _settings = nullptr)
		{
			auto tc = create<edit_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& password(int _id, std::function<void(edit_control&)> _settings = nullptr)
		{
			auto tc = create<password_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}
		control_builder& scrollbar(int _id, std::function<void(scrollbar_control&)> _settings = nullptr)
		{
			auto tc = create<scrollbar_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& listview(int _id, std::function<void(listview_control&)> _settings = nullptr)
		{
			auto tc = create<listview_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& treeview(int _id, std::function<void(treeview_control&)> _settings = nullptr)
		{
			auto tc = create<treeview_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& header(int _id, std::function<void(header_control&)> _settings = nullptr)
		{
			auto tc = create<header_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& toolbar(int _id, std::function<void(toolbar_control&)> _settings = nullptr)
		{
			auto tc = create<toolbar_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& statusbar(int _id, std::function<void(statusbar_control&)> _settings = nullptr)
		{
			auto tc = create<statusbar_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& hotkey(int _id, std::function<void(hotkey_control&)> _settings = nullptr)
		{
			auto tc = create<hotkey_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& animate(int _id, std::function<void(animate_control&)> _settings = nullptr)
		{
			auto tc = create<animate_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& richedit(int _id, std::function<void(richedit_control&)> _settings = nullptr)
		{
			auto tc = create<richedit_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& draglistbox(int _id, std::function<void(draglistbox_control&)> _settings = nullptr)
		{
			auto tc = create<draglistbox_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}


		control_builder& comboboxex(int _id, std::function<void(comboboxex_control&)> _settings = nullptr)
		{
			auto tc = create<comboboxex_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& datetimepicker(int _id, std::function<void(datetimepicker_control&)> _settings = nullptr)
		{
			auto tc = create<datetimepicker_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& monthcalendar(int _id, std::function<void(monthcalendar_control&)> _settings = nullptr)
		{
			auto tc = create<monthcalendar_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}


		control_builder& minimize_button(int _id, std::function<void(minimize_button_control&)> _settings = nullptr)
		{
			auto tc = create<minimize_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
			;
		}

		control_builder& maximize_button(int _id, std::function<void(maximize_button_control&)> _settings = nullptr)
		{
			auto tc = create<maximize_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& close_button(int _id, std::function<void(close_button_control&)> _settings = nullptr)
		{
			auto tc = create<close_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& menu_button(int _id, std::function<void(menu_button_control&)> _settings = nullptr)
		{
			auto tc = create<menu_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& corona_button(int _id, std::function<void(corona_button_control&)> _settings = nullptr)
		{
			auto tc = create<corona_button_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& camera(int _id, std::function<void(camera_control&)> _settings = nullptr)
		{
			auto tc = create<camera_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& grid(int _id, std::function<void(grid_control&)> _settings = nullptr)
		{
			auto tc = create<grid_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& chart(int _id, std::function<void(chart_control&)> _settings = nullptr)
		{
			auto tc = create<chart_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& slide(int _id, std::function<void(slide_control&)> _settings = nullptr)
		{
			auto tc = create<slide_control>(_id);
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			return *this;
		}

		control_builder& tab_button(int _id, std::function<void(tab_button_control&)> _settings = nullptr);
		control_builder& tab_view(int _id, std::function<void(tab_view_control&)> _settings = nullptr);
		control_builder& caption_bar(int _id, std::function<void(caption_bar_control&)> _settings = nullptr);
		control_builder& status_bar(int _id, std::function<void(status_bar_control&)> _settings = nullptr);
		control_builder& image(int _id, std::function<void(image_control&)> _settings = nullptr);

		std::shared_ptr<control_base> from_json(json _control_properties, layout_rect _default = {});
	};

	class tab_pane
	{
	public:
		int id;
		std::string name;
		std::vector<json> create_objects;
		std::vector<json> create_classes;
	};

	class tab_pane_instance
	{
	public:
		tab_pane					  pane;
		std::shared_ptr<container_control> tab_form;
		json						  tab_form_data;
	};

	class tab_view_control : public windows_control
	{
		std::vector<tab_pane_instance> tab_panes;
		std::shared_ptr<frame_layout> content_frame;
		presentation_base* current_presentation;
		page_base* current_page;
		int active_id;

		void init()
		{
			children.clear();

			control_builder builder;

			on_create = [this](std::shared_ptr<direct2dContext>& _context, control_base* _item)
				{
					_context->setBrush(border_brush.get(), &inner_bounds);
				};

			auto main = builder.column_begin(id_counter::next(), [this](column_layout& _settings) {
				_settings.set_size(1.0_container, 1.0_container);
				_settings.set_item_size(1.0_container, 1.0_container);
				});

			auto tab_row = main.row_begin(id_counter::next(), [](row_layout& _settings) {
				_settings.set_size(1.0_container, 40.0_px);
				});

			auto frame_row = main.frame_begin(id_counter::next(), [this](frame_layout& _settings) {
				_settings.set_size(1.0_container, 1.0_remaining);
				});

			auto froot = frame_row.get_root();
			content_frame = std::dynamic_pointer_cast<frame_layout>(froot);

			for (int i = 0; i < tab_panes.size(); i++)
			{
				auto dat = tab_panes[i];

				if (not i and active_id <= 0)
				{
					active_id = dat.pane.id;
				}

				tab_row.tab_button(dat.pane.id, [this, dat](tab_button_control& _tb) {
					_tb.text = dat.pane.id;
					_tb.active_id = &active_id;
					_tb.tab_selected = [this](tab_button_control& _src)->void
						{
							tab_selected(_src);
						};
				});
			}

			main.apply_controls(this);
		}

	public:

		virtual const char* get_window_class() { return "Corona2dControl"; }
		virtual DWORD get_window_style() { return DefaultWindowStyles; }
		virtual DWORD get_window_ex_style() { return WS_EX_LAYERED; }

		std::shared_ptr<generalBrushRequest>	background_brush;
		std::shared_ptr<generalBrushRequest>	border_brush;

		std::weak_ptr<applicationBase> host;
		std::weak_ptr<direct2dChildWindow> window;
		std::function<void(std::shared_ptr<direct2dContext>& _context, tab_view_control*)> on_draw;
		std::function<void(std::shared_ptr<direct2dContext>& _context, tab_view_control*)> on_create;

		tab_view_control()
		{
			id = id_counter::next();
			current_presentation = nullptr;
			current_page = nullptr;
			set_border_color("#C0C0C0");
			active_id = 0;
		}

		tab_view_control(const tab_view_control& _src)
		{
			tab_panes = _src.tab_panes;
			background_brush = _src.background_brush;
			border_brush = _src.border_brush;
			on_draw = _src.on_draw;
			on_create = _src.on_create;
			current_presentation = nullptr;
			current_page = nullptr;
			active_id = 0;

			set_border_color("#C0C0C0");
		}

		tab_view_control(control_base* _parent, int _id)
		{
			id = _id;
			current_presentation = nullptr;
			current_page = nullptr;

			set_border_color("#C0C0C0");
		}

		virtual ~tab_view_control()
		{
		}

		virtual void arrange(control_base *_parent, rectangle _bounds)
		{
			set_bounds(_parent, _bounds, true);
		}

		virtual bool is_control_message(int _key)
		{
			bool is_message = is_focused and 
				(_key == VK_TAB or 
					_key == VK_LEFT or _key == VK_RIGHT or 
					_key == VK_UP or _key == VK_DOWN or 
					_key == VK_PRIOR or _key == VK_NEXT or 
					_key == VK_HOME or _key == VK_END ||
					_key == VK_DELETE or _key == VK_INSERT ||
					_key == VK_RETURN or _key == VK_DELETE);
			return is_message;
		}

		virtual point get_remaining(point _ctx)
		{
			return _ctx;
		}


		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<tab_view_control>(*this);
			return tv;
		}

		void set_tabs(std::vector<tab_pane> _new_panes)
		{
			tab_panes.clear();
			for (auto tp : _new_panes) {
				tab_pane_instance tpi;
				tpi.pane = tp;
				tab_panes.push_back(tpi);
			}
			init();
			tab_selected(active_id);
		}

		void tab_selected(std::vector<tab_pane_instance>::iterator tbi)
		{
			if (tbi != tab_panes.end())
			{
				active_id = tbi->pane.id;

				contents_generator<tab_pane_instance*> cg;

				// set contents will clone this for us.
                // this all has been rather fixed or should be so we should not need a contents generator here 
				// but this is the only way to get the data to the content frame without making it a member of the tab view control 
				// which would be a nightmare to manage.
                // that AI comment is from the future and I have no idea what it means but I am leaving it here for posterity.
				cg.data = &*tbi;
				cg.generator = [this](tab_pane_instance* _tp, control_base* _args)
					{
						_args->children.clear();
/*						if (not _tp->tab_form) {
							_tp->tab_form = std::make_shared<form_control>(this, active_id);
							_tp->tab_form->set_model(_tp->pane.form);
						}
						_args->children.push_back(_tp->tab_form);
	*/				};
				content_frame->set_contents(this, cg);

				if (current_presentation and current_page) {
					content_frame->on_subscribe(current_presentation, current_page);
				}
			}
			else if (tab_panes.size()>0) {
				tab_selected(tab_panes.begin());
			}
		}

		void tab_selected(int _active_id)
		{
			auto tbi = std::find_if(tab_panes.begin(), tab_panes.end(), [this](tab_pane_instance& _tb) {
				return _tb.pane.id == this->active_id;
				});
			tab_selected(tbi);
		}

		void tab_selected(tab_button_control& _tab)
		{
			auto tbi = std::find_if(tab_panes.begin(), tab_panes.end(), [this](tab_pane_instance& _tb) {
				return _tb.pane.id == this->active_id;
				});
			tab_selected(tbi);
		}

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			current_presentation = _presentation;
			current_page = _page;
			for (auto child : children) {
				child->on_subscribe(_presentation, _page);
			}
		}



		virtual void create(std::shared_ptr<direct2dContext>& _context, std::weak_ptr<applicationBase> _host) override
		{
			host = _host;
			if (auto phost = _host.lock()) {
				if (inner_bounds.x < 0 or inner_bounds.y < 0 or inner_bounds.w < 0 or inner_bounds.h < 0) {
					throw std::logic_error("inner bounds not initialized");
				}
				window = phost->createDirect2Window(id, inner_bounds);
			}
			if (on_create) {
				on_create(_context, this);
			}
			for (auto child : children) {
				child->create(_context, _host);
			}
			windows_control::create(_context, _host);
		}

		void destroy()
		{
			for (auto child : children) {
				child->destroy();
			}
		}

		void on_resize()
		{
			auto ti = typeid(*this).name();

			if (auto pwindow = window.lock())
			{
				pwindow->moveWindow(inner_bounds.x, inner_bounds.y, inner_bounds.w, inner_bounds.h);
			}
		}

		virtual void draw_impl(std::shared_ptr<direct2dContext>& _context)
		{
			bool adapter_blown_away = false;

			auto& context = _context;
			const char* border_name = nullptr;
			const char* background_name = nullptr;

			if (border_brush->get_name())
			{
				_context->setBrush(border_brush.get(), &inner_bounds);
				auto dc = _context->getDeviceContext();
				border_name = border_brush->get_name();
			}

			if (background_brush->get_name())
			{
				_context->setBrush(background_brush.get(), &inner_bounds);
				auto dc = _context->getDeviceContext();
				background_name = background_brush->get_name();
			}

			if (background_name) {
				rectangle r = inner_bounds;
				_context->drawRectangle(&r, "", 0.0, background_name);
			}

			if (on_draw)
			{
				on_draw(_context, this);
			} 

			if (is_focused and border_name) 
			{
				rectangle r = inner_bounds;
				_context->drawRectangle(&r, border_name, 4, "");
			}

			for (auto& child : children) {
				try {
					child->draw(_context);
				}
				catch (std::exception exc)
				{
					system_monitoring_interface::active_mon->log_exception(exc);
				}
			}
		}

		virtual void draw(std::shared_ptr<direct2dContext>& _dest)
		{

		}

		virtual void render(std::shared_ptr<direct2dContext>& _dest)
		{
			draw_impl(_dest);
		}

		tab_view_control& set_origin(measure _x, measure _y)
		{
			box.x = _x;
			box.y = _y;
			return *this;
		}

		tab_view_control& set_size(measure _width, measure _height)
		{
			box.width = _width;
			box.height = _height;
			return *this;
		}

		tab_view_control& set_background_color(solidBrushRequest _brushFill)
		{
			background_brush = std::make_shared<generalBrushRequest>(_brushFill);
			background_brush->set_name(typeid(*this).name() );
			return *this;
		}

		tab_view_control& set_background_color(std::string _color)
		{
			background_brush = std::make_shared<generalBrushRequest>();
			background_brush->setColor(_color);
			background_brush->set_name(typeid(*this).name());
			return *this;
		}

		tab_view_control& set_border_color(solidBrushRequest _brushFill)
		{
			border_brush = std::make_shared<generalBrushRequest>(_brushFill);
			border_brush->set_name(typeid(*this).name());
			return *this;
		}

		tab_view_control& set_border_color(std::string _color)
		{
			border_brush = std::make_shared<generalBrushRequest>();
			border_brush->setColor(_color);
			border_brush->set_name( typeid(*this).name() );
			return *this;
		}

		tab_view_control& set_position(layout_rect _new_layout)
		{
			box = _new_layout;
			return *this;
		}

		tab_view_control& set_margin(measure _item_space)
		{
			margin = _item_space;
			return *this;
		}
	};


	class caption_bar_control : public container_control
	{
		void init()
		{
			control_builder cb;

			auto main_row = cb.row_begin(id_counter::next(), [](row_layout& rl) {
				rl.set_size(1.0_container, 1.0_container);
				rl.set_content_align(visual_alignment::align_far);
				rl.set_content_cross_align(visual_alignment::align_center);
				rl.set_style(presentation_style_factory::get_current()->get_style()->CaptionStyle);
				});

			if (image_file.size())
			{
				auto image_column = main_row.column_begin(id_counter::next(), [](column_layout& cl) {
					cl.set_size(75.0_px, 1.0_container);
					cl.set_content_align(visual_alignment::align_center);
					});

				image_column.image(image_control_id, image_file, [](image_control& _settings) {
					_settings.box.x = 10.0_px;
					_settings.set_size(1.0_container, 1.0_aspect);
					});
			}

			auto padding_column = main_row.column_begin(id_counter::next(), [this](column_layout& cl) {
				cl.set_size(16.0_px, 1.0_container);
				cl.set_content_align(visual_alignment::align_center);
				})
				.end();

			auto title_column = main_row.column_begin(id_counter::next(), [this](column_layout& cl) {
				cl.set_size(1.0_remaining, 1.0_container);
				cl.set_content_align(visual_alignment::align_center);
				})
				.title(title_name, [this](title_control& control) {
						control.set_nchittest(HTCAPTION);
						control.set_size(1.0_container, 0.5_container);
					}, title_id)
				.subtitle(subtitle_name, [this](subtitle_control& control) {
						control.set_nchittest(HTCAPTION);
						control.set_size(1.0_container, 0.5_container);
					}, subtitle_id)
			.end();

			auto frame_buttons_container = main_row.column_begin(id_counter::next(), [](column_layout& rl) {
				rl.set_size(260.0_px, 1.0_container);
				rl.set_content_align(visual_alignment::align_center);
				});

			auto frame_buttons = frame_buttons_container.row_begin(id_counter::next(), [](row_layout& cl) {
				cl.set_content_align(visual_alignment::align_far);
				cl.set_size(250.0_px, 50.0_px);
				});

			frame_buttons.menu_button(menu_button_id, [](auto& _ctrl) { _ctrl.set_size(50.0_px, 50.0_px); })
				.minimize_button(min_button_id, [](auto& _ctrl) { _ctrl.set_size(50.0_px, 50.0_px); })
				.maximize_button(max_button_id, [](auto& _ctrl) { _ctrl.set_size(50.0_px, 50.0_px); })
				.close_button(close_button_id, [](auto& _ctrl) { _ctrl.set_size(50.0_px, 50.0_px); });

			cb.apply_controls(this);

		}

	public:

		int menu_button_id;
		int min_button_id;
		int max_button_id;
		int close_button_id;
		int image_control_id;
		std::string image_file;
		std::string corporate_name;
		std::string title_name;
		std::string subtitle_name;
		int title_id;
		int subtitle_id;
		measure title_start;

		caption_bar_control()
		{
			menu_button_id = 0;
			image_control_id = 0;
		}

		caption_bar_control(const caption_bar_control& _src) = default;

		caption_bar_control(control_base *_parent, int _id) : container_control(_parent, _id)
		{
			menu_button_id = id_counter::next();
			image_control_id = id_counter::next();
			min_button_id = id_counter::next();
			max_button_id = id_counter::next();
			close_button_id = id_counter::next();
			title_id = id_counter::next();
			subtitle_id = id_counter::next();
		}

		std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<caption_bar_control>(*this);
			return tv;
		}

		void build()
		{
			init();
		}

		virtual ~caption_bar_control() { ; }

		void set_status(std::string _status, std::string _detail)
		{
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override
		{
			set_bounds(_parent, *_ctx);
			for (auto child : children) {
				child->arrange(this, _ctx);
			}
		}

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			for (auto child : children) {
				child->on_subscribe(_presentation, _page);
			}
		}

		virtual void get_json(json& _dest)
		{
			container_control::get_json(_dest);

			_dest.put_member("image_file", image_file);
			_dest.put_member("corporate_name", corporate_name);
			_dest.put_member("title_name", title_name);
			_dest.put_member("subtitle_name", subtitle_name);
			
			json_parser jp;
			json jtitle_start = jp.create_object();
			corona::get_json(jtitle_start, title_start);
			_dest.put_member("title_start", jtitle_start);
		}

		virtual void put_json(json& _src)
		{
			container_control::put_json(_src);
			image_file = _src["image_file"].as_string();
			corporate_name = _src["corporate_name"].as_string();
			title_name = _src["title_name"].as_string();
			subtitle_name = _src["subtitle_name"].as_string()	;

			json jtitle_start = _src["title_start"];
			corona::put_json(title_start, jtitle_start);
		}

	};

	class status_bar_control : public container_control
	{
	public:

		void init()
		{
			control_builder cb;
			cb.row_begin(id_counter::status_bar_id, [this](row_layout& rl) {
				rl.set_size(1.0_container, 80.0_px);
				rl.set_content_align(visual_alignment::align_near);
				rl.set_content_cross_align(visual_alignment::align_near);
				rl.set_item_margin(10.0_px);
				})
				.column_begin(id_counter::next(), [](column_layout& cl) {
					cl.set_content_align(visual_alignment::align_center);
					cl.set_content_cross_align(visual_alignment::align_near);
					cl.set_size(.3_container, 1.0_container);
					cl.set_item_margin(0.0_px);
					})
					.title(id_counter::status_text_title_id, [](title_control& control) {
						control.set_size(300.0_px, 1.2_fontgr);
						})
					.subtitle(id_counter::status_text_subtitle_id, [](subtitle_control& control) {
						control.set_size(300.0_px, 1.2_fontgr);
						})
				.end()
			.end();

			cb.apply_controls(this);
		}

		presentation_style* st = nullptr;

		status_bar_control() { ; }
		status_bar_control(const status_bar_control& _src) { ; }
		status_bar_control(control_base* _parent, int _id) : container_control(_parent, _id) { ; }

		std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<status_bar_control>(*this);
			return tv;
		}

		virtual ~status_bar_control() { ; }
	};

	// implementation

	control_builder& control_builder::image(int _id, std::function<void(image_control&)> _settings)
	{
		auto tc = create<image_control>(_id);
		apply_item_sizes(tc);
		if (_settings) {
			_settings(*tc);
		}
		return *this;
	}

	control_builder& control_builder::tab_button(int _id, std::function<void(tab_button_control&)> _settings)
	{
		auto tc = create<tab_button_control>(_id);
		apply_item_sizes(tc);
		if (_settings) {
			_settings(*tc);
		}
		return *this;
	}

	control_builder& control_builder::tab_view(int _id, std::function<void(tab_view_control&)> _settings)
	{
		auto tc = create<tab_view_control>(_id);
		apply_item_sizes(tc);
		if (_settings) {
			_settings(*tc);
		}
		return *this;
	}

	control_builder& control_builder::caption_bar(int _id, std::function<void(caption_bar_control&)> _settings)
	{

		auto cp = root.get();
		std::shared_ptr<caption_bar_control> tc;
		tc = std::make_shared<caption_bar_control>(cp, _id);
		if (tc) {
			root->children.push_back(tc);
//			std::cout << " " << typeid(*this).name() << " ->create:" << typeid(control_type).name() << std::endl;
			apply_item_sizes(tc);
			if (_settings) {
				_settings(*tc);
			}
			tc->build();
		}
		return *this;
	} 

	control_builder& control_builder::status_bar(int _id, std::function<void(status_bar_control&)> _settings)
	{
		auto tc = create<status_bar_control>(_id);
		apply_item_sizes(tc);
		if (_settings) {
			_settings(*tc);
		}
		return *this;
	}

	std::shared_ptr<control_base> control_builder::from_json(json _control_properties, layout_rect _default)
	{
		json_parser jp;

		json control_properties = _control_properties;

		std::string class_name = control_properties["class_name"].as_string();
		std::string field_name = control_properties["name"].as_string();
		json control_data = control_properties["data"];

		int id = _control_properties.get_member("id").as_int();

		int field_id = id ? id : id_counter::next();

		std::string default_text = "";
		call_status default_status;

		if (class_name == "title")
		{
			title(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "subtitle")
		{
			subtitle(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "command_button")
		{
			command_button(default_text, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "authorscredit")
		{
			authorscredit(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "chaptertitle")
		{
			chaptertitle(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "chaptersubtitle")
		{
			chaptersubtitle(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "paragraph")
		{
			paragraph(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "code")
		{
			code(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "label")
		{
			label(default_text, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "error")
		{
			error(default_status, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "status")
		{
			status(default_status, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "success")
		{
			success(default_status, [&control_properties, control_data](auto& _ctrl) ->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				}, field_id);
		}
		else if (class_name == "row")
		{
			row_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "column")
		{
			column_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "absolute")
		{
			absolute_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "row_view")
		{
			row_view_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "items_view")
		{
			items_view_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "absolute_view")
		{
			absolute_view_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "frame")
		{
			frame_begin(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "listbox")
		{
			listbox(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "combobox")
		{
			combobox(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "password")
		{
			password(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "edit")
		{
			edit(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "listview")
		{
			listview(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "treeview")
		{
			treeview(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "header")
		{
			header(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "toolbar")
		{
			toolbar(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "statusbar")
		{
			statusbar_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "hotkey")
		{
			hotkey(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "animate")
		{
			animate(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "edit")
		{
			edit(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "richedit")
		{
			richedit(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "draglistbox")
		{
			draglistbox(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "comboboxex")
		{
			comboboxex(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "datetimepicker")
		{
			datetimepicker(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "monthcalendar")
		{
			monthcalendar(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "radiobutton_list")
		{
			radiobutton_list(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "checkbox_list")
		{
			checkbox_list(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "edit_field")
		{
			edit_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "read_only_field")
		{
			readonly_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "password_field")
		{
			password_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "number_field")
		{
			number_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "listview_field")
		{
			listview_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "treeview_field")
		{
			treeview_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "header_field")
		{
			header_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "toolbar_field")
		{
			toolbar_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "statusbar_field")
		{
			statusbar_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "hotkey_field")
		{
			hotkey_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "animate_field")
		{
			animate_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "richedit_field")
		{
			richedit_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "draglistbox_field")
		{
			draglistbox_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "combobox_field")
		{
			combobox_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "comboboxex_field")
		{
			comboboxex_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "datetimepicker_field")
		{
			datetimepicker_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "monthcalendar_field")
		{
			monthcalendar_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "radiobutton_list_field")
		{
			radiobutton_list_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "checkbox_list_field")
		{
			checkbox_list_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "checkbox_field")
		{
			checkbox_field(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}

		else if (class_name == "minimize_button")
		{
			minimize_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "maximize_button")
		{
			maximize_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "close_button")
		{
			close_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "menu_button")
		{
			menu_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "corona_button")
		{
			corona_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "link_button")
		{
			link_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
				}
		else if (class_name == "push_button")
		{
			push_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "radio_button")
		{
			radio_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "checkbox")
		{
			checkbox(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "camera")
		{
			camera(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "grid")
		{
			grid(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "chart")
		{
			chart(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "slide")
		{
			slide(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "grid")
		{
			grid(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "tab_button")
		{
			tab_button(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "tab_view")
		{
			tab_view(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "caption_bar")
		{
			caption_bar(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "status_bar")
		{
			status_bar(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else if (class_name == "image")
		{
			image(field_id, [&control_properties, control_data](auto& _ctrl)->void {
				_ctrl.put_json(control_properties);
				_ctrl.set_data(control_data);
				});
		}
		else 
		{
			std::string msg;
			msg = std::format("class_name '{0}' is invalid", class_name);
			system_monitoring_interface::active_mon->log_warning(msg);
			std::cout << "Currently the following control classes are supported.  Set class_name to one of these." << std::endl;
			std::cout << "Special types" << std::endl;
			std::cout << "image, camera, " << std::endl;
			std::cout << "Text Box types" << std::endl;
			std::cout << "title, subtitle, chaptertitle, chaptersubtitle, paragraph, " << std::endl;
			std::cout << "code, label, error, status, success" << std::endl;
			std::cout << "Layout types" << class_name << std::endl;
			std::cout << "row, column, absolute, row_view, absolute_view, items_view, grid, slide, frame, tab_button, tab_view" << class_name << std::endl;
			std::cout << "Windows SDK types" << class_name << std::endl;
			std::cout << "combobox, listbox, edit, password, listview, treeview, header, toolbar, statusbar, hotkey, animate, richedit, draglistbox, comboboxex, datetimepicker, monthcalendar, radiobutton_list, checkbox_list" << std::endl;
			std::cout << "System Button types" << class_name << std::endl;
			std::cout << "caption_bar, minimize_button, maximize_button, close_button, menu_button, corona_button" << std::endl;
			std::cout << "Field types" << class_name << std::endl;
			std::cout << "combobox_field, listbox_field, edit_field, password_field, listview_field, treeview_field, header_field, toolbar_field, statusbar_field, hotkey_field, animate_field, richedit_field, draglistbox_field, comboboxex_field, datetimepicker_field, monthcalendar_field, radiobutton_list_field, checkbox_list_field" << std::endl;
			std::cout << "" << class_name << std::endl;
		}

		std::shared_ptr<control_base> ret = get<control_base>(field_id);

		if (ret) {
			json children = _control_properties.get_member("children");
			if (children.array()) {

				control_builder cb;
				for (auto child : children)
				{
					auto new_child = cb.from_json(child);
					if (new_child) {
						ret->children.push_back(new_child);
					}
				}
			}
		}

		return ret;
	}

	class radiobutton_list_control :
		public column_layout
	{
	protected:
		list_data choices;
		json data;

	public:
		radiobutton_list_control() { ; }
		radiobutton_list_control(const radiobutton_list_control& _src) = default;
		radiobutton_list_control(control_base* _parent, int _id) : column_layout(_parent, _id) { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<radiobutton_list_control>(*this);
			return tv;
		}

		virtual ~radiobutton_list_control() { ; }

		void list_changed()
		{
			control_builder cb;

			children.clear();

			int count = choices.items.size();

			for (int i = 0; i < count; i++)
			{
				json item = choices.items.get_element(i);
				int id = item.get_member(choices.id_field).as_int();
				std::string text = item.get_member(choices.text_field).as_string();
				bool selected = item.get_member(choices.selected_field).as_bool();
				cb.radio_button(id, text, [item, this, i](radiobutton_control& _rbc) {
					_rbc.json_field_name = choices.selected_field;
					_rbc.is_group = i == 0;
					_rbc.set_data(item);
					});
			}
		}

		virtual json get_data()
		{
			json result;
			if (not json_field_name.empty()) {
				json_parser jp;
				result = jp.create_object();
				json result_array = jp.create_array();

				for (auto child : children)
				{
					json data = child->get_data();
					result_array.put_element(-1, data);
				}

				result.put_member(json_field_name, result_array);

			}
			return result;
		}

		virtual json set_data(json _data)
		{
			data = _data;
			if (_data.has_member(json_field_name)) {
				json field_items = _data[json_field_name];
				if (field_items.array()) {
					json as_object = field_items.array_to_object(

						[this](json& _item)->std::string {
							return _item.get_member(choices.id_field).as_string();
						},
						[](json& _item)->json {
							return _item;
						}
					);
					for (auto child : children) {
						json existing = child->get_data();
						std::string key = choices.id_field;
						json item = as_object.get_member(key);
						child->set_data(item);
					}
				}
			}
			return _data;
		}

		void set_list(list_data& _choices)
		{
			choices = _choices;
			list_changed();
		}

		virtual void on_create()
		{
			list_changed();
		}

	};

	class checkbox_list_control :
		public column_layout
	{
	protected:
		list_data choices;
		json data;

	public:
		checkbox_list_control() { ; }
		checkbox_list_control(const checkbox_list_control& _src) = default;
		checkbox_list_control(control_base* _parent, int _id) : column_layout(_parent, _id) { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<checkbox_list_control>(*this);
			return tv;
		}

		virtual ~checkbox_list_control() { ; }

		void list_changed()
		{
			control_builder cb;

			children.clear();

			int count = choices.items.size();

			for (int i = 0; i < count; i++)
			{
				json item = choices.items.get_element(i);
				int id = item.get_member(choices.id_field).as_int();
				std::string text = item.get_member(choices.text_field).as_string();
				bool selected = item.get_member(choices.selected_field).as_bool();
				cb.checkbox(id, text, [item, this](checkbox_control& _rbc) {
					_rbc.json_field_name = choices.selected_field;
					_rbc.set_data(item);
					});
			}
		}

		virtual json get_data()
		{
			json result;
			if (not json_field_name.empty()) {
				json_parser jp;
				result = jp.create_object();
				json result_array = jp.create_array();

				for (auto child : children)
				{
					json data = child->get_data();
					result_array.put_element(-1, data);
				}

				result.put_member(json_field_name, result_array);

			}
			return result;
		}

		virtual json set_data(json _data)
		{
			data = _data;
			if (_data.has_member(json_field_name)) {
				json field_items = _data[json_field_name];
				if (field_items.array()) {
					json as_object = field_items.array_to_object(

						[this](json& _item)->std::string {
							return _item.get_member(choices.id_field).as_string();
						},
						[](json& _item)->json {
							return _item;
						}
					);
					for (auto child : children) {
						json existing = child->get_data();
						std::string key = choices.id_field;
						json item = as_object.get_member(key);
						child->set_data(item);
					}
				}
			}
			return _data;
		}

		void set_list(list_data& _choices)
		{
			choices = _choices;
			list_changed();
		}

		virtual void on_create()
		{
			list_changed();
		}

	};

	void corona_button_control::init()
	{
		set_origin(0.0_px, 0.0_px);
		set_size(100.0_px, 30.0_px);
	}

	void frame_layout::set_contents(int _batch_id, presentation_base* _presentation, page_base* _parent_page, page_base* _contents)
	{
		control_builder cb;

		for (auto child : children) {
			child->on_unsubscribe(_presentation, _parent_page);
		}
		children.clear();

		for (auto srcchild : _contents->root->children)
		{
			auto new_child = srcchild->clone();
			new_child->set_hit_word(hit_words);
			children.push_back(new_child);
		}

		arrange_children();

		for (auto child : children) {
			child->on_subscribe(_presentation, _parent_page);
			child->set_data(data);
			child->loaded(_batch_id);
		}
	}

	json corona_set_property_command::execute(comm_bus_app_interface* bus)
	{
		json obj;
		control_base* cb = {};
		if (not control_name.empty()) {
			cb = bus->find_control(control_name);
			if (this->property_name == "text") {
				text_control_base* tcb = dynamic_cast<text_control_base*>(cb);
				if (tcb != nullptr) {
					tcb->set_text(value);
				}
				else {
					text_display_control* tdc = dynamic_cast<text_display_control*>(cb);
					if (tdc != nullptr) {
						tdc->set_text(value);
					}
				}
			}
		}
		return obj;
	}
}
