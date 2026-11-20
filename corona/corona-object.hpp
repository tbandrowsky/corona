#pragma once

namespace corona
{

    class corona_object_timepoint
    {
    public:
        double  elapsed_seconds = 0.0;
        json    values;
    };

    class corona_object_history
    {
        json original;
        std::map<double, corona_object_timepoint> history;
        std::set<std::string> tween_fields;

    public:

        void clear()
        {
            history.clear();
            tween_fields.clear();
        }

        void set_original(json _original)
        {
            original = _original;
        }

        void set_tween_fields(std::set<std::string> _fields)
        {
            tween_fields = _fields;
        }

        void put_change(corona_object_timepoint& _timepoint)
        {
            if (history.contains(_timepoint.elapsed_seconds)) {
                history[_timepoint.elapsed_seconds].values.merge(_timepoint.values);
            }
            else {
                corona_object_timepoint ntp;
                ntp.elapsed_seconds = _timepoint.elapsed_seconds;
                ntp.values = _timepoint.values.clone();
                history[_timepoint.elapsed_seconds] = ntp;
            }
        }

        json get_time(double _time)
        {
            json_parser jp;

            if (history.size() == 0)
                return original;

            json result = original.clone();

            if (history.begin() != history.end()) {
                auto end_time = history.end();
                end_time--;
                if (end_time->first > 0.0)
                {
                    _time = fmod(_time, end_time->first);
                }
            }

            if (history.size() == 1) {
                auto default_history = history.begin();
                json values = default_history->second.values;
                result.merge(values);
                return result;
            }

            auto start_time = history.lower_bound(_time);
            auto next_time = history.upper_bound(_time);

            if (next_time == history.end()) {
                result.merge(start_time->second.values);
                return result;
            }

            while (start_time != history.begin() && start_time->first > _time)
            {
                start_time--;
            }

            if (tween_fields.empty()) {
                result.merge(start_time->second.values);
                return result;
            }

            for (auto tf : tween_fields) {
                if (start_time != history.end() && next_time != history.end()) {
                    double t = (next_time->second.elapsed_seconds - start_time->second.elapsed_seconds);
                    if (t > 0) {
                        using namespace DirectX;

                        double p = (_time - start_time->second.elapsed_seconds) / t;
                        if (start_time->second.values.has_member(tf) &&
                            next_time->second.values.has_member(tf))
                        {
                            auto v1 = start_time->second.values[tf];
                            auto v2 = next_time->second.values[tf];
                            if (v1.is_vector() && v2.is_vector()) {
                                XMVECTOR d1 = v1.as_vector();
                                XMVECTOR d2 = v2.as_vector();
                                XMVECTOR dt = XMVectorSubtract(d2, d1);
                                XMVECTOR dv = XMVectorAdd(d1, XMVectorScale(dt, static_cast<float>(p)));
                                result.put_member_vector(tf, dv);
                            }
                            else {
                                double d1 = v1.as_double();
                                double d2 = v2.as_double();
                                double dt = d2 - d1;
                                double dv = d1 + (dt * p);
                                result.put_member_double(tf, dv);
                            }
                        }
                    }
                }
            }

            return result;
        }
    };

    class corona_object : public corona_object_interface
    {
        json_object data;

    public:

        corona_database* db = nullptr;

        corona_object() = default;
        corona_object(const corona_object& _src) = default;
        corona_object(corona_object&& _src) = default;
        corona_object& operator =(const corona_object& _src) = default;
        corona_object& operator =(corona_object&& _src) = default;

        corona_object(corona_database* _db) : db(_db)
        {

        }

        corona_object(corona_database* _db, json_object _data) : db(_db), data(_data)
        {

        }

        virtual void get_json(json& _dest) const
        {
            _dest = data;
            _dest.put_member_string(class_name_field, class_name);
            _dest.put_member_i64(object_id_field, object_id);
            _dest.put_member_string("created_by", created_by);
            _dest.put_member_string("updated_by", updated_by);
            _dest.put_member("created", created);
            _dest.put_member("updated", updated);
        }

        virtual void put_json(json& _src)
        {
            data = to_object(_src);
            class_name = _src[class_name_field].as_string();
            object_id = _src[object_id_field].as_int64_t();
            created_by = _src["created_by"].as_string();
            updated_by = _src["updated_by"].as_string();
            created = _src["created"].as_date_time();
            updated = _src["updated"].as_date_time();
        }

        virtual void apply_json(json& _src)
        {
            json_parser jp;
            json body = jp.create_object();
            get_json(body);
            body.merge(_src);
            put_json(body);
        }

        virtual std::string get_item_type() const
        {
            return class_name;
        }

        virtual object_reference to_reference() const
        {
            object_reference ref;

            ref.class_name = class_name;
            ref.object_id = object_id;

            return ref;
        }

        virtual chest_item to_chest_item(int _quantity) const
        {
            chest_item ci;

            ci.reference.class_name = class_name;
            ci.reference.object_id = object_id;
            ci.item_type = get_item_type();
            ci.quantity = _quantity;

            return ci;
        }

        virtual bool operator < (corona_object& _src)
        {
            return to_reference() < _src.to_reference();
        }

        template <typename T>
        bool is()
        {
            T* t = dynamic_cast<T*>(this);
            return t != nullptr;
        }

        template <typename T>
        T* as()
        {
            T* t = dynamic_cast<T*>(this);
            return t;
        }

        virtual std::shared_ptr<corona_object_interface> clone() const
        {
            json_parser jp;
            json body = jp.create_object();
            get_json(body);
            auto p = std::make_shared<corona_object>(*this);
            p->put_json(body);
            return p;
        }

        std::string get_string(std::string _field_name)
        {
            return data[_field_name]->as_string();
        }

        DirectX::XMVECTOR get_vector(std::string _field_name)
        {
            return data[_field_name]->as_vector();
        }

        double get_double(std::string _field_name)
        {
            return data[_field_name]->as_double();
        }

        int64_t get_int64(std::string _field_name)
        {
            return data[_field_name]->as_int64_t();
        }

        date_time get_datetime(std::string _field_name)
        {
            return data[_field_name]->as_date_time();
        }

        std::string put_string(std::string _field_name, std::string  _value)
        {
            data.put_member(_field_name, _value);
            return _value;
        }

        DirectX::XMVECTOR put_vector(std::string _field_name, DirectX::XMVECTOR _value)
        {
            data.put_member(_field_name, _value);
            return _value;
        }

        double put_double(std::string _field_name, double _value)
        {
            data.put_member(_field_name, _value);
            return _value;
        }

        int64_t put_int64(std::string _field_name, int64_t _value)
        {
            data.put_member(_field_name, _value);
            return _value;
        }

        date_time put_datetime(std::string _field_name, date_time _value)
        {
            data.put_member(_field_name, _value);
            return _value;
        }

        std::shared_ptr<workflow_field> get_workflow(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_workflow(data, _field_name);
            return wf;
        }

        std::shared_ptr<chest_field> get_chest(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_chest(data, _field_name);
            return wf;
        }

        std::shared_ptr<selection_field> get_selection(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_selection(data, _field_name);
            return wf;
        }

        std::shared_ptr<pathDto> get_path(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_path(data, _field_name);
            return wf;
        }

        std::shared_ptr<bitmapInstanceDto> get_bitmap(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_bitmap(data, _field_name);
            return wf;
        }

        std::shared_ptr<generalBrushRequest> get_brush(std::string _field_name)
        {
            auto classd = db->read_lock_class(class_name);
            auto wf = classd->get_brush(data, _field_name);
            return wf;
        }

        void put_workflow(std::string _field_name, std::shared_ptr<workflow_field> _workflow)
        {
            json jdata = to_object(data);
            auto classd = db->read_lock_class(class_name);
            classd->put_workflow(jdata, _field_name, _workflow);
        }

        void put_chest(std::string _field_name, std::shared_ptr<chest_field> _chest)
        {
            json jdata = to_object(data);
            auto classd = db->read_lock_class(class_name);
            classd->put_chest(jdata, _field_name, _chest);
        }

        void put_selection(std::string _field_name, std::shared_ptr<selection_field> _selection)
        {
            json jdata = to_object(data);
            auto classd = db->read_lock_class(class_name);
            classd->put_selection(jdata, _field_name, _selection);
        }

        void put_path(std::string _field_name, std::shared_ptr<pathDto> _path)
        {
            json jdata = to_object(data);
            auto classd = db->read_lock_class(class_name);
            classd->put_path(jdata, _field_name, _path);
        }

        void put_bitmap(std::string _field_name, std::shared_ptr<bitmapInstanceDto> _bitmap)
        {
            json jdata = to_object(data);
            auto classd = db->read_lock_class(class_name);
            classd->put_bitmap(jdata, _field_name, _bitmap);
        }

    };

    class event_message_base {
    public:
        std::string user_token;

        virtual void get_json(json& _dest)
        {
            _dest.put_member("user_token", user_token);
        }
        virtual void put_json(json& _src)
        {
            user_token = _src["user_token"].as_string();
        }
    };

    template <typename T>
    class event_message : public event_message_base
    {
    public:
        std::string event_type;
        std::string user_token;
        T event_data;

        virtual void get_json(json& _dest)
        {
            _dest.put_member("user_token", user_token);
           corona::get_json(_dest, event_data);
        }
        virtual void put_json(json& _src)
        {
            user_token = _src["user_token"].as_string();
            corona::put_json(_src, event_data);
        }
    };

    class gamepad_event_message : public event_message<gamepad_event>   
    {
    public:

        gamepad_event_message()
        {
            event_type = "gamepad_event";
        }

    };

    class gamepad_button_down_event_message : public event_message<gamepad_button_down_event>
    {
    public:
        gamepad_button_down_event_message()
        {
            event_type = "gamepad_button_down_event";
        }
    };

    class gamepad_button_up_event_message : public event_message<gamepad_button_up_event>
    {
    public:
        gamepad_button_up_event_message()
        {
            event_type = "gamepad_button_up_event";
        }
    };

    class gamepad_trigger_down_event_message : public event_message<gamepad_trigger_down_event>
    {
    public:
        gamepad_trigger_down_event_message()
        {
            event_type = "gamepad_trigger_down_event";
        }
    };

    class gamepad_trigger_up_event_message : public event_message<gamepad_trigger_up_event>
    {
    public:
        gamepad_trigger_up_event_message()
        {
            event_type = "gamepad_trigger_up_event";
        }
    };

    class gamepad_thumbstick_move_event_message : public event_message<gamepad_thumbstick_move_event>
    {
    public:
        gamepad_thumbstick_move_event_message()
        {
            event_type = "gamepad_thumbstick_move_event";
        }
    };

    class key_press_event_message : public event_message<key_press_event>
    {
    public:
        key_press_event_message()
        {
            event_type = "key_press_event";
        }
    };

    class mouse_move_event_message : public event_message<mouse_move_event>
    {
    public:
        mouse_move_event_message()
        {
            event_type = "mouse_move_event";
        } 
    };

    class mouse_wheel_event_message : public event_message<mouse_wheel_event>
    {
    public:

        mouse_wheel_event_message()
        {
            event_type = "mouse_wheel_event";
        }

    };

    class mouse_left_click_event_message : public event_message<mouse_left_click_event>
    {
    public:

        mouse_left_click_event_message()
        {
            event_type = "mouse_left_click_event_message";
        }

    };

    class mouse_right_click_event_message : public event_message<mouse_right_click_event>
    {
    public:
        mouse_right_click_event_message()
        {
            event_type = "mouse_right_click_event_message";
        }
    };

    class model_run_object : public corona_object, public job
    {
    public:

        std::string user_token;

        std::string model_name;
        std::string model_description;
        
        virtual void get_json(json& _dest) const override
        {
            corona_object::get_json(_dest);
            _dest.put_member_string("model_name", model_name);
            _dest.put_member_string("model_description", model_description);
        }

        virtual void put_json(json& _src) override
        {
            corona_object::put_json(_src);
            model_name = _src["model_name"].as_string();
            model_description = _src["model_description"].as_string();
        }

        virtual void on_gamepad_event(gamepad_event_message& _msg) {}
        virtual void on_key_press_event(key_press_event_message& _msg) {}
        virtual void on_mouse_move_event(mouse_move_event_message& _msg) {}
        virtual void on_mouse_wheel_event(mouse_wheel_event_message& _msg) {}
        virtual void on_mouse_left_click_event(mouse_left_click_event_message& _msg) {}
        virtual void on_mouse_right_click_event(mouse_right_click_event_message& _msg) {}
        virtual void on_gamepad_button_down_event(gamepad_button_down_event_message& _msg) {}
        virtual void on_gamepad_button_up_event(gamepad_button_up_event_message& _msg) {}
        virtual void on_gamepad_trigger_down_event(gamepad_trigger_down_event_message& _msg) {}
        virtual void on_gamepad_trigger_up_event(gamepad_trigger_up_event_message& _msg) {}
        virtual void on_gamepad_thumbstick_move_event(gamepad_thumbstick_move_event_message& _msg) {}

        virtual void on_message(json& _message) 
        {
            std::string event_type = _message["event_type"].as_string();

            if (event_type == "gamepad_event")
            {
                gamepad_event_message msg;
                msg.put_json(_message);
                on_gamepad_event(msg);
            }
            else if (event_type == "gamepad_button_down_event")
            {
                gamepad_button_down_event_message msg;
                msg.put_json(_message);
                on_gamepad_button_down_event(msg);
            }
            else if (event_type == "gamepad_button_up_event")
            {
                gamepad_button_up_event_message msg;
                msg.put_json(_message);
                on_gamepad_button_up_event(msg);
            }
            else if (event_type == "gamepad_trigger_down_event")
            {
                gamepad_trigger_down_event_message msg;
                msg.put_json(_message);
                on_gamepad_trigger_down_event(msg);
            }
            else if (event_type == "gamepad_trigger_up_event")
            {
                gamepad_trigger_up_event_message msg;
                msg.put_json(_message);
                on_gamepad_trigger_up_event(msg);
            }
            else if (event_type == "gamepad_thumbstick_move_event")
            {
                gamepad_thumbstick_move_event_message msg;
                msg.put_json(_message);
                on_gamepad_thumbstick_move_event(msg);
            }

        }

        virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
        {

        }

        virtual bool queued(job_queue* _queue)
        {

        }

    };

    enum class corona_grant_type
    {
        none,
        any,
        own,
        team,
        teamorown
    };

    class corona_permission : public corona_object
    {
    public:
        corona_permission() : get_grant(corona_grant_type::none), put_grant(corona_grant_type::none), delete_grant(corona_grant_type::none), alter_grant(corona_grant_type::none)
        {

        }
        corona_permission(const corona_permission& _src) = default;
        corona_permission(corona_permission&& _src) = default;
        corona_permission& operator=(const corona_permission& _src) = default;
        corona_permission& operator=(corona_permission&& _src) = default;

        std::vector<std::string> grant_classes;
        corona_grant_type get_grant;
        corona_grant_type put_grant;
        corona_grant_type delete_grant;
        corona_grant_type alter_grant;

        virtual void get_json(json& _dest) const override
        {
            corona_object::get_json(_dest);
            json_parser jp;
            json jclasses = jp.create_array();
            for (const auto& e : grant_classes) {
                jclasses.push_back(e);
            }
            _dest.put_member("grant_classes", jclasses);
            _dest.put_member_string("get", grant_type_to_string(get_grant));
            _dest.put_member_string("put", grant_type_to_string(put_grant));
            _dest.put_member_string("delete", grant_type_to_string(delete_grant));
            _dest.put_member_string("alter", grant_type_to_string(alter_grant));
        }

        virtual void put_json(json& _src) override
        {
            corona_object::put_json(_src);
            json_parser jp;
            json jclasses = _src["grant_classes"];
            grant_classes = jclasses.to_string_array();
            get_grant = string_to_grant_type(_src["get"].as_string());
            put_grant = string_to_grant_type(_src["put"].as_string());
            delete_grant = string_to_grant_type(_src["delete"].as_string());
            alter_grant = string_to_grant_type(_src["alter"].as_string());
        }

    private:

        corona_grant_type string_to_grant_type( std::string _type) const
        {
            std::transform(_type.begin(), _type.end(), _type.begin(), [](unsigned char c) { return std::tolower(c); });
            if (_type == "none") return corona_grant_type::none;
            if (_type == "any") return corona_grant_type::any;
            if (_type == "own") return corona_grant_type::own;
            if (_type == "team") return corona_grant_type::team;
            if (_type == "teamorown") return corona_grant_type::teamorown;
            return corona_grant_type::none;
        }

        std::string grant_type_to_string(corona_grant_type _type) const
        {
            switch (_type) {
            case corona_grant_type::none: return "none";
            case corona_grant_type::any: return "any";
            case corona_grant_type::own: return "own";
            case corona_grant_type::team: return "team";
            case corona_grant_type::teamorown: return "teamorown";
            default: return "none";
            }
        }
    };

    template <typename T> class function_scheduler : public corona_object
    {
    public:


        function_scheduler(double _seconds, T _value, std::function<void(T src)> _task)
            : remaining_seconds(0), frequency_seconds(_seconds), duration_seconds(_seconds), task(_task), value(_value)
        {

        }

        function_scheduler()
        {
            frequency_seconds = 1;
            remaining_seconds = 0;
            duration_seconds = 2;
            enabled = false;
            value = {};
            task = {};
        }

        function_scheduler(const function_scheduler& _src) = default;
        function_scheduler(function_scheduler&& _src) = default;
        function_scheduler& operator=(const function_scheduler& _src) = default;
        function_scheduler& operator=(function_scheduler&& _src) = default;

        std::function<void(T src)>	task;
        double						frequency_seconds;
        double						remaining_seconds;
        double						duration_seconds;
        T							value;
        bool						enabled;

        bool execute(double elapsed)
        {
            if (!enabled)
                return false;

            duration_seconds -= elapsed;
            remaining_seconds -= elapsed;

            if (duration_seconds < 0) {
                return false;
            }

            if (remaining_seconds <= 0.0)
            {
                try
                {
                    if (task)
                    {
                        task(value);
                    }
                }
                catch (std::exception exc)
                {
                }
                remaining_seconds = frequency_seconds;
                return true;
            }

            return false;
        }

        void get_json(json& _dest) const
        {
            corona_object::get_json(_dest);
            _dest.put_member_double("frequency_seconds", frequency_seconds);
            _dest.put_member_double("remaining_seconds", remaining_seconds);
            _dest.put_member_double("duration_seconds", duration_seconds);
            _dest.put_member_bool("enabled", enabled);
        }

        void put_json(json& _src)
        {
            corona_object::put_json(_src);
            frequency_seconds = _src["frequency_seconds"].as_double();
            remaining_seconds = _src["remaining_seconds"].as_double();
            duration_seconds = _src["duration_seconds"].as_double();
            enabled = _src["enabled"].as_bool();
        }
    };

    // T must derive from Base (or be Base itself)
    template<typename T>
    concept is_corona_object = std::derived_from<T, corona_object>;

    std::shared_ptr<corona_object_interface> corona_database::construct(json _data)
    {
        std::shared_ptr<corona_object> result;

        if (_data.object())
        {
            std::string class_name = _data["class_name"].as_string();
            auto ctor = object_factory.find(class_name);

            if (ctor != object_factory.end())
            {
                auto f = ctor->second(this, _data);
                result = std::dynamic_pointer_cast<corona_object>(f);
            }
            else
            {
                result = std::make_shared<corona_object>(this, _data);
            }
        }
        return result;
    }

}
